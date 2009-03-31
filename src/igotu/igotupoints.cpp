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

#include "exception.h"
#include "igotupoints.h"
#include "xmlutils.h"

#include <QDateTime>
#include <QTextStream>

#include <QtEndian>

namespace igotu
{

IgotuPoint::IgotuPoint(const QByteArray &record) :
    record(record)
{
    if (record.size() != 32)
        throw IgotuError(tr("Invalid record size"));
}

IgotuPoint::~IgotuPoint()
{
}

bool IgotuPoint::isValid() const
{
    // This test is used by @trip PC
    if (latitude() == 0 && longitude() == 0)
        return false;
    // TODO:this is too simple, location and time information can still be valid
    return (uchar(record[0]) & ~0x04) == 0;
}

unsigned IgotuPoint::flags() const
{
    return uchar(record[0]);
}

double IgotuPoint::longitude() const
{
    return 1e-7 * qFromBigEndian<qint32>
        (reinterpret_cast<const uchar*>(record.data()) + 0x10);
}

double IgotuPoint::latitude() const
{
    return 1e-7 * qFromBigEndian<qint32>
        (reinterpret_cast<const uchar*>(record.data()) + 0x0c);
}

double IgotuPoint::elevation() const
{
    return 1e-2 * qFromBigEndian<qint32>
        (reinterpret_cast<const uchar*>(record.data()) + 0x14);
}

double IgotuPoint::speed() const
{
    return 1e-2 * 3.6 * qFromBigEndian<quint16>
        (reinterpret_cast<const uchar*>(record.data()) + 0x18);
}

double IgotuPoint::course() const
{
    return 1e-2 * qFromBigEndian<quint16>
        (reinterpret_cast<const uchar*>(record.data()) + 0x1a);
}

double IgotuPoint::ehpe() const
{
    return 1e-2 * 0x10 * (qFromBigEndian<quint16>
            (reinterpret_cast<const uchar*>(record.data()) + 0x06) & 0x0fff);
}

unsigned IgotuPoint::timeout() const
{
    return uchar(record[0x1c]);
}

unsigned IgotuPoint::msvsQcn() const
{
    return uchar(record[0x1d]);
}

unsigned IgotuPoint::weightCriteria() const
{
    return uchar(record[0x1e]);
}

unsigned IgotuPoint::sleepTime() const
{
    return uchar(record[0x1f]);
}

QList<unsigned> IgotuPoint::satellites() const
{
    QList<unsigned> result;
    unsigned map = qFromBigEndian<quint32>
        (reinterpret_cast<const uchar*>(record.data()) + 0x08);
    for (unsigned i = 0; i < 32; ++i)
        if (map & (1 << i))
            result << i + 1;
    return result;
}

QDateTime IgotuPoint::dateTime() const
{
    const unsigned date = qFromBigEndian<quint32>
        (reinterpret_cast<const uchar*>(record.data())) & 0x00ffffff;
    const unsigned sec = qFromBigEndian<quint16>
        (reinterpret_cast<const uchar*>(record.data()) + 4);

    return QDateTime
        (QDate(2000 + ((date >> 20) & 0xf), (date >> 16) & 0xf,
               (date >> 11) & 0x1f),
         QTime((date >> 6) & 0x1f, date & 0x3f, sec / 1000, sec % 1000),
         Qt::UTC);
}

bool IgotuPoint::isWayPoint() const
{
    return record[0] & 0x04;
}

QByteArray IgotuPoint::hex() const
{
    return record.toHex();
}

// IgotuPoints ===================================================================

IgotuPoints::IgotuPoints(const QByteArray &dump, int count) :
    dump(dump),
    count(count)
{
    if (this->count == -1)
        this->count = (dump.size() - 0x1000) / 0x20;
    if (this->count < 0)
        this->count = 0;
}

IgotuPoints::~IgotuPoints()
{
}

QList<IgotuPoint> IgotuPoints::points() const
{
    QList<IgotuPoint> result;
    for (unsigned j = 0; j < unsigned(count); ++j)
        result.append(IgotuPoint(dump.mid(0x1000 + j * 0x20, 32)));
    return result;
}

unsigned IgotuPoints::logInterval() const
{
    return dump[0x107] + 1;
}

bool IgotuPoints::isIntervalChangeEnabled() const
{
    return dump[0x108] != '\x00';
}

QString IgotuPoints::gpx() const
{
    QString result;
    QTextStream out(&result);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
           "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 "
           "http://www.topografix.com/GPX/1/1/gpx.xsd\" "
           "creator=\"igotu2gpx\" "
           "version=\"1.1\">\n";

    Q_FOREACH (const IgotuPoint &point, points()) {
        if (!point.isWayPoint() || !point.isValid())
            continue;
        out << xmlIndent(1) << "<wpt "
            << qSetRealNumberPrecision(6)
            << "lat=\"" << point.latitude() << "\" "
            << "lon=\"" << point.longitude() << "\">\n"
            << qSetRealNumberPrecision(2)
            << xmlIndent(2) << "<ele>" << point.elevation() << "</ele>\n"
            << xmlIndent(2) << "<time>" << point.dateTime().toString
                (QLatin1String("yyyy-MM-dd'T'hh:mm:ss.zzz'Z'")) << "</time>\n"
            << xmlIndent(1) << "</wpt>\n";
    }

    out << xmlIndent(1) << "<trk>\n";
    out << xmlIndent(2) << "<trkseg>\n";
    Q_FOREACH (const IgotuPoint &point, points()) {
        if (!point.isValid())
            continue;
        out << xmlIndent(3) << "<trkpt "
            << qSetRealNumberPrecision(6)
            << "lat=\"" << point.latitude() << "\" "
            << "lon=\"" << point.longitude() << "\">\n"
            << qSetRealNumberPrecision(2)
            << xmlIndent(4) << "<ele>" << point.elevation() << "</ele>\n"
            << xmlIndent(4) << "<time>" << point.dateTime().toString
                (QLatin1String("yyyy-MM-dd'T'hh:mm:ss.zzz'Z'")) << "</time>\n"
            << xmlIndent(4) << "<speed>" << point.speed() << "</speed>\n"
            << xmlIndent(3) << "</trkpt>\n";
    }
    out << xmlIndent(2) << "</trkseg>\n";
    out << xmlIndent(1) << "</trk>\n";
    out << "</gpx>\n";

    out.flush();

    return result;
}

} // namespace igotu

