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
#include "messages.h"

#include <QtEndian>

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
    bool ignoreProtocolErrors;
};

// Put translations in the right context
//
// TRANSLATOR igotu::IgotuCommand

// IgotuCommandPrivate =========================================================

int IgotuCommandPrivate::receiveResponseSize()
{
    QByteArray data(connection->receive(3));
    if (data.size() != 3)
        throw IgotuProtocolError(IgotuCommand::tr
                ("Response too short: expected %1, got %2 bytes")
                .arg(3).arg(data.size()));
    if (data[0] != '\x93')
        throw IgotuProtocolError(IgotuCommand::tr("Invalid reply packet: %1")
                .arg(QString::fromAscii(data.toHex())));
    return qFromBigEndian<qint16>(reinterpret_cast<const uchar*>
            (data.data() + 1));
}

QByteArray IgotuCommandPrivate::receiveResponseRemainder(unsigned size)
{
    const QByteArray result = connection->receive(size);
    if (unsigned(result.size()) != size)
        throw IgotuProtocolError(IgotuCommand::tr
                ("Response remainder too short: expected %1, got %2 bytes")
                .arg(size).arg(result.size()));
    return result;
}

unsigned IgotuCommandPrivate::sendCommand(const QByteArray &data)
{
    QByteArray command(data);
    unsigned pieces = (command.size() + 7) / 8;
    command += QByteArray(pieces * 8 - command.size(), 0);

    if (command.isEmpty())
        return 0;

    command[command.size() - 1] = -std::accumulate(command.data() + 0,
            command.data() + command.size() - 2, 0);
    int responseSize = 0;
    for (unsigned i = 0; i < pieces; ++i) {
        if (connection->mode().testFlag(DataConnection::NonBlockingPurge))
            connection->purge();
        connection->send(command.mid(i * 8, 8));
        responseSize = receiveResponseSize();
        if (responseSize < 0)
            throw IgotuDeviceError(IgotuCommand::tr("Device responded with "
                        "error code: %1").arg(responseSize));
        if (responseSize != 0 && i + 1 < pieces)
            throw IgotuProtocolError(IgotuCommand::tr
                    ("Non-empty intermediate reply packet: %1")
                    .arg(QString::fromAscii(data.toHex())));
    }
    return responseSize;
}

// IgotuCommand ================================================================

IgotuCommand::IgotuCommand(DataConnection *connection, const QByteArray
        &command, bool receiveRemainder) :
    d(new IgotuCommandPrivate)
{
    d->connection = connection;
    d->command = command;
    d->receiveRemainder = receiveRemainder;
    d->ignoreProtocolErrors = false;
}

IgotuCommand::~IgotuCommand()
{
}

QByteArray IgotuCommand::command() const
{
    return d->command;
}

void IgotuCommand::setCommand(const QByteArray &command)
{
    d->command = command;
}

DataConnection *IgotuCommand::connection() const
{
    return d->connection;
}

void IgotuCommand::setConnection(DataConnection *connection)
{
    d->connection = connection;
}

bool IgotuCommand::receiveRemainder() const
{
    return d->receiveRemainder;
}

void IgotuCommand::setReceiveRemainder(bool value)
{
    d->receiveRemainder = value;
}

bool IgotuCommand::ignoreProtocolErrors() const
{
    return d->ignoreProtocolErrors;
}

void IgotuCommand::setIgnoreProtocolErrors(bool value)
{
    d->ignoreProtocolErrors = value;
}

QByteArray IgotuCommand::sendAndReceive()
{
    unsigned protocolErrors = 0;
    unsigned deviceErrors = 0;
    try {
        Q_FOREVER {
            unsigned size;
            QByteArray remainder;

            try {
                size = d->sendCommand(d->command);
                if (size > 0 && d->receiveRemainder)
                    remainder = d->receiveResponseRemainder(size);
            } catch (const IgotuProtocolError &e) {
                // ignore protocol errors if switched to NMEA mode
                if (d->ignoreProtocolErrors) {
                    Messages::verboseMessage(tr("Command: %1")
                                .arg(QString::fromAscii(d->command.toHex())));
                    Messages::verboseMessage(tr("Failed protocol (ignored): %1")
                                .arg(QString::fromLocal8Bit(e.what())));
                    return remainder;
                }
                // Assume this was caused by some spurious NMEA messages
                ++protocolErrors;
                if (protocolErrors <= 5) {
                    Messages::verboseMessage(tr("Command: %1")
                                .arg(QString::fromAscii(d->command.toHex())));
                    Messages::verboseMessage(tr("Failed protocol: %1")
                                .arg(QString::fromLocal8Bit(e.what())));
                    continue;
                }
                throw;
            } catch (const IgotuDeviceError &e) {
                // Device error codes mean we can try again
                ++deviceErrors;
                if (deviceErrors <= 3) {
                    Messages::verboseMessage(tr("Command: %1")
                                .arg(QString::fromAscii(d->command.toHex())));
                    Messages::verboseMessage(tr("Device failure: %1")
                                .arg(QString::fromLocal8Bit(e.what())));
                    continue;
                }
                throw;
            }

            Messages::verboseMessage(tr("Command: %1")
                        .arg(QString::fromAscii(d->command.toHex())));
            Messages::verboseMessage(tr("Result: %1")
                        .arg(QString::fromAscii(remainder.toHex())));
            return remainder;
        }
    } catch (const std::exception &e) {
        Messages::verboseMessage(tr("Command: %1")
                    .arg(QString::fromAscii(d->command.toHex())));
        Messages::verboseMessage(tr("Failed: %1")
                    .arg(QString::fromLocal8Bit(e.what())));
        throw;
    }
}

} // namespace igotu
