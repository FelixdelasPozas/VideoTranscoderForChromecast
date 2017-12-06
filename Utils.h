/*
  File: Utils.h
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

#ifndef UTILS_H_
#define UTILS_H_

// C++
#include <functional>

// Qt
#include <QDir>
#include <QString>
#include <QPair>
#include <QMutex>

namespace Utils
{
  extern const QStringList MOVIE_FILE_EXTENSIONS;
  extern const QString TEMPORAL_FILE_EXTENSION;
  static QMutex s_mutex;

  /** \brief Returns true if the file given as parameter has a video extension.
   * \param[in] file file QFileInfo struct.
   *
   */
  bool isVideoFile(const QFileInfo &file);

  /** \brief Checks the given directory for existance and readability. If the directory is
   *  not readable or doesn't exist it will try the parent recursively until returning a valid
   *  path or the user home directory.
   * \param[in] directory path.
   */
  const QString validDirectoryCheck(const QString &directory);

  /** \brief Returns the files in the specified directory tree that has the specified extensions.
   * \param[in] rootDir starting directory.
   * \param[in] filters extensions of the files to return.
   * \param[in] with_subdirectories boolean that indicates if all the files in the subdiretories that
   *            comply the conditions must be returned.
   * \param[in] condition additional condition that the files must comply with.
   *
   * NOTE: while this will return any file info (that complies with the filter and the conditions) mp3
   *       files will be returned first. That's because i want to process those before anything else.
   *       Yes, it's arbitrary but doesn't affect the results.
   *
   */
  QList<QFileInfo> findFiles(const QDir rootDirectory,
                             const QStringList extensions,
                             bool with_subdirectories = true,
                             const std::function<bool (const QFileInfo &)> &condition = [](const QFileInfo &info) { return true; });

  /** \class TranscoderConfiguration
   * \brief Implements the configuration storage/management.
   *
   */
  class TranscoderConfiguration
  {
    public:
      enum class VideoCodec { VP8 = 0, VP9, H264, H265 }; /** video codec identifiers. */
      enum class AudioCodec { VORBIS = 0, MP3, AAC };     /** audio codec identifiers. */

      /** \brief TranscoderConfiguration class constructor.
       *
       */
      TranscoderConfiguration();

      /** \brief Loads the configuration data from disk.
       *
       */
      void load();

      /** \brief Saves the configuration to disk.
       *
       */
      void save() const;

      /** \brief Returns the root directory to start searching for files.
       *
       */
      const QString &rootDirectory() const;

      /** \brief Sets the root directory to start searching for files to transcode.
       * \param[in] path root directory path.
       *
       */
      void setRootDirectory(const QString &path);

      /** \brief Returns the number of simultaneous threads in the transcoding process.
       *
       */
      int numberOfThreads() const;

      /** \brief Sets the number of simultaneous threads to use in the transcoding process.
       * \param[in] value number of threads.
       *
       */
      void setNumberOfThreads(int value);

      /** \brief Returns the output file video codec.
       *
       */
      const VideoCodec videoCodec() const;

      /** \brief Sets the output file video codec.
       * \param[in] codec Video codec identifier.
       *
       */
      void setVideoCodec(const VideoCodec codec);

      /** \brief Returns the output file audio codec.
       *
       */
      const AudioCodec audioCodec() const;

      /** \brief Sets the output file audio codec.
       * \param[in] codec Audio codec identifier.
       *
       */
      void setAudioCodec(const AudioCodec codec);

      /** \brief Returns the output file video bitrate.
       *
       */
      const int videoBitrate() const;

      /** \brief Sets the output file video bitrate.
       * \param[in] bitrate Integer value.
       *
       */
      void setVideoBitrate(const int bitrate);

      /** \brief Returns the output file audio bitrate.
       *
       */
      const int audioBitrate() const;

      /** \brief Sets the output file audio bitrate.
       *
       */
      void setAudioBitrate(const int bitrate);

      /** \brief Returns the value of 'embed subtitles in video' value.
       *
       */
      const bool embedSubtitles() const;

      /** \brief Sets the value of the 'embed subtitles in video'.
       * \param[in] value boolean value.
       *
       */
      void setEmbedSubtitles(const bool value);

      /** \brief Returns true if the video and audio codec pair is valid.
       *
       */
      const bool isValid() const;

    private:
      QString    m_root_directory;    /** last used directory.                                                */
      int        m_number_of_threads; /** number of threads to use.                                           */
      VideoCodec m_videoCodec;        /** output video codec.                                                 */
      int        m_videoBitrate;      /** output video bitrate.                                               */
      AudioCodec m_audioCodec;        /** output audio codec.                                                 */
      int        m_audioBitrate;      /** output audio bitrate.                                               */
      bool       m_embedSubtitles;    /** true if subtitles are to be embedded in the video, false otherwise. */


      /** settings key strings. */
      static const QString ROOT_DIRECTORY;
      static const QString NUMBER_OF_THREADS;
      static const QString VIDEO_CODEC;
      static const QString VIDEO_BITRATE;
      static const QString AUDIO_CODEC;
      static const QString AUDIO_BITRATE;
      static const QString EMBED_SUBTITLES;
  };
}

#endif // UTILS_H_
