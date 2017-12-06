/*
 File: ConfigurationDialog.cpp
 Created on: 06/12/2017
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
#include <ConfigurationDialog.h>

// Qt
#include <QLineEdit>

using namespace Utils;

//--------------------------------------------------------------------
ConfigurationDialog::ConfigurationDialog(Utils::TranscoderConfiguration& config, QWidget* parent, Qt::WindowFlags flags)
: QDialog        {parent, flags}
, m_configuration{config}
{
  setupUi(this);

  m_videoCodec->setCurrentIndex(static_cast<int>(config.videoCodec()));

  connect(m_videoCodec, SIGNAL(currentIndexChanged(int)), this, SLOT(fillComboBoxes()));

  fillComboBoxes();

  switch(config.audioCodec())
  {
    case TranscoderConfiguration::AudioCodec::VORBIS:
      break;
    case TranscoderConfiguration::AudioCodec::MP3:
      m_audioCodec->setCurrentIndex(0);
      break;
    case TranscoderConfiguration::AudioCodec::AAC:
      m_audioCodec->setCurrentIndex(1);
      break;
    default:
      Q_ASSERT(false);
      break;
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::accept()
{
  auto videoIndex = m_videoCodec->currentIndex();

  m_configuration.setVideoCodec(static_cast<TranscoderConfiguration::VideoCodec>(videoIndex));

  switch(videoIndex)
  {
    case 0:
    case 1:
      m_configuration.setAudioCodec(Utils::TranscoderConfiguration::AudioCodec::VORBIS);
      break;
    case 2:
    case 3:
      switch(m_audioCodec->currentIndex())
      {
        case 0:
          m_configuration.setAudioCodec(Utils::TranscoderConfiguration::AudioCodec::MP3);
          break;
        case 1:
          m_configuration.setAudioCodec(Utils::TranscoderConfiguration::AudioCodec::AAC);
          break;
        default:
          Q_ASSERT(false);
          break;
      }
      break;
    default:
      Q_ASSERT(false);
      break;
  }

  m_configuration.setEmbedSubtitles(m_embedSubtitles->isChecked());

  QDialog::accept();
}

//--------------------------------------------------------------------
void ConfigurationDialog::fillComboBoxes()
{
  auto audioIndex = m_audioCodec->currentIndex();
  m_audioCodec->clear();

  switch(m_videoCodec->currentIndex())
  {
    case 0:
    case 1:
      m_audioCodec->insertItem(0, "Vorbis");
      m_audioCodec->setEnabled(false);
      break;
    case 2:
    case 3:
      m_audioCodec->insertItem(0, "MP3");
      m_audioCodec->insertItem(1, "AAC");
      if(audioIndex == 0) // changes between H.264 & H.265 shouldn't change audio index.
      {
        m_audioCodec->setEnabled(true);
      }
      else
      {
        m_audioCodec->setCurrentIndex(audioIndex);
      }
      break;
    default:
      Q_ASSERT(false);
      break;
  }
}
