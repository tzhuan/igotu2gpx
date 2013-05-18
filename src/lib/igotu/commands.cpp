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

#include "commands.h"
#include "dataconnection.h"
#include "exception.h"
#include "messages.h"

#include <QTime>

#include <QtEndian>

namespace igotu
{

// Put translations in the right context
//
// TRANSLATOR igotu::IgotuCommand

// NmeaSwitchCommand ===========================================================

NmeaSwitchCommand::NmeaSwitchCommand(DataConnection *connection, bool enable) :
    IgotuCommand(connection),
    enable(enable)
{
    QByteArray command("\x93\x01\x01\0\0\0\0\0\0\0\0\0\0\0\0", 15);
    command[3] = enable ? '\x00' : '\x03';
    setCommand(command);

    if (enable)
        setIgnoreProtocolErrors(true);
}

QByteArray NmeaSwitchCommand::sendAndReceive()
{
    if (!enable)
        connection()->purge();
    return IgotuCommand::sendAndReceive();
}

// IdentificationCommand =======================================================

IdentificationCommand::IdentificationCommand(DataConnection *connection) :
    IgotuCommand(connection)
{
    QByteArray command("\x93\x0a\0\0\0\0\0\0\0\0\0\0\0\0\0", 15);
    setCommand(command);
}

QByteArray IdentificationCommand::sendAndReceive()
{
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (result.size() < 6)
        throw Exception(IgotuCommand::tr("Response too short"));
    id = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>
            (result.data()));
    version = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>
            (result.data() + 4));
    return result;
}

unsigned IdentificationCommand::serialNumber() const
{
    return id;
}

unsigned IdentificationCommand::firmwareVersion() const
{
    return version;
}

QString IdentificationCommand::firmwareVersionString() const
{
    return QString().sprintf("%u.%02u", version >> 8, version & 0xff);
}

// ModelCommand ================================================================

ModelCommand::ModelCommand(DataConnection *connection) :
    IgotuCommand(connection)
{
    QByteArray command("\x93\x05\x04\x00\x03\x01\x9f\0\0\0\0\0\0\0\0", 15);
    setCommand(command);
}

QByteArray ModelCommand::sendAndReceive()
{
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (result.size() < 3)
        throw Exception(IgotuCommand::tr("Response too short"));
    const unsigned part1 = qFromBigEndian<quint16>
        (reinterpret_cast<const uchar*>(result.data()));
    const unsigned part2 =
        *reinterpret_cast<const uchar*>(result.data() + 2);

    name = IgotuCommand::tr("Unknown (%1)").arg(QString::fromAscii
            (result.toHex()));
    id = Unknown;
    if (part1 == 0xC220) {
        switch (part2) {
        case 0x13:
            name = IgotuCommand::tr("GT-100");
            id = Gt100;
            break;
        case 0x14:
            name = IgotuCommand::tr("GT-200");
            id = Gt200;
            break;
        case 0x15:
            name = IgotuCommand::tr("GT-120");
            id = Gt120;
            break;
        case 0x17:
            name = IgotuCommand::tr("GT-200e");
            id = Gt200e;
            break;
        }
    }

    return result;
}

ModelCommand::Model ModelCommand::modelId() const
{
    return id;
}

QString ModelCommand::modelName() const
{
    return name;
}

// CountCommand ================================================================

CountCommand::CountCommand(DataConnection *connection) :
    IgotuCommand(connection)
{
    QByteArray command("\x93\x0b\x03\x00\x1d\0\0\0\0\0\0\0\0\0\0", 15);
    setCommand(command);
}

QByteArray CountCommand::sendAndReceive()
{
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (result.size() < 3)
        throw Exception(IgotuCommand::tr("Response too short"));
    count = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>
            (result.data() + 1));
    return result;
}

unsigned CountCommand::trackPointCount() const
{
    return count;
}

// ReadCommand =================================================================

ReadCommand::ReadCommand(DataConnection *connection, unsigned pos,
        unsigned size) :
    IgotuCommand(connection),
    size(size)
{
    QByteArray command("\x93\x05\x07\x00\x00\x04\x03\0\0\0\0\0\0\0\0", 15);
    command[3] = (size >> 0x08) & 0xff;
    command[4] = (size >> 0x00) & 0xff;
    command[7] = (pos >> 0x10) & 0xff;
    command[8] = (pos >> 0x08) & 0xff;
    command[9] = (pos >> 0x00) & 0xff;
    setCommand(command);
}

QByteArray ReadCommand::sendAndReceive()
{
    result = IgotuCommand::sendAndReceive();
    if (unsigned(result.size()) < size)
        throw Exception(IgotuCommand::tr("Wrong response length"));
    return result;
}

QByteArray ReadCommand::data() const
{
    return result;
}

// WriteCommand ================================================================

WriteCommand::WriteCommand(DataConnection *connection, unsigned writeMode,
        unsigned pos, const QByteArray &data) :
    IgotuCommand(connection),
    data(data)
{
    QByteArray command("\x93\x06\x07\x00\x00\x04\0\0\0\0\0\0\0\0\0", 15);
    command[3] = (data.size() >> 0x08) & 0xff;
    command[4] = (data.size() >> 0x00) & 0xff;
    command[6] = writeMode;
    command[7] = (pos >> 0x10) & 0xff;
    command[8] = (pos >> 0x08) & 0xff;
    command[9] = (pos >> 0x00) & 0xff;
    setCommand(command);

    setReceiveRemainder(false);
}

QByteArray WriteCommand::sendAndReceive()
{
    IgotuCommand::sendAndReceive();
    for (unsigned i = 0; i < (unsigned(data.size()) + 6) / 7; ++i)
        IgotuCommand(connection(), data.mid(i * 7, 7)).sendAndReceive();
    return QByteArray();
}

// UnknownWriteCommand1 ========================================================

UnknownWriteCommand1::UnknownWriteCommand1(DataConnection *connection,
        unsigned mode) :
    IgotuCommand(connection)
{
    QByteArray command("\x93\x06\x04\x00\x00\x01\x06\0\0\0\0\0\0\0\0", 15);
    command[4] = mode;
    setCommand(command);
}

QByteArray UnknownWriteCommand1::sendAndReceive()
{
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (!result.isEmpty())
        throw Exception(IgotuCommand::tr("Response too long"));
    return result;
}

// UnknownWriteCommand2 ========================================================

UnknownWriteCommand2::UnknownWriteCommand2(DataConnection *connection,
        unsigned size) :
    IgotuCommand(connection),
    size(size)
{
    QByteArray command("\x93\x05\x04\x00\x00\x01\x05\0\0\0\0\0\0\0\0", 15);
    command[3] = (size >> 0x08) & 0xff;
    command[4] = (size >> 0x00) & 0xff;
    setCommand(command);
}

QByteArray UnknownWriteCommand2::sendAndReceive()
{
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (unsigned(result.size()) != size)
        throw Exception(IgotuCommand::tr("Wrong response length"));
    return result;
}

// UnknownWriteCommand3 ========================================================

UnknownWriteCommand3::UnknownWriteCommand3(DataConnection *connection) :
    IgotuCommand(connection)
{
    QByteArray command("\x93\x0d\x07\0\0\0\0\0\0\0\0\0\0\0\0", 15);
    setCommand(command);
}

QByteArray UnknownWriteCommand3::sendAndReceive()
{
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (!result.isEmpty())
        throw Exception(IgotuCommand::tr("Response too long"));
    return result;
}

// UnknownPurgeCommand1 ========================================================

UnknownPurgeCommand1::UnknownPurgeCommand1(DataConnection *connection,
        unsigned mode) :
    IgotuCommand(connection)
{
    QByteArray command("\x93\x0c\0\0\0\0\0\0\0\0\0\0\0\0\0", 15);
    command[3] = mode;
    setCommand(command);
}

QByteArray UnknownPurgeCommand1::sendAndReceive()
{
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (!result.isEmpty())
        throw Exception(IgotuCommand::tr("Response too long"));
    return result;
}

// UnknownPurgeCommand2 ========================================================

UnknownPurgeCommand2::UnknownPurgeCommand2(DataConnection *connection) :
    IgotuCommand(connection)
{
    QByteArray command("\x93\x08\x02\0\0\0\0\0\0\0\0\0\0\0\0", 15);
    setCommand(command);
}

QByteArray UnknownPurgeCommand2::sendAndReceive()
{
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (!result.isEmpty())
        throw Exception(IgotuCommand::tr("Response too long"));
    return result;
}

// TimeCommand =================================================================

TimeCommand::TimeCommand(DataConnection *connection, const QTime &time) :
    IgotuCommand(connection)
{
    QByteArray command("\x93\x09\x03\0\0\0\0\0\0\0\0\0\0\0\0", 15);
    command[3] = time.hour();
    command[4] = time.minute();
    command[5] = time.second();
    setCommand(command);
}

QByteArray TimeCommand::sendAndReceive()
{
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (!result.isEmpty())
        throw Exception(IgotuCommand::tr("Response too long"));
    return result;
}

} // namespace igotu

