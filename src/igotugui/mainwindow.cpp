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

#include "igotu/commands.h"
#include "igotu/igotupoints.h"

#include "mainwindow.h"
#include "ui_igotugui.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>

#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#include "igotu/libusbconnection.h"
#endif
#ifdef Q_OS_WIN32
#include "igotu/win32serialconnection.h"
#endif

using namespace igotu;

// MainWindow ==================================================================

MainWindow::MainWindow()
{
    ui.reset(new Ui::IgotuDialog);
    ui->setupUi(this);
    QPushButton * const saveButton = ui->buttonBox->addButton(tr("Save..."),
            QDialogButtonBox::ActionRole);
    QPushButton * const reloadButton = ui->buttonBox->addButton(tr("Reload"),
            QDialogButtonBox::ActionRole);

    connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
    connect(reloadButton, SIGNAL(clicked()), this, SLOT(reload()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::save()
{
    if (contents.isEmpty())
        return;

    IgotuPoints igotuPoints(contents, count);
    QByteArray gpxData = igotuPoints.gpx().toUtf8();

    QString filePath = QFileDialog::getSaveFileName(this, QString(), QString(),
            tr("GPX files (*.gpx)"));

    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, QString(),
                tr("Unable to create file: %1")
                .arg(file.errorString()));
        return;
    }
    if (file.write(gpxData) != gpxData.length())
        QMessageBox::critical(this, QString(),
                tr("Unable to save to file: %1")
                .arg(file.errorString()));
}

void MainWindow::reload()
{
    connection.reset();
    ui->textBrowser->clear();
    contents.clear();
    count = 0;

    try {
#ifdef Q_OS_WIN32
        connection.reset(new Win32SerialConnection);
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACX)
        connection.reset(new LibusbConnection);
#endif

        NmeaSwitchCommand(connection.get(), false).sendAndReceive();

        QString status;
        IdentificationCommand id(connection.get());
        id.sendAndReceive();
        status += tr("S/N: %1\n").arg(id.serialNumber());
        status += tr("Firmware version: %1\n").arg(id.firmwareVersion());
        status += tr("Model: %1\n").arg(id.deviceName());
        CountCommand countCommand(connection.get());
        countCommand.sendAndReceive();
        count = countCommand.trackPointCount();
        status += tr("Number of trackpoints: %1\n").arg(count);
        ui->textBrowser->setText(status);

        for (unsigned i = 0;
                i < 1 + (count + 0x7f) / 0x80; ++i) {
            fprintf(stderr, "Dumping datablock %u...\n", i + 1);
            QByteArray data = ReadCommand(connection.get(), i * 0x1000,
                    0x1000).sendAndReceive();
            contents += data;
        }

        NmeaSwitchCommand(connection.get(), true).sendAndReceive();
    } catch (const std::exception &e) {
        QMessageBox::critical(this, QString(),
                tr("Unable to create gps tracker connection: %1")
                .arg(QString::fromLocal8Bit(e.what())));
        connection.reset();
        ui->textBrowser->clear();
        contents.clear();
        count = 0;
    }
}
