/*
 File: VideoTranscoder.cpp
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
#include <VideoTranscoder.h>
#include <AboutDialog.h>

// Qt
#include <QKeyEvent>
#include <QEvent>
#include <QSettings>
#include <QDir>

// C++
#include <thread>

const QString OUTPUT_DIRECTORY  = "Output directory";
const QString PROCESSORS_NUMBER = "Processors number";

//--------------------------------------------------------------------
VideoTranscoder::VideoTranscoder()
{
  setupUi(this);

  m_threads->setMaximum(std::thread::hardware_concurrency());

  connectUI();

  loadConfiguration();
}

//--------------------------------------------------------------------
VideoTranscoder::~VideoTranscoder()
{
  saveConfiguration();
}

//--------------------------------------------------------------------
bool VideoTranscoder::event(QEvent* e)
{
  if(e->type() == QEvent::KeyPress)
  {
    auto ke = dynamic_cast<QKeyEvent *>(e);
    if(ke && ke->key() == Qt::Key_Escape)
    {
      e->accept();
      close();
      return true;
    }
  }

  return QMainWindow::event(e);
}

//--------------------------------------------------------------------
void VideoTranscoder::onAboutButtonPressed()
{
  AboutDialog dialog{this};
  dialog.exec();
}

//--------------------------------------------------------------------
void VideoTranscoder::onConfigurationButtonPressed()
{
}

//--------------------------------------------------------------------
void VideoTranscoder::onStartButtonPressed()
{
}

//--------------------------------------------------------------------
void VideoTranscoder::connectUI()
{
  connect(m_aboutButton, SIGNAL(pressed()), this, SLOT(onAboutButtonPressed()));
  connect(m_configButton, SIGNAL(pressed()), this, SLOT(onConfigurationButtonPressed()));
  connect(m_startButton, SIGNAL(pressed()), this, SLOT(onStartButtonPressed()));
}

//--------------------------------------------------------------------
void VideoTranscoder::loadConfiguration()
{
  QSettings settings("Felix de las Pozas Alvarez", "Video Transcoder");
  auto outputDir = settings.value(OUTPUT_DIRECTORY, QDir::homePath()).toString();
  auto numThreads = settings.value(PROCESSORS_NUMBER, m_threads->value()).toInt();

  m_directoryText->setText(QDir::toNativeSeparators(outputDir));
  m_threads->setValue(numThreads);
}

//--------------------------------------------------------------------
void VideoTranscoder::saveConfiguration()
{
  QSettings settings("Felix de las Pozas Alvarez", "Video Transcoder");
  settings.setValue(OUTPUT_DIRECTORY, m_directoryText->text());
  settings.setValue(PROCESSORS_NUMBER, m_threads->value());

  settings.sync();
}
