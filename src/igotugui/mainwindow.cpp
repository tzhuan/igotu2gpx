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
#include "igotu/paths.h"
#include "igotu/pluginloader.h"
#include "igotu/utils.h"

#include "iconstorage.h"
#include "mainwindow.h"
#include "plugindialog.h"
#include "preferencesdialog.h"
#include "trackvisualizer.h"
#include "ui_igotugui.h"

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
    QTabWidget *tabs;
    IgotuControl *control;
    QProgressBar *progress;
    QPointer<PreferencesDialog> preferences;
    PluginLoader *pluginLoader;
    QList<TrackVisualizer*> visualizers;
};

// MainWindowPrivate ===========================================================

void MainWindowPrivate::on_actionAbout_triggered()
{
    QMessageBox::about(p, MainWindow::tr("About Igotu2gpx"), MainWindow::tr(
        "<h3>Igotu2gpx %1</h3><br/><br/>"
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

void MainWindowPrivate::on_actionAboutPlugins_triggered()
{
    try {
        PluginDialog *pluginDialog = new PluginDialog(p);
        pluginDialog->show();
    } catch (const std::exception &e) {
        QMessageBox::critical(p, tr("Plugin Error"),
                tr("Unable to display available plugins\n%1")
                .arg(QString::fromLocal8Bit(e.what())));
    }
}

void MainWindowPrivate::on_actionDebug_triggered()
{
    const QString message = MainWindow::tr(
        "<h3>Versions</h3>"
            "Compiled against Qt %1<br/>"
            "Running with Qt %2"
        "<h3>Icon Directories</h3>%4"
        "<h3>Plugin Directories</h3>%5").arg(
            QLatin1String(QT_VERSION_STR),
            QLatin1String(qVersion()),
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
                QString(), MainWindow::tr
                ("This function is highly experimental and may brick your GPS "
                 "tracker! Only use it if your GPS tracker has been identified "
                 "correctly. Do you really want to remove all tracks from the "
                 "GPS tracker?"),
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

        preferences->show();
    } else {
        // TODO: needs also a raise?
        preferences->activateWindow();
    }
}

void MainWindowPrivate::on_control_infoStarted()
{
    startBackgroundAction(MainWindow::tr("Retrieving info..."));
}

void MainWindowPrivate::on_control_infoFinished(const QString &info)
{
    ui->info->setText(info);
    control->contents();
}

void MainWindowPrivate::on_control_infoFailed(const QString &message)
{
    abortBackgroundAction(MainWindow::tr("Unable to obtain info from GPS tracker: %1")
            .arg(message));
}

void MainWindowPrivate::on_control_contentsStarted()
{
    startBackgroundAction(MainWindow::tr("Retrieving data..."));
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
        ui->actionSave->setEnabled(true);

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
        abortBackgroundAction(MainWindow::tr("Unable to obtain trackpoints from GPS tracker: %1")
                .arg(QString::fromLocal8Bit(e.what())));
    }
}

void MainWindowPrivate::on_control_contentsFailed(const QString &message)
{
    abortBackgroundAction(MainWindow::tr("Unable to obtain trackpoints from GPS tracker: %1")
            .arg(message));
}

void MainWindowPrivate::on_control_purgeStarted()
{
    startBackgroundAction(MainWindow::tr("Purging data..."));
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
    abortBackgroundAction(MainWindow::tr("Unable to purge GPS tracker: %1").arg(message));
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
    qDebug("activated");
    visualizers[0]->highlightTrack(track);
    if (tabs)
        tabs->setCurrentIndex(0);
}

void MainWindowPrivate::saveTracksRequested
        (const QList<QList<igotu::IgotuPoint> > &tracks)
{
    try {
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

        const QByteArray gpxData = IgotuPoints::gpx(tracks, control->utcOffset());
        if (file.write(gpxData) != gpxData.length())
            throw IgotuError(MainWindow::tr("Unable to save to file: %1")
                    .arg(file.errorString()));
    } catch (const std::exception &e) {
        QMessageBox::critical(p, MainWindow::tr("File Error"),
                MainWindow::tr("Unable to save trackpoints: %1")
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

    d->pluginLoader = new PluginLoader(this);
    d->pluginLoader->setObjectName(QLatin1String("pluginLoader"));

    connectSlotsByNameToPrivate(this, d.get());

    d->control->setDevice(PreferencesDialog::currentDevice());
    d->control->setUtcOffset(PreferencesDialog::currentUtcOffset());

    QMultiMap<int, TrackVisualizerCreator*> visualizerMap;
    Q_FOREACH (TrackVisualizerCreator * const creator,
            PluginLoader().availablePlugins<TrackVisualizerCreator>())
        visualizerMap.insert(creator->visualizerPriority(), creator);
    const QList<TrackVisualizerCreator*> creators = visualizerMap.values();
    if (creators.count() == 1) {
        d->tabs = NULL;

        TrackVisualizer * const visualizer =
            creators[0]->createTrackVisualizer(d->ui->centralWidget);
        d->visualizers.append(visualizer);
        d->ui->centralWidget->layout()->addWidget(visualizer);
    } else {
        d->tabs = new QTabWidget(d->ui->centralWidget);
        d->ui->centralWidget->layout()->addWidget(d->tabs);

        Q_FOREACH (TrackVisualizerCreator * const creator, creators) {
            TrackVisualizer * const visualizer =
                creator->createTrackVisualizer();
            d->visualizers.append(visualizer); d->tabs->addTab(visualizer,
                    visualizer->tabTitle());
        }
    }
    for (unsigned i = 0; i < unsigned(d->visualizers.size()); ++i) {
        connect(d->visualizers[i], SIGNAL(saveTracksRequested(QList<QList<igotu::IgotuPoint>>)),
                d.get(), SLOT(saveTracksRequested(QList<QList<igotu::IgotuPoint>>)));
        connect(d->visualizers[i], SIGNAL(trackActivated(QList<igotu::IgotuPoint>)),
                d.get(), SLOT(trackActivated(QList<igotu::IgotuPoint>)));
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
