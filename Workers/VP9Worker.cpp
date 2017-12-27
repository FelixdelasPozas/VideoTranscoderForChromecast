/*
 File: VP9Worker.cpp
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
#include <Workers/VP9Worker.h>

//--------------------------------------------------------------------
VP9Worker::VP9Worker(const QFileInfo& sourceInfo, const int videoBitrate, const int audioBitrate)
: Worker{sourceInfo, videoBitrate, audioBitrate}
{
}

//--------------------------------------------------------------------
void VP9Worker::run_implementation()
{
}

//--------------------------------------------------------------------
const QString VP9Worker::outputExtension() const
{
  return ".vp9";
}

