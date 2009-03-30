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

#ifndef _IGOTU_SRC_IGOTU_WAYPOINTS_H_
#define _IGOTU_SRC_IGOTU_WAYPOINTS_H_

#include "global.h"

#include <boost/scoped_ptr.hpp>

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QList>

namespace igotu
{

class IgotuPointPrivate;

class IGOTU_EXPORT IgotuPoint
{
    Q_DECLARE_TR_FUNCTIONS(IgotuPoint)
public:
    /* 32 byte records:
     * 0x00 uint8  flag byte:
     *               0x04: waypoint
     * 0x01 uint8  date:
     *               0xf0: year - 2000 (TODO: only good until 2015?)
     *               0x0f: month, starting with 1
     * 0x02 uint16 date:
     *               0xf800: day of month, starting with 1
     *               0x07c0: hour
     *               0x003f: minute
     * 0x04 uint16 date:
     *               0xffff: seconds * 1000
     * ...
     * 0x0c uint32 latitude in degrees * 1e7
     * 0x10 uint32 longitude in degrees * 1e7
     * 0x14 uint32 elevation in m * 1e2
     * 0x18 uint16 speed in m/s * 1e2
     * ...
     *
     */
    IgotuPoint(const QByteArray &record);
    ~IgotuPoint();

    // in degrees
    double longitude() const;
    // in degrees
    double latitude() const;
    // in m
    double elevation() const;
    // in km/h
    double speed() const;

    QDateTime dateTime() const;

    bool isWayPoint() const;

    bool isValid() const;

    unsigned unknown1() const;
    unsigned unknown2() const;

    QByteArray unknownDataDump() const;

private:
    QByteArray record;
};

class IgotuPointsPrivate;

class IGOTU_EXPORT IgotuPoints
{
public:
    IgotuPoints(const QByteArray &dump, int count = -1);
    ~IgotuPoints();

    QList<IgotuPoint> points() const;
    QString gpx() const;

    // in s
    unsigned logInterval() const;
    bool isIntervalChangeEnabled() const;

private:
    QByteArray dump;
    int count;
};

} // namespace igotu

#endif

