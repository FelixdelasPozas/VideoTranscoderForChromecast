/*
 File: VideoTranscoder.h
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

#ifndef VIDEOTRANSCODER_H_
#define VIDEOTRANSCODER_H_

// Project
#include <ui_VideoTranscoder.h>

// Qt
#include <QMainWindow>

class QEvent;

/** \class VideoTranscoder
 * \brief Implements the application main dialog.
 *
 */
class VideoTranscoder
: public QMainWindow
, private Ui_VideoTranscoder
{
    Q_OBJECT
  public:
    /** \brief VideoTranscoder class constructor.
     *
     */
    explicit VideoTranscoder();

    /** \brief VideoTranscoder class virtual destructor.
     *
     */
    virtual ~VideoTranscoder();

  protected:
    bool event(QEvent *e) override;

  private slots:
    /** \brief Shows the egocentric about dialog.
     *
     */
    void onAboutButtonPressed();

    /** \brief Shows the configuration dialog.
     *
     */
    void onConfigurationButtonPressed();

    /** \brief Launches the processing window.
     *
     */
    void onStartButtonPressed();

  private:
    /** \brief Connects the buttons signals to its slots.
     *
     */
    void connectUI();

    /** \brief Loads the application configuration from the registry.
     *
     */
    void loadConfiguration();

    /** \brief Saves the application configuration to the registry.
     *
     */
    void saveConfiguration();
};

#endif // VIDEOTRANSCODER_H_
