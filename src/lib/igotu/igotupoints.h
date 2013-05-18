/******************************************************************************
 * Copyright (C) 2009  Michael Hofmann <mh21@mh21.de>                         *
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
#include <QMetaType>

namespace igotu
{

class IGOTU_EXPORT IgotuPoint
{
    Q_DECLARE_TR_FUNCTIONS(igotu::IgotuPoint)
public:
    IgotuPoint();
    IgotuPoint(const QByteArray &record);
    ~IgotuPoint();

    bool isValid() const;
    bool isWayPoint() const;
    bool isTrackStart() const;

    QDateTime dateTime() const;
    // offset in seconds
    QString humanDateTimeString(int utcOffset = 0) const;
    // offset in seconds
    QString dateTimeString(int utcOffset = 0) const;

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

class IGOTU_EXPORT IgotuPoints
{
    Q_DECLARE_TR_FUNCTIONS(igotu::IgotuPoints)
public:
    IgotuPoints(const QByteArray &dump, unsigned count);
    ~IgotuPoints();

    // all trackpoints
    QList<IgotuPoint> points() const;
    // isValid() && isWayPoint()
    QList<IgotuPoint> wayPoints() const;
    // isValid() and grouped into tracks
    QList<QList<IgotuPoint> > tracks() const;

private:
    QByteArray dump;
    int count;
};

} // namespace igotu

Q_DECLARE_METATYPE(QList<igotu::IgotuPoint>)

#endif

