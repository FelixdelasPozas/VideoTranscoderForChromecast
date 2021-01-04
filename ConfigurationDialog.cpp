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
#include <QFile>
#include <QTextStream>

using namespace Utils;

//--------------------------------------------------------------------
ConfigurationDialog::ConfigurationDialog(Utils::TranscoderConfiguration& config, QWidget* parent, Qt::WindowFlags flags)
: QDialog        (parent, flags)
, m_configuration{config}
{
  setupUi(this);

  m_videoCodec->setCurrentIndex(static_cast<int>(config.videoCodec()));
  m_audioLanguage->setCurrentIndex(static_cast<int>(m_configuration.preferredAudioLanguage()));
  m_extractSubtitles->setChecked(m_configuration.extractSubtitles());
  m_subtitleLanguage->setCurrentIndex(static_cast<int>(m_configuration.preferredSubtitleLanguage()));
  m_audioChannels->setValue(m_configuration.audioChannelsNum());
  m_themeCombo->setCurrentIndex(config.visualTheme().compare("Light") == 0 ? 0 : 1);
  m_audioCodec->setEnabled(false);

  updateFormatComboBoxes();

  connect(m_videoCodec, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFormatComboBoxes()));
  connect(m_themeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeTheme(int)));
}

//--------------------------------------------------------------------
void ConfigurationDialog::accept()
{
  const auto videoIndex = m_videoCodec->currentIndex();

  m_configuration.setVideoCodec(static_cast<TranscoderConfiguration::VideoCodec>(videoIndex));

  switch(videoIndex)
  {
    case 0:
    case 1:
      m_configuration.setAudioCodec(Utils::TranscoderConfiguration::AudioCodec::VORBIS);
      break;
    case 2:
    case 3:
      m_configuration.setAudioCodec(Utils::TranscoderConfiguration::AudioCodec::AAC);
      break;
    default:
      Q_ASSERT(false);
      break;
  }

  m_configuration.setPreferredAudioLanguage(static_cast<TranscoderConfiguration::Language>(m_audioLanguage->currentIndex()));
  m_configuration.setExtractSubtitles(m_extractSubtitles->isChecked());
  m_configuration.setPreferredSubtitleLanguage(static_cast<TranscoderConfiguration::Language>(m_subtitleLanguage->currentIndex()));
  const auto theme = qApp->styleSheet();
  m_configuration.setVisualTheme(theme.isEmpty() ? "Light":"Dark");

  QDialog::accept();
}

//--------------------------------------------------------------------
void ConfigurationDialog::updateFormatComboBoxes()
{
  m_audioCodec->clear();

  switch(m_videoCodec->currentIndex())
  {
    case 0:
    case 1:
      m_audioCodec->insertItem(0, "Vorbis");
      break;
    case 2:
    case 3:
      m_audioCodec->insertItem(0, "AAC");
      break;
    default:
      Q_ASSERT(false);
      break;
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::changeTheme(int index)
{
  Utils::setApplicationTheme(index == 0 ? "Light":"Dark");
}

//--------------------------------------------------------------------
void ConfigurationDialog::reject()
{
  const QString current = qApp->styleSheet().isEmpty() ? "Light":"Dark";
  const QString theme   = m_configuration.visualTheme();

  if(theme.compare(current, Qt::CaseInsensitive) != 0)
  {
    Utils::setApplicationTheme(theme);
  }

  QDialog::reject();
}
