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

#include "igotuconfig.h"

namespace igotu
{

// ScheduleTableEntry ==========================================================

ScheduleTableEntry::ScheduleTableEntry() :
    dump(QByteArray(64, char(0xff)))
{
}

ScheduleTableEntry::ScheduleTableEntry(const QByteArray &dump) :
    dump(dump)
{
    if (dump.size() < 64) {
        this->dump += QByteArray(64 - dump.size(), char(0xff));
        qCritical("Invalid dump size");
    }
}

ScheduleTableEntry::~ScheduleTableEntry()
{
}

bool ScheduleTableEntry::isValid() const
{
    return dump.mid(0, 4) != QByteArray(4, '\xff');
}

QByteArray ScheduleTableEntry::memoryDump() const
{
    return dump;
}

unsigned ScheduleTableEntry::logInterval() const
{
    return uchar(dump[0x07]) + 1;
}

void ScheduleTableEntry::setLogInterval(unsigned seconds)
{
    RETURN_IF_FAIL(seconds >= 1 && seconds <= 256);
    dump[0x07] = seconds - 1;
}

unsigned ScheduleTableEntry::changedLogInterval() const
{
    return uchar(dump[0x0a]) + 1;
}

void ScheduleTableEntry::setChangedLogInterval(unsigned seconds)
{
    RETURN_IF_FAIL(seconds >= 1 && seconds <= 256);
    dump[0x0a] = seconds - 1;
}

double ScheduleTableEntry::intervalChangeSpeed() const
{
    return 1e-2 * 3.6 * qFromBigEndian<quint16>
        (reinterpret_cast<const uchar*>(dump.data()) + 0x08);
}

void ScheduleTableEntry::setIntervalChangeSpeed(double value)
{
    qToBigEndian<quint16>(qRound(value / (3.6 * 1e-2)),
        reinterpret_cast<uchar*>(dump.data()) + 0x08);
}

QTime ScheduleTableEntry::startTime() const
{
    if (dump.mid(0x00, 2) == QByteArray(2, '\xff'))
        return QTime(-1, -1);
    return QTime(dump[0], dump[1]);
}

QTime ScheduleTableEntry::endTime() const
{
    if (dump.mid(0x02, 2) == QByteArray(2, '\xff'))
        return QTime(-1, -1);
    return QTime(dump[2], dump[3]);
}

// IgotuConfig =================================================================

IgotuConfig::IgotuConfig() :
    dump(QByteArray(0x1000, char(0xff)))
{
}

IgotuConfig::IgotuConfig(const QByteArray &dump) :
    dump(dump)
{
    if (unsigned(dump.size()) < 0x1000) {
        this->dump += QByteArray(0x1000 - dump.size(), char(0xff));
        qCritical("Invalid dump size");
    }
}

IgotuConfig::~IgotuConfig()
{
}

bool IgotuConfig::isValid() const
{
    return dump != QByteArray(0x1000, '\xff');
}

QByteArray IgotuConfig::memoryDump() const
{
    return dump;
}

bool IgotuConfig::isScheduleTableEnabled() const
{
    return (uchar(dump[0x0003]) & 0x04) != 0;
}

bool IgotuConfig::ledsEnabled() const
{
    return (uchar(dump[0x0003]) & 0x80) == 0;
}

bool IgotuConfig::isButtonEnabled() const
{
    return (uchar(dump[0x0003]) & 0x40) == 0;
}

QList<unsigned> IgotuConfig::scheduleTablePlans() const
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

QList<ScheduleTableEntry> IgotuConfig::scheduleTableEntries(unsigned plan) const
{
    RETURN_VAL_IF_FAIL(plan >= 1 && plan <= 7, QList<ScheduleTableEntry>()
            << ScheduleTableEntry() << ScheduleTableEntry()
            << ScheduleTableEntry() << ScheduleTableEntry());

    QList<ScheduleTableEntry> result;
    for (unsigned i = 0; i < 4; ++i)
        result += ScheduleTableEntry(dump.mid(plan * 0x0100 + i * 0x0040,
                    0x40));
    return result;
}

void IgotuConfig::setScheduleTableEntry(unsigned plan, unsigned index,
        const ScheduleTableEntry &entry)
{
    RETURN_IF_FAIL(plan >= 1 && plan <= 7);
    RETURN_IF_FAIL(index < 4);

    dump.replace(plan * 0x0100 + index * 0x0040, 0x40, entry.memoryDump());
}

QDate IgotuConfig::firstScheduleDate() const
{
    unsigned days = qFromBigEndian<quint16>
            (reinterpret_cast<const uchar*>(dump.data() + 1));
    return QDate::fromJulianDay(QDate(2000, 1, 1).toJulianDay() + days - 1);
}

unsigned IgotuConfig::dateOffset() const
{
    return uchar(dump[0x0008]);
}

unsigned IgotuConfig::securityVersion() const
{
    return uchar(dump[0x0800]);
}

bool IgotuConfig::isPasswordEnabled() const
{
    return securityVersion() == 0 && dump[0x0801];
}

QString IgotuConfig::password() const
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

} // namespace igotu
