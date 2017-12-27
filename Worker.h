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
     * \param[in] videoBitrate Output video bitrate.
     * \param[in] audioBitrate Output audio bitrate.
     *
     */
    explicit Worker(const QFileInfo &source_info, const int videoBitrate, const int audioBitrate);
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

  protected:
    const int m_videoBitrate; /** video bitrate. */
    const int m_audioBitrate; /** audio bitrate. */

  private:
    /** \brief Returns true if the input file can be read and false otherwise.
     *
     */
    bool check_input_file_permissions();

    /** \brief Returns true if the program can write in the output directory and false otherwise.
     *
     */
    bool check_output_file_permissions();

    /** \brief Returns the output file extension as a QString.
     *
     */
    virtual const QString outputExtension() const = 0;

    const QFileInfo m_source_info;  /** source file information.                             */
    const QString   m_source_path;  /** source file path.                                    */
    bool            m_fail;         /** true on process success, false otherwise.            */
    bool            m_stop;         /** true if the process needs to abort, false otherwise. */
};

#endif // WORKER_H_
