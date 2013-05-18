/******************************************************************************
 * Copyright (C) 2009  Michael Hofmann <mh21@mh21.de>                         *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with this program; if not, write to the Free Software Foundation, Inc.,    *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.                *
 ******************************************************************************/

#ifndef _IGOTU2GPX_SRC_IGOTUGUI_CONFIGURATIONDIALOG_H_
#define _IGOTU2GPX_SRC_IGOTUGUI_CONFIGURATIONDIALOG_H_

#include <boost/scoped_ptr.hpp>

#include <QDialog>
#include <QVariantMap>

class ConfigurationDialogPrivate;

namespace igotu
{
class IgotuConfig;
}

class ConfigurationDialog : public QDialog
{
    Q_OBJECT
    friend class ConfigurationDialogPrivate;
    friend class MainWindowPrivate;
public:
    ConfigurationDialog(const igotu::IgotuConfig &config, QWidget *parent = NULL);
    ~ConfigurationDialog();

    QVariantMap config() const;

protected:
    boost::scoped_ptr<ConfigurationDialogPrivate> d;
};

#endif

