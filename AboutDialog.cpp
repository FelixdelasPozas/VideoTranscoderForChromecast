/*
 File: AboutDialog.cpp
 Created on: 01/12/2017
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
#include <AboutDialog.h>
#include "version.h"

// Qt
#include <QEvent>
#include <QKeyEvent>
#include <QDateTime>

// libvpx
#include <vpx_version.h>

// libav
#include <avversion.h>

const QString AboutDialog::VERSION = QString("version 1.0.0");

//-----------------------------------------------------------------
AboutDialog::AboutDialog(QWidget *parent, Qt::WindowFlags flags)
: QDialog(parent, flags)
{
  setupUi(this);

  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & ~(Qt::WindowMaximizeButtonHint) & ~(Qt::WindowMinimizeButtonHint));

  auto compilation_date = QString(__DATE__);
  auto compilation_time = QString("(") + QString(__TIME__) + QString(")");

  m_compilationDate->setText(tr("Compiled on %1 %2 build %3").arg(compilation_date).arg(compilation_time).arg(QString::number(BUILD_NUMBER)));
  m_version->setText(VERSION);

  m_qtVersion->setText(tr("version %1").arg(qVersion()));
  m_libavVersion->setText(tr("version %1").arg(LIBAV_VERSION));
  m_libvpxVersion->setText(tr("version %1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH));

  auto now = QDateTime::currentDateTime();
  m_copyright->setText(tr("Copyright %1 Félix de las Pozas Álvarez").arg(now.date().year()));
}
