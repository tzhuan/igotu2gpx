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

#ifndef _IGOTU2GPX_SRC_IGOTU_COMMANDS_H_
#define _IGOTU2GPX_SRC_IGOTU_COMMANDS_H_

#include "igotucommand.h"

namespace igotu
{

class IGOTU_EXPORT NmeaSwitchCommand : public IgotuCommand
{
public:
    NmeaSwitchCommand(DataConnection *connection, bool enable);

    virtual QByteArray sendAndReceive();

private:
    bool enable;
};

class IGOTU_EXPORT IdentificationCommand : public IgotuCommand
{
public:
    IdentificationCommand(DataConnection *connection);

    virtual QByteArray sendAndReceive();

    unsigned serialNumber() const;
    unsigned firmwareVersion() const; // 0xAAII AA major II minor
    QString firmwareVersionString() const;

private:
    unsigned id;
    unsigned version;
};

class IGOTU_EXPORT ModelCommand : public IgotuCommand
{
public:
    ModelCommand(DataConnection *connection);

    virtual QByteArray sendAndReceive();

    enum Model {
        Unknown,
        // Gt100,
        Gt120,
        Gt200
    };

    Model modelId() const;
    QString modelName() const;

private:
    Model id;
    QString name;
};

class IGOTU_EXPORT CountCommand : public IgotuCommand
{
public:
    CountCommand(DataConnection *connection, bool gt120BugWorkaround);

    virtual QByteArray sendAndReceive();

    unsigned trackPointCount() const;

private:
    unsigned count;
    bool bugWorkaround;
};

class IGOTU_EXPORT ReadCommand : public IgotuCommand
{
public:
    ReadCommand(DataConnection *connection, unsigned pos, unsigned size);

    virtual QByteArray sendAndReceive();

    QByteArray data() const;

private:
    unsigned size;
    QByteArray result;
};

class IGOTU_EXPORT WriteCommand : public IgotuCommand
{
public:
    WriteCommand(DataConnection *connection, unsigned writeMode, unsigned pos,
            const QByteArray &data);

    virtual QByteArray sendAndReceive();

private:
    QByteArray data;
};

class IGOTU_EXPORT UnknownWriteCommand1 : public IgotuCommand
{
public:
    UnknownWriteCommand1(DataConnection *connection, unsigned mode);

    virtual QByteArray sendAndReceive();
};

class IGOTU_EXPORT UnknownWriteCommand2 : public IgotuCommand
{
public:
    UnknownWriteCommand2(DataConnection *connection, unsigned size);

    virtual QByteArray sendAndReceive();

private:
    unsigned size;
};

class IGOTU_EXPORT UnknownPurgeCommand1 : public IgotuCommand
{
public:
    UnknownPurgeCommand1(DataConnection *connection, unsigned mode);

    virtual QByteArray sendAndReceive();
};

class IGOTU_EXPORT UnknownPurgeCommand2 : public IgotuCommand
{
public:
    UnknownPurgeCommand2(DataConnection *connection);

    virtual QByteArray sendAndReceive();
};

} // namespace igotu

#endif

