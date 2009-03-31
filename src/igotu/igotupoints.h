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

