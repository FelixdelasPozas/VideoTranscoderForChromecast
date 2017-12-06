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
#include <Utils.h>

// Qt
#include <QKeyEvent>
#include <QEvent>
#include <QSettings>
#include <QDir>
#include <QFileDialog>

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

  m_configuration.load();
}

//--------------------------------------------------------------------
VideoTranscoder::~VideoTranscoder()
{
  m_configuration.save();
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
  connect(m_directoryButton, SIGNAL(pressed()), this, SLOT(onDirectoryButtonPressed()));
}

//--------------------------------------------------------------------
void VideoTranscoder::onDirectoryButtonPressed()
{
  QFileDialog fileBrowser;
  fileBrowser.setDirectory(Utils::validDirectoryCheck(m_directoryText->text()));
  fileBrowser.setWindowTitle("Select root directory");
  fileBrowser.setFileMode(QFileDialog::Directory);
  fileBrowser.setOption(QFileDialog::DontUseNativeDialog, false);
  fileBrowser.setOption(QFileDialog::ShowDirsOnly);
  fileBrowser.setViewMode(QFileDialog::List);
  fileBrowser.setWindowIcon(QIcon(":/MusicTranscoder/folder.ico"));

  if(fileBrowser.exec() == QDialog::Accepted)
  {
    auto newDirectory = QDir::toNativeSeparators(fileBrowser.selectedFiles().first());
    m_configuration.setRootDirectory(newDirectory);
    m_directoryText->setText(newDirectory);
  }
}
