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

#include "igotu/exception.h"
#include "igotu/igotucontrol.h"
#include "igotu/igotupoints.h"
#include "igotu/utils.h"

#include "mainwindow.h"
#include "ui_igotugui.h"
#include "waitdialog.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPointer>
#include <QProgressBar>
#include <QSettings>
#include <QTimer>

using namespace igotu;

class MainWindowPrivate : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void on_actionAbout_activated();
    void on_actionConnect_activated();
    void on_actionSaveAs_activated();
    void on_actionPreferences_activated();
    void on_actionQuit_activated();

    void on_control_infoStarted();
    void on_control_infoFinished(const QString &info);
    void on_control_infoFailed(const QString &message);

    void on_control_contentsStarted();
    void on_control_contentsBlocksFinished(unsigned num, unsigned total);
    void on_control_contentsFinished(const QByteArray &contents, unsigned count);
    void on_control_contentsFailed(const QString &message);

public:
    void critical(const QString &text);

    MainWindow *p;

    boost::scoped_ptr<Ui::MainWindow> ui;
    IgotuControl *control;
    QPointer<WaitDialog> waiter;
    bool initialConnect;
};

void MainWindowPrivate::on_actionSaveAs_activated()
{
    control->contents();
}

void MainWindowPrivate::on_actionAbout_activated()
{
    QMessageBox::about(p, tr("About Igotu2gpx"), tr(
        "<h3>Igotu2gpx</h3><br/><br/>"
        "Shows the configuration and decodes the stored tracks and waypoints "
        "of a MobileAction i-gotU USB GPS travel logger."
        "<br/><br/>"
        "This program is licensed to you under the terms of the GNU General "
        "Public License. See the file LICENSE that came with this software "
        "for further details."
        "<br/><br/>"
        "Copyright (C) 2009 Michael Hofmann."
        "<br/><br/>"
        "The program is provided AS IS with NO WARRANTY OF ANY KIND, "
        "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR "
        "A PARTICULAR PURPOSE."
        "<br/>"));
}

void MainWindowPrivate::on_actionConnect_activated()
{
    control->info();
}

void MainWindowPrivate::on_actionQuit_activated()
{
    QCoreApplication::quit();
}

void MainWindowPrivate::on_actionPreferences_activated()
{
    QString device = control->device();

    bool ok;
    device = QInputDialog::getText(p, tr("Preferences"),
            tr("Device (usb:<vendor>:<product> or serial:<n>)"),
            QLineEdit::Normal, device, &ok);
    // TODO: should be non-modal
    if (!ok)
        return;

    QSettings().setValue(QLatin1String("device"), device);
    control->setDevice(device);
}

void MainWindowPrivate::on_control_infoStarted()
{
    if (initialConnect)
        return;

    waiter = new WaitDialog(tr("Retrieving info..."),
            tr("Please wait..."), p);
    waiter->setWindowFlags(waiter->windowFlags() | Qt::Sheet);
    waiter->exec();
}

void MainWindowPrivate::on_control_infoFinished(const QString &info)
{
    delete waiter;

    ui->textBrowser->setText(info);

    initialConnect = false;
}

void MainWindowPrivate::on_control_infoFailed(const QString &message)
{
    delete waiter;

    ui->textBrowser->clear();

    if (!initialConnect)
        critical(tr("Unable to connect to gps tracker: %1").arg(message));

    initialConnect = false;

}

void MainWindowPrivate::on_control_contentsStarted()
{
    waiter = new WaitDialog(tr("Retrieving data..."),
            tr("Please wait..."), p);
    waiter->setWindowFlags(waiter->windowFlags() | Qt::Sheet);
    waiter->progressBar()->setMaximum(1);
    waiter->exec();
}

void MainWindowPrivate::on_control_contentsBlocksFinished(unsigned num,
        unsigned total)
{
    waiter->progressBar()->setMaximum(total);
    waiter->progressBar()->setValue(num);
}

void MainWindowPrivate::on_control_contentsFinished(const QByteArray &contents,
        unsigned count)
{
    delete waiter;

    try {
        IgotuPoints igotuPoints(contents, count);
        QByteArray gpxData = igotuPoints.gpx().toUtf8();

        QString filePath = QFileDialog::getSaveFileName(p, tr("Save GPS data"),
                QDateTime::currentDateTime().toString(QLatin1String
                    ("yyyy-MM-dd-hh-mm-ss")) + QLatin1String(".gpx"),
                tr("GPX files (*.gpx)"));

        if (filePath.isEmpty())
            return;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
            throw IgotuError(tr("Unable to create file: %1")
                    .arg(file.errorString()));

        if (file.write(gpxData) != gpxData.length())
            throw IgotuError(tr("Unable to save to file: %1")
                    .arg(file.errorString()));
    } catch (const std::exception &e) {
        critical(tr("Unable to save data: %1")
                .arg(QString::fromLocal8Bit(e.what())));
    }
}

void MainWindowPrivate::on_control_contentsFailed(const QString &message)
{
    delete waiter;

    critical(tr("Unable to connect to gps tracker: %1").arg(message));
}

void MainWindowPrivate::critical(const QString &text)
{
    QPointer<QMessageBox> messageBox(new QMessageBox(QMessageBox::Critical,
                QString(), text, QMessageBox::Ok, p));
    messageBox->setWindowFlags(messageBox->windowFlags() | Qt::Sheet);
    messageBox->exec();
    delete messageBox;
}

// MainWindow ==================================================================

MainWindow::MainWindow() :
    d(new MainWindowPrivate)
{
    d->p = this;

    d->ui.reset(new Ui::MainWindow);
    d->ui->setupUi(this);

    d->control = new IgotuControl(this);
    d->control->setObjectName(QLatin1String("control"));

    connectSlotsByNameToPrivate(this, d.get());

    if (!QSettings().contains(QLatin1String("device")))
        QSettings().setValue(QLatin1String("device"), d->control->device());
    else
        d->control->setDevice(QSettings().value
                (QLatin1String("device")).toString());

    d->initialConnect = true;
    QTimer::singleShot(0, d->ui->actionConnect, SLOT(trigger()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    while (!d->control->queuesEmpty()) {
        // will only be reached at initial connect
        QPointer<QDialog> waiter(new WaitDialog
                (tr("Please wait until all tasks are finished..."),
                 tr("Please wait..."),
                 this));
        waiter->setWindowFlags(waiter->windowFlags() | Qt::Sheet);
        d->control->notify(waiter, "reject");
        waiter->exec();
        delete waiter;
    }
}

#include "mainwindow.moc"
