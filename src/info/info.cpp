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

#include "igotu/exception.h"

#include <boost/scoped_ptr.hpp>

#include <QCoreApplication>
#include <QFile>

#include <QtEndian>

using namespace igotu;

int main()
{
    try {
        QFile file(QLatin1String("igotu.dump"));
        if (!file.open(QIODevice::ReadOnly))
            throw IgotuError(QCoreApplication::tr("Unable to read file"));

        QByteArray data = file.readAll();

        // First 0x1000 bytes seem to be config area
        printf("Log interval: %us\n", data[0x107] + 1);
        printf("Interval change: %s\n", data[0x108] == '\x00' ? "disabled" : "enabled");
        //0x0109: 0xa0 -> above 15km/h
        //0x0109: 0x15 -> above 10km/h
        printf("Above %u?, use %us\n", unsigned(data[0x109]), data[0x10a] + 1);

        for (unsigned i = 0x1000; i < unsigned(data.size()); i += 0x20) {
            QByteArray record = data.mid(i, 32);

            QByteArray hexrecord = record.toHex();
            hexrecord.replace(2, 10, "__________");
            hexrecord.replace(24, 28, "____________________________");
//            hexrecord.replace(2, 10, "DDDDDDSSSS");
//            hexrecord.replace(24, 28, "AAAAAAAAOOOOOOOOEEEEEEEESSSS");

            unsigned flg = uchar(record[0]);
            unsigned date = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(record.data())) & 0x00ffffff;
            unsigned sec = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(record.data()) + 4);

            int lat = qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(record.data()) + 12);
            int lon = qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(record.data()) + 16);
            int ele = qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(record.data()) + 20);
            int spe = qFromBigEndian<qint16>(reinterpret_cast<const uchar*>(record.data()) + 24);

            printf("Record %u\n", (i - 0x1000) / 0x20);
            printf("  Unknown %s\n", hexrecord.data());
            if (flg & 0x04)
                printf("  Waypoint\n");
            // TODO: the year is only good until 2016?
            printf("  Date %04u-%02u-%02uT%02u:%02u:%06.3fZ\n",
                    2000 + ((date >> 20) & 0xf), (date >> 16) & 0xf, (date >> 11) & 0x1f,
                    (date >> 6) & 0x1f, date & 0x3f, 1e-3 * sec);
            printf("  Latitude %.6f\n", 1e-7 * lat);
            printf("  Longitude %.6f\n", 1e-7 * lon);
            printf("  Elevation %.1f m\n", 1e-2 * ele);
            printf("  Speed %.1f km/s\n", 1e-2 * spe * 3.6);
        }
    } catch (const std::exception &e) {
        printf("Exception: %s\n", e.what());
    }

    return 0;
}
