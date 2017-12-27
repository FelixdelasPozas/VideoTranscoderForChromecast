/*
 File: H264Worker.h
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

#ifndef WORKERS_H264WORKER_H_
#define WORKERS_H264WORKER_H_

// Project
#include <Worker.h>

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
     * \param[in] videoBitrate Output video bitrate.
     * \param[in] audioBitrate Output audio bitrate.
     *
     */
    explicit H264Worker(const QFileInfo &sourceInfo, const int videoBitrate, const int audioBitrate);

    /** \brief H264Worker class virtual destructor.
     *
     */
    virtual ~H264Worker()
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

#endif // WORKERS_H264WORKER_H_
