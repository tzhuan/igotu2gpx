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

#include "ui_waitdialog.h"
#include "waitdialog.h"

#include <QCloseEvent>

class WaitDialogPrivate
{
public:
    Ui::WaitDialog ui;
};

// WaitDialog ==================================================================

WaitDialog::WaitDialog(const QString &message, const QString &title,
        QWidget *parent) :
    QDialog(parent),
    d(new WaitDialogPrivate)
{
    d->ui.setupUi(this);

    d->ui.message->setText(message);
    setWindowTitle(title);
}

WaitDialog::~WaitDialog()
{
}

QLabel *WaitDialog::messageLabel()
{
    return d->ui.message;
}

QProgressBar *WaitDialog::progressBar()
{
    return d->ui.progressBar;
}

void WaitDialog::closeEvent(QCloseEvent *event)
{
    // Do nothing
    event->ignore();
}
