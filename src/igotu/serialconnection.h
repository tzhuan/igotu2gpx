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

#ifndef _IGOTU_SRC_IGOTU_SERIALCONNECTION_H_
#define _IGOTU_SRC_IGOTU_SERIALCONNECTION_H_

#include "dataconnection.h"

#include <boost/scoped_ptr.hpp>

#include <QCoreApplication>

namespace igotu
{

class SerialConnectionPrivate;

class IGOTU_EXPORT SerialConnection : public DataConnection
{
    Q_DECLARE_TR_FUNCTIONS(SerialConnection)
public:
    SerialConnection(unsigned device);
    ~SerialConnection();

    virtual void send(const QByteArray &query);
    virtual QByteArray receive(unsigned expected);

private:
    DECLARE_PRIVATE(SerialConnection)
protected:
    DECLARE_PRIVATE_DATA(SerialConnection)
};

} // namespace igotu

#endif

