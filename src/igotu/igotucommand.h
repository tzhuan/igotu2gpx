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

#ifndef _IGOTU2GPX_SRC_IGOTU_IGOTUCOMMAND_H_
#define _IGOTU2GPX_SRC_IGOTU_IGOTUCOMMAND_H_

#include "global.h"

#include <QCoreApplication>
#include <boost/scoped_ptr.hpp>

namespace igotu
{

class DataConnection;

class IgotuCommandPrivate;

class IGOTU_EXPORT IgotuCommand
{
    Q_DECLARE_TR_FUNCTIONS(IgotuCommand)
public:
    IgotuCommand(DataConnection *connection,
            const QByteArray &command = QByteArray(),
            bool receiveRemainder = true);
    virtual ~IgotuCommand();

    DataConnection *connection() const;
    void setConnection(DataConnection *connection);

    QByteArray command() const;
    void setCommand(const QByteArray &command);

    bool receiveRemainder() const;
    void setReceiveRemainder(bool value);

    bool ignoreProtocolErrors() const;
    void setIgnoreProtocolErrors(bool value);

    bool purgeBuffersBeforeSend() const;
    void setPurgeBuffersBeforeSend(bool value);

    virtual QByteArray sendAndReceive();

private:
    boost::scoped_ptr<IgotuCommandPrivate> d;
};

} // namespace igotu

#endif

