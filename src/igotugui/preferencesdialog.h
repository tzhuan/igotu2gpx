/******************************************************************************
 * Copyright (C) 2009  Michael Hofmann <mh21@piware.de>                       *
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

#ifndef _SRC_IGOTUGUI_PREFERENCESDIALOG_H_
#define _SRC_IGOTUGUI_PREFERENCESDIALOG_H_

#include <boost/scoped_ptr.hpp>

#include <QDialog>

class PreferencesDialogPrivate;
class MainWindowPrivate;

class PreferencesDialog : public QDialog
{
    Q_OBJECT
    friend class PreferencesDialogPrivate;
    friend class MainWindowPrivate;
public:
    PreferencesDialog(QWidget *parent = NULL);
    ~PreferencesDialog();

    static QString currentDevice();

protected:
    boost::scoped_ptr<PreferencesDialogPrivate> d;

Q_SIGNALS:
    void deviceChanged(const QString &device);
};

#endif

