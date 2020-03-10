/*
 File: Worker.h
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

#ifndef WORKER_H_
#define WORKER_H_

// Project
#include <Utils.h>

// Qt
#include <QThread>
#include <QFileInfo>

// libav
extern "C"
{
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#include <libavutil/fifo.h>
#include <libavresample/avresample.h>
}

/** \class Worker
 * \brief Transcoder thread.
 *
 */
class Worker
: public QThread
{
    Q_OBJECT
  public:
    /** \brief Worker class constructor.
     * \param[in] source_info QFileInfo struct of the source file.
     * \param[in] config Configuration struct reference.
     *
     */
    explicit Worker(const boost::filesystem::path &source_info, const Utils::TranscoderConfiguration &config);
    virtual ~Worker()
    {}

    /** \brief Aborts the conversion process.
     *
     */
    void stop();

    /** \brief Returns true if the process has been aborted and false otherwise.
     *
     */
    bool has_been_cancelled();

    /** \brief Returns true if the process has failed to finish it's job.
     *
     */
    bool has_failed();

  signals:
    /** \brief Emits a error message signal.
     * \param[in] message error message.
     *
     */
    void error_message(const QString message) const;

    /** \brief Emits an information message signal.
     * \param[in] message information message.
     *
     */
    void information_message(const QString message) const;

    /** \brief Emits a progress signal.
     * \param[in] value progress in [0-100].
     *
     */
    void progress(int value) const;

  protected:
    virtual void run() override final;

    /** \brief Helper method to get a user-friendly description of a libav error code.
     *
     */
    QString av_error_string(const int error_number) const;

    /** \brief Returns true if the input file audio needs to be processed.
     *
     */
    bool needsAudioProcessing() const;

    /** \brief Returns true if the input file video needs to be processed.
     *
     */
    bool needsVideoProcessing() const;

    /** \brief Returns true if the input file subtitles needs to be processed.
     *
     */
    bool needsSubtitleProcessing() const;

  protected:
    const Utils::TranscoderConfiguration &m_configuration;            /** Configuration struct reference.        */
    QFile                                 m_input_file;               /** input file handle.                     */
    QFile                                 m_output_file;              /** output file handle.                    */
    AVCodec                              *m_audio_decoder;            /** input audio decoder.                   */
    AVCodec                              *m_video_decoder;            /** input video decoder.                   */
    AVCodec                              *m_subtitle_decoder;         /** input subtitle decoder.                */
    AVCodecContext                       *m_audio_decoder_context;    /** input audio decoder context.           */
    AVCodecContext                       *m_video_decoder_context;    /** input video decoder context.           */
    AVCodecContext                       *m_subtitle_decoder_context; /** input subtitle decoder context.        */
    AVFormatContext                      *m_input_context;            /** input container context.               */
    AVFrame                              *m_frame;                    /** libav frame (decoded data).            */
    int                                   m_audio_stream_id;          /** id of the audio stream in the file.    */
    int                                   m_video_stream_id;          /** id of the video stream in the file.    */
    int                                   m_subtitle_stream_id;       /** id of the subtitle stream in the file. */
    AVFormatContext                      *m_output_context;           /** output container context.              */
    AVCodecContext                       *m_audio_coder_context;      /** output audio coder context.            */
    AVCodecContext                       *m_video_coder_context;      /** output video coder context.            */
    AVCodec                              *m_audio_coder;              /** output audio decoder.                  */
    AVCodec                              *m_video_coder;              /** output video decoder.                  */
    AVPacket                             *m_packet;                   /** libav packet (encoded data).           */
    const boost::filesystem::path         m_source_info;              /** source file information.               */
    const boost::filesystem::path         m_source_path;              /** source file path.                      */

    static const int s_io_buffer_size = 16384 + AV_INPUT_BUFFER_PADDING_SIZE;

    static const QList<int> VALID_VIDEO_CODECS; /** Valid video codecs for a Chromecast valid video. */
    static const QList<int> VALID_AUDIO_CODECS; /** Valid audio codecs for a Chromecast valid audio. */

  private:
    /** \brief Returns true if the input file can be read and false otherwise.
     *
     */
    bool check_input_file_permissions();

    /** \brief Returns true if the program can write in the output directory and false otherwise.
     *
     */
    bool check_output_file_permissions();

    /** \brief Initializes libav and opens the input file with the decoders.
     *
     */
    bool init_libav();

    /** \brief De-Initializes libav and deletes all assigned memory.
     *
     */
    void deinit_libav();

    /** \brief Custom I/O read for libav, using a QFile.
     * \param[in] opaque pointer to the reader.
     * \param[in] buffer buffer to fill
     * \param[in] buffer_size buffer size.
     *
     */
    static int custom_IO_read(void *opaque, unsigned char *buffer, int buffer_size);

    /** \brief Custom I/O seek for libav, using a QFile.
     * \param[in] opaque pointer to the reader.
     * \param[in] offset seek value.
     * \param[in] whence seek direction.
     *
     */
    static long long int custom_IO_seek(void *opaque, long long int offset, int whence);

    /** \brief Returns the output file extension as a QString.
     *
     */
    virtual std::wstring outputExtension() const = 0;

    /** \brief Returns the audio codec id in libav.
     *
     */
    virtual AVCodecID audioCodecId() const = 0;

    /** \brief Returns the video codec id in libav.
     *
     */
    virtual AVCodecID videoCodecId() const = 0;

    /** \brief Returns true if the input video file needs to be processed because either the video or
     * the audio is not in a correct format or the subtitles needs to be extracted.
     *
     */
    bool inputNeedsProcessing() const;

    /** \brief Opens the output context and configures it.
     *
     */
    virtual bool create_output();

    Worker(const Worker &) = delete;
    Worker& operator=(const Worker&) = delete;

    bool            m_fail;         /** true on process success, false otherwise.            */
    bool            m_stop;         /** true if the process needs to abort, false otherwise. */

    AVStream *m_audioStream; /** output audio stream. */
    AVStream *m_videoStream; /** output video stream. */
};

/** \class H264Worker
 * \brief Transcodes the input file into a H.264 video file.
 *
 */
class H264Worker
: public Worker
{
  public:
    /** \brief H264Worker class constructor.
     * \param[in] sourceInfo QFileInfo struct of input file.
     * \param[in] config Configuration struct reference.
     *
     */
    explicit H264Worker(const boost::filesystem::path &sourceInfo, const Utils::TranscoderConfiguration &config)
    : Worker(sourceInfo, config)
    {};

    /** \brief H264Worker class virtual destructor.
     *
     */
    virtual ~H264Worker()
    {}

  private:
    virtual AVCodecID audioCodecId() const
    { return (m_configuration.audioCodec() == Utils::TranscoderConfiguration::AudioCodec::AAC ? AV_CODEC_ID_AAC : AV_CODEC_ID_MP3); }

    virtual AVCodecID videoCodecId() const
    { return AV_CODEC_ID_H264; }

    virtual std::wstring outputExtension() const
    { return L".mp4"; }
};

/** \class H265Worker
 * \brief Transcodes the input file into a H.265 video file.
 *
 */
class H265Worker
: public Worker
{
  public:
    /** \brief H265Worker class constructor.
     * \param[in] sourceInfo QFileInfo struct of input file.
     * \param[in] config Configuration struct reference.
     *
     */
    explicit H265Worker(const boost::filesystem::path &sourceInfo, const Utils::TranscoderConfiguration &config)
    : Worker(sourceInfo, config)
    {};

    /** \brief H265Worker class virtual destructor.
     *
     */
    virtual ~H265Worker()
    {}

  private:
    virtual AVCodecID audioCodecId() const
    { return (m_configuration.audioCodec() == Utils::TranscoderConfiguration::AudioCodec::AAC ? AV_CODEC_ID_AAC : AV_CODEC_ID_MP3); }

    virtual AVCodecID videoCodecId() const
    { return AV_CODEC_ID_HEVC; }

    virtual std::wstring outputExtension() const
    { return L".mp4"; }
};

/** \class VP8Worker
 * \brief Transcodes the input file into a VP8 video file.
 *
 */
class VP8Worker
: public Worker
{
  public:
    /** \brief VP8Worker class constructor.
     * \param[in] sourceInfo QFileInfo struct of input file.
     * \param[in] config Configuration struct reference.
     *
     */
    explicit VP8Worker(const boost::filesystem::path &sourceInfo, const Utils::TranscoderConfiguration &config)
    : Worker(sourceInfo, config)
    {};

    /** \brief VP8Worker class virtual destructor.
     *
     */
    virtual ~VP8Worker()
    {}

  private:
    virtual AVCodecID audioCodecId() const
    { return AV_CODEC_ID_VORBIS; }

    virtual AVCodecID videoCodecId() const
    { return AV_CODEC_ID_VP8; }

    virtual std::wstring outputExtension() const
    { return L".vp8"; }
};

/** \class VP9Worker
 * \brief Transcodes the input file into a VP9 video file.
 *
 */
class VP9Worker
: public Worker
{
  public:
    /** \brief VP9Worker class constructor.
     * \param[in] sourceInfo QFileInfo struct of input file.
     * \param[in] config Configuration struct reference.
     *
     */
    explicit VP9Worker(const boost::filesystem::path &sourceInfo, const Utils::TranscoderConfiguration &config)
    : Worker(sourceInfo, config)
    {};

    /** \brief VP8Worker class virtual destructor.
     *
     */
    virtual ~VP9Worker()
    {}

  private:
    virtual AVCodecID audioCodecId() const
    { return AV_CODEC_ID_VORBIS; }

    virtual AVCodecID videoCodecId() const
    { return AV_CODEC_ID_VP9; }

    virtual std::wstring outputExtension() const
    { return L".vp9"; }
};

#endif // WORKER_H_
