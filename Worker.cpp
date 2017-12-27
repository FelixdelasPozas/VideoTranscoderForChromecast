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

//--------------------------------------------------------------------
Worker::Worker(const QFileInfo& source_info, const int videoBitrate, const int audioBitrate)
: m_videoBitrate {videoBitrate}
, m_audioBitrate {audioBitrate}
, m_source_info  {source_info}
, m_source_path  {m_source_info.absoluteFilePath().remove(m_source_info.absoluteFilePath().split(QDir::separator()).last())}
, m_fail         {false}
, m_stop         {false}
{
}

//--------------------------------------------------------------------
void Worker::stop()
{
  emit information_message(QString("Transcoder for '%1' has been cancelled.").arg(m_source_info.absoluteFilePath()));
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
  if(check_input_file_permissions() && check_output_file_permissions())
  {
    run_implementation();
  }

  emit progress(100);
}

//--------------------------------------------------------------------
bool Worker::check_input_file_permissions()
{
  QFile file(m_source_info.absoluteFilePath());
  if(file.exists() && !file.open(QFile::ReadOnly))
  {
    emit error_message(QString("Can't open file '%1' but it exists, check for permissions.").arg(m_source_info.absoluteFilePath()));
    m_fail = true;
    return false;
  }

  file.close();
  return true;
}

//--------------------------------------------------------------------
bool Worker::check_output_file_permissions()
{
  QFile file(m_source_path + m_source_info.baseName() + outputExtension());
  if(!file.open(QFile::WriteOnly|QFile::Truncate))
  {
    emit error_message(QString("Can't create files in '%1' path, check for permissions.").arg(m_source_info.absoluteFilePath()));
    m_fail = true;
    return false;
  }

  file.close();
  file.remove();
  return true;
}
