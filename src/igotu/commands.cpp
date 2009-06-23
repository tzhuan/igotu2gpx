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

#include "commands.h"
#include "dataconnection.h"
#include "exception.h"

#include <QtEndian>

namespace igotu
{

// TODO: some of these return codes should be localized

// NmeaSwitchCommand ===========================================================

NmeaSwitchCommand::NmeaSwitchCommand(DataConnection *connection, bool enable) :
    IgotuCommand(connection)
{
    QByteArray command("\x93\x01\x01\0\0\0\0\0\0\0\0\0\0\0\0", 15);
    command[3] = enable ? '\x00' : '\x03';
    setCommand(command);

    if (enable)
        setIgnoreProtocolErrors(true);
    else
        setPurgeBuffersBeforeSend(true);
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
    // TODO: what are the other 4 bytes?
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (result.size() < 6)
        throw IgotuError(tr("Response too short"));
    id = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>
            (result.data()));
    version = QString().sprintf("%u.%02u",
            *reinterpret_cast<const uchar*>(result.data() + 4),
            *reinterpret_cast<const uchar*>(result.data() + 5));
    return result;
}

unsigned IdentificationCommand::serialNumber() const
{
    return id;
}

QString IdentificationCommand::firmwareVersion() const
{
    return version;
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
        throw IgotuError(tr("Response too short"));
    const unsigned part1 = qFromBigEndian<quint16>
        (reinterpret_cast<const uchar*>(result.data()));
    const unsigned part2 =
        *reinterpret_cast<const uchar*>(result.data() + 2);

    name = QString::fromLatin1("Unknown (%1)").arg(QString::fromAscii
            (result.toHex()));
    id = Unknown;
    if (part1 == 0xC220) {
        switch (part2) {
        case 0x14:
            name = QLatin1String("GT-200");
            id = Gt200;
            break;
        case 0x15:
            name = QLatin1String("GT-120");
            id = Gt120;
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

CountCommand::CountCommand(DataConnection *connection,
        bool gt120BugWorkaround) :
    IgotuCommand(connection),
    gt120BugWorkaround(gt120BugWorkaround)
{
    QByteArray command("\x93\x0b\x03\x00\x1d\0\0\0\0\0\0\0\0\0\0", 15);
    setCommand(command);
}

QByteArray CountCommand::sendAndReceive()
{
    // TODO: what is the first byte?
    const QByteArray result = IgotuCommand::sendAndReceive();
    if (result.size() < 3)
        throw IgotuError(tr("Response too short"));
    count = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>
            (result.data() + 1));

    // TODO: there seems to be a firmware bug that sends the last response code
    // again (apparently only for this command); because libusb support is
    // synchronous, we cannot be sure that the next command purges the transmit
    // buffer before.
    if (gt120BugWorkaround)
        connection()->send(QByteArray(), true);

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
        throw IgotuError(tr("Wrong response length"));
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
        throw IgotuError(tr("Response too long"));
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
        throw IgotuError(tr("Wrong response length"));
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
        throw IgotuError(tr("Response too long"));
    return result;
}
} // namespace igotu

