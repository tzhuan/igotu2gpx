/******************************************************************************
 * Copyright (C) 2009 Michael Hofmann <mh21@piware.de>                        *
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

#include "igotu/igotupoints.h"
#include "igotu/paths.h"
#include "igotu/utils.h"

#include "trackvisualizer.h"

#include <marble/MarbleDirs.h>
#include <marble/MarbleMap.h>
#include <marble/MarbleModel.h>
#include <marble/MarbleWidget.h>
#include <marble/RenderPlugin.h>

#include <QDir>
#include <QTemporaryFile>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

using namespace igotu;

#if MARBLE_VERSION >= 0x000700
using namespace Marble;
#endif

class MarbleVisualizer: public TrackVisualizer
{
    Q_OBJECT
public:
    MarbleVisualizer(QWidget *parent = NULL);

    virtual Flags flags() const;
    virtual void setTracks(const igotu::IgotuPoints &points, int utcOffset);
    virtual QString tabTitle() const;
    virtual void highlightTrack(const QList<IgotuPoint> &track);
    virtual void saveSelectedTracks();

Q_SIGNALS:
    void saveTracksRequested(const QList<QList<igotu::IgotuPoint> > &tracks);
    void trackActivated(const QList<igotu::IgotuPoint> &tracks);
    // void trackSelectionChanged(bool selected); // no selection supported

private:
    void initMarble();

private:
    QLayout *verticalLayout;
    MarbleWidget *tracks;
    boost::scoped_ptr<QTemporaryFile> kmlFile;
};

class MarbleVisualizerCreator :
    public QObject,
    public TrackVisualizerCreator
{
    Q_OBJECT
    Q_INTERFACES(TrackVisualizerCreator)
public:
    virtual QString trackVisualizer() const;
    virtual int visualizerPriority() const;
    virtual AppearanceModes supportedVisualizerAppearances() const;

    virtual TrackVisualizer *createTrackVisualizer(AppearanceMode mode,
            QWidget *parent = NULL) const;
};

Q_EXPORT_PLUGIN2(marbleVisualizer, MarbleVisualizerCreator)

// MarbleVisualizer ============================================================

MarbleVisualizer::MarbleVisualizer(QWidget *parent) :
    TrackVisualizer(parent)
{
    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setMargin(0);

    tracks = NULL;
    initMarble();
}

void MarbleVisualizer::initMarble()
{
    delete tracks;

#if defined(Q_OS_MAC) || defined(Q_OS_WIN32)
    MarbleDirs::setMarblePluginPath(Paths::mainPluginDirectory() +
            QLatin1String("/marble"));
    MarbleDirs::setMarbleDataPath(Paths::mainDataDirectory() +
            QLatin1String("/marble"));
#endif
    tracks = new MarbleWidget(this);
    tracks->setObjectName(QLatin1String("tracks"));

    verticalLayout->addWidget(tracks);

    tracks->setProjection(Mercator);
    tracks->setMapThemeId(QLatin1String
            ("earth/openstreetmap/openstreetmap.dgml"));
    tracks->setShowGrid(false);
    tracks->goHome(Instant);
    // TODO: disable plugins that are not used (wikipedia)
}

void MarbleVisualizer::setTracks(const igotu::IgotuPoints &points,
        int utcOffset)
{
    Q_UNUSED(utcOffset);

    // TODO: crashes marble, ugly hacky workaround
//    if (kmlFile)
//        tracks->removePlaceMarkKey(kmlFile->fileName());
    double oldLongitude = 0;
    double oldLatitude = 0;
    int oldZoom = 0;
    bool restoreView = false;
    if (tracks) {
        oldLongitude = tracks->centerLongitude();
        oldLatitude = tracks->centerLatitude();
        oldZoom = tracks->zoom();
        restoreView = true;
    }
    initMarble();

    const QList<QList<IgotuPoint> > trackPoints = points.tracks();

    // Filename split so it does not trigger "make todo", do not delete the
    // spaces between the strings otherwise qmake/moc does not detect the moc
    // include at the end
    kmlFile.reset(new QTemporaryFile(QDir::tempPath() +
                QLatin1String("/igotu2gpx_temp_XX" "XX" "XX.kml")));
    if (!kmlFile->open()) {
        qCritical("Unable to create temporary kml file: %s",
                qPrintable(kmlFile->errorString()));
        return;
    }
    kmlFile->write(pointsToKml(trackPoints, false));
    kmlFile->flush();
    tracks->model()->addGeoDataFile(kmlFile->fileName());
    if (!trackPoints.isEmpty()) {
        highlightTrack(trackPoints.at(0));
    } else if (restoreView) {
        tracks->setCenterLongitude(oldLongitude);
        tracks->setCenterLatitude(oldLatitude);
    }
    if (restoreView) {
        tracks->zoomView(oldZoom);
    }
}

QString MarbleVisualizer::tabTitle() const
{
    return tr("Map");
}

TrackVisualizer::Flags MarbleVisualizer::flags() const
{
    return 0;
}

void MarbleVisualizer::highlightTrack(const QList<IgotuPoint> &track)
{
    if (track.isEmpty())
        return;

    tracks->setCenterLongitude(track.at(0).longitude());
    tracks->setCenterLatitude(track.at(0).latitude());
}

void MarbleVisualizer::saveSelectedTracks()
{
    qCritical("Visualizer does not support track selection");
}

// MarbleVisualizerCreator =====================================================

QString MarbleVisualizerCreator::trackVisualizer() const
{
    return QLatin1String("marble");
}

int MarbleVisualizerCreator::visualizerPriority() const
{
    return 0;
}

TrackVisualizerCreator::AppearanceModes
MarbleVisualizerCreator::supportedVisualizerAppearances() const
{
    return MainWindowAppearance;
}

TrackVisualizer *MarbleVisualizerCreator::createTrackVisualizer
        (AppearanceMode mode, QWidget *parent) const
{
    Q_UNUSED(mode);

    return new MarbleVisualizer(parent);
}

#include "marblevisualizer.moc"
