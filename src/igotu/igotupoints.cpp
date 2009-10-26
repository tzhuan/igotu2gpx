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

#include "igotupoints.h"
#include "xmlutils.h"

#include <QDateTime>
#include <QTextStream>

#include <QtEndian>

namespace igotu
{

IgotuPoint::IgotuPoint() :
    record(QByteArray(32, char(0xff)))
{
}

IgotuPoint::IgotuPoint(const QByteArray &record) :
    record(record)
{
    if (record.size() < 32) {
        this->record += QByteArray(32 - record.size(), char(0xff));
        qCritical("Invalid dump size");
    }
}

IgotuPoint::~IgotuPoint()
{
}

bool IgotuPoint::isValid() const
{
    // This test is used by @trip PC
    if (latitude() == 0 && longitude() == 0)
        return false;
    if (!dateTime().isValid())
        return false;
    return (uchar(record[0]) & 0x20) == 0;
}

bool IgotuPoint::isWayPoint() const
{
    return uchar(record[0]) & 0x04;
}

bool IgotuPoint::isTrackStart() const
{
    return uchar(record[0]) & 0x40;
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

QString IgotuPoint::dateTimeString(int utcOffset) const
{
    const QDateTime date = dateTime().addSecs(utcOffset);

    QString result = date.toString(QLatin1String("yyyy-MM-dd'T'hh:mm:ss.zzz"));
    if (utcOffset == 0)
        result += QLatin1Char('Z');
    else
        result += QString::fromLatin1("%1%2:%3")
            .arg(utcOffset < 0 ? QLatin1Char('-') : QLatin1Char('+'))
            .arg((utcOffset / 3600) % 24, 2, 10, QLatin1Char('0'))
            .arg((utcOffset / 60) % 60, 2, 10, QLatin1Char('0'));
    return result;
}

QString IgotuPoint::humanDateTimeString(int utcOffset) const
{
    const QDateTime date = dateTime().addSecs(utcOffset);

    QString result = date.toString(QLatin1String("yyyy-MM-dd hh:mm"));
    if (utcOffset != 0)
        result += QString::fromLatin1("%1%2:%3")
            .arg(utcOffset < 0 ? QLatin1Char('-') : QLatin1Char('+'))
            .arg((utcOffset / 3600) % 24, 2, 10, QLatin1Char('0'))
            .arg((utcOffset / 60) % 60, 2, 10, QLatin1Char('0'));
    return result;
}

QByteArray IgotuPoint::hex() const
{
    return record.toHex();
}

// IgotuPoints =================================================================

IgotuPoints::IgotuPoints(const QByteArray &dump, unsigned count) :
    dump(dump),
    count(count)
{
    if (count * 0x20 > unsigned(dump.size())) {
        this->dump += QByteArray(count * 0x20 - dump.size(), char(0xff));
        qCritical("Invalid dump size");
    }
}

IgotuPoints::~IgotuPoints()
{
}

QList<IgotuPoint> IgotuPoints::points() const
{
    QList<IgotuPoint> result;
    for (unsigned j = 0; j < unsigned(count); ++j)
        result.append(IgotuPoint(dump.mid(j * 0x20, 32)));
    return result;
}

QList<IgotuPoint> IgotuPoints::wayPoints() const
{
    QList<IgotuPoint> result;
    Q_FOREACH (const IgotuPoint &point, points())
        if (point.isValid() && point.isWayPoint())
            result.append(point);
    return result;
}

QList<QList<IgotuPoint> > IgotuPoints::tracks() const
{
    QList<QList<IgotuPoint> > result;
    QList<IgotuPoint> current;
    Q_FOREACH (const IgotuPoint &point, points()) {
        if (point.isTrackStart() && !current.isEmpty()) {
            result.append(current);
            current.clear();
        }
        if (!point.isValid())
            continue;
        current.append(point);
    }
    if (!current.isEmpty())
        result.append(current);
    return result;
}

} // namespace igotu

