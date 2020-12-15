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

// C++
#include <iostream>
#include <cassert>
#include <string.h>

// libav
extern "C"
{
#include <libavutil/avutil.h>
#include <libavutil/rational.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/dict.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
}

const QList<int> Worker::VALID_VIDEO_CODECS = { AV_CODEC_ID_VP8, AV_CODEC_ID_VP9, AV_CODEC_ID_H264, AV_CODEC_ID_HEVC };
const QList<int> Worker::VALID_AUDIO_CODECS = { AV_CODEC_ID_MP3, AV_CODEC_ID_AAC, AV_CODEC_ID_VORBIS };

const std::wstring SUBTITLE_EXTENSION = L".srt";

//--------------------------------------------------------------------
Worker::Worker(const std::filesystem::path &source_info, const Utils::TranscoderConfiguration &config)
: m_configuration           {config}
, m_input_context           {nullptr}
, m_output_context          {nullptr}
, m_output_subtitle_context {nullptr}
, m_frame                   {nullptr}
, m_packet                  {nullptr}
, m_source_info             (source_info)
, m_source_path             (source_info.parent_path())
, m_last_mux_dts            {0}
, m_fail                    {false}
, m_stop                    {false}
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
        const bool transcodeAudio     = m_audio_stream.encoder != nullptr;
        const bool transcodeVideo     = m_video_stream.encoder != nullptr;
        const bool transcodeSubtitles = m_subtitle_stream.encoder != nullptr;

        QStringList processingStrings;
        if(transcodeAudio)     processingStrings.push_back(tr("audio"));
        if(transcodeVideo)     processingStrings.push_back(tr("video"));
        if(transcodeSubtitles) processingStrings.push_back(tr("subtitles"));

        QString processingText;
        switch(processingStrings.size())
        {
          case 1:
            processingText = processingStrings.first();
            break;
          case 2:
            processingText = processingStrings.join(" and ");
            break;
          case 3:
            processingText = processingStrings.first() + ", " + processingStrings[1] + " and " + processingStrings.last();
            break;
          default:
            processingText = "unknown";
            break;
        }

        const auto filename = QString::fromStdString(m_source_info.stem().string());
        const auto message = tr("Transcoding '%1': %2").arg(filename).arg(processingText);
        emit information_message(message);

        int value = 0;
        int progressVal = 0;
        while(0 == (value = av_read_frame(m_input_context, m_packet)) && !has_been_cancelled())
        {
          const int currentProgress = m_input_context->pb->pos * 100 / m_input_file.size();
          if(progressVal != currentProgress)
          {
            progressVal = currentProgress;
            emit progress(progressVal);
          }

          if(m_packet->stream_index == m_audio_stream.id)
          {
            if(transcodeAudio)
            {
              if(!process_stream_packet(m_audio_stream))
              {
                emit error_message(tr("Error transcoding audio frame for file '%1'.").arg(filename));
                break;
              }
            }
            else
            {
              m_packet->stream_index = m_audio_stream.stream->index;
              av_interleaved_write_frame(m_output_context, m_packet);
            }
          }
          else if(m_packet->stream_index == m_video_stream.id)
          {
            if(transcodeVideo)
            {
              if(!process_stream_packet(m_video_stream))
              {
                emit error_message(tr("Error transcoding video frame for file '%1'.").arg(filename));
                break;
              }
            }
            else
            {
              m_packet->stream_index = m_video_stream.stream->index;
              av_interleaved_write_frame(m_output_context, m_packet);
            }
          }
          else if(m_packet->stream_index == m_subtitle_stream.id)
          {
            if(transcodeSubtitles)
            {
              if(!process_stream_packet(m_subtitle_stream))
              {
                emit error_message(tr("Error transcoding subtitle frame for file '%1'.").arg(filename));
                break;
              }
            }
          }

          av_packet_unref(m_packet);
        }

        if(value == AVERROR_EOF)
        {
          flush_streams();
        }
        else if(value < 0)
        {
          emit error_message(tr("Error while transcoding '%1'. Error is: %2.").arg(m_input_file.fileName()).arg(av_error_string(value)));
          m_fail = true;
        }
      }
      else
      {
        emit error_message(tr("Unable to create output or configure it for file: '%1'").arg(m_input_file.fileName()));
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

  if(needsSubtitleProcessing())
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

  avcodec_register_all();
  av_register_all();
  avfilter_register_all();

  av_log_set_callback(log_callback);

  auto source_name = QString::fromStdWString(m_source_info.wstring());
  m_input_file.setFileName(source_name);
  if(!m_input_file.open(QIODevice::ReadOnly))
  {
    emit error_message(QString("Couldn't open input file '%1'.").arg(source_name));
    return false;
  }

  auto ioBuffer = reinterpret_cast<unsigned char *>(av_malloc(s_io_buffer_size)); // can get freed with av_free() by libav
  if(!ioBuffer)
  {
    emit error_message(QString("Couldn't allocate buffer for custom libav IO for file: '%1'.").arg(source_name));
    return false;
  }

  auto avioContext = avio_alloc_context(ioBuffer, s_io_buffer_size - AV_INPUT_BUFFER_PADDING_SIZE, 0, reinterpret_cast<void*>(&m_input_file), &custom_IO_read, nullptr, &custom_IO_seek);
  if(!avioContext)
  {
    emit error_message(QString("Couldn't allocate context for custom libav IO for file: '%1'.").arg(source_name));
    return false;
  }

  avioContext->seekable = 0;
  avioContext->write_flag = 0;

  m_input_context = avformat_alloc_context();
  if(!m_input_context)
  {
    emit error_message(QString("Couldn't allocate input context for file: '%1'.").arg(source_name));
    return false;
  }

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
    if(m_input_context->streams[id]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      if(m_audio_stream.id == AVERROR_STREAM_NOT_FOUND)
      {
        m_audio_stream.id = id;
      }

      const auto lang = av_dict_get(m_input_context->streams[id]->metadata, "language", nullptr, 0);

      if(lang)
      {
        if((strcmp(lang->value, "spa") == 0) && (m_configuration.preferredAudioLanguage() == Utils::TranscoderConfiguration::Language::SPANISH))
        {
          m_audio_stream.id = id;
          break;
        }

        if((strcmp(lang->value, "eng") == 0) && (m_configuration.preferredAudioLanguage() == Utils::TranscoderConfiguration::Language::ENGLISH))
        {
          m_audio_stream.id = id;
          break;
        }
      }
    }
  }

  if (m_audio_stream.id < 0)
  {
    emit error_message(QString("Couldn't find any suitable audio stream in '%1'.").arg(source_name));
    return false;
  }

  av_find_best_stream(m_input_context, AVMEDIA_TYPE_AUDIO, m_audio_stream.id, -1, &m_audio_stream.decoder, 0);

  if(!m_audio_stream.decoder)
  {
    emit error_message(QString("Couldn't find audio decoder for '%1'.").arg(source_name));
    return false;
  }

  m_audio_stream.decoderContext = m_input_context->streams[m_audio_stream.id]->codec;
  value = avcodec_parameters_to_context(m_audio_stream.decoderContext, m_input_context->streams[m_audio_stream.id]->codecpar);
  if(value < 0)
  {
    emit error_message(tr("Unable to copy parameters to audio decoder context for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  value = avcodec_open2(m_audio_stream.decoderContext, m_audio_stream.decoder, nullptr);
  if (value < 0)
  {
    emit error_message(QString("Couldn't open audio decoder for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  if(!avcodec_is_open(m_audio_stream.decoderContext))
  {
    emit error_message(QString("Couldn't open audio decoder for '%1'.").arg(source_name));
    return false;
  }

  m_video_stream.id = av_find_best_stream(m_input_context, AVMEDIA_TYPE_VIDEO, -1, -1, &m_video_stream.decoder, 0);
  if (m_video_stream.id < 0)
  {
    emit error_message(QString("Couldn't find any video stream in '%1'. Error is \"%2\".").arg(source_name).arg(av_error_string(m_video_stream.id)));
    return false;
  }

  if(!m_video_stream.decoder)
  {
    emit error_message(QString("Couldn't find video decoder for '%1'.").arg(source_name));
    return false;
  }

  m_video_stream.decoderContext = m_input_context->streams[m_video_stream.id]->codec;
  value = avcodec_parameters_to_context(m_video_stream.decoderContext, m_input_context->streams[m_video_stream.id]->codecpar);
  if(value < 0)
  {
    emit error_message(tr("Unable to copy parameters to video decoder context for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  m_video_stream.decoderContext->framerate = m_input_context->streams[m_video_stream.id]->avg_frame_rate;

  value = avcodec_open2(m_video_stream.decoderContext, m_video_stream.decoder, nullptr);
  if (value < 0 || !avcodec_is_open(m_video_stream.decoderContext))
  {
    emit error_message(QString("Couldn't open video decoder for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
    return false;
  }

  if(m_configuration.extractSubtitles())
  {
    for(unsigned int id = 0; id < m_input_context->nb_streams; id++)
    {
      if(m_input_context->streams[id]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
      {
        if(m_subtitle_stream.id == AVERROR_STREAM_NOT_FOUND)
        {
          m_subtitle_stream.id = id;
        }

        const auto lang = av_dict_get(m_input_context->streams[id]->metadata, "language", nullptr, 0);

        if(lang)
        {
          if((strcmp(lang->value, "spa") == 0) && (m_configuration.preferredSubtitleLanguage() == Utils::TranscoderConfiguration::Language::SPANISH))
          {
            m_subtitle_stream.id = id;
            break;
          }

          if((strcmp(lang->value, "eng") == 0) && (m_configuration.preferredSubtitleLanguage() == Utils::TranscoderConfiguration::Language::ENGLISH))
          {
            m_subtitle_stream.id = id;
            break;
          }
        }
      }
    }

    if(m_subtitle_stream.id != AVERROR_STREAM_NOT_FOUND)
    {
      av_find_best_stream(m_input_context, AVMEDIA_TYPE_SUBTITLE, m_subtitle_stream.id, -1, &m_subtitle_stream.decoder, 0);

      if(!m_subtitle_stream.decoder)
      {
        emit error_message(QString("Couldn't find a suitable subtitle decoder for '%1'.").arg(source_name));
        return false;
      }

      m_subtitle_stream.decoderContext = m_input_context->streams[m_subtitle_stream.id]->codec;
      value = avcodec_parameters_to_context(m_subtitle_stream.decoderContext, m_input_context->streams[m_subtitle_stream.id]->codecpar);
      if(value < 0)
      {
        emit error_message(tr("Unable to copy parameters to subtitle decoder context for '%1'. Error is \"%2\"").arg(source_name).arg(av_error_string(value)));
        return false;
      }
    }
  }

  m_packet = av_packet_alloc();
  av_init_packet(m_packet);
  m_frame  = av_frame_alloc();

  m_packet->data = nullptr;
  m_packet->size = 0;

  return true;
}

//--------------------------------------------------------------------
void Worker::deinit_libav()
{
  if(m_input_file.isOpen())
  {
    m_input_file.close();
  }

  if(m_input_context)
  {
    if(m_input_context->pb && m_input_context->pb->buffer)
    {
      av_free(m_input_context->pb->buffer);
      m_input_context->pb->buffer = nullptr;
    }
    avformat_close_input(&m_input_context);
  }

  if(m_output_context && !m_fail)
  {
    int value;
    if((value = av_write_trailer(m_output_context)) != 0)
    {
      const auto filename = QString::fromStdWString(m_source_info.wstring());
      emit error_message(tr("Unable to write trailer for video file for '%1' Error: %2.").arg(filename).arg(av_error_string(value)));
    }

    if (!(m_output_context->oformat->flags & AVFMT_NOFILE))
    {
      if((value = avio_close(m_output_context->pb)) < 0)
      {
        const auto filename = QString::fromStdWString(m_source_info.wstring());
        emit error_message(tr("Unable to close video file for '%1' Error: %2.").arg(filename).arg(av_error_string(value)));
      }
    }
  }

  if(m_output_subtitle_context && !m_fail)
  {
    int value;
    if((value = av_write_trailer(m_output_subtitle_context)) != 0)
    {
      const auto filename = QString::fromStdWString(m_source_info.wstring());
      emit error_message(tr("Unable to write trailer for subtitle file for '%1' Error: %2.").arg(filename).arg(av_error_string(value)));
    }

    if (!(m_output_subtitle_context->oformat->flags & AVFMT_NOFILE))
    {
      if((value = avio_close(m_output_subtitle_context->pb)) < 0)
      {
        const auto filename = QString::fromStdWString(m_source_info.wstring());
        emit error_message(tr("Unable to close subtitle file for '%1' Error: %2.").arg(filename).arg(av_error_string(value)));
      }
    }
  }

  for(auto context: {m_input_context, m_output_context, m_output_subtitle_context})
  {
    if(context) avformat_free_context(context);
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
  std::cout << "needs audio" << std::endl;

  if(m_audio_stream.id == AVERROR_STREAM_NOT_FOUND) return false;

  auto stream = m_input_context->streams[m_audio_stream.id];
  if(stream->codecpar->channels != m_configuration.audioChannelsNum())
    return true;

  switch(m_configuration.videoCodec())
  {
    case Utils::TranscoderConfiguration::VideoCodec::VP8:
    case Utils::TranscoderConfiguration::VideoCodec::VP9:
      return stream->codecpar->codec_id != AV_CODEC_ID_VORBIS;
      break;
    case Utils::TranscoderConfiguration::VideoCodec::H264:
    case Utils::TranscoderConfiguration::VideoCodec::H265:
      return stream->codecpar->codec_id != AV_CODEC_ID_AAC;
      break;
    default:
      break;
  }

  return true;
}

//-----------------------------------------------------------------
bool Worker::needsVideoProcessing() const
{
  std::cout << "needs video" << std::endl;
  if(m_video_stream.id == AVERROR_STREAM_NOT_FOUND) return false;

  return (m_input_context->streams[m_video_stream.id]->codecpar->codec_id != videoCodecId());
}

//-----------------------------------------------------------------
bool Worker::needsSubtitleProcessing() const
{
  std::cout << "needs subtitles" << std::endl;
  return (m_subtitle_stream.id != AVERROR_STREAM_NOT_FOUND && m_configuration.extractSubtitles());
}

//-----------------------------------------------------------------
bool Worker::create_output()
{
  std::cout << "create output enter" << std::endl;

  QMutexLocker lock(&Utils::s_mutex);

  if(needsAudioProcessing() || needsVideoProcessing())
  {
    const auto filename = QString::fromStdWString(m_source_info.wstring() + outputExtension());
    auto format = av_guess_format(nullptr, filename.toStdString().c_str(), nullptr);
    if(!format)
    {
      emit error_message(tr("Unable to guess format for output file: '%1'.").arg(filename));
      return false;
    }

    m_output_context = avformat_alloc_context();
    if(!m_output_context)
    {
      emit error_message(tr("Unable to allocate output context for file: '%1'.").arg(filename));
      return false;
    }

    m_output_context->oformat = format;
    m_output_context->oformat->flags |= AVFMT_VARIABLE_FPS;
    m_output_context->subtitle_codec_id = AV_CODEC_ID_NONE;
    strcpy(m_output_context->filename, filename.toStdString().c_str());

    if(needsVideoProcessing())
    {
      std::cout << "needs video processing" << std::endl;
      m_video_stream.name = "video";
      m_video_stream.output_file = m_output_context;

      m_output_context->video_codec_id = m_output_context->oformat->video_codec = videoCodecId();

      m_video_stream.encoder = avcodec_find_encoder(videoCodecId());
      if(!m_video_stream.encoder)
      {
        emit error_message(tr("Unable to find video encoder for file '%1'.").arg(filename));
        return false;
      }

      m_video_stream.stream = avformat_new_stream(m_output_context, m_video_stream.encoder);
      if(!m_video_stream.stream)
      {
        emit error_message(tr("Error creating video stream for file '%1'.").arg(filename));
        return false;
      }

      m_video_stream.stream->codec = avcodec_alloc_context3(m_video_stream.encoder);
      if(!m_video_stream.stream->codec)
      {
        emit error_message(tr("Unable to allocate video encoder context for file '%1'.").arg(filename));
        return false;
      }

      m_video_stream.encoderContext = m_video_stream.stream->codec;

      const auto inputStream = m_input_context->streams[m_video_stream.id];
      m_video_stream.encoderContext->time_base           = inputStream->time_base;
      m_video_stream.encoderContext->width               = inputStream->codec->width;
      m_video_stream.encoderContext->height              = inputStream->codec->height;
      m_video_stream.encoderContext->sample_aspect_ratio = inputStream->codec->sample_aspect_ratio;
      m_video_stream.encoderContext->framerate           = inputStream->avg_frame_rate;
      m_video_stream.encoderContext->pix_fmt             = m_video_stream.encoder->pix_fmts[0];
      m_video_stream.encoderContext->bit_rate            = inputStream->codec->bit_rate * 0.9;
      m_video_stream.encoderContext->refcounted_frames   = 0;

      m_video_stream.stream->duration  = inputStream->duration;
      m_video_stream.stream->time_base = inputStream->time_base;
      m_video_stream.stream->avg_frame_rate = inputStream->avg_frame_rate;

      if(m_video_stream.encoderContext->bit_rate == 0)
      {
        m_video_stream.encoderContext->bit_rate = 1500000;
      }

      // some formats want stream headers to be separate
      if (format->flags & AVFMT_GLOBALHEADER) m_video_stream.encoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

      AVDictionary *dictionary = nullptr;
      av_dict_set(&dictionary, "threads", "auto", 0);

      auto value = avcodec_open2(m_video_stream.stream->codec, m_video_stream.encoder, &dictionary);
      if(value < 0)
      {
        emit error_message(tr("Error opening video context for file '%1'. Error: %2.").arg(filename).arg(av_error_string(value)));
        return false;
      }

      value = avcodec_parameters_from_context(m_video_stream.stream->codecpar, m_video_stream.encoderContext);
      if(value < 0)
      {
        emit error_message(tr("Error copying parameters from video context. Error: %1.").arg(av_error_string(value)));
        return false;
      }

      if(!init_video_filters()) return false;
    }
    else
    {
      std::cout << "dont need video processing codec -> " << m_input_context->streams[m_video_stream.id]->codecpar->codec_id << std::endl;
      m_output_context->video_codec_id = m_input_context->streams[m_video_stream.id]->codecpar->codec_id;
      m_output_context->oformat->video_codec = m_output_context->video_codec_id;

      m_video_stream.stream = avformat_new_stream(m_output_context, nullptr);
      if(!m_video_stream.stream)
      {
        emit information_message(tr("Unable to create video stream to copy for file '%1'.").arg(filename));
        return false;
      }

      auto value = avcodec_parameters_copy(m_video_stream.stream->codecpar, m_input_context->streams[m_video_stream.id]->codecpar);
      if(value < 0)
      {
        emit information_message(tr("Unable to copy the input parameters for output video stream for file '%1'.").arg(filename));
        return false;
      }
    }

    if(needsAudioProcessing())
    {
      std::cout << "needs audio processing" << std::endl;
      m_audio_stream.name = "audio";
      m_audio_stream.output_file = m_output_context;

      m_output_context->audio_codec_id = m_output_context->oformat->audio_codec = audioCodecId();

      m_audio_stream.encoder = avcodec_find_encoder(audioCodecId());
      if(!m_audio_stream.encoder)
      {
        emit error_message(tr("Unable to find audio encoder for file '%1'.").arg(filename));
        return false;
      }

      m_audio_stream.stream = avformat_new_stream(m_output_context, m_audio_stream.encoder);
      if(!m_audio_stream.stream)
      {
        emit error_message(tr("Error creating audio stream for file '%1'.").arg(filename));
        return false;
      }

      m_audio_stream.stream->codec = avcodec_alloc_context3(m_audio_stream.encoder);
      if(!m_audio_stream.stream->codec)
      {
        emit error_message(tr("Unable to allocate audio encoder context for file '%1'.").arg(filename));
        return false;
      }

      m_audio_stream.encoderContext = m_audio_stream.stream->codec;

      // some formats want stream headers to be separate
      if (format->flags & AVFMT_GLOBALHEADER) m_audio_stream.encoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

      const auto inputStream = m_input_context->streams[m_audio_stream.id];

      m_audio_stream.encoderContext->sample_fmt         = m_audio_stream.encoder->sample_fmts[0];
      m_audio_stream.encoderContext->sample_rate        = inputStream->codec->sample_rate;
      m_audio_stream.encoderContext->channels           = std::min(m_configuration.audioChannelsNum(), inputStream->codec->channels);
      m_audio_stream.encoderContext->channel_layout     = av_get_default_channel_layout(m_audio_stream.encoderContext->channels);
      m_audio_stream.encoderContext->bit_rate           = inputStream->codec->bit_rate;
      m_audio_stream.encoderContext->bit_rate_tolerance = inputStream->codec->bit_rate_tolerance;
      m_audio_stream.encoderContext->time_base          = AVRational{1,m_audio_stream.encoderContext->sample_rate};
      //m_audio_stream.encoderContext->time_base          = AVRational{m_audio_stream.encoderContext->frame_size,m_audio_stream.encoderContext->sample_rate};

      m_audio_stream.stream->duration = (inputStream->duration * m_input_context->streams[m_audio_stream.id]->time_base.num * m_audio_stream.encoderContext->time_base.den)/(m_input_context->streams[m_audio_stream.id]->time_base.den);
      m_audio_stream.stream->time_base = m_audio_stream.encoderContext->time_base;

      AVDictionary *dictionary = nullptr;
      av_dict_set(&dictionary, "threads", "auto", 0);
      if(audioCodecId() == AV_CODEC_ID_AAC) av_dict_set(&dictionary, "strict", "experimental", 0);

      auto value = avcodec_open2(m_audio_stream.encoderContext, m_audio_stream.encoder, &dictionary);
      if(value < 0)
      {
        emit error_message(tr("Error opening audio context for file '%1'. Error: %2.").arg(filename).arg(av_error_string(value)));
        return false;
      }

      value = avcodec_parameters_from_context(m_audio_stream.stream->codecpar, m_audio_stream.encoderContext);
      if(value < 0)
      {
        emit error_message(tr("Error copying parameters from audio context. Error: %1.").arg(av_error_string(value)));
        return false;
      }

      if(!init_audio_filters()) return false;
    }
    else
    {
      std::cout << "dont need audio processing codec " << m_input_context->streams[m_audio_stream.id]->codecpar->codec_id << std::endl;
      m_output_context->audio_codec_id = m_input_context->streams[m_audio_stream.id]->codecpar->codec_id;
      m_output_context->oformat->audio_codec = m_output_context->audio_codec_id;

      m_audio_stream.stream = avformat_new_stream(m_output_context, nullptr);
      if(!m_audio_stream.stream)
      {
        emit information_message(tr("Unable to create audio stream to copy for file '%1'.").arg(filename));
        return false;
      }

      auto value = avcodec_parameters_copy(m_audio_stream.stream->codecpar, m_input_context->streams[m_audio_stream.id]->codecpar);
      if(value < 0)
      {
        emit information_message(tr("Unable to copy the input parameters for output audio stream for file '%1'.").arg(filename));
        return false;
      }
    }

    /* open the output file, if needed */
    if (!(format->flags & AVFMT_NOFILE))
    {
      const auto value = avio_open(&m_output_context->pb, filename.toStdString().c_str(), AVIO_FLAG_WRITE);
      if (value < 0)
      {
        emit error_message(tr("Error opening output file '%1'. Error: %2.").arg(filename).arg(av_error_string(value)));
        return false;
      }
    }

    const auto value = avformat_write_header(m_output_context, nullptr);
    if(value < 0)
    {
      emit error_message(tr("Unable to write header of file '%1'.").arg(filename));
      return false;
    }
  }

  if(needsSubtitleProcessing())
  {
    const auto filename = QString::fromStdWString(m_source_info.wstring() + SUBTITLE_EXTENSION);
    auto format = av_guess_format(nullptr, filename.toStdString().c_str(), nullptr);
    if(!format)
    {
      emit error_message(tr("Unable to guess format for output file: '%1'.").arg(filename));
      return false;
    }

    m_output_subtitle_context = avformat_alloc_context();
    if(!m_output_subtitle_context)
    {
      emit error_message(tr("Unable to allocate output context for file: '%1'.").arg(filename));
      return false;
    }

    m_subtitle_stream.name = "subtitle";
    m_subtitle_stream.output_file = m_output_subtitle_context;

    m_output_subtitle_context->oformat = format;
    strcpy(m_output_subtitle_context->filename, filename.toStdString().c_str());

    m_subtitle_stream.encoder = avcodec_find_encoder(AV_CODEC_ID_SRT);
    if(!m_subtitle_stream.encoder)
    {
      emit error_message(tr("Unable to find subtitle encoder for file '%1'.").arg(filename));
      return false;
    }

    m_subtitle_stream.encoderContext = avcodec_alloc_context3(m_subtitle_stream.encoder);
    if(!m_subtitle_stream.encoderContext)
    {
      emit error_message(tr("Unable to allocate subtitle encoder context for file '%1'.").arg(filename));
      return false;
    }

    m_subtitle_stream.encoderContext->time_base = (AVRational){1, 1000};

    // some formats want stream headers to be separate
    if (format->flags & AVFMT_GLOBALHEADER) m_subtitle_stream.encoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    AVDictionary *dictionary = nullptr;
    av_dict_set(&dictionary, "threads", "auto", 0);

    auto value = avcodec_open2(m_subtitle_stream.encoderContext, m_subtitle_stream.encoder, &dictionary);
    if(value < 0)
    {
      emit error_message(tr("Error opening subtitle context for file '%1'. Error: %2.").arg(filename).arg(av_error_string(value)));
      return false;
    }

    m_subtitle_stream.stream = avformat_new_stream(m_output_subtitle_context, m_subtitle_stream.encoder);
    if(!m_subtitle_stream.stream) return false;

    auto paramsV = avcodec_parameters_alloc();

    value = avcodec_parameters_from_context(paramsV, m_subtitle_stream.encoderContext);
    if(value < 0)
    {
      emit error_message(tr("Error copying parameters from subtitle context. Error: %1.").arg(av_error_string(value)));
      return false;
    }
    m_subtitle_stream.stream->codecpar = paramsV;

    /* open the output file, if needed */
    if (!(format->flags & AVFMT_NOFILE))
    {
      const auto value = avio_open(&m_output_subtitle_context->pb, filename.toStdString().c_str(), AVIO_FLAG_WRITE);
      if (value < 0)
      {
        emit error_message(tr("Error opening subtitle output file '%1'. Error: %2.").arg(filename).arg(av_error_string(value)));
        return false;
      }
    }

    value = avformat_write_header(m_output_subtitle_context, nullptr);
    if(value < 0)
    {
      emit error_message(tr("Unable to write header of file '%1'.").arg(filename));
      return false;
    }

  }
  std::cout << "create output exit" << std::endl;
  return true;
}

//-----------------------------------------------------------------------------
void Worker::log_callback(void *ptr, int level, const char *fmt, va_list vl)
{
  char buffer[1024];
  const auto length = vsprintf(buffer, fmt, vl);
  std::cout << "libav log level " << level << " -> " << std::string(buffer, length) << std::endl;
}

//-----------------------------------------------------------------------------
std::wstring Worker::outputExtension() const
{
  std::wstring result;

  switch(m_configuration.videoCodec())
  {
    case Utils::TranscoderConfiguration::VideoCodec::VP8:
      return L".vp8";
      break;
    case Utils::TranscoderConfiguration::VideoCodec::VP9:
      return L".vp9";
      break;
    case Utils::TranscoderConfiguration::VideoCodec::H264:
    case Utils::TranscoderConfiguration::VideoCodec::H265:
      return L".mp4";
      break;
    default:
      break;
  }

  emit error_message(tr("Invalid video codec"));

  return L".invalid";
}

//-----------------------------------------------------------------------------
AVCodecID Worker::audioCodecId() const
{
  switch(m_configuration.audioCodec())
  {
    case Utils::TranscoderConfiguration::AudioCodec::AAC:
      return AV_CODEC_ID_AAC;
      break;
    case Utils::TranscoderConfiguration::AudioCodec::MP3:
      return AV_CODEC_ID_MP3;
      break;
    case Utils::TranscoderConfiguration::AudioCodec::VORBIS:
      return AV_CODEC_ID_VORBIS;
      break;
    default:
      break;
  }

  emit error_message(tr("Invalid audio codec"));

  return AV_CODEC_ID_NONE;
}

//-----------------------------------------------------------------------------
AVCodecID Worker::videoCodecId() const
{
  switch(m_configuration.videoCodec())
  {
    case Utils::TranscoderConfiguration::VideoCodec::H264:
      return AV_CODEC_ID_H264;
      break;
    case Utils::TranscoderConfiguration::VideoCodec::H265:
      return AV_CODEC_ID_HEVC;
      break;
    case Utils::TranscoderConfiguration::VideoCodec::VP8:
      return AV_CODEC_ID_VP8;
      break;
    case Utils::TranscoderConfiguration::VideoCodec::VP9:
      return AV_CODEC_ID_VP9;
      break;
    default:
      break;
  }

  emit error_message(tr("Invalid video codec"));

  return AV_CODEC_ID_NONE;
}

//-----------------------------------------------------------------------------
bool Worker::process_stream_packet(Stream &stream)
{
  // Try to decode the packet into a frame/multiple frames.
  auto result = avcodec_send_packet(stream.decoderContext, m_packet);

  if(result == AVERROR(EAGAIN))
  {
    // shouldn't happen as we flush the decoder after sending a packet.
  }
  else if(result < 0)
  {
    const auto filename = QString::fromStdWString(m_source_info.wstring());
    emit error_message(tr("Error sending packet to %1 decoder. Input file '%2'. Error: %3").arg(stream.name).arg(filename).arg(av_error_string(result)));
    return false;
  }

  while(0 == (result = avcodec_receive_frame(stream.decoderContext, m_frame)))
  {
    if(stream.infilter == nullptr)
    {
      auto value = avcodec_send_frame(stream.encoderContext, m_frame);
      if(value < 0)
      {
        const auto filename = QString::fromStdWString(m_source_info.wstring());
        emit error_message(tr("Error sending frame to %1 encoder. Input file '%2'. Error: %3").arg(stream.name).arg(filename).arg(av_error_string(result)));
        return false;
      }

      continue;
    }

    auto value = av_buffersrc_add_frame(stream.infilter, m_frame);
    if(value < 0)
    {
      const auto filename = QString::fromStdWString(m_source_info.wstring());
      emit error_message(tr("Error sending frame to %1 buffer. Input file '%2'. Error: %3").arg(stream.name).arg(filename).arg(av_error_string(result)));
      return false;
    }

    while(true)
    {
      if(stream.encoderContext->frame_size != 0 && m_packet)
      {
        value = av_buffersink_get_samples(stream.outfilter, m_frame, stream.encoderContext->frame_size);
      }
      else
      {
        value = av_buffersink_get_frame(stream.outfilter, m_frame);
      }

      if(value < 0)
      {
        if ((value == AVERROR(EAGAIN)) || (value == AVERROR_EOF)) break;

        const auto filename = QString::fromStdWString(m_source_info.wstring());
        emit error_message(tr("Error receiving frame from %1 buffer sink. Input file '%2'. Error: %3").arg(stream.name).arg(filename).arg(av_error_string(result)));
        return false;
      }

      value = avcodec_send_frame(stream.encoderContext, m_frame);
      if(value < 0 && value != AVERROR(EAGAIN))
      {
        const auto filename = QString::fromStdWString(m_source_info.wstring());
        emit error_message(tr("Error sending frame to %1 encoder. Input file '%2'. Error: %3").arg(stream.name).arg(filename).arg(av_error_string(result)));
        return false;
      }

      while(0 == (value = avcodec_receive_packet(stream.encoderContext, m_packet)))
      {
        if(m_packet)
        {
          m_packet->pts = m_packet->dts = stream.pts;

          if (m_audio_stream.stream->index == stream.stream->index)
          {
            m_packet->duration = stream.encoderContext->frame_size;
          }
          else
          {
            m_packet->duration = 1;
          }

          stream.pts += m_packet->duration;

          if (m_packet->pts != static_cast<long long>(AV_NOPTS_VALUE))
            m_packet->pts =  av_rescale_q(m_packet->pts, stream.stream->codec->time_base, stream.stream->time_base);
          if (m_packet->dts != static_cast<long long>(AV_NOPTS_VALUE))
            m_packet->dts = av_rescale_q(m_packet->dts, stream.stream->codec->time_base, stream.stream->time_base);

          m_packet->stream_index = stream.stream->index;
        }

        value = av_interleaved_write_frame(stream.output_file, m_packet);
        if (value < 0)
        {
          const auto filename = QString::fromStdWString(m_source_info.wstring());
          emit error_message(tr("Error writing packet to output for %1 encoder. Input file '%2'. Error: %3").arg(stream.name).arg(filename).arg(av_error_string(result)));
          return false;
        }
      }

      if (value < 0 && value != AVERROR(EAGAIN))
      {
        const auto filename = QString::fromStdWString(m_source_info.wstring());
        emit error_message(tr("Error receiving packet from %1 encoder. Input file '%2'. Error: %3").arg(stream.name).arg(filename).arg(av_error_string(result)));
        return false;
      }
    }
  }

  if(result < 0 && result != AVERROR_EOF && result != AVERROR(EAGAIN))
  {
    const auto filename = QString::fromStdWString(m_source_info.wstring());
    emit error_message(QString("Error reading frame from %1 stream. input file '%2. Error: %3").arg(stream.name).arg(filename).arg(av_error_string(result)));
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Worker::init_audio_filters()
{
  const auto filename = QString::fromStdString(m_source_info.stem().string());

  /* Create a new filtergraph, which will contain all the filters. */
  m_audio_stream.filter_graph = avfilter_graph_alloc();
  if (!m_audio_stream.filter_graph)
  {
    emit error_message(tr("Unable to allocate audio filter graph for file '%1'.").arg(filename));
    return false;
  }

  /* Create the abuffer filter used for feeding the data into the graph. */
  auto abuffer = avfilter_get_by_name("abuffer");
  if (!abuffer)
  {
    emit error_message(tr("Unable to allocate audio filter buffer for file '%1'.").arg(filename));
    return false;
  }

  m_audio_stream.infilter = avfilter_graph_alloc_filter(m_audio_stream.filter_graph, abuffer, "src");
  if (!m_audio_stream.infilter)
  {
    emit error_message(tr("Unable to allocate audio filter buffer for file '%1'.").arg(filename));
    return false;
  }

  char buffer[256];
  av_get_channel_layout_string(buffer, sizeof(buffer), m_audio_stream.decoderContext->channels, av_get_default_channel_layout(m_audio_stream.decoderContext->channels));

  QString bufferParams = tr("sample_fmt=%1:time_base=%2/%3:sample_rate=%4:channel_layout=%5")
      .arg(QString::fromLatin1(av_get_sample_fmt_name(m_audio_stream.decoderContext->sample_fmt)))
      .arg(m_audio_stream.decoderContext->time_base.num).arg(m_audio_stream.decoderContext->time_base.den)
      .arg(m_audio_stream.decoderContext->sample_rate)
      .arg(QString::fromLatin1(buffer));

  /* Now initialize the filter. */
  auto value = avfilter_init_str(m_audio_stream.infilter, bufferParams.toStdString().c_str());
  if (value < 0)
  {
    emit error_message(tr("Unable to initialize audio filter buffer context for file '%1'.").arg(filename));
    return false;
  }

  /* Create the aformat filter it ensures that the output is of the format we want. */
  auto aformat = avfilter_get_by_name("aformat");
  if (!aformat)
  {
    emit error_message(tr("Unable to allocate audio filter format for file '%1'.").arg(filename));
    return false;
  }

  auto aformat_ctx = avfilter_graph_alloc_filter(m_audio_stream.filter_graph, aformat, "aformat");
  if (!aformat_ctx)
  {
    emit error_message(tr("Unable to allocate audio filter format context for file '%1'.").arg(filename));
    return false;
  }

  av_get_channel_layout_string(buffer, sizeof(buffer), m_audio_stream.encoderContext->channels, av_get_default_channel_layout(m_audio_stream.encoderContext->channels));

  QString formatParams = tr("sample_fmts=%1:sample_rates=%2:channel_layouts=%3")
      .arg(QString::fromLatin1(av_get_sample_fmt_name(m_audio_stream.encoderContext->sample_fmt)))
      .arg(m_audio_stream.decoderContext->sample_rate)
      .arg(QString::fromLatin1(buffer));

  value = avfilter_init_str(aformat_ctx, formatParams.toStdString().c_str());
  if (value < 0)
  {
    emit error_message(tr("Unable to initialize audio filter format context for file '%1'.").arg(filename));
    return false;
  }

  /* Finally create the abuffersink filter used to get the filtered data out of the graph. */
  auto abuffersink = avfilter_get_by_name("abuffersink");
  if (!abuffersink)
  {
    emit error_message(tr("Unable to allocate audio filter buffersink for file '%1'.").arg(filename));
    return false;
  }

  m_audio_stream.outfilter = avfilter_graph_alloc_filter(m_audio_stream.filter_graph, abuffersink, "sink");
  if (!m_audio_stream.outfilter)
  {
    emit error_message(tr("Unable to allocate audio filter buffersink context for file '%1'.").arg(filename));
    return false;
  }

  /* This filter takes no options. */
  value = avfilter_init_str(m_audio_stream.outfilter, nullptr);
  if (value < 0)
  {
    emit error_message(tr("Unable to initialize audio filter buffersink for file '%1'.").arg(filename));
    return false;
  }

  /* Connect the filters;
   * in this simple case the filters just form a linear chain. */
  value = avfilter_link(m_audio_stream.infilter, 0, aformat_ctx, 0);
  if (value >= 0)
      value = avfilter_link(aformat_ctx, 0, m_audio_stream.outfilter, 0);
  if (value < 0)
  {
    emit error_message(tr("Unable to connect audio filters for file '%1'.").arg(filename));
    return false;
  }

  /* Configure the graph. */
  value = avfilter_graph_config(m_audio_stream.filter_graph, nullptr);
  if (value < 0)
  {
    emit error_message(tr("Unable to configure audio filter graph for file '%1'.").arg(filename));
    return false;
  }

  return true;
}


//-----------------------------------------------------------------------------
bool Worker::init_video_filters()
{
  const auto filename = QString::fromStdString(m_source_info.stem().string());

  /* Create a new filtergraph, which will contain all the filters. */
  m_video_stream.filter_graph = avfilter_graph_alloc();
  if (!m_video_stream.filter_graph)
  {
    emit error_message(tr("Unable to allocate video filter graph for file '%1'.").arg(filename));
    return false;
  }

  /* Create the abuffer filter used for feeding the data into the graph. */
  auto buffer = avfilter_get_by_name("buffer");
  if (!buffer)
  {
    emit error_message(tr("Unable to allocate video filter buffer for file '%1'.").arg(filename));
    return false;
  }

  m_video_stream.infilter = avfilter_graph_alloc_filter(m_video_stream.filter_graph, buffer, "src");
  if (!m_video_stream.infilter)
  {
    emit error_message(tr("Unable to allocate audio filter buffer context for file '%1'.").arg(filename));
    return false;
  }

  QString bufferParams = tr("width=%1:height=%2:pix_fmt=%3:time_base=%4/%5")
      .arg(m_video_stream.decoderContext->width).arg(m_video_stream.decoderContext->height)
      .arg(QString::fromLatin1(av_get_pix_fmt_name(m_video_stream.decoderContext->pix_fmt)))
      .arg(m_video_stream.decoderContext->time_base.num).arg(m_video_stream.decoderContext->time_base.den);

  /* Now initialize the filter. */
  auto value = avfilter_init_str(m_video_stream.infilter, bufferParams.toStdString().c_str());
  if (value < 0)
  {
    emit error_message(tr("Unable to initialize video filter buffer context for file '%1'.").arg(filename));
    return false;
  }

  /* Create the format filter it ensures that the output is of the format we want. */
  auto format = avfilter_get_by_name("format");
  if (!format)
  {
    emit error_message(tr("Unable to allocate video filter format for file '%1'.").arg(filename));
    return false;
  }

  auto format_ctx = avfilter_graph_alloc_filter(m_video_stream.filter_graph, format, "format");
  if (!format_ctx)
  {
    emit error_message(tr("Unable to allocate video filter format context for file '%1'.").arg(filename));
    return false;
  }

  QString formatParams = tr("pix_fmts=%1")
    .arg(QString::fromLatin1(av_get_pix_fmt_name(m_video_stream.encoderContext->pix_fmt)));

  value = avfilter_init_str(format_ctx, formatParams.toStdString().c_str());
  if (value < 0)
  {
    emit error_message(tr("Unable to initialize video filter format context for file '%1'.").arg(filename));
    return false;
  }

  /* Finally create the buffersink filter used to get the filtered data out of the graph. */
  auto buffersink = avfilter_get_by_name("buffersink");
  if (!buffersink)
  {
    emit error_message(tr("Unable to allocate video filter buffersink for file '%1'.").arg(filename));
    return false;
  }

  m_video_stream.outfilter = avfilter_graph_alloc_filter(m_video_stream.filter_graph, buffersink, "sink");
  if (!m_video_stream.outfilter)
  {
    emit error_message(tr("Unable to allocate video filter buffersink context for file '%1'.").arg(filename));
    return false;
  }

  /* This filter takes no options. */
  value = avfilter_init_str(m_video_stream.outfilter, nullptr);
  if (value < 0)
  {
    emit error_message(tr("Unable to initialize video filter buffersink for file '%1'.").arg(filename));
    return false;
  }

  /* Connect the filters;
   * in this simple case the filters just form a linear chain. */
  value = avfilter_link(m_video_stream.infilter, 0, format_ctx, 0);
  if (value >= 0)
      value = avfilter_link(format_ctx, 0, m_video_stream.outfilter, 0);
  if (value < 0)
  {
    emit error_message(tr("Unable to connect video filters for file '%1'.").arg(filename));
    return false;
  }

  /* Configure the graph. */
  value = avfilter_graph_config(m_video_stream.filter_graph, nullptr);
  if (value < 0)
  {
    emit error_message(tr("Unable to configure audio filter graph for file '%1'.").arg(filename));
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool Worker::flush_streams()
{
  for(auto stream: {m_audio_stream, m_video_stream, m_subtitle_stream})
  {
    av_free_packet(m_packet);
    m_packet = nullptr;

    if(stream.encoder)
    {
      process_stream_packet(stream);
      avcodec_flush_buffers(stream.decoderContext);
      avcodec_flush_buffers(stream.encoderContext);
    }
  }

  return true;
}
