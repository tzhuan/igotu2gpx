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

#ifndef _IGOTU2GPX_SRC_IGOTU_IGOTUDATA_H_
#define _IGOTU2GPX_SRC_IGOTU_IGOTUDATA_H_

#include "igotupoints.h"
#include "igotuconfig.h"

#include "global.h"

namespace igotu
{

class IGOTU_EXPORT IgotuData
{
    Q_DECLARE_TR_FUNCTIONS(igotu::IgotuData)
public:
    IgotuData(const QByteArray &dump, unsigned count);
    ~IgotuData();

    IgotuPoints points() const;
    IgotuConfig config() const;

    QByteArray memoryDump() const;

private:
    QByteArray dump;
    int count;
};

} // namespace igotu

#endif

