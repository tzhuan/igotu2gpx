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

#ifndef _IGOTU2GPX_SRC_IGOTU_IGOTUCONFIG_H_
#define _IGOTU2GPX_SRC_IGOTU_IGOTUCONFIG_H_

#include "global.h"

#include <QCoreApplication>
#include <QTime>

namespace igotu
{

class IGOTU_EXPORT ScheduleTableEntry
{
    Q_DECLARE_TR_FUNCTIONS(igotu::ScheduleTableEntry)
public:
    ScheduleTableEntry();
    ScheduleTableEntry(const QByteArray &dump);
    ~ScheduleTableEntry();

    bool isValid() const;

    QByteArray memoryDump() const;

    // in s
    unsigned logInterval() const;
    void setLogInterval(unsigned seconds);
    // in s
    unsigned changedLogInterval() const;
    void setChangedLogInterval(unsigned seconds);
    // in km/h, 0 = disable
    double intervalChangeSpeed() const;
    void setIntervalChangeSpeed(double value);

    QTime startTime() const;
    QTime endTime() const;

private:
    QByteArray dump;
};

class IGOTU_EXPORT IgotuConfig
{
    Q_DECLARE_TR_FUNCTIONS(igotu::IgotuConfig)
public:
    IgotuConfig();
    IgotuConfig(const QByteArray &dump);
    ~IgotuConfig();

    bool isValid() const;

    QByteArray memoryDump() const;

    unsigned securityVersion() const;
    bool isPasswordEnabled() const;
    QString password() const;

    bool isScheduleTableEnabled() const;
    // one-based plan
    QList<unsigned> scheduleTablePlans() const;
    // one-based plan
    QList<ScheduleTableEntry> scheduleTableEntries(unsigned plan) const;
    // one-based plan
    void setScheduleTableEntry(unsigned plan, unsigned index, const ScheduleTableEntry &entry);
    QDate firstScheduleDate() const;
    unsigned dateOffset() const;

    bool isButtonEnabled() const;
    bool ledsEnabled() const;

private:
    QByteArray dump;
};

} // namespace igotu

#endif

