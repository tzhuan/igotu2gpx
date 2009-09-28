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

#include "igotu/commonmessages.h"
#include "igotu/exception.h"
#include "igotu/igotucontrol.h"
#include "igotu/igotupoints.h"
#include "igotu/paths.h"
#include "igotu/pluginloader.h"
#include "igotu/utils.h"

#include "iconstorage.h"
#include "mainwindow.h"
#include "plugindialog.h"
#include "preferencesdialog.h"
#include "trackvisualizer.h"
#include "ui_igotugui.h"
#include "updatenotification.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QTabWidget>
#include <QTimer>

using namespace igotu;

class MainWindowPrivate : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void on_actionAbout_triggered();
    void on_actionAboutPlugins_triggered();
    void on_actionDebug_triggered();
    void on_actionReload_triggered();
    void on_actionSave_triggered();
    void on_actionPurge_triggered();
    void on_actionCancel_triggered();
    void on_actionPreferences_triggered();
    void on_actionQuit_triggered();

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

    void on_update_newVersionAvailable(const QString &version,
            const QString &name, const QUrl &url);

    void trackActivated(const QList<igotu::IgotuPoint> &track);
    void saveTracksRequested(const QList<QList<igotu::IgotuPoint> > &tracks);

public:
    void startBackgroundAction(const QString &text);
    void updateBackgroundAction(unsigned value, unsigned maximum);
    void stopBackgroundAction();
    void abortBackgroundAction(const QString &text);

    MainWindow *p;

    boost::scoped_ptr<IgotuPoints> lastTrackPoints;
    boost::scoped_ptr<Ui::MainWindow> ui;
    IgotuControl *control;
    UpdateNotification *update;
    QProgressBar *progress;
    QPointer<PreferencesDialog> preferences;
    PluginLoader *pluginLoader;
    QList<TrackVisualizer*> visualizers;
};

// Put translations in the right context
//
// TRANSLATOR igotu::Common

// MainWindowPrivate ===========================================================

void MainWindowPrivate::on_actionAbout_triggered()
{
    QMessageBox::about(p, MainWindow::tr("About Igotu2gpx"), Common::tr(
                "<h3>Igotu2gpx %1</h3><br/>"
                "Downloads tracks and waypoints from MobileAction i-gotU USB GPS travel loggers.<br/><br/>"
                "Copyright (C) 2009 Michael Hofmann.<br/>"
                "License GPLv3+: GNU GPL version 3 or later (http://gnu.org/licenses/gpl.html)<br/>"
                "This is free software: you are free to change and redistribute it.<br/>"
                "There is NO WARRANTY, to the extent permitted by law.")
        .arg(QLatin1String(IGOTU_VERSION_STR)));
}

void MainWindowPrivate::on_actionAboutPlugins_triggered()
{
    PluginDialog *pluginDialog = new PluginDialog(p);
    pluginDialog->show();
}

void MainWindowPrivate::on_actionDebug_triggered()
{
    const QString message = MainWindow::tr(
        "<h3>Versions</h3>"
            "Compiled against Qt %1<br/>"
            "Running with Qt %2"
        "<h3>Translation Directories</h3>%3"
        "<h3>Icon Directories</h3>%4"
        "<h3>Plugin Directories</h3>%5").arg(
            QLatin1String(QT_VERSION_STR),
            QLatin1String(qVersion()),
            Paths::translationDirectories().join(QLatin1String("<br/>")),
            Paths::iconDirectories().join(QLatin1String("<br/>")),
            Paths::pluginDirectories().join(QLatin1String("<br/>")));
    QMessageBox::information(p, MainWindow::tr("Debug Information"), message);
}

void MainWindowPrivate::on_actionQuit_triggered()
{
    QCoreApplication::quit();
}

void MainWindowPrivate::on_actionReload_triggered()
{
    control->info();
}

void MainWindowPrivate::on_actionSave_triggered()
{
    if (lastTrackPoints)
        saveTracksRequested(lastTrackPoints->tracks());
}

void MainWindowPrivate::on_actionPurge_triggered()
{
    QPointer<QMessageBox> messageBox(new QMessageBox(QMessageBox::Question,
                MainWindow::tr("Clear Memory"), MainWindow::tr
                ("This function is highly experimental and may brick your GPS "
                 "tracker! Only use it if your GPS tracker has been identified "
                 "correctly. Do you really want to remove all tracks from the "
                 "GPS tracker?"),
                QMessageBox::Cancel, p));
    QPushButton * const purgeButton = messageBox->addButton
        (MainWindow::tr("Clear Memory"), QMessageBox::AcceptRole);
    if (messageBox->style()->styleHint
            (QStyle::SH_DialogButtonBox_ButtonsHaveIcons))
        purgeButton->setIcon(IconStorage::get(IconStorage::PurgeIcon));
    messageBox->setDefaultButton(purgeButton);
    // necessary so MacOS X gives us a sheet
#if defined(Q_OS_MACX)
    messageBox->setWindowModality(Qt::WindowModal);
#endif
    messageBox->exec();

    if (messageBox->clickedButton() == purgeButton)
        control->purge();

    delete messageBox;
}

void MainWindowPrivate::on_actionCancel_triggered()
{
    control->cancel();
}

void MainWindowPrivate::on_actionPreferences_triggered()
{
    if (!preferences) {
        preferences = new PreferencesDialog(p);
        preferences->setAttribute(Qt::WA_DeleteOnClose);

        QObject::connect(preferences, SIGNAL(deviceChanged(QString)),
                control, SLOT(setDevice(QString)));
        QObject::connect(preferences, SIGNAL(utcOffsetChanged(int)),
                control, SLOT(setUtcOffset(int)));
        QObject::connect(preferences,
                SIGNAL(updateNotificationChanged(UpdateNotification::Type)),
                update,
                SLOT(setUpdateNotification(UpdateNotification::Type)));

        preferences->show();
    } else {
        // TODO: needs also a raise?
        preferences->activateWindow();
    }
}

void MainWindowPrivate::on_update_newVersionAvailable(const QString &version,
        const QString &name, const QUrl &url)
{
    QPointer<QMessageBox> messageBox(new QMessageBox(QMessageBox::Information,
                MainWindow::tr("New Version Available"), MainWindow::tr
                ("You can download and install a newer version of "
                    "igotu2gpx: %1")
                .arg(name), QMessageBox::NoButton, p));
    QPushButton * const laterButton = messageBox->addButton
        (MainWindow::tr("Later"), QMessageBox::RejectRole);
    QPushButton * const neverButton = messageBox->addButton
        (MainWindow::tr("Never"), QMessageBox::RejectRole);
    QPushButton * const getButton = messageBox->addButton
        (MainWindow::tr("Go to Download Page"), QMessageBox::AcceptRole);
    messageBox->setEscapeButton(laterButton);
    messageBox->setDefaultButton(getButton);
    // necessary so MacOS X gives us a sheet
#if defined(Q_OS_MACX)
    messageBox->setWindowModality(Qt::WindowModal);
#endif
    messageBox->exec();

    const QAbstractButton * const clickedButton = messageBox->clickedButton();
    if (clickedButton == getButton) {
        QDesktopServices::openUrl(url);
    } else if (clickedButton == neverButton) {
        update->setIgnoredVersion(version);
    }

    delete messageBox;
}

void MainWindowPrivate::on_control_infoStarted()
{
    startBackgroundAction(Common::tr("Downloading configuration..."));
}

void MainWindowPrivate::on_control_infoFinished(const QString &info)
{
    ui->info->setText(info);
    control->contents();
}

void MainWindowPrivate::on_control_infoFailed(const QString &message)
{
    abortBackgroundAction(Common::tr
            ("Unable to download configuration from GPS tracker: %1").arg(message));
}

void MainWindowPrivate::on_control_contentsStarted()
{
    startBackgroundAction(Common::tr("Downloading tracks..."));
}

void MainWindowPrivate::on_control_contentsBlocksFinished(uint num,
        uint total)
{
    updateBackgroundAction(num, total);
}

void MainWindowPrivate::on_control_contentsFinished(const QByteArray &contents,
        uint count)
{
    try {
        lastTrackPoints.reset(new IgotuPoints(contents, count));
        ui->actionSave->setEnabled(count > 0);

        QString errorMessage;
        Q_FOREACH (TrackVisualizer *visualizer, visualizers) {
            try {
                visualizer->setTracks(*lastTrackPoints, control->utcOffset());
            } catch (const std::exception &e) {
                errorMessage = QString::fromLocal8Bit(e.what());
            }
        }
        if (!errorMessage.isEmpty())
            throw IgotuError(errorMessage);

        stopBackgroundAction();
    } catch (const std::exception &e) {
        abortBackgroundAction(Common::tr
                ("Unable to download trackpoints from GPS tracker: %1")
                .arg(QString::fromLocal8Bit(e.what())));
    }
}

void MainWindowPrivate::on_control_contentsFailed(const QString &message)
{
    abortBackgroundAction(Common::tr
            ("Unable to download trackpoints from GPS tracker: %1")
            .arg(message));
}

void MainWindowPrivate::on_control_purgeStarted()
{
    startBackgroundAction(Common::tr("Clearing memory..."));
}

void MainWindowPrivate::on_control_purgeBlocksFinished(uint num,
        uint total)
{
    updateBackgroundAction(num, total);
}

void MainWindowPrivate::on_control_purgeFinished()
{
    stopBackgroundAction();
}

void MainWindowPrivate::on_control_purgeFailed(const QString &message)
{
    abortBackgroundAction(Common::tr
            ("Unable to clear memory of GPS tracker: %1").arg(message));
}

void MainWindowPrivate::startBackgroundAction(const QString &text)
{
    ui->actionCancel->setEnabled(true);
    ui->actionReload->setEnabled(false);
    ui->actionPurge->setEnabled(false);
    p->statusBar()->showMessage(text);
    updateBackgroundAction(0, 1);
    progress->show();
}

void MainWindowPrivate::updateBackgroundAction(unsigned value, unsigned maximum)
{
    progress->setMaximum(maximum);
    progress->setValue(value);
}

void MainWindowPrivate::stopBackgroundAction()
{
    abortBackgroundAction(QString());
}

void MainWindowPrivate::abortBackgroundAction(const QString &text)
{
    ui->actionCancel->setEnabled(false);
    ui->actionReload->setEnabled(true);
    ui->actionPurge->setEnabled(true);
    progress->hide();
    p->statusBar()->showMessage(text);
}

void MainWindowPrivate::trackActivated(const QList<IgotuPoint> &track)
{
    if (visualizers.isEmpty())
        return;
    visualizers[0]->highlightTrack(track);
}

void MainWindowPrivate::saveTracksRequested
        (const QList<QList<igotu::IgotuPoint> > &tracks)
{
    if (tracks.isEmpty())
        return;

    try {
        const QDateTime date = tracks.count() == 1 ?
            tracks[0][0].dateTime() : QDateTime::currentDateTime();
        QString filePath = QFileDialog::getSaveFileName(p,
                MainWindow::tr("Save GPS Tracks"),
                date.toString(QLatin1String("yyyy-MM-dd-hh-mm-ss")) +
                QLatin1String(".gpx"),
                MainWindow::tr("GPX files (*.gpx)"));

        if (filePath.isEmpty())
            return;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
            throw IgotuError(MainWindow::tr("Unable to create file: %1")
                    .arg(file.errorString()));

        const QByteArray gpxData = IgotuPoints::gpx(tracks,
                control->utcOffset());
        if (file.write(gpxData) != gpxData.length())
            throw IgotuError(MainWindow::tr("Unable to save file: %1")
                    .arg(file.errorString()));
    } catch (const std::exception &e) {
        QMessageBox::critical(p, MainWindow::tr("File Error"),
                MainWindow::tr("Unable to save tracks: %1")
                .arg(QString::fromLocal8Bit(e.what())));
    }
}

// MainWindow ==================================================================

MainWindow::MainWindow() :
    d(new MainWindowPrivate)
{
    d->p = this;

    d->ui.reset(new Ui::MainWindow);
    d->ui->setupUi(this);

#if QT_VERSION >= 0x040600
    setToolButtonStyle(Qt::ToolButtonFollowStyle);
#endif

    d->ui->actionReload->setIcon
        (IconStorage::get(IconStorage::ReloadIcon));
    d->ui->actionPurge->setIcon
        (IconStorage::get(IconStorage::PurgeIcon));
    d->ui->actionCancel->setIcon
        (IconStorage::get(IconStorage::CancelIcon));
    d->ui->actionSave->setIcon
        (IconStorage::get(IconStorage::SaveIcon));
    d->ui->actionQuit->setIcon
        (IconStorage::get(IconStorage::QuitIcon));
    d->ui->actionAbout->setIcon
        (IconStorage::get(IconStorage::GuiIcon));

    d->control = new IgotuControl(this);
    d->control->setObjectName(QLatin1String("control"));

    d->update = new UpdateNotification(this);
    d->update->setObjectName(QLatin1String("update"));

    d->pluginLoader = new PluginLoader(this);
    d->pluginLoader->setObjectName(QLatin1String("pluginLoader"));

    connectSlotsByNameToPrivate(this, d.get());

    d->control->setDevice(PreferencesDialog::currentDevice());
    d->control->setUtcOffset(PreferencesDialog::currentUtcOffset());
    d->update->setUpdateNotification
        (PreferencesDialog::currentUpdateNotification());

    QMultiMap<int, TrackVisualizerCreator*> mainVisualizerMap;
    QMultiMap<int, TrackVisualizerCreator*> dockVisualizerMap;
    Q_FOREACH (TrackVisualizerCreator * const creator,
            PluginLoader().availablePlugins<TrackVisualizerCreator>()) {
        if (creator->supportedVisualizerAppearances()
                .testFlag(TrackVisualizerCreator::MainWindowAppearance))
            mainVisualizerMap.insert(creator->visualizerPriority(), creator);
        if (creator->supportedVisualizerAppearances()
                .testFlag(TrackVisualizerCreator::DockWidgetAppearance))
            dockVisualizerMap.insert(creator->visualizerPriority(), creator);
    }
    QList<TrackVisualizerCreator*> mainCreators = mainVisualizerMap.values();
    QList<TrackVisualizerCreator*> dockCreators = dockVisualizerMap.values();
    if (mainCreators.isEmpty() && !dockCreators.isEmpty())
        mainCreators.append(dockCreators.takeFirst());
    if (!mainCreators.isEmpty()) {
        TrackVisualizer * const visualizer =
            mainCreators[0]->createTrackVisualizer
            (TrackVisualizerCreator::MainWindowAppearance,
             d->ui->centralWidget);
        d->visualizers.append(visualizer);
        d->ui->centralWidget->layout()->addWidget(visualizer);

        while (!dockCreators.isEmpty() && mainCreators[0] == dockCreators[0])
            dockCreators.removeFirst();
        if (!dockCreators.isEmpty()) {
            QDockWidget *dockWidget = new QDockWidget(this);
            dockWidget->setFeatures(QDockWidget::DockWidgetFloatable |
                    QDockWidget::DockWidgetMovable);
            dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea |
                    Qt::RightDockWidgetArea);
            TrackVisualizer * const dockVisualizer =
                dockCreators[0]->createTrackVisualizer
                (TrackVisualizerCreator::DockWidgetAppearance,
                 d->ui->centralWidget);
            dockWidget->setWidget(dockVisualizer);
            d->visualizers.append(dockVisualizer);
            addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
        }
    }
    for (unsigned i = 0; i < unsigned(d->visualizers.size()); ++i) {
        connect(d->visualizers[i],
                SIGNAL(saveTracksRequested(QList<QList<igotu::IgotuPoint>>)),
                d.get(),
                SLOT(saveTracksRequested(QList<QList<igotu::IgotuPoint>>)));
        connect(d->visualizers[i],
                SIGNAL(trackActivated(QList<igotu::IgotuPoint>)),
                d.get(),
                SLOT(trackActivated(QList<igotu::IgotuPoint>)));
    }

    // Progress bar
    d->progress = new QProgressBar(this);
    d->progress->setMaximum(0);
    d->progress->setSizePolicy
        (QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    d->progress->setMinimumWidth(120);
    d->progress->setMaximumHeight(16);
    d->progress->hide();
    statusBar()->addPermanentWidget(d->progress);

    QTimer::singleShot(0, d->update, SLOT(runScheduledCheck()));
    QTimer::singleShot(0, d->ui->actionReload, SLOT(trigger()));
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    while (!d->control->queuesEmpty())
        d->control->cancel();
}

#include "mainwindow.moc"
