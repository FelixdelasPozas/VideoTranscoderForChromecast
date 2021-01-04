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
#include <libavfilter/avfilter.h>
}

// C++
#include <filesystem>

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
    explicit Worker(const std::filesystem::path &source_info, const Utils::TranscoderConfiguration &config);

    /** \brief Worker class virtual destructor.
     *
     */
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
    const Utils::TranscoderConfiguration &m_configuration; /** Configuration struct reference. */

  private:
    /** \struct Stream
     * \brief Contains all the variables necessary for libav stream transcoding.
     *
     */
    struct Stream
    {
      int              id;             /** stream libav id of the input stream. */
      QString          name;           /** stream name (audio/video/subtitle).  */
      AVCodec         *decoder;        /** stream decoder.                      */
      AVCodecContext  *decoderContext; /** stream decoder context.              */
      AVCodec         *encoder;        /** stream encoder.                      */
      AVCodecContext  *encoderContext; /** stream encoder context.              */
      AVStream        *stream;         /** libav stream.                        */
      AVFormatContext *output_file;    /** stream output file.                  */
      AVFilterGraph   *filter_graph;   /** stream filter graph.                 */
      AVFilterContext *infilter;       /** input filter.                        */
      AVFilterContext *outfilter;      /** output filter.                       */
      long long        pts;            /** last pts muxed.                      */
      long long        start_dts;      /** first dts.                           */
      AVRational       time_base;      /** stream time base.                    */


      /** \brief Stream struct constructor.
       *
       */
      Stream(): id{AVERROR_STREAM_NOT_FOUND}, decoder{nullptr}, decoderContext{nullptr}, encoder{nullptr},
                encoderContext{nullptr}, stream{nullptr}, output_file{nullptr}, filter_graph{nullptr},
                infilter{nullptr}, outfilter{nullptr}, pts{0}, start_dts{0}
                {};
    };

    Stream                      m_audio_stream;    /** audio stream variables.           */
    Stream                      m_video_stream;    /** video stream variables.           */
    Stream                      m_subtitle_stream; /** subtitle stream variables.        */
    QFile                       m_input_file;      /** input file handle.                */
    AVFormatContext            *m_input_context;   /** input container context.          */
    QFile                       m_output_file;     /** output file handle.               */
    AVFormatContext            *m_output_context;  /** output container context.         */
    QFile                       m_subtitle_file;   /** output subtitle file handler.     */
    AVFrame                    *m_frame;           /** libav frame (decoded data).       */
    AVPacket                   *m_packet;          /** libav packet (encoded data).      */
    const std::filesystem::path m_source_info;     /** source file information.          */

    static const int s_io_buffer_size = 16384 + AV_INPUT_BUFFER_PADDING_SIZE;

    static const QList<int> VALID_VIDEO_CODECS; /** Valid video codecs for a Chromecast valid video. */
    static const QList<int> VALID_AUDIO_CODECS; /** Valid audio codecs for a Chromecast valid audio. */

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

    /** \brief Returns the audio codec id in libav.
     *
     */
    AVCodecID audioCodecId() const;

    /** \brief Returns the video codec id in libav.
     *
     */
    AVCodecID videoCodecId() const;

    /** \brief Returns true if the input video file needs to be processed because either the video or
     * the audio is not in a correct format or the subtitles needs to be extracted.
     *
     */
    bool inputNeedsProcessing() const;

    /** \brief Opens the output context and configures it. Returns true on success and false otherwise.
     *
     */
    bool create_output();

    /** \brief libav log callback remove when release.
     *
     */
    static void log_callback(void *ptr, int level, const char *fmt, va_list vl);

    /** \brief Initializes the filters needed for audio trasncoding due to different frame sizes. Returns
     * true on success and false otherwise.
     *
     */
    bool init_audio_filters();

    /** \brief Initializes the filters needed for video transcoding. Returns true on success and false otherwise.
     *
     */
    bool init_video_filters();

    /** \brief Helper method that process a stream packet to frame and then encodes it and pushes it to output file.
     * Returns true on success and false otherwise.
     * \param[in] stream Stream to process current m_packet information.
     *
     */
    bool process_av_packet(Stream &stream);

    /** \brief Flushes all streams and finishes.
     *
     */
    bool flush_streams();

    /** \brief Writes the current packet to the given stream. Returns true on success and false otherwise.
     * \param[in] stream Stream of the packet.
     *
     */
    bool write_av_packet(Stream &stream);

    /** \brief Writes a packet of SRT data to the subtitle file. Returns true on success and false otherwise.
     *
     */
    bool write_srt_packet();

    Worker(const Worker &) = delete;
    Worker(Worker &&) = delete;
    Worker& operator=(const Worker&) = delete;

    bool            m_fail;         /** true on process success, false otherwise.            */
    bool            m_stop;         /** true if the process needs to abort, false otherwise. */
};

extern int hwaccel_lax_profile_check;
extern AVBufferRef *hw_device_ctx;

#endif // WORKER_H_
