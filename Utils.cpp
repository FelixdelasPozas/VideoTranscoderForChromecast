/*
 File: Utils.cpp
 Created on: 17/4/2015
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
#include "Utils.h"

// Qt
#include <QSettings>
#include <QDirIterator>
#include <QTemporaryFile>
#include <QApplication>
#include <QFile>
#include <QTextStream>

// C++
#include <thread>

const std::vector<std::wstring> Utils::MOVIE_FILE_EXTENSIONS   = { L".mp4", L".avi", L".ogv", L".webm", L".mkv", L".mpg", L".mpeg" };

const QString Utils::TranscoderConfiguration::ROOT_DIRECTORY     = QObject::tr("Root directory");
const QString Utils::TranscoderConfiguration::NUMBER_OF_THREADS  = QObject::tr("Number of threads");
const QString Utils::TranscoderConfiguration::VIDEO_CODEC        = QObject::tr("Video codec");
const QString Utils::TranscoderConfiguration::VIDEO_BITRATE      = QObject::tr("Video bitrate");
const QString Utils::TranscoderConfiguration::AUDIO_CODEC        = QObject::tr("Audio codec");
const QString Utils::TranscoderConfiguration::AUDIO_BITRATE      = QObject::tr("Audio bitrate");
const QString Utils::TranscoderConfiguration::AUDIO_CHANNELS_NUM = QObject::tr("Number of channels");
const QString Utils::TranscoderConfiguration::AUDIO_LANGUAGE     = QObject::tr("Preferred audio language");
const QString Utils::TranscoderConfiguration::SUBTITLE_EXTRACT   = QObject::tr("Extract subtitles");
const QString Utils::TranscoderConfiguration::SUBTITLE_LANGUAGE  = QObject::tr("Preferred subtitle language");
const QString Utils::TranscoderConfiguration::THEME              = QObject::tr("Visual theme");

//-----------------------------------------------------------------
bool Utils::isVideoFile(const std::filesystem::path &file)
{
  if(std::filesystem::is_regular_file(file))
  {
    auto extension = file.extension().wstring();
    for(auto &c: extension) c = std::tolower(c);

    return std::find(MOVIE_FILE_EXTENSIONS.cbegin(), MOVIE_FILE_EXTENSIONS.cend(), extension) != MOVIE_FILE_EXTENSIONS.cend();
  }

  return false;
}

//-----------------------------------------------------------------
std::vector<std::filesystem::path> Utils::findFiles(const std::filesystem::path &initialDir,
                                                    const std::vector<std::wstring> &extensions,
                                                    bool with_subdirectories,
                                                    const std::function<bool (const std::filesystem::path &)> &condition)
{
  std::vector<std::filesystem::path> filesFound;

  if(!initialDir.empty() && std::filesystem::is_directory(initialDir))
  {
    for(auto &name: std::filesystem::directory_iterator(initialDir))
    {
      const auto entry = name.path();
      if(entry.filename() == "." || entry.filename() == "..") continue;

      if(with_subdirectories && std::filesystem::is_directory(entry))
      {
        auto files = findFiles(name, extensions, with_subdirectories, condition);

        if(!files.empty()) std::copy(files.begin(), files.end(), std::back_inserter(filesFound));
      }
      else
      {
        auto extension = entry.extension().wstring();
        for(auto &c: extension) c = std::tolower(c);

        if(std::find(extensions.cbegin(), extensions.cend(), extension) != extensions.cend() && condition(name))
        {
          filesFound.emplace_back(name);
        }
      }
    }
  }

  return filesFound;
}

//-----------------------------------------------------------------
Utils::TranscoderConfiguration::TranscoderConfiguration()
: m_number_of_threads             {0}
, m_videoCodec                    {VideoCodec::VP8}
, m_videoBitrate                  {1000}
, m_audioCodec                    {AudioCodec::VORBIS}
, m_audioBitrate                  {128}
, m_outputLanguage                {Language::DEFAULT}
, m_extractSubtitles              {true}
, m_audioChannels                 {2}
, m_subtitleLanguage              {Language::DEFAULT}
, m_theme                         {true}
{
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::load()
{
  QSettings settings("Felix de las Pozas Alvarez", "VideoTranscoder");

  m_root_directory    = std::filesystem::path(settings.value(ROOT_DIRECTORY, QDir::currentPath()).toString().toStdWString());
  m_number_of_threads = settings.value(NUMBER_OF_THREADS, std::thread::hardware_concurrency() /2).toInt();
  m_videoCodec        = static_cast<VideoCodec>(settings.value(VIDEO_CODEC, 0).toInt());
  m_videoBitrate      = settings.value(VIDEO_BITRATE, 0).toInt();
  m_audioCodec        = static_cast<AudioCodec>(settings.value(AUDIO_CODEC, 0).toInt());
  m_audioBitrate      = settings.value(AUDIO_BITRATE, 0).toInt();
  m_outputLanguage    = static_cast<Language>(settings.value(AUDIO_LANGUAGE, 0).toInt());
  m_extractSubtitles  = settings.value(SUBTITLE_EXTRACT, true).toBool();
  m_audioChannels     = settings.value(AUDIO_CHANNELS_NUM, 2).toInt();
  m_subtitleLanguage  = static_cast<Language>(settings.value(SUBTITLE_LANGUAGE, 0).toInt());
  m_theme             = settings.value(THEME, true).toBool();

  // go to parent or home if the saved directory no longer exists.
  m_root_directory = validDirectoryCheck(m_root_directory);
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::save() const
{
  QSettings settings("Felix de las Pozas Alvarez", "VideoTranscoder");

  settings.setValue(ROOT_DIRECTORY, QString::fromStdWString(validDirectoryCheck(m_root_directory).wstring()));
  settings.setValue(NUMBER_OF_THREADS, m_number_of_threads);
  settings.setValue(VIDEO_CODEC, static_cast<int>(m_videoCodec));
  settings.setValue(VIDEO_BITRATE, m_videoBitrate);
  settings.setValue(AUDIO_CODEC, static_cast<int>(m_audioCodec));
  settings.setValue(AUDIO_BITRATE, m_audioBitrate);
  settings.setValue(AUDIO_LANGUAGE, static_cast<int>(m_outputLanguage));
  settings.setValue(SUBTITLE_EXTRACT, m_extractSubtitles);
  settings.setValue(AUDIO_CHANNELS_NUM, m_audioChannels);
  settings.setValue(SUBTITLE_LANGUAGE, static_cast<int>(m_subtitleLanguage));
  settings.setValue(THEME, m_theme);

  settings.sync();
}

//-----------------------------------------------------------------
std::filesystem::path Utils::TranscoderConfiguration::rootDirectory() const
{
  return m_root_directory;
}

//-----------------------------------------------------------------
int Utils::TranscoderConfiguration::numberOfThreads() const
{
  return m_number_of_threads;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setRootDirectory(const std::filesystem::path& path)
{
  m_root_directory = path;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setNumberOfThreads(int value)
{
  m_number_of_threads = value;
}

//-----------------------------------------------------------------
Utils::TranscoderConfiguration::VideoCodec Utils::TranscoderConfiguration::videoCodec() const
{
  return m_videoCodec;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setVideoCodec(const VideoCodec codec)
{
  m_videoCodec = codec;
}

//-----------------------------------------------------------------
Utils::TranscoderConfiguration::AudioCodec Utils::TranscoderConfiguration::audioCodec() const
{
  return m_audioCodec;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setAudioCodec(const AudioCodec codec)
{
  m_audioCodec = codec;
}

//-----------------------------------------------------------------
int Utils::TranscoderConfiguration::videoBitrate() const
{
  return m_videoBitrate;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setVideoBitrate(const int bitrate)
{
  m_videoBitrate = bitrate;
}

//-----------------------------------------------------------------
int Utils::TranscoderConfiguration::audioBitrate() const
{
  return m_audioBitrate;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setAudioBitrate(const int bitrate)
{
  m_audioBitrate = bitrate;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setPreferredAudioLanguage(const Language language)
{
  m_outputLanguage = language;
}

//-----------------------------------------------------------------
Utils::TranscoderConfiguration::Language Utils::TranscoderConfiguration::preferredAudioLanguage() const
{
  return m_outputLanguage;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setPreferredSubtitleLanguage(const Language language)
{
  m_subtitleLanguage = language;
}

//-----------------------------------------------------------------
Utils::TranscoderConfiguration::Language Utils::TranscoderConfiguration::preferredSubtitleLanguage() const
{
  return m_subtitleLanguage;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setExtractSubtitles(const bool value)
{
  m_extractSubtitles = value;
}

//-----------------------------------------------------------------
int Utils::TranscoderConfiguration::audioChannelsNum() const
{
  return m_audioChannels;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setAudioNumberOfChannels(const int channelsNum)
{
  m_audioChannels = std::min(7, std::max(2, channelsNum));
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::extractSubtitles() const
{
  return m_extractSubtitles;
}

//-----------------------------------------------------------------
bool Utils::TranscoderConfiguration::isValid() const
{
  return (m_videoCodec == VideoCodec::VP8 && m_audioCodec == AudioCodec::VORBIS) ||
         (m_videoCodec == VideoCodec::VP9 && m_audioCodec == AudioCodec::VORBIS) ||
         (m_videoCodec == VideoCodec::H264 && m_audioCodec == AudioCodec::AAC) ||
         (m_videoCodec == VideoCodec::H265 && m_audioCodec == AudioCodec::AAC);
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setVisualTheme(const QString &theme)
{
  m_theme = (theme.compare("light", Qt::CaseInsensitive) == 0);
}

//-----------------------------------------------------------------
QString Utils::TranscoderConfiguration::visualTheme() const
{
  return (m_theme ? "Light":"Dark");
}

//-----------------------------------------------------------------
std::filesystem::path Utils::validDirectoryCheck(const std::filesystem::path& directory)
{
  auto current = directory;

  while(!std::filesystem::is_directory(current) && !current.empty() && !(current.root_directory() == current))
  {
    current = current.parent_path();
  }

  if(!std::filesystem::is_directory(current) || current.empty() || current.root_directory() == current)
  {
    current = std::filesystem::path(QDir::homePath().toStdWString());
  }

  return current;
}

//-----------------------------------------------------------------
void Utils::setApplicationTheme(const QString &theme)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  QString sheet;

  if (theme.compare("Light", Qt::CaseInsensitive) != 0)
  {
    QFile file(":qdarkstyle/style.qss");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&file);
    sheet = ts.readAll();
  }

  qApp->setStyleSheet(sheet);

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------
bool Utils::toUCS2(const std::filesystem::path &filename)
{
  if(std::filesystem::exists(filename))
  {
    QFile file(QString::fromStdWString(filename.wstring()));
    if(!file.open(QIODevice::ReadWrite)) return false;

    QString contents = file.readAll();
    file.resize(0);

    contents.prepend(QChar::ByteOrderMark);
    file.write(QByteArray::fromRawData(reinterpret_cast<const char*>(contents.constData()), contents.size()*2));
    file.flush();
    file.close();
  }

  return false;
}
