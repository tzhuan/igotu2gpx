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

#include "igotu/igotudata.h"
#include "igotu/xmlutils.h"

#include "fileexporter.h"

#include <QTextStream>

using namespace igotu;

class GpxExporter :
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
    virtual QByteArray save(const IgotuData &data,
            bool tracksAsSegments, int utcOffset) const;
};

Q_EXPORT_PLUGIN2(gpxExporter, GpxExporter)

// GpxExporter =====================================================

FileExporter::Mode GpxExporter::mode() const
{
    return FileExporter::TrackExport;
}

int GpxExporter::exporterPriority() const
{
    return 0;
}

QString GpxExporter::formatName() const
{
    return QLatin1String("gpx");
}

QString GpxExporter::formatDescription() const
{
    return tr("GPS exchange format");
}

QString GpxExporter::fileExtension() const
{
    return QLatin1String("gpx");
}

QString GpxExporter::fileType() const
{
    return tr("GPX files (%1)").arg(QLatin1String("*.") + fileExtension());
}

QByteArray GpxExporter::save(const IgotuData &data, bool tracksAsSegments,
        int utcOffset) const
{
    return save(data.points().tracks(), tracksAsSegments, utcOffset);
}

QByteArray GpxExporter::save(const QList<QList<IgotuPoint> > &tracks,
        bool tracksAsSegments, int utcOffset) const
{
    QByteArray result;
    QTextStream out(&result);
    out.setCodec("UTF-8");
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
           "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 "
           "http://www.topografix.com/GPX/1/1/gpx.xsd\" "
           "creator=\"igotu2gpx\" "
           "version=\"1.1\">\n";

    Q_FOREACH (const QList<IgotuPoint> &track, tracks) {
        Q_FOREACH (const IgotuPoint &point, track) {
            if (!point.isWayPoint())
                continue;
            out << xmlIndent(1) << "<wpt "
                << qSetRealNumberPrecision(6)
                << "lat=\"" << point.latitude() << "\" "
                << "lon=\"" << point.longitude() << "\">\n"
                << qSetRealNumberPrecision(2)
                << xmlIndent(2) << "<ele>" << point.elevation() << "</ele>\n"
                << xmlIndent(2) << "<time>" << point.dateTimeString(utcOffset)
                    << "</time>\n"
                << xmlIndent(2) << "<sat>" << point.satellites().count()
                    << "</sat>\n"
                << xmlIndent(1) << "</wpt>\n";
        }
    }

    if (tracksAsSegments)
        out << xmlIndent(1) << "<trk>\n";
    Q_FOREACH (const QList<IgotuPoint> &track, tracks) {
        if (!tracksAsSegments)
            out << xmlIndent(1) << "<trk>\n";
        out << xmlIndent(2) << "<trkseg>\n";
        Q_FOREACH (const IgotuPoint &point, track)
            out << xmlIndent(3) << "<trkpt "
                << qSetRealNumberPrecision(6)
                << "lat=\"" << point.latitude() << "\" "
                << "lon=\"" << point.longitude() << "\">\n"
                << qSetRealNumberPrecision(2)
                << xmlIndent(4) << "<ele>" << point.elevation() << "</ele>\n"
                << xmlIndent(4) << "<time>" << point.dateTimeString(utcOffset)
                    << "</time>\n"
                << xmlIndent(4) << "<sat>" << point.satellites().count()
                    << "</sat>\n"
                << xmlIndent(4) << "<speed>" << point.speed() / 3.6
                    << "</speed>\n"
                << xmlIndent(3) << "</trkpt>\n";
        out << xmlIndent(2) << "</trkseg>\n";
        if (!tracksAsSegments)
            out << xmlIndent(1) << "</trk>\n";
    }
    if (tracksAsSegments)
        out << xmlIndent(1) << "</trk>\n";
    out << "</gpx>\n";

    out.flush();

    return result;
}

#include "gpxexporter.moc"
