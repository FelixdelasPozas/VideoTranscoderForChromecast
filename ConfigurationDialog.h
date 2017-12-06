/*
 File: ConfigurationDialog.h
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

#ifndef CONFIGURATIONDIALOG_H_
#define CONFIGURATIONDIALOG_H_

#include <ui_ConfigurationDialog.h>

// Qt
#include <QDialog>
#include <Utils.h>

/** \class ConfigurationDialog
 * \brief Dialog for application configuration.
 *
 */
class ConfigurationDialog
: public QDialog
, private Ui_ConfigurationDialog
{
    Q_OBJECT
  public:
    /** \brief ConfigurationDialog class constructor.
     * \param[in] config Application configuration object reference.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \param[in] flags Window flags.
     *
     */
    explicit ConfigurationDialog(Utils::TranscoderConfiguration &config, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    /** \brief ConfigurationDialog class virtual destructor.
     *
     */
    virtual ~ConfigurationDialog()
    {}

    virtual void accept() override;

  private slots:
    /** \brief Helper method to fill the combo boxes with the values according to configuration or user interaction.
     *
     */
    void fillComboBoxes();

  private:
    Utils::TranscoderConfiguration &m_configuration; /** application configuration. */
};

#endif // CONFIGURATIONDIALOG_H_
