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

#ifndef _IGOTU2GPX_SRC_IGOTU_FILEEXPORTER_H_
#define _IGOTU2GPX_SRC_IGOTU_FILEEXPORTER_H_

#include "global.h"
#include "igotupoints.h"

#include <QtPlugin>

namespace igotu
{

class IgotuData;

class FileExporter
{
public:
    virtual ~FileExporter()
    {
    }

    enum Flag {
        TrackExport = 0x01,
    };
    Q_DECLARE_FLAGS(Mode, Flag)
    virtual Mode mode() const = 0;

    // lower is better
    virtual int exporterPriority() const = 0;
    virtual QString formatName() const = 0;
    virtual QString formatDescription() const = 0;
    virtual QString fileExtension() const = 0;
    virtual QString fileType() const = 0;
    virtual QByteArray save(const QList<QList<IgotuPoint> > &tracks,
            bool tracksAsSegments, int utcOffset) const = 0;
    virtual QByteArray save(const IgotuData &data,
            bool tracksAsSegments, int utcOffset) const = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FileExporter::Mode)

} // namespace igotu

Q_DECLARE_INTERFACE(igotu::FileExporter, "de.mh21.igotu2gpx.fileexporter/1.0")

#endif
