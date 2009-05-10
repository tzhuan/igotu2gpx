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

#ifndef _IGOTU2GPX_SRC_IGOTU_IGOTUPOINTS_H_
#define _IGOTU2GPX_SRC_IGOTU_IGOTUPOINTS_H_

#include "global.h"

#include <boost/scoped_ptr.hpp>

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QList>

namespace igotu
{

class IGOTU_EXPORT IgotuPoint
{
    Q_DECLARE_TR_FUNCTIONS(IgotuPoint)
public:
    IgotuPoint(const QByteArray &record);
    ~IgotuPoint();

    bool isValid() const;
    bool isWayPoint() const;

    QDateTime dateTime() const;
    // in degrees
    double longitude() const;
    // in degrees
    double latitude() const;
    // in m
    double elevation() const;
    // in km/h
    double speed() const;
    // in degrees
    double course() const;

    QList<unsigned> satellites() const;
    // in m
    double ehpe() const;

    unsigned flags() const;
    // in seconds
    unsigned timeout() const;
    unsigned msvsQcn() const;
    unsigned weightCriteria() const;
    unsigned sleepTime() const;

    QByteArray hex() const;

private:
    QByteArray record;
};

class IGOTU_EXPORT ScheduleTableEntry
{
    Q_DECLARE_TR_FUNCTIONS(ScheduleTableEntry)
public:
    ScheduleTableEntry(const QByteArray &entry);
    ~ScheduleTableEntry();

    bool isValid() const;

    // in s
    unsigned logInterval() const;
    // in s
    unsigned changedLogInterval() const;
    // in km/h
    double intervalChangeSpeed() const;
    bool isIntervalChangeEnabled() const;

    QTime startTime() const;
    QTime endTime() const;

private:
    QByteArray entry;
};

class IGOTU_EXPORT IgotuPoints
{
    Q_DECLARE_TR_FUNCTIONS(IgotuPoints)
public:
    IgotuPoints(const QByteArray &dump, unsigned count);
    ~IgotuPoints();

    QList<IgotuPoint> points() const;
    QString gpx() const;

    bool isValid() const;

    unsigned securityVersion() const;
    bool isPasswordEnabled() const;
    QString password() const;

    bool isScheduleTableEnabled() const;
    // one-based
    QList<unsigned> scheduleTablePlans() const;
    QList<ScheduleTableEntry> scheduleTableEntries(unsigned plan) const;
    QDate firstScheduleDate() const;
    unsigned dateOffset() const;

    bool isButtonEnabled() const;
    bool ledsEnabled() const;

private:
    QByteArray dump;
    int count;
};

} // namespace igotu

#endif

