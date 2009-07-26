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

#include "igotu/exception.h"
#include "igotu/igotupoints.h"
#include "igotu/paths.h"

#include "trackvisualizer.h"

#include <marble/MarbleDirs.h>
#include <marble/MarbleMap.h>
#include <marble/MarbleWidget.h>

#include <QDir>
#include <QTemporaryFile>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

using namespace igotu;

class MarbleVisualizer: public TrackVisualizer
{
    Q_OBJECT
public:
    MarbleVisualizer(QWidget *parent = NULL);

    virtual void setTracks(const igotu::IgotuPoints &points, int utcOffset);
    virtual QString tabTitle() const;
    virtual int priority() const;
    virtual void highlightTrack(const QList<IgotuPoint> &track);

Q_SIGNALS:
    void saveTracksRequested(const QList<QList<igotu::IgotuPoint> > &tracks);
    void trackActivated(const QList<igotu::IgotuPoint> &tracks);

private:
    void initMarble();

private:
    QLayout *verticalLayout;
    Marble::MarbleWidget *tracks;
    boost::scoped_ptr<QTemporaryFile> kmlFile;
};

class MarbleVisualizerCreator :
    public QObject,
    public TrackVisualizerCreator
{
    Q_OBJECT
    Q_INTERFACES(TrackVisualizerCreator)
public:
    virtual QStringList trackVisualizers() const;
    virtual TrackVisualizer *createTrackVisualizer(const QString &name,
            QWidget *parent = NULL) const;
};

Q_EXPORT_PLUGIN2(marbleVisualizer, MarbleVisualizerCreator)

static QByteArray pointsToKml(const QList<IgotuPoint> &wayPoints, const QList<QList<IgotuPoint> > &tracks)
{
    QByteArray result;
    QTextStream out(&result);
    out.setCodec("UTF-8");
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
           "<Document>\n"
           "<Style id=\"line\">\n"
           "    <LineStyle>\n"
           "    <color>73FF0000</color>\n"
           "    <width>5</width>\n"
           "    </LineStyle>\n"
           "</Style>\n";

    Q_FOREACH (const IgotuPoint &point, wayPoints) {
        out << "<Placemark>\n"
               "<Point>\n"
               "<coordinates>\n";
        out << point.longitude() << ',' << point.latitude() << '\n';
        out << "</coordinates>\n"
               "</Point>\n"
               "</Placemark>\n";
    }

    out << "<Folder>\n";
    Q_FOREACH (const QList<IgotuPoint> &track, tracks) {
        out << "<Placemark>\n"
               "<styleUrl>#line</styleUrl>\n"
               "<LineString>\n"
               "<tessellate>1</tessellate>\n"
               "<coordinates>\n";
        Q_FOREACH (const IgotuPoint &point, track)
            out << point.longitude() << ',' << point.latitude() << '\n';
        out << "</coordinates>\n"
               "</LineString>\n"
               "</Placemark>\n";
    }

    out << "</Folder>\n";

    out << "</Document>\n"
           "</kml>\n";

    out.flush();

    return result;
}

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

#ifdef Q_OS_MACX
    Marble::MarbleDirs::setMarblePluginPath(Paths::macPluginDirectory() + QLatin1String("/marble"));
    Marble::MarbleDirs::setMarbleDataPath(Paths::macDataDirectory() + QLatin1String("/marble"));
#endif
    tracks = new Marble::MarbleWidget(this);
    tracks->setObjectName(QLatin1String("tracks"));

    verticalLayout->addWidget(tracks);

#if MARBLE_VERSION < 0x000800
    // This is a hack to get a HttpDownloadManager instance from MarbleMap
    // because we can't instantiate it ourselves
    tracks->map()->setDownloadUrl(QUrl());
#endif
//    tracks->map()->setProjection(Marble::Mercator);
    tracks->setMapThemeId(QLatin1String("earth/openstreetmap/openstreetmap.dgml"));
    // TODO: disable plugins that are not used (wikipedia)
}

void MarbleVisualizer::setTracks(const igotu::IgotuPoints &points, int utcOffset)
{
    Q_UNUSED(utcOffset);

    // TODO: crashes marble, ugly hacky workaround
//    if (kmlFile)
//        tracks->removePlaceMarkKey(kmlFile->fileName());
    double oldLongitude;
    double oldLatitude;
    int oldZoom;
    bool restoreView = false;
    if (tracks) {
        oldLongitude = tracks->centerLongitude();
        oldLatitude = tracks->centerLatitude();
        oldZoom = tracks->zoom();
        restoreView = true;
    }
    initMarble();

    const QList<IgotuPoint> wayPoints = points.wayPoints();
    const QList<QList<IgotuPoint> > trackPoints = points.tracks();

    kmlFile.reset(new QTemporaryFile(QDir::tempPath() +
                QLatin1String("/igotu2gpx_temp_XXXXXX.kml")));
    if (!kmlFile->open())
        throw IgotuError(tr("Unable to create kml file: %1")
                .arg(kmlFile->errorString()));
    kmlFile->write(pointsToKml(wayPoints, trackPoints));
    kmlFile->flush();
#if MARBLE_VERSION < 0x000800
    tracks->addPlaceMarkFile(kmlFile->fileName());
#else
    tracks->addPlacemarkFile(kmlFile->fileName());
#endif
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

int MarbleVisualizer::priority() const
{
    return 0;
}

void MarbleVisualizer::highlightTrack(const QList<IgotuPoint> &track)
{
    tracks->setCenterLongitude(track.at(0).longitude());
    tracks->setCenterLatitude(track.at(0).latitude());
}

// MarbleVisualizerCreator =====================================================

QStringList MarbleVisualizerCreator::trackVisualizers() const
{
    return QStringList() << QLatin1String("marble");
}

TrackVisualizer *MarbleVisualizerCreator::createTrackVisualizer
    (const QString &name, QWidget *parent) const
{
    if (name == QLatin1String("marble"))
        return new MarbleVisualizer(parent);

    return NULL;
}

#include "marblevisualizer.moc"
