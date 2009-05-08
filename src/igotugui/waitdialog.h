/******************************************************************************
 * Copyright (C) 2007  Michael Hofmann <mh21@piware.de>                       *
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

#ifndef _IGOTU2GPX_SRC_IGOTUGUI_WAITDIALOG_H_
#define _IGOTU2GPX_SRC_IGOTUGUI_WAITDIALOG_H_

#include <boost/scoped_ptr.hpp>

#include <QDialog>

class WaitDialogPrivate;

class WaitDialog : public QDialog
{
    Q_OBJECT
public:
    WaitDialog(const QString &message, const QString &title,
            QWidget *parent = NULL);
    ~WaitDialog();

protected:
    // Does not close the window
    virtual void closeEvent(QCloseEvent *event);

protected:
    boost::scoped_ptr<WaitDialogPrivate> d;
};

#endif
