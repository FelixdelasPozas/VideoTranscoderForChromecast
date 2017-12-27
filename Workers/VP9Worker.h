/*
 File: VP9Worker.h
 Created on: 27/12/2017
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

#ifndef WORKERS_VP9WORKER_H_
#define WORKERS_VP9WORKER_H_

// Project
#include <Worker.h>

/** \class VP9Worker
 * \brief Transcodes the input file into a VP9 video file.
 *
 */
class VP9Worker
: public Worker
{
  public:
    /** \brief VP9Worker class constructor.ç
     * \param[in] sourceInfo QFileInfo struct of input file.
     * \param[in] videoBitrate Output video bitrate.
     * \param[in] audioBitrate Output audio bitrate.
     *
     */
    explicit VP9Worker(const QFileInfo &sourceInfo, const int videoBitrate, const int audioBitrate);

    /** \brief VP9Worker class virtual destructor.
     *
     */
    virtual ~VP9Worker()
    {}

  protected:
    /** \brief Implements the transcoding process to the format.
     *
     */
    virtual void run_implementation();

  private:
    /** \brief Returns the output file extension as a QString.
     *
     */
    virtual const QString outputExtension() const;
};

#endif // WORKERS_VP9WORKER_H_
