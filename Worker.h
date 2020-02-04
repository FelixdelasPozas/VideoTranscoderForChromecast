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
#include <libavcodec/avcodec.h>
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
    explicit Worker(const QFileInfo &source_info, const Utils::TranscoderConfiguration &config);
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

    /** \brief Implements the transcoding process to the format.
     *
     */
    virtual void run_implementation() = 0;

    /** \brief Helper method to get a user-friendly description of a libav error code.
     *
     */
    QString av_error_string(const int error_number) const;

  protected:
    int                                   m_videoBitrate;             /** output file video bitrate.             */
    int                                   m_audioBitrate;             /** output file audio bitrate.             */
    const Utils::TranscoderConfiguration &m_configuration;            /** Configuration struct reference.        */
    QFile                                 m_input_file;               /** input file handle.                     */
    QFile                                 m_output_file;              /** output file handle.                    */
    AVCodec                              *m_audio_decoder;            /** libav audio decoder.                   */
    AVCodec                              *m_video_decoder;            /** libav video decoder.                   */
    AVCodec                              *m_subtitle_decoder;         /** libav subtitle decoder.                */
    AVCodecContext                       *m_audio_decoder_context;    /** libav audio decoder context.           */
    AVCodecContext                       *m_video_decoder_context;    /** libav video decoder context.           */
    AVCodecContext                       *m_subtitle_decoder_context; /** libav subtitle decoder context.        */
    AVFrame                              *m_frame;                    /** libav frame (decoded data).            */
    int                                   m_audio_stream_id;          /** id of the audio stream in the file.    */
    int                                   m_video_stream_id;          /** id of the video stream in the file.    */
    int                                   m_subtitle_stream_id;       /** id of the subtitle stream in the file. */
    AVFormatContext                      *m_libav_context;            /** libav context.                         */
    AVPacket                             *m_packet;                   /** libav packet (encoded data).           */

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
    virtual const QString outputExtension() const = 0;

    /** \brief Returns true if the input video file needs to be processed because either the video or
     * the audio is not in a correct format or the subtitles needs to be extracted.
     *
     */
    const bool inputNeedsProcessing() const;

    Worker(const Worker &) = delete;
    Worker& operator=(const Worker&) = delete;

    const QFileInfo m_source_info;  /** source file information.                             */
    const QString   m_source_path;  /** source file path.                                    */
    bool            m_fail;         /** true on process success, false otherwise.            */
    bool            m_stop;         /** true if the process needs to abort, false otherwise. */
};

#endif // WORKER_H_
