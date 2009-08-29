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

IgotuPoint::IgotuPoint(const QByteArray &record) :
    record(record)
{
    if (record.size() < 32) {
        this->record += QByteArray(32 - record.size(), char(0xff));
        qWarning("Invalid record size");
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

// ScheduleTableEntry ==========================================================

ScheduleTableEntry::ScheduleTableEntry(const QByteArray &entry) :
    entry(entry)
{
    if (entry.size() < 64) {
        this->entry += QByteArray(64 - entry.size(), char(0xff));
        qWarning("Invalid entry size");
    }
}

ScheduleTableEntry::~ScheduleTableEntry()
{
}

bool ScheduleTableEntry::isValid() const
{
    return entry.mid(0, 4) != QByteArray(4, '\xff');
}

unsigned ScheduleTableEntry::logInterval() const
{
    return uchar(entry[0x07]) + 1;
}

unsigned ScheduleTableEntry::changedLogInterval() const
{
    return uchar(entry[0x0a]) + 1;
}

double ScheduleTableEntry::intervalChangeSpeed() const
{
    return 1e-2 * 3.6 * qFromBigEndian<quint16>
        (reinterpret_cast<const uchar*>(entry.data()) + 0x08);
}

bool ScheduleTableEntry::isIntervalChangeEnabled() const
{
    return intervalChangeSpeed() != 0.0;
}

QTime ScheduleTableEntry::startTime() const
{
    if (entry.mid(0x00, 2) == QByteArray(2, '\xff'))
        return QTime(-1, -1);
    return QTime(entry[0], entry[1]);
}

QTime ScheduleTableEntry::endTime() const
{
    if (entry.mid(0x02, 2) == QByteArray(2, '\xff'))
        return QTime(-1, -1);
    return QTime(entry[2], entry[3]);
}

// IgotuPoints =================================================================

IgotuPoints::IgotuPoints(const QByteArray &dump, unsigned count) :
    dump(dump),
    count(count)
{
    if (0x1000 + count * 0x20 > unsigned(dump.size())) {
	this->dump += QByteArray(0x1000 + count * 0x20 - dump.size(), char(0xff));
        qWarning("Invalid dump size");
    }
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

bool IgotuPoints::isValid() const
{
    return dump.mid(0, 0x1000) != QByteArray(0x1000, '\xff');
}

bool IgotuPoints::isScheduleTableEnabled() const
{
    return (uchar(dump[0x0003]) & 0x04) != 0;
}

bool IgotuPoints::ledsEnabled() const
{
    return (uchar(dump[0x0003]) & 0x80) == 0;
}

bool IgotuPoints::isButtonEnabled() const
{
    return (uchar(dump[0x0003]) & 0x40) == 0;
}

QList<unsigned> IgotuPoints::scheduleTablePlans() const
{
    unsigned count = uchar(dump[0x0000]);
    QList<unsigned> result;
    // in dumps, only addresses up to 0x83 have been used
    for (unsigned i = 0x0009, j = 0; i < 0x0100; ++i) {
        unsigned lowPlan = uchar(dump[i]) & 0x0f;
        unsigned highPlan = uchar(dump[i]) >> 4;
        if (j++ >= count)
            break;
        result += lowPlan;
        if (j++ >= count)
            break;
        result += highPlan;
    }
    return result;
}

QList<ScheduleTableEntry> IgotuPoints::scheduleTableEntries(unsigned plan) const
{
    QList<ScheduleTableEntry> result;
    for (unsigned i = 0; i < 4; ++i)
        result += ScheduleTableEntry(dump.mid(plan * 0x0100 + i * 0x0040,
                    0x40));
    return result;
}

QDate IgotuPoints::firstScheduleDate() const
{
    unsigned days = qFromBigEndian<quint16>
            (reinterpret_cast<const uchar*>(dump.data() + 1));
    return QDate::fromJulianDay(QDate(2000, 1, 1).toJulianDay() + days - 1);
}

unsigned IgotuPoints::dateOffset() const
{
    return uchar(dump[0x0008]);
}

unsigned IgotuPoints::securityVersion() const
{
    return uchar(dump[0x0800]);
}

bool IgotuPoints::isPasswordEnabled() const
{
    return securityVersion() == 0 && dump[0x0801];
}

QString IgotuPoints::password() const
{
    if (securityVersion() != 0)
        return QString();

    const unsigned length = qMin(uchar('\x80'), uchar(dump[0x0802]));
    QVector<ushort> result(length / 2);
    for (unsigned i = 0; i < unsigned(result.size()); ++i)
        result[i] = qFromLittleEndian<ushort>
            (reinterpret_cast<const uchar*>(dump.data() + 0x0803 + i * 2));
    return QString::fromUtf16(result.data(), result.size());
}

QByteArray IgotuPoints::gpx(int utcOffset) const
{
    return gpx(tracks(), utcOffset);
}

QByteArray IgotuPoints::gpx(const QList<QList<IgotuPoint> > &tracks,
        int utcOffset)
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

    Q_FOREACH (const QList<IgotuPoint> &track, tracks) {
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
        out << xmlIndent(1) << "</trk>\n";
    }
    out << "</gpx>\n";

    out.flush();

    return result;
}

} // namespace igotu

