/*
 File: Worker.cpp
 Created on: 8 dic. 2017
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project
#include <Worker.h>
#include <QDebug>

// C++
#include <iostream>

// libav
extern "C"
{
#include <libavutil/avutil.h>
}

const QList<int> Worker::VALID_VIDEO_CODECS = { AV_CODEC_ID_VP8, AV_CODEC_ID_VP9, AV_CODEC_ID_H264, AV_CODEC_ID_HEVC };
const QList<int> Worker::VALID_AUDIO_CODECS = { AV_CODEC_ID_MP3, AV_CODEC_ID_AAC, AV_CODEC_ID_VORBIS };

const std::wstring SUBTITLE_EXTENSION = L".srt";

//--------------------------------------------------------------------
Worker::Worker(const boost::filesystem::path &source_info, const Utils::TranscoderConfiguration &config)
: m_configuration           {config}
, m_audio_decoder           {nullptr}
, m_video_decoder           {nullptr}
, m_subtitle_decoder        {nullptr}
, m_audio_decoder_context   {nullptr}
, m_video_decoder_context   {nullptr}
, m_subtitle_decoder_context{nullptr}
, m_input_context           {nullptr}
, m_frame                   {nullptr}
, m_audio_stream_id         {-1}
, m_video_stream_id         {-1}
, m_subtitle_stream_id      {-1}
, m_output_context          {nullptr}
, m_audio_coder_context     {nullptr}
, m_video_coder_context     {nullptr}
, m_audio_coder             {nullptr}
, m_video_coder             {nullptr}
, m_packet                  {nullptr}
, m_source_info             (source_info)
, m_source_path             (source_info.parent_path())
, m_fail                    {false}
, m_stop                    {false}
, m_audioStream             {nullptr}
, m_videoStream             {nullptr}
{
}

//--------------------------------------------------------------------
void Worker::stop()
{
  emit information_message(QString("Transcoder for '%1' has been cancelled.").arg(QString::fromStdWString(m_source_info.wstring())));
  m_stop = true;
}

//--------------------------------------------------------------------
bool Worker::has_been_cancelled()
{
  return m_stop;
}

//--------------------------------------------------------------------
bool Worker::has_failed()
{
  return m_fail;
}

//--------------------------------------------------------------------
void Worker::run()
{
  if(check_input_file_permissions() && init_libav() && check_output_file_permissions())
  {
    if(inputNeedsProcessing())
    {
      if(create_output())
      {
        return;
//        int value = 0;
//        while(0 == (value = av_read_frame(m_input_context, m_packet)) && !has_been_cancelled())
//        {
//          switch(m_packet->stream_index)
//          {
//            case m_audio_stream_id:
//              break;
//            case m_video_stream_id:
//              break;
//            case m_subtitle_stream_id:
//              break;
//            default:
//              // discard
//              break;
//          }
//        }
      }
      else
      {
        emit error_message(tr("Unable to create output or configure it for file: '%1").arg(m_input_file.fileName()));
        m_fail = true;
      }
    }
    else
    {
      emit information_message(tr("Not processed: '%1' is already in the correct format for Chromecast").arg(m_input_file.fileName()));
    }

    deinit_libav();
  }
  else
  {
    m_fail = true;
  }

  emit progress(100);
}

//--------------------------------------------------------------------
bool Worker::check_input_file_permissions()
{
  const auto qFilename = QString::fromStdWString(m_source_info.wstring());

  QFile file(qFilename);
  if(file.exists() && !file.open(QFile::ReadOnly))
  {
    emit error_message(QString("Can't open file '%1' but it exists, check for permissions.").arg(qFilename));
    m_fail = true;
    return false;
  }

  file.close();
  return true;
}

//--------------------------------------------------------------------
bool Worker::check_output_file_permissions()
{
  if(needsAudioProcessing() || needsVideoProcessing())
  {
    const auto qFilename = QString::fromStdWString(m_source_info.stem().wstring() + outputExtension());
    QFile outputFile(qFilename);
    if(!outputFile.open(QFile::WriteOnly|QFile::Truncate))
    {
      emit error_message(QString("Unable to create output file in '%1' path, check for permissions.").arg(qFilename));
      return false;
    }

    outputFile.close();
    outputFile.remove();
  }

  if(m_configuration.extractSubtitles())
  {
    const auto qFilename = QString::fromStdWString(m_source_info.stem().wstring() + SUBTITLE_EXTENSION);
    QFile outputFile(qFilename);
    if(!outputFile.open(QFile::WriteOnly|QFile::Truncate))
    {
      emit error_message(QString("Unable to create subtitle file in '%1' path, check for permissions.").arg(qFilename));
      return false;
    }

    outputFile.close();
    outputFile.remove();
  }

  return true;
}

//--------------------------------------------------------------------
bool Worker::init_libav()
{
  QMutexLocker lock(&Utils::s_mutex);

  av_register_all();

  auto source_name = QString::fromStdWString(m_source_info.wstring());
  m_input_file.setFileName(source_name);
  if(!m_input_file.open(QIODevice::ReadOnly))
  {
    emit error_message(QString("Couldn't open input file '%1'.").arg(source_name));
    return false;
  }

  auto ioBuffer = reinterpret_cast<unsigned char *>(av_malloc(s_io_buffer_size)); // can get freed with av_free() by libav
  if(nullptr == ioBuffer)
  {
    emit error_message(QString("Couldn't allocate buffer for custom libav IO for file: '%1'.").arg(source_name));
    return false;
  }

  auto avioContext = avio_alloc_context(ioBuffer, s_io_buffer_size - AV_INPUT_BUFFER_PADDING_SIZE, 0, reinterpret_cast<void*>(&m_input_file), &custom_IO_read, nullptr, &custom_IO_seek);
  if(nullptr == avioContext)
  {
    emit error_message(QString("Couldn't allocate context for custom libav IO for file: '%1'.").arg(source_name));
    return false;
  }

  avioContext->seekable = 0;
  avioContext->write_flag = 0;

  m_input_context = avformat_alloc_context();
  m_input_context->pb = avioContext;
  m_input_context->flags |= AVFMT_FLAG_CUSTOM_IO;

  auto value = avformat_open_input(&m_input_context, source_name.toStdString().c_str(), nullptr, nullptr);
  if (value < 0)
  {
    emit error_message(QString("Couldn't open file: '%1' with libav. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  // avoids a warning message from libav when the duration can't be calculated accurately. this increases the lookup frames.
  m_input_context->max_analyze_duration *= 1000;

  value = avformat_find_stream_info(m_input_context, nullptr);
  if(value < 0)
  {
    emit error_message(QString("Couldn't get the information of '%1'. Error is \"%2\".").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  for(unsigned int id = 0; id < m_input_context->nb_streams; id++)
  {
    if(m_input_context->streams[id]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      auto lang = av_dict_get(m_input_context->streams[id]->metadata, "language", nullptr, 0);

      if(!lang)
      {
        m_audio_stream_id = id;
        break;
      }

      if(strcmp(lang->value, "spa"))
      {
        m_audio_stream_id = id;
        if (m_configuration.preferredAudioLanguage() == Utils::TranscoderConfiguration::Language::SPANISH)
        {
          break;
        }
      }

      if(strcmp(lang->value, "eng"))
      {
        m_audio_stream_id = id;
        if (m_configuration.preferredAudioLanguage() == Utils::TranscoderConfiguration::Language::ENGLISH)
        {
          break;
        }
      }
    }
  }

  if (m_audio_stream_id < 0)
  {
    emit error_message(QString("Couldn't find any audio stream in '%1'. Error is \"%2\".").arg(source_name).arg(av_error_string(m_audio_stream_id)));
    return false;
  }

  av_find_best_stream(m_input_context, AVMEDIA_TYPE_AUDIO, m_audio_stream_id, -1, &m_audio_decoder, 0);

  if(!m_audio_decoder)
  {
    emit error_message(QString("Couldn't find audio decoder for '%1'.").arg(source_name));
    return false;
  }

  // NOTE: the use of streams is deprecated but I couldn't find another way to correctly init a
  // decoder context with the correct parameters found during av_format_find_info(). The method:
  // avcodec_alloc_context3(const AVCodec *codec) returns an uninitalized context that fails in
  // the send_packet() API. Also, in the examples it's done this way. Why if it's being deprecated?
  m_audio_decoder_context = m_input_context->streams[m_audio_stream_id]->codec;

  value = avcodec_open2(m_audio_decoder_context, m_audio_decoder, nullptr);
  if (value < 0 || !avcodec_is_open(m_audio_decoder_context))
  {
    emit error_message(QString("Couldn't open audio decoder for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  m_video_stream_id = av_find_best_stream(m_input_context, AVMEDIA_TYPE_VIDEO, -1, -1, &m_video_decoder, 0);
  if (m_video_stream_id < 0)
  {
    emit error_message(QString("Couldn't find any video stream in '%1'. Error is \"%2\".").arg(source_name).arg(av_error_string(m_video_stream_id)));
    return false;
  }

  if(!m_video_decoder)
  {
    emit error_message(QString("Couldn't find video decoder for '%1'.").arg(source_name));
    return false;
  }

  m_video_decoder_context = m_input_context->streams[m_video_stream_id]->codec;

  value = avcodec_open2(m_video_decoder_context, m_video_decoder, nullptr);
  if (value < 0 || !avcodec_is_open(m_video_decoder_context))
  {
    emit error_message(QString("Couldn't open video decoder for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  if(m_configuration.extractSubtitles())
  {
    for(unsigned int id = 0; id < m_input_context->nb_streams; id++)
    {
      if(m_input_context->streams[id]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE)
      {
        auto lang = av_dict_get(m_input_context->streams[id]->metadata, "language", nullptr, 0);

        if(!lang)
        {
          m_audio_stream_id = id;
          break;
        }

        if(strcmp(lang->value, "spa"))
        {
          m_subtitle_stream_id = id;
          if (m_configuration.preferredSubtitleLanguage() == Utils::TranscoderConfiguration::Language::SPANISH)
          {
            break;
          }
        }

        if(strcmp(lang->value, "eng"))
        {
          m_subtitle_stream_id = id;
          if (m_configuration.preferredSubtitleLanguage() == Utils::TranscoderConfiguration::Language::ENGLISH)
          {
            break;
          }
        }
      }
    }

    if(m_subtitle_stream_id != -1)
    {
      av_find_best_stream(m_input_context, AVMEDIA_TYPE_SUBTITLE, m_subtitle_stream_id, -1, &m_subtitle_decoder, 0);

      if(!m_subtitle_decoder)
      {
        emit error_message(QString("Couldn't find subtitle decoder for '%1'.").arg(source_name));
        return false;
      }
    }
  }

  m_packet = av_packet_alloc();
  m_frame  = av_frame_alloc();

  return true;
}

//--------------------------------------------------------------------
void Worker::deinit_libav()
{
  if(m_input_file.isOpen())
  {
    m_input_file.close();
  }

  for(auto context: {m_audio_decoder_context, m_video_decoder_context, m_subtitle_decoder_context, m_audio_coder_context, m_video_coder_context})
  {
    if(context) avcodec_free_context(&context);
  }

  if(m_input_context)
  {
    if(m_input_context->pb) av_free(m_input_context->pb->buffer);
    avformat_close_input(&m_input_context);
  }

  if(m_output_context)
  {
    if (!(m_output_context->oformat->flags & AVFMT_NOFILE))
    {
      avio_close(m_output_context->pb);
    }

    av_write_trailer(m_output_context);
    avformat_free_context(m_output_context);
    m_output_context = nullptr;
  }

  if(m_frame)  av_frame_free(&m_frame);
  if(m_packet) av_packet_free(&m_packet);
}

//-----------------------------------------------------------------
int Worker::custom_IO_read(void* opaque, unsigned char* buffer, int buffer_size)
{
  auto reader = reinterpret_cast<QFile *>(opaque);
  return reader->read(reinterpret_cast<char *>(buffer), buffer_size);
}

//-----------------------------------------------------------------
long long int Worker::custom_IO_seek(void* opaque, long long int offset, int whence)
{
  auto reader = reinterpret_cast<QFile *>(opaque);
  switch(whence)
  {
    case AVSEEK_SIZE:
      return reader->size();
    case SEEK_SET:
      return reader->seek(offset);
    case SEEK_END:
      return reader->seek(reader->size());
    case SEEK_CUR:
      return reader->pos();
      break;
    default:
      Q_ASSERT(false);
  }
  return 0;
}

//-----------------------------------------------------------------
QString Worker::av_error_string(const int error_num) const
{
  char buffer[255];
  av_strerror(error_num, buffer, sizeof(buffer));

  return QString(buffer);
}

//-----------------------------------------------------------------
bool Worker::inputNeedsProcessing() const
{
  return needsAudioProcessing() || needsVideoProcessing() || needsSubtitleProcessing();
}

//-----------------------------------------------------------------
bool Worker::needsAudioProcessing() const
{
  return (!VALID_AUDIO_CODECS.contains(m_audio_decoder->id) || m_audio_decoder_context->channels != m_configuration.audioChannelsNum());
}

//-----------------------------------------------------------------
bool Worker::needsVideoProcessing() const
{
  return (!VALID_VIDEO_CODECS.contains(m_video_decoder->id));
}

//-----------------------------------------------------------------
bool Worker::needsSubtitleProcessing() const
{
  return (m_subtitle_stream_id != AVERROR_STREAM_NOT_FOUND && m_configuration.extractSubtitles());
}

//-----------------------------------------------------------------
bool Worker::create_output()
{
  auto filename = QString::fromStdWString(m_source_info.stem().wstring() + outputExtension());

  auto format = av_guess_format(nullptr, filename.toStdString().c_str(), nullptr);

  if(!format) return false;

  m_output_context = avformat_alloc_context();

  if(!m_output_context) return false;

  m_audio_coder = avcodec_find_encoder(audioCodecId());

  if(!m_audio_coder) return false;

  m_audio_coder_context = avcodec_alloc_context3(m_audio_coder);

  if(!m_audio_coder_context) return false;

  // some formats want stream headers to be separate
  if (format->flags & AVFMT_GLOBALHEADER) m_audio_coder_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  m_video_coder = avcodec_find_encoder(videoCodecId());

  if(!m_video_coder) return false;

  m_video_coder_context = avcodec_alloc_context3(m_video_coder);

  if(!m_video_coder_context) return false;

  // some formats want stream headers to be separate
  if (format->flags & AVFMT_GLOBALHEADER) m_video_coder_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if(avcodec_parameters_to_context(m_audio_coder_context, m_input_context->streams[m_audio_stream_id]->codecpar) < 0) return false;
  if(avcodec_parameters_to_context(m_video_coder_context, m_input_context->streams[m_video_stream_id]->codecpar) < 0) return false;

  m_audioStream = avformat_new_stream(m_output_context, m_audio_coder);

  if(!m_audioStream) return false;

  m_videoStream = avformat_new_stream(m_output_context, m_video_coder);

  if(!m_videoStream) return false;

  if(avcodec_open2(m_audio_coder_context, m_audio_coder, nullptr) < 0) return false;
  if(avcodec_open2(m_video_coder_context, m_video_coder, nullptr) < 0) return false;

  /* open the output file, if needed */
  if (!(format->flags & AVFMT_NOFILE))
  {
    if (avio_open(&m_output_context->pb, filename.toStdString().c_str(), AVIO_FLAG_WRITE) < 0) return false;
  }

  if(avformat_write_header(m_output_context, nullptr) < 0) return false;

  return true;
}
