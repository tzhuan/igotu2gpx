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

#include "igotu/commonmessages.h"
#include "igotu/exception.h"
#include "igotu/fileexporter.h"
#include "igotu/igotucontrol.h"
#include "igotu/igotudata.h"
#include "igotu/messages.h"
#include "igotu/paths.h"
#include "igotu/pluginloader.h"
#include "igotu/utils.h"

#include "configurationdialog.h"
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
    void on_actionSaveAll_triggered();
    void on_actionSaveSelected_triggered();
    void on_actionPurge_triggered();
    void on_actionConfigureTracker_triggered();
    void on_actionCancel_triggered();
    void on_actionPreferences_triggered();
    void on_actionQuit_triggered();

    void on_control_commandStarted(const QString &message);
    void on_control_commandRunning(uint num, uint total);
    void on_control_commandSucceeded();
    void on_control_commandFailed(const QString &failed);

    void on_control_infoRetrieved(const QString &info, const QByteArray &contents);
    void on_control_contentsRetrieved(const QByteArray &contents, uint count);

    void on_update_newVersionAvailable(const QString &version,
            const QString &name, const QUrl &url);

    void trackActivated(const QList<igotu::IgotuPoint> &track);
    void saveTracksRequested(const QList<QList<igotu::IgotuPoint> > &tracks);
    void trackSelectionChanged(bool selected);

public:
    void startBackgroundAction(const QString &text);
    void updateBackgroundAction(unsigned value, unsigned maximum);
    void stopBackgroundAction();
    void abortBackgroundAction(const QString &text);

private:
    QString savedTrackFileName(bool raw, const IgotuPoint &point,
            FileExporter **currentExporter);
    void saveTracks(const QList<QList<IgotuPoint> > &tracks);

public:
    MainWindow *p;

    boost::scoped_ptr<IgotuData> lastTrackPoints;
    boost::scoped_ptr<IgotuConfig> lastConfig;
    boost::scoped_ptr<Ui::MainWindow> ui;
    IgotuControl *control;
    UpdateNotification *update;
    QProgressBar *progress;
    QPointer<PreferencesDialog> preferences;
    PluginLoader *pluginLoader;
    QList<TrackVisualizer*> visualizers;
    QList<FileExporter*> exporters;
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
    control->contents();
}

void MainWindowPrivate::on_actionSaveAll_triggered()
{
    if (lastTrackPoints)
        saveTracks(QList<QList<IgotuPoint> >());
}

void MainWindowPrivate::on_actionSaveSelected_triggered()
{
    Q_FOREACH (TrackVisualizer * const visualizer, visualizers) {
        if (visualizer->flags().testFlag(TrackVisualizer::HasTrackSelection)) {
            visualizer->saveSelectedTracks();
            return;
        }
    }
    qCritical("No visualizer with track selection found");
}

void MainWindowPrivate::on_actionPurge_triggered()
{
    QPointer<QMessageBox> messageBox(new QMessageBox(QMessageBox::Question,
                MainWindow::tr("Clear Memory"), MainWindow::tr
                ("Do you really want to remove all tracks from the "
                 "GPS tracker?"),
                QMessageBox::Cancel, p));
    QPushButton * const purgeButton = messageBox->addButton
        (MainWindow::tr("Clear Memory"), QMessageBox::AcceptRole);
    if (messageBox->style()->styleHint
            (QStyle::SH_DialogButtonBox_ButtonsHaveIcons))
        purgeButton->setIcon(IconStorage::get(IconStorage::PurgeIcon));
    messageBox->setDefaultButton(purgeButton);
    // necessary so MacOS X gives us a sheet
#if defined(Q_OS_MAC)
    messageBox->setWindowModality(Qt::WindowModal);
#endif
    messageBox->exec();

    if (messageBox->clickedButton() == purgeButton) {
        control->purge();
        control->info();
        control->contents();
    }

    delete messageBox;
}

void MainWindowPrivate::on_actionConfigureTracker_triggered()
{
    RETURN_IF_FAIL(lastConfig);
    QPointer<ConfigurationDialog> dialog
        (new ConfigurationDialog(*lastConfig, p));
    if (dialog->exec() == QDialog::Accepted)
        control->configure(dialog->config());

    delete dialog;
}

void MainWindowPrivate::on_actionCancel_triggered()
{
    control->cancel();
}

void MainWindowPrivate::on_actionPreferences_triggered()
{
    if (preferences) {
        preferences->activateWindow();
        return;
    }

    preferences = new PreferencesDialog(p);
    preferences->setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(preferences, SIGNAL(deviceChanged(QString)),
            control, SLOT(setDevice(QString)));
    QObject::connect(preferences, SIGNAL(utcOffsetChanged(int)),
            control, SLOT(setUtcOffset(int)));
    QObject::connect(preferences, SIGNAL(updateNotificationChanged(UpdateNotification::Type)),
            update, SLOT(setUpdateNotification(UpdateNotification::Type)));
    QObject::connect(preferences, SIGNAL(tracksAsSegmentsChanged(bool)),
            control, SLOT(setTracksAsSegments(bool)));

    preferences->show();
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
#if defined(Q_OS_MAC)
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

void MainWindowPrivate::on_control_commandStarted(const QString &message)
{
    startBackgroundAction(message);
}

void MainWindowPrivate::on_control_commandFailed(const QString &message)
{
    abortBackgroundAction(message);
}

void MainWindowPrivate::on_control_commandRunning(uint num, uint total)
{
    updateBackgroundAction(num, total);
}

void MainWindowPrivate::on_control_commandSucceeded()
{
    stopBackgroundAction();
}

void MainWindowPrivate::on_control_infoRetrieved(const QString &info, const QByteArray &contents)
{
    lastConfig.reset(new IgotuConfig(contents));
    ui->info->setText(info);
}

void MainWindowPrivate::on_control_contentsRetrieved(const QByteArray &contents,
        uint count)
{
    lastTrackPoints.reset(new IgotuData(contents, count));
    lastConfig.reset(new IgotuConfig(lastTrackPoints->config()));
    ui->actionSaveAll->setEnabled(count > 0);

    Q_FOREACH (TrackVisualizer *visualizer, visualizers) {
        try {
            visualizer->setTracks(lastTrackPoints->points(), control->utcOffset());
        } catch (const std::exception &e) {
            qCritical("Unable to set tracks in visualizer: %s", e.what());
        }
    }
}

void MainWindowPrivate::startBackgroundAction(const QString &text)
{
    ui->actionCancel->setEnabled(true);
    ui->actionReload->setEnabled(false);
    ui->actionPurge->setEnabled(false);
    ui->actionConfigureTracker->setEnabled(false);
    p->statusBar()->showMessage(text);
}

void MainWindowPrivate::updateBackgroundAction(unsigned value, unsigned maximum)
{
    if (maximum > 0) {
        progress->show();
        progress->setMaximum(maximum);
        progress->setValue(value);
    } else {
        progress->hide();
    }
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
    ui->actionConfigureTracker->setEnabled(true);
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
    if (!tracks.isEmpty())
        saveTracks(tracks);
}

void MainWindowPrivate::trackSelectionChanged(bool selected)
{
    ui->actionSaveSelected->setEnabled(selected);
}

QString MainWindowPrivate::savedTrackFileName(bool raw, const IgotuPoint &point,
        FileExporter **currentExporter)
{
    const QDateTime date = point.isValid() ?
        point.dateTime() : QDateTime::currentDateTime();

    QList<FileExporter*> selectedExporters;
    Q_FOREACH (FileExporter *exporter, exporters)
        if (raw || exporter->mode().testFlag(FileExporter::TrackExport))
            selectedExporters.append(exporter);
    RETURN_VAL_IF_FAIL(!selectedExporters.isEmpty(), QString());

    QStringList filters;
    QStringList filterExtensions;
    Q_FOREACH (FileExporter *exporter, selectedExporters) {
            filters.append(exporter->fileType());
            filterExtensions.append(exporter->fileExtension());
    }
    QString selectedFilter;

    QString fileTemplate =
        date.toString(QLatin1String("yyyy-MM-dd-hh-mm-ss")) +
        QLatin1Char('.') + selectedExporters.at(0)->fileExtension();

    const QString filePath = QFileDialog::getSaveFileName(p,
            MainWindow::tr("Save GPS Tracks"),
            fileTemplate, filters.join(QLatin1String(";;")), &selectedFilter);

    if (filePath.isEmpty())
        return filePath;

    // There is a bug in QGtkStyle for Qt < 4.6.0 that will not return the
    // selected filter, so just guess it from the filename
    // const int index = filters.indexOf(selectedFilter);
    const int index = filterExtensions.indexOf(QFileInfo(filePath).suffix());
    *currentExporter = selectedExporters.at(index < 0 ? 0 : index);

    return filePath;
}

void MainWindowPrivate::saveTracks
        (const QList<QList<IgotuPoint> > &tracks)
{
    try {
        FileExporter *exporter;
        const QString filePath = savedTrackFileName(tracks.isEmpty(),
                tracks.value(0).value(0), &exporter);
        if (filePath.isEmpty())
            return;

        Q_ASSERT(exporter);

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
            throw Exception(MainWindow::tr("Unable to create file: %1")
                    .arg(file.errorString()));

        QByteArray data;
        if (tracks.isEmpty())
            data = exporter->save(*lastTrackPoints,
                control->tracksAsSegments(), control->utcOffset());
        else
            data = exporter->save(tracks,
                control->tracksAsSegments(), control->utcOffset());
        if (file.write(data) != data.length())
            throw Exception(MainWindow::tr("Unable to save file: %1")
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

    setToolButtonStyle(Qt::ToolButtonFollowStyle);

    d->ui->actionReload->setIcon
        (IconStorage::get(IconStorage::ReloadIcon));
    d->ui->actionPurge->setIcon
        (IconStorage::get(IconStorage::PurgeIcon));
    d->ui->actionCancel->setIcon
        (IconStorage::get(IconStorage::CancelIcon));
    d->ui->actionSaveAll->setIcon
        (IconStorage::get(IconStorage::SaveIcon));
    d->ui->actionConfigureTracker->setIcon
        (IconStorage::get(IconStorage::ConfigureIcon));
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
    d->control->setTracksAsSegments(PreferencesDialog::currentTracksAsSegments());

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
    Q_FOREACH(TrackVisualizer * const visualizer, d->visualizers) {
        connect(visualizer,
                SIGNAL(saveTracksRequested(QList<QList<igotu::IgotuPoint>>)),
                d.get(),
                SLOT(saveTracksRequested(QList<QList<igotu::IgotuPoint>>)));
        connect(visualizer,
                SIGNAL(trackActivated(QList<igotu::IgotuPoint>)),
                d.get(),
                SLOT(trackActivated(QList<igotu::IgotuPoint>)));
    }
    Q_FOREACH(TrackVisualizer * const visualizer, d->visualizers) {
        if (visualizer->flags().testFlag(TrackVisualizer::HasTrackSelection)) {
            connect(visualizer, SIGNAL(trackSelectionChanged(bool)),
                    d.get(), SLOT(trackSelectionChanged(bool)));
            break;
        }
    }

    QMultiMap<int, FileExporter*> fileExporterMap;
    Q_FOREACH (FileExporter * const exporter,
            PluginLoader().availablePlugins<FileExporter>())
        fileExporterMap.insert(exporter->exporterPriority(), exporter);
    d->exporters = fileExporterMap.values();

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
