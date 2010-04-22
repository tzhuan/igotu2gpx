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

#include "igotudata.h"

namespace igotu
{

// IgotuData =================================================================

IgotuData::IgotuData(const QByteArray &dump, unsigned count) :
    dump(dump),
    count(count)
{
    if (0x1000 + count * 0x20 > unsigned(dump.size())) {
        this->dump += QByteArray(0x1000 + count * 0x20 - dump.size(), char(0xff));
        qCritical("Invalid dump size");
    }
}

IgotuData::~IgotuData()
{
}

QByteArray IgotuData::memoryDump() const
{
    return dump;
}

IgotuPoints IgotuData::points() const
{
    return IgotuPoints(dump.mid(0x1000), count);
}

IgotuConfig IgotuData::config() const
{
    return IgotuConfig(dump.left(0x1000));
}

} // namespace igotu

