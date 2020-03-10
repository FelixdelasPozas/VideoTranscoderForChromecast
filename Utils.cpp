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

// C++
#include <thread>

// Boost
#include <boost/algorithm/string.hpp>

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

//-----------------------------------------------------------------
bool Utils::isVideoFile(const boost::filesystem::path &file)
{
  if(boost::filesystem::is_regular_file(file))
  {
    const auto extension = boost::algorithm::to_lower_copy(file.extension().wstring());

    return std::find(MOVIE_FILE_EXTENSIONS.cbegin(), MOVIE_FILE_EXTENSIONS.cend(), extension) != MOVIE_FILE_EXTENSIONS.cend();
  }

  return false;
}

//-----------------------------------------------------------------
std::vector<boost::filesystem::path> Utils::findFiles(const boost::filesystem::path &initialDir,
                                                      const std::vector<std::wstring> &extensions,
                                                      bool with_subdirectories,
                                                      const std::function<bool (const boost::filesystem::path &)> &condition)
{
  std::vector<boost::filesystem::path> filesFound;

  if(!initialDir.empty() && boost::filesystem::is_directory(initialDir))
  {
    for(boost::filesystem::directory_entry &it: boost::filesystem::directory_iterator(initialDir))
    {
      const auto name = it.path();
      if(name.filename_is_dot() || name.filename_is_dot_dot()) continue;

      if(with_subdirectories && boost::filesystem::is_directory(name))
      {
        auto files = findFiles(name, extensions, with_subdirectories, condition);

        if(!files.empty()) std::copy(files.begin(), files.end(), std::back_inserter(filesFound));
      }
      else
      {
        const auto extension = boost::algorithm::to_lower_copy(name.extension().wstring());

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
{
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::load()
{
  QSettings settings("Felix de las Pozas Alvarez", "VideoTranscoder");

  m_root_directory    = boost::filesystem::path(settings.value(ROOT_DIRECTORY, QDir::currentPath()).toString().toStdWString());
  m_number_of_threads = settings.value(NUMBER_OF_THREADS, std::thread::hardware_concurrency() /2).toInt();
  m_videoCodec        = static_cast<VideoCodec>(settings.value(VIDEO_CODEC, 0).toInt());
  m_videoBitrate      = settings.value(VIDEO_BITRATE, 0).toInt();
  m_audioCodec        = static_cast<AudioCodec>(settings.value(AUDIO_CODEC, 0).toInt());
  m_audioBitrate      = settings.value(AUDIO_BITRATE, 0).toInt();
  m_outputLanguage    = static_cast<Language>(settings.value(AUDIO_LANGUAGE, 0).toInt());
  m_extractSubtitles  = settings.value(SUBTITLE_EXTRACT, true).toBool();
  m_audioChannels     = settings.value(AUDIO_CHANNELS_NUM, 2).toInt();
  m_subtitleLanguage  = static_cast<Language>(settings.value(SUBTITLE_LANGUAGE, 0).toInt());

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

  settings.sync();
}

//-----------------------------------------------------------------
const boost::filesystem::path& Utils::TranscoderConfiguration::rootDirectory() const
{
  return m_root_directory;
}

//-----------------------------------------------------------------
int Utils::TranscoderConfiguration::numberOfThreads() const
{
  return m_number_of_threads;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setRootDirectory(const boost::filesystem::path& path)
{
  m_root_directory = path;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setNumberOfThreads(int value)
{
  m_number_of_threads = value;
}

//-----------------------------------------------------------------
const Utils::TranscoderConfiguration::VideoCodec Utils::TranscoderConfiguration::videoCodec() const
{
  return m_videoCodec;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setVideoCodec(const VideoCodec codec)
{
  m_videoCodec = codec;
}

//-----------------------------------------------------------------
const Utils::TranscoderConfiguration::AudioCodec Utils::TranscoderConfiguration::audioCodec() const
{
  return m_audioCodec;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setAudioCodec(const AudioCodec codec)
{
  m_audioCodec = codec;
}

//-----------------------------------------------------------------
const int Utils::TranscoderConfiguration::videoBitrate() const
{
  return m_videoBitrate;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setVideoBitrate(const int bitrate)
{
  m_videoBitrate = bitrate;
}

//-----------------------------------------------------------------
const int Utils::TranscoderConfiguration::audioBitrate() const
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
const Utils::TranscoderConfiguration::Language Utils::TranscoderConfiguration::preferredAudioLanguage() const
{
  return m_outputLanguage;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setPreferredSubtitleLanguage(const Language language)
{
  m_subtitleLanguage = language;
}

//-----------------------------------------------------------------
const Utils::TranscoderConfiguration::Language Utils::TranscoderConfiguration::preferredSubtitleLanguage() const
{
  return m_subtitleLanguage;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setExtractSubtitles(const bool value)
{
  m_extractSubtitles = value;
}

//-----------------------------------------------------------------
const int Utils::TranscoderConfiguration::audioChannelsNum() const
{
  return m_audioChannels;
}

//-----------------------------------------------------------------
void Utils::TranscoderConfiguration::setAudioNumberOfChannels(const int channelsNum)
{
  m_audioChannels = std::min(7, std::max(2, channelsNum));
}

//-----------------------------------------------------------------
const bool Utils::TranscoderConfiguration::extractSubtitles() const
{
  return m_extractSubtitles;
}

//-----------------------------------------------------------------
const bool Utils::TranscoderConfiguration::isValid() const
{
  return (m_videoCodec == VideoCodec::VP8 && m_audioCodec == AudioCodec::VORBIS) ||
         (m_videoCodec == VideoCodec::VP9 && m_audioCodec == AudioCodec::VORBIS) ||
         (m_videoCodec == VideoCodec::H264 && m_audioCodec == AudioCodec::MP3) ||
         (m_videoCodec == VideoCodec::H265 && m_audioCodec == AudioCodec::MP3) ||
         (m_videoCodec == VideoCodec::H264 && m_audioCodec == AudioCodec::AAC) ||
         (m_videoCodec == VideoCodec::H265 && m_audioCodec == AudioCodec::AAC);
}

//-----------------------------------------------------------------
boost::filesystem::path Utils::validDirectoryCheck(const boost::filesystem::path& directory)
{
  auto current = directory;

  while(!boost::filesystem::is_directory(current) && !current.empty() && !(current.root_directory() == current))
  {
    current = current.parent_path();
  }

  if(!boost::filesystem::is_directory(current) || current.empty() || current.root_directory() == current)
  {
    current = boost::filesystem::path(QDir::homePath().toStdWString());
  }

  return current;
}
