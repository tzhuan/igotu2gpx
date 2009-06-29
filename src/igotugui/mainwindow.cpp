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
#include "iconstorage.h"
#include "mainwindow.h"
#include "preferencesdialog.h"
#include "ui_igotugui.h"
#include "waitdialog.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QTimer>

using namespace igotu;

class MainWindowPrivate : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void on_actionAbout_activated();
    void on_actionInfo_activated();
    void on_actionSave_activated();
    void on_actionPurge_activated();
    void on_actionPreferences_activated();
    void on_actionQuit_activated();

    void on_control_infoStarted();
    void on_control_infoFinished(const QString &info);
    void on_control_infoFailed(const QString &message);

    void on_control_contentsStarted();
    void on_control_contentsBlocksFinished(uint num, uint total);
    void on_control_contentsFinished(const QByteArray &contents, uint count);
    void on_control_contentsFailed(const QString &message);

    void on_control_purgeStarted();
    void on_control_purgeBlocksFinished(uint num, uint total);
    void on_control_purgeFinished();
    void on_control_purgeFailed(const QString &message);

public:
    void critical(const QString &text);
    void wait(const QString &text, bool busyIndicator);

    MainWindow *p;

    boost::scoped_ptr<Ui::MainWindow> ui;
    IgotuControl *control;
    QPointer<WaitDialog> waiter;
    QPointer<PreferencesDialog> preferences;
    bool initialConnect;
};

// MainWindowPrivate ===========================================================

void MainWindowPrivate::on_actionAbout_activated()
{
    QMessageBox::about(p, MainWindow::tr("About igotu2gpx"), MainWindow::tr(
        "<h3>igotu2gpx %1</h3><br/><br/>"
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
        "<br/>").arg(QLatin1String(IGOTU_VERSION_STR)));
}

void MainWindowPrivate::on_actionQuit_activated()
{
    QCoreApplication::quit();
}

void MainWindowPrivate::on_actionInfo_activated()
{
    control->info();
}

void MainWindowPrivate::on_actionSave_activated()
{
    control->contents();
}

void MainWindowPrivate::on_actionPurge_activated()
{
    QPointer<QMessageBox> messageBox(new QMessageBox(QMessageBox::Question,
                QString(), MainWindow::tr
                ("This function is highly experimental and may brick your GPS "
                 "tracker! Do you really want to remove all tracks from the "
                 "tracker?"),
                QMessageBox::Cancel, p));
    QPushButton * const purgeButton = messageBox->addButton
        (MainWindow::tr("Purge"), QMessageBox::AcceptRole);
    if (messageBox->style()->styleHint
            (QStyle::SH_DialogButtonBox_ButtonsHaveIcons))
        purgeButton->setIcon(IconStorage::get(IconStorage::PurgeIcon));
    messageBox->setDefaultButton(purgeButton);
    // necessary so MacOS X gives us a sheet
    messageBox->setWindowModality(Qt::WindowModal);
    messageBox->exec();

    if (messageBox->clickedButton() == purgeButton)
        control->purge();

    delete messageBox;
}

void MainWindowPrivate::on_actionPreferences_activated()
{
    if (!preferences) {
        preferences = new PreferencesDialog(p);
        preferences->setAttribute(Qt::WA_DeleteOnClose);

        QObject::connect(preferences, SIGNAL(deviceChanged(QString)),
                control, SLOT(setDevice(QString)));
        QObject::connect(preferences, SIGNAL(utcOffsetChanged(int)),
                control, SLOT(setUtcOffset(int)));

        preferences->show();
    } else {
        // TODO: needs also a raise?
        preferences->activateWindow();
    }
}

void MainWindowPrivate::on_control_infoStarted()
{
    wait(MainWindow::tr("Retrieving info..."), false);
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
        critical(MainWindow::tr("Unable to obtain info from GPS tracker: %1").arg(message));

    initialConnect = false;

}

void MainWindowPrivate::on_control_contentsStarted()
{
    wait(MainWindow::tr("Retrieving data..."), false);
}

void MainWindowPrivate::on_control_contentsBlocksFinished(uint num,
        uint total)
{
    waiter->progressBar()->setMaximum(total);
    waiter->progressBar()->setValue(num);
}

void MainWindowPrivate::on_control_contentsFinished(const QByteArray &contents,
        uint count)
{
    delete waiter;

    try {
        IgotuPoints igotuPoints(contents, count);
        QByteArray gpxData = igotuPoints.gpx(control->utcOffset()).toUtf8();

        QString filePath = QFileDialog::getSaveFileName(p, MainWindow::tr("Save GPS data"),
                QDateTime::currentDateTime().toString(QLatin1String
                    ("yyyy-MM-dd-hh-mm-ss")) + QLatin1String(".gpx"),
                MainWindow::tr("GPX files (*.gpx)"));

        if (filePath.isEmpty())
            return;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
            throw IgotuError(MainWindow::tr("Unable to create file: %1")
                    .arg(file.errorString()));

        if (file.write(gpxData) != gpxData.length())
            throw IgotuError(MainWindow::tr("Unable to save to file: %1")
                    .arg(file.errorString()));
    } catch (const std::exception &e) {
        critical(MainWindow::tr("Unable to save data: %1")
                .arg(QString::fromLocal8Bit(e.what())));
    }

    try {
        control->info();
    } catch (const std::exception &e) {
        critical(MainWindow::tr("Unable to obtain info from GPS tracker: %1")
                .arg(QString::fromLocal8Bit(e.what())));
    }
}

void MainWindowPrivate::on_control_contentsFailed(const QString &message)
{
    delete waiter;

    critical(MainWindow::tr("Unable to obtain trackpoints from GPS tracker: %1").arg(message));
}

void MainWindowPrivate::on_control_purgeStarted()
{
    wait(MainWindow::tr("Purging data..."), false);
}

void MainWindowPrivate::on_control_purgeBlocksFinished(uint num,
        uint total)
{
    waiter->progressBar()->setMaximum(total);
    waiter->progressBar()->setValue(num);
}

void MainWindowPrivate::on_control_purgeFinished()
{
    delete waiter;

    try {
        control->info();
    } catch (const std::exception &e) {
        critical(MainWindow::tr("Unable to obtain info from GPS tracker: %1")
                .arg(QString::fromLocal8Bit(e.what())));
    }
}

void MainWindowPrivate::on_control_purgeFailed(const QString &message)
{
    delete waiter;

    critical(MainWindow::tr("Unable to purge GPS tracker: %1").arg(message));
}

void MainWindowPrivate::critical(const QString &text)
{
    QPointer<QMessageBox> messageBox(new QMessageBox(QMessageBox::Critical,
                QString(), text, QMessageBox::Ok, p));
    // necessary so MacOS X gives us a sheet
    messageBox->setWindowModality(Qt::WindowModal);
    messageBox->exec();
    delete messageBox;
}

void MainWindowPrivate::wait(const QString &text, bool busyIndicator)
{
    waiter = new WaitDialog(text, MainWindow::tr("Please wait..."), p);
    if (!busyIndicator)
        waiter->progressBar()->setMaximum(1);
    // necessary so MacOS X gives us a sheet
    waiter->setWindowModality(Qt::WindowModal);
    waiter->setParent(p, Qt::Sheet);
    waiter->exec();
}

// MainWindow ==================================================================

MainWindow::MainWindow() :
    d(new MainWindowPrivate)
{
    d->p = this;

    d->ui.reset(new Ui::MainWindow);
    d->ui->setupUi(this);

    d->ui->actionInfo->setIcon
        (IconStorage::get(IconStorage::InfoIcon));
    d->ui->actionSave->setIcon
        (IconStorage::get(IconStorage::SaveIcon));
    d->ui->actionPurge->setIcon
        (IconStorage::get(IconStorage::PurgeIcon));
    d->ui->actionQuit->setIcon
        (IconStorage::get(IconStorage::QuitIcon));
    d->ui->actionAbout->setIcon
        (IconStorage::get(IconStorage::GuiIcon));

    d->control = new IgotuControl(this);
    d->control->setObjectName(QLatin1String("control"));

    connectSlotsByNameToPrivate(this, d.get());

    d->control->setDevice(PreferencesDialog::currentDevice());
    d->control->setUtcOffset(PreferencesDialog::currentUtcOffset());

    d->initialConnect = true;
    QTimer::singleShot(0, d->ui->actionInfo, SLOT(trigger()));
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
        d->control->notify(waiter, "reject");
        waiter->exec();
        delete waiter;
    }
}

#include "mainwindow.moc"
