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

#include "fileexporter.h"

#include <QTextStream>

using namespace igotu;

class DetailsExporter :
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

Q_EXPORT_PLUGIN2(gpxExporter, DetailsExporter)

// DetailsExporter =====================================================

FileExporter::Mode DetailsExporter::mode() const
{
    return 0;
}

int DetailsExporter::exporterPriority() const
{
    return 300;
}

QString DetailsExporter::formatName() const
{
    return QLatin1String("details");
}

QString DetailsExporter::formatDescription() const
{
    return QLatin1String("track point details");
}

QString DetailsExporter::fileExtension() const
{
    return QLatin1String("txt");
}

QString DetailsExporter::fileType() const
{
    return tr("Text files (%1)").arg(QLatin1String("*.") + fileExtension());
}

QByteArray DetailsExporter::save(const IgotuPoints &points, bool tracksAsSegments,
        int utcOffset) const
{
    Q_UNUSED(tracksAsSegments);

    QByteArray result;
    QTextStream out(&result);
    out.setCodec("UTF-8");

    unsigned index = 0;
    Q_FOREACH (const IgotuPoint &igotuPoint, points.points()) {
        // No localization to enable parsing
        out << QString::fromLatin1("Record %1\n").arg(++index);
        if (igotuPoint.isWayPoint())
            out << QString::fromLatin1("  Waypoint\n");
        if (igotuPoint.isTrackStart())
            out << QString::fromLatin1("  Track start\n");
        out << QString::fromLatin1("  Date %1\n")
                .arg(igotuPoint.dateTimeString(utcOffset));
        out << QString::fromLatin1("  Latitude %1\n")
                .arg(igotuPoint.latitude(), 0, 'f', 6);
        out << QString::fromLatin1("  Longitude %1\n")
                .arg(igotuPoint.longitude(), 0, 'f', 6);
        out << QString::fromLatin1("  Elevation %1 m\n")
                .arg(igotuPoint.elevation(), 0, 'f', 1);
        out << QString::fromLatin1("  Speed %1 km/h\n")
                .arg(igotuPoint.speed(), 0, 'f', 1);
        out << QString::fromLatin1("  Course %1 degrees\n")
                .arg(igotuPoint.course(), 0, 'f', 2);
        out << QString::fromLatin1("  EHPE %1 m\n")
                .arg(igotuPoint.ehpe(), 0, 'f', 2);
        QString satellites = QLatin1String("  Satellites:");
        Q_FOREACH (unsigned satellite, igotuPoint.satellites())
            satellites += QString::fromLatin1(" %1").arg(satellite);
        out << satellites << '\n';
        out << QString::fromLatin1("  Flags 0x%1\n")
                .arg(igotuPoint.flags(), 2, 16, QLatin1Char('0'));
        out << QString::fromLatin1("  Timeout %1 s\n")
                .arg(igotuPoint.timeout());
        out << QString::fromLatin1("  MSVs_QCN %1\n")
                .arg(igotuPoint.msvsQcn());
        out << QString::fromLatin1("  Weight criteria 0x%1\n")
            .arg(igotuPoint.weightCriteria(), 2, 16, QLatin1Char('0'));
        out << QString::fromLatin1("  Sleep time %1\n")
                .arg(igotuPoint.sleepTime());
    }

    out.flush();

    return result;
}

QByteArray DetailsExporter::save(const QList<QList<IgotuPoint> > &tracks,
        bool tracksAsSegments, int utcOffset) const
{
    Q_UNUSED(tracks);
    Q_UNUSED(tracksAsSegments);
    Q_UNUSED(utcOffset);
    qWarning("Unable to export tracks to details format");
    return QByteArray();
}

#include "detailsexporter.moc"
