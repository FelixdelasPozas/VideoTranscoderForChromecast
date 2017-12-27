/*
 File: H264Worker.cpp
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

// Project
#include <Workers/H264Worker.h>

//--------------------------------------------------------------------
H264Worker::H264Worker(const QFileInfo& sourceInfo, const int videoBitrate, const int audioBitrate)
: Worker{sourceInfo, videoBitrate, audioBitrate}
{
}

//--------------------------------------------------------------------
void H264Worker::run_implementation()
{
}

//--------------------------------------------------------------------
const QString H264Worker::outputExtension() const
{
  return ".mp4";
}

