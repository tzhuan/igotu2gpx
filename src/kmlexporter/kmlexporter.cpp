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

#include "igotu/utils.h"
#include "igotu/xmlutils.h"

#include "fileexporter.h"

#include <QTextStream>

using namespace igotu;

class KmlExporter :
    public QObject,
    public FileExporter
{
    Q_OBJECT
    Q_INTERFACES(igotu::FileExporter)
public:
    virtual Mode mode() const;
    virtual int exporterPriority() const;
    virtual QString formatName() const;
    virtual QString formatDescription() const;
    virtual QString fileExtension() const;
    virtual QString fileType() const;
    virtual QByteArray save(const QList<QList<IgotuPoint> > &tracks,
            bool tracksAsSegments, int utcOffset) const;
    virtual QByteArray save(const IgotuPoints &points,
            bool tracksAsSegments, int utcOffset) const;
};

Q_EXPORT_PLUGIN2(gpxExporter, KmlExporter)

// KmlExporter =====================================================

FileExporter::Mode KmlExporter::mode() const
{
    return FileExporter::TrackExport;
}

int KmlExporter::exporterPriority() const
{
    return 100;
}

QString KmlExporter::formatName() const
{
    return QLatin1String("kml");
}

QString KmlExporter::formatDescription() const
{
    return tr("Google Earth");
}

QString KmlExporter::fileExtension() const
{
    return QLatin1String("kml");
}

QString KmlExporter::fileType() const
{
    return tr("KML files (%1)").arg(QLatin1String("*.") + fileExtension());
}

QByteArray KmlExporter::save(const IgotuPoints &points, bool tracksAsSegments,
        int utcOffset) const
{
    return save(points.tracks(), tracksAsSegments, utcOffset);
}

QByteArray KmlExporter::save(const QList<QList<IgotuPoint> > &tracks,
        bool tracksAsSegments, int utcOffset) const
{
    Q_UNUSED(utcOffset);
    return pointsToKml(tracks, tracksAsSegments);
}

#include "kmlexporter.moc"
