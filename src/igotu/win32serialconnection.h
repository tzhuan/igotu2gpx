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

#ifndef _IGOTU2GPX_SRC_IGOTU_WIN32SERIALCONNECTION_H_
#define _IGOTU2GPX_SRC_IGOTU_WIN32SERIALCONNECTION_H_

#include "dataconnection.h"

namespace igotu
{

class Win32SerialConnectionPrivate;

class IGOTU_EXPORT Win32SerialConnection : public DataConnection
{
    Q_DECLARE_TR_FUNCTIONS(Win32SerialConnection)
public:
    Win32SerialConnection(unsigned port);
    ~Win32SerialConnection();

    virtual void send(const QByteArray &query);
    virtual QByteArray receive(unsigned expected);

private:
    DECLARE_PRIVATE(Win32SerialConnection)
protected:
    DECLARE_PRIVATE_DATA(Win32SerialConnection)
};

} // namespace igotu

#endif

