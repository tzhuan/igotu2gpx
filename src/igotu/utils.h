/******************************************************************************
 * Copyright (C) 2007  Michael Hofmann <mh21@piware.de>                       *
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

#ifndef _IGOTU2GPX_SRC_IGOTU_UTILS_H_
#define _IGOTU2GPX_SRC_IGOTU_UTILS_H_

#include "global.h"
#include "igotupoints.h"

class QMetaObject;
class QObject;

namespace igotu
{

IGOTU_EXPORT unsigned colorTableEntry(unsigned index);

IGOTU_EXPORT QByteArray pointsToKml(const QList<QList<IgotuPoint> > &tracks,
        bool tracksAsSegments);

IGOTU_EXPORT void connectSlotsByNameToPrivate(QObject *publicObject, QObject
        *privateObject);

IGOTU_EXPORT int enumKeyToValue(const QMetaObject &metaObject, const char *type,
        const char *key);

IGOTU_EXPORT const char *enumValueToKey(const QMetaObject &metaObject, const
        char *type, int value);

IGOTU_EXPORT QString dump(const QByteArray &data);
IGOTU_EXPORT QString dumpDiff(const QByteArray &oldData, const QByteArray &newData);

} // namespace igotu

#endif

