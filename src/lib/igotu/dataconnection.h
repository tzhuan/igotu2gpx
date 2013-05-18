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

#ifndef _IGOTU2GPX_SRC_IGOTU_DATACONNECTION_H_
#define _IGOTU2GPX_SRC_IGOTU_DATACONNECTION_H_

#include "global.h"

#include <QtPlugin>

namespace igotu
{

class DataConnection
{
public:
    virtual ~DataConnection()
    {
    }

    virtual void send(const QByteArray &query) = 0;
    virtual QByteArray receive(unsigned expected) = 0;
    virtual void purge() = 0;
};

class DataConnectionCreator
{
public:
    virtual ~DataConnectionCreator()
    {
    }

    virtual QString dataConnection() const = 0;
    // lower is better
    virtual int connectionPriority() const = 0;
    virtual QString defaultConnectionId() const = 0;
    virtual DataConnection *createDataConnection(const QString &id) const = 0;
};

} // namespace igotu

Q_DECLARE_INTERFACE(igotu::DataConnectionCreator,
        "de.mh21.igotu2gpx.dataconnection/1.0")

#endif
