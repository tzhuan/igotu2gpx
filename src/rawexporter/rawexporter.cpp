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

#include "igotu/xmlutils.h"

#include "fileexporter.h"

#include <QTextStream>

using namespace igotu;

class RawExporter :
    public QObject,
    public FileExporter
{
    Q_OBJECT
    Q_INTERFACES(igotu::FileExporter)
public:
    virtual Mode mode() const;
    virtual int exporterPriority() const;
    virtual QString formatName() const;
    virtual QString formatDescription() const;
    virtual QString fileExtension() const;
    virtual QString fileType() const;
    virtual QByteArray save(const QList<QList<IgotuPoint> > &tracks,
            bool tracksAsSegments, int utcOffset) const;
    virtual QByteArray save(const IgotuPoints &points,
            bool tracksAsSegments, int utcOffset) const;
};

Q_EXPORT_PLUGIN2(rawExporter, RawExporter)

// RawExporter =====================================================

FileExporter::Mode RawExporter::mode() const
{
    return 0;
}

int RawExporter::exporterPriority() const
{
    return 200;
}

QString RawExporter::formatName() const
{
    return QLatin1String("raw");
}

QString RawExporter::formatDescription() const
{
    return tr("raw dump of the flash memory");
}

QString RawExporter::fileExtension() const
{
    return QLatin1String("raw");
}

QString RawExporter::fileType() const
{
    return tr("Raw files (%1)").arg(QLatin1String("*.") + fileExtension());
}

QByteArray RawExporter::save(const IgotuPoints &points, bool tracksAsSegments,
        int utcOffset) const
{
    Q_UNUSED(tracksAsSegments);
    Q_UNUSED(utcOffset);
    return points.memoryDump();
}

QByteArray RawExporter::save(const QList<QList<IgotuPoint> > &tracks,
        bool tracksAsSegments, int utcOffset) const
{
    Q_UNUSED(tracks);
    Q_UNUSED(tracksAsSegments);
    Q_UNUSED(utcOffset);
    qWarning("Unable to export tracks to raw format");
    return QByteArray();
}

#include "rawexporter.moc"
