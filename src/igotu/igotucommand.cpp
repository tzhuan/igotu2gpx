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

#include "dataconnection.h"
#include "exception.h"
#include "igotucommand.h"

#include <numeric>

namespace igotu
{

class IgotuCommandPrivate
{
public:
    unsigned sendCommand(const QByteArray &data);
    int receiveResponseSize();
    QByteArray receiveResponseRemainder(unsigned size);

    DataConnection *connection;
    QByteArray command;
    bool receiveRemainder;
};

// IgotuCommandPrivate =========================================================

int IgotuCommandPrivate::receiveResponseSize()
{
    QByteArray data(connection->receive(3));
    if (data[0] != '\x93')
        throw IgotuError(IgotuCommand::tr("Invalid reply packet: %1")
                .arg(QString::fromAscii(data.toHex())));
    return qFromBigEndian<qint16>(reinterpret_cast<const uchar*>(data.data() + 1));
}

QByteArray IgotuCommandPrivate::receiveResponseRemainder(unsigned size)
{
    return connection->receive(size);
}

unsigned IgotuCommandPrivate::sendCommand(const QByteArray &data)
{
    QByteArray command(data);
    unsigned pieces = (command.size() + 7) / 8;
    command += QByteArray(pieces * 8 - command.size(), 0);
    command[command.size() - 1] = -std::accumulate(command.data() + 0,
            command.data() + command.size() - 2, 0);
    Q_FOREVER {
        bool failed = false;
        int responseSize;
        for (unsigned i = 0; i < pieces && !failed; ++i) {
            connection->send(command.mid(i * 8, 8));
            responseSize = receiveResponseSize();
            if (responseSize < 0) {
                failed = true;
            } else if (responseSize != 0 && i + 1 < pieces) {
                throw IgotuError(IgotuCommand::tr("Non-empty intermediate reply packet: %1")
                        .arg(QString::fromAscii(data.toHex())));
            }
        }
        if (!failed)
            return responseSize;
    }
}

// IgotuCommand ================================================================

IgotuCommand::IgotuCommand(DataConnection *connection, const QByteArray
        &command, bool receiveRemainder) :
    dataPtr(new IgotuCommandPrivate)
{
    D(IgotuCommand);

    d->connection = connection;
    d->command = command;
    d->receiveRemainder = receiveRemainder;
}

IgotuCommand::~IgotuCommand()
{
}

QByteArray IgotuCommand::command() const
{
    D(const IgotuCommand);

    return d->command;
}

void IgotuCommand::setCommand(const QByteArray &command)
{
    D(IgotuCommand);

    d->command = command;
}

DataConnection *IgotuCommand::connection() const
{
    D(const IgotuCommand);

    return d->connection;
}

void IgotuCommand::setConnection(DataConnection *connection)
{
    D(IgotuCommand);

    d->connection = connection;
}

bool IgotuCommand::receiveRemainder() const
{
    D(const IgotuCommand);

    return d->receiveRemainder;
}

void IgotuCommand::setReceiveRemainder(bool value)
{
    D(IgotuCommand);

    d->receiveRemainder = value;
}

QByteArray IgotuCommand::sendAndReceive(bool handleErrors)
{
    D(IgotuCommand);

    Q_FOREVER {
        unsigned size;
        try {
            size = d->sendCommand(d->command);
        } catch (const std::exception &e) {
            if (handleErrors)
                continue;
            throw;
        }
        if (size && d->receiveRemainder)
            return d->receiveResponseRemainder(size);
        return QByteArray();
    }
}

} // namespace igotu
