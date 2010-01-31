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
        qCritical("Invalid table entry dump size");
    }
}

ScheduleTableEntry::~ScheduleTableEntry()
{
}

ScheduleTableEntry ScheduleTableEntry::gt120DefaultEntry()
{
    return ScheduleTableEntry(QByteArray
        ("\x00\x00\x00\x00\x3c\x3c\x00\x05\x00\x00\x00\xc8\x0c\x01\x2c\x32"
         "\x05\x14\x01\x01\x01\x02\x02\x05\x0a\x0a\x18\x6a\x0c\x35\x04\xe2"
         "\x00\xbb\x01\x17\x1e\x00\x01\x08\x08\x04\x04\x02\x02\x02\x02\x00"
         "\x00\x20\x0f\x01\x01\x01\x01\x28\x00\x00\x00\x00\x00\x00\x00\x00",
         64));
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

IgotuConfig IgotuConfig::gt120DefaultConfig()
{
    IgotuConfig config;
    // Set flags to zero
    config.memoryDump()[0x0003] = 0x00;
    // Some timeouts?
    config.memoryDump()[0x0005] = 0x0f;
    config.memoryDump()[0x0007] = 0x3c;
    config.setScheduleTableEnabled(false);
    config.setScheduleTablePlans(QList<unsigned>() << 1);
    config.setFirstScheduleDate(QDate::currentDate());
    config.setDateOffset(0);
    for (unsigned plan = 1; plan <= 7; ++plan)
        for (unsigned index = 0; index < 4; ++index)
            config.setScheduleTableEntry(plan, index,
                    ScheduleTableEntry::ScheduleTableEntry::gt120DefaultEntry());
    // Various stuff where nobody knows whether it's important
    config.memoryDump().replace(0x0800, 0x0800, QByteArray(0x0800, char(0x00)));
    config.memoryDump().replace(0x0900, 0x0030, QByteArray
            ("\xa0\xa2\x00\x19\x80\xff\xd2\x7b\xeb\x00\x4b\xc8\x62\x00\x28\x92"
             "\xaf\x00\x01\x77\x00\x01\x3a\x18\x84\x05\xa7\x0c\x88\x09\x24\xb0"
             "\xb3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
             0x0030));
    config.memoryDump().replace(0x0a00, 0x0050, QByteArray
            ("\x32\x11\xa0\xa2\x00\x09\x97\x00\x00\x00\xc8\x00\x00\x00\xc8\x02"
             "\x27\xb0\xb3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
             "\x32\x17\xa0\xa2\x00\x0f\xa7\x00\x00\x03\xe8\x00\x2d\xc6\xc0\x00"
             "\x00\x07\x08\x00\x01\x03\x19\xb0\xb3\x00\x00\x00\x00\x00\x00\x00"
             "\x32\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
             0x0050));
    config.memoryDump().replace(0x0ff0, 0x0010, QByteArray
            ("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
             0x0010));
    return config;
}

bool IgotuConfig::isValid() const
{
    return dump != QByteArray(0x1000, '\xff');
}

QByteArray IgotuConfig::memoryDump() const
{
    return dump;
}

QByteArray &IgotuConfig::memoryDump()
{
    return dump;
}

bool IgotuConfig::isScheduleTableEnabled() const
{
    return (uchar(dump[0x0003]) & 0x04) != 0;
}

void IgotuConfig::setScheduleTableEnabled(bool value)
{
    dump[0x0003] = (dump[0x0003] & ~0x04) | (value ? 0x04 : 0);
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

void IgotuConfig::setScheduleTablePlans(const QList<unsigned> &plans)
{
    const unsigned count = plans.size();
    dump[0x0000] = count;
    // in dumps, only addresses up to 0x83 have been used
    for (unsigned i = 0x0009, j = 0; i < 0x0100; ++i) {
        dump[i] = 0xff;
        if (j >= count)
            continue;
        dump[i] = (dump[i] & 0xf0) + plans[j++];
        if (j >= count)
            continue;
        dump[i] = (dump[i] & 0x0f) + (plans[j++] << 4);
    }
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
            (reinterpret_cast<const uchar*>(dump.data() + 0x0001));
    return QDate::fromJulianDay(QDate(2000, 1, 1).toJulianDay() + days - 1);
}

void IgotuConfig::setFirstScheduleDate(const QDate &date)
{
    unsigned days = date.toJulianDay() + 1 - QDate(2000, 1, 1).toJulianDay();
    qToBigEndian<quint16>(days, reinterpret_cast<uchar*>(dump.data() + 0x0001));
}

unsigned IgotuConfig::dateOffset() const
{
    return uchar(dump[0x0008]);
}

void IgotuConfig::setDateOffset(unsigned offset)
{
    dump[0x0008] = offset;
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
