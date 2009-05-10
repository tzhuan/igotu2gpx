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

#include "iconstorage.h"
#include "paths.h"

#include <QDir>
#include <QFile>
#include <QMap>
#include <QMutex>

class IconStoragePrivate
{
public:
    QIcon get(IconStorage::IconName name);

private:
    static QString fileName(IconStorage::IconName name);

    QMap <IconStorage::IconName, QIcon> cache;
    QMutex lock;
};

Q_GLOBAL_STATIC(IconStoragePrivate, iconStoragePrivate)

// IconStoragePrivate ==========================================================

QIcon IconStoragePrivate::get(IconStorage::IconName name)
{
    QMutexLocker locker(&lock);

    QRegExp regExp(QLatin1String("(\\d+)x\\1"));

    if (cache.contains(name))
        return cache.value(name);

    Q_FOREACH (const QString &dir, Paths::iconDirectories()) {
        const QStringList sizes = QDir(dir).entryList(QDir::Dirs);
        Q_FOREACH (const QString &size, sizes) {
            if (!regExp.exactMatch(size))
                continue;
            const QString filePath = dir + QLatin1Char('/') + size +
                QLatin1Char('/') + fileName(name) + QLatin1String(".png");
            const unsigned numSize = regExp.cap(1).toUInt();
            if (QFile::exists(filePath))
                cache[name].addFile(filePath, QSize(numSize, numSize));
        }
    }

    return cache.value(name);
}

QString IconStoragePrivate::fileName(IconStorage::IconName name)
{
    switch(name) {
    case IconStorage::IgotuIcon:
        return QLatin1String("apps/igotugui");
    default:
        return QString();
    }
}

// IconStorage =================================================================

QIcon IconStorage::get(IconStorage::IconName name)
{
    return iconStoragePrivate()->get(name);
}
