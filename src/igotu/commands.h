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

#ifndef _IGOTU_SRC_IGOTU_COMMANDS_H_
#define _IGOTU_SRC_IGOTU_COMMANDS_H_

#include "igotucommand.h"

namespace igotu
{

class IGOTU_EXPORT NmeaSwitchCommand : public IgotuCommand
{
public:
    NmeaSwitchCommand(DataConnection *connection, bool enable);
};

class IGOTU_EXPORT IdentificationCommand : public IgotuCommand
{
public:
    IdentificationCommand(DataConnection *connection);

    virtual QByteArray sendAndReceive(bool handleErrors = false);

    unsigned serialNumber() const;

private:
    unsigned id;
};

class IGOTU_EXPORT ReadCommand : public IgotuCommand
{
public:
    ReadCommand(DataConnection *connection, unsigned pos, unsigned size);

    virtual QByteArray sendAndReceive(bool handleErrors = false);

    QByteArray data() const;

private:
    QByteArray result;
};

class IGOTU_EXPORT WriteCommand : public IgotuCommand
{
public:
    WriteCommand(DataConnection *connection, unsigned pos, const QByteArray &data);

    virtual QByteArray sendAndReceive(bool handleErrors = false);

    void setData(const QByteArray &data);
    QByteArray data() const;

    void setPosition(unsigned value);
    unsigned position() const;

private:
    unsigned pos;
    QByteArray contents;
};
} // namespace igotu

#endif

