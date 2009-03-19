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
#include "igotu/igotupoints.h"
#include "igotu/optionutils.h"

#include <boost/program_options.hpp>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

#include <QtEndian>

#include <iostream>

using namespace igotu;

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString filePath, usb, serial;

    po::options_description options("Options");
    options.add_options()
        ("help", "this help message")
        ("file,f", po::value<QString>(&filePath),
         "input file")
        ("gpx,g",
         "output in gpx format")
    ;
    po::positional_options_description positionalOptions;
    positionalOptions.add("file", 1);

    po::variables_map variables;

    try {
        po::store(po::command_line_parser(arguments())
                .options(options)
                .positional(positionalOptions).run(), variables);
        po::notify(variables);

        if (variables.count("help") || filePath.isEmpty()) {
            std::cout << "Usage:\n  "
                << qPrintable(QFileInfo(app.applicationFilePath()).fileName())
                << " [OPTIONS...] file\n\n";
            std::cout << options << "\n";
            return 1;
        }
    } catch (const std::exception &e) {
        printf("Unable to parse command line: %s\n", e.what());
        return 2;
    }

    try {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            throw IgotuError(QCoreApplication::tr("Unable to read file"));

        QByteArray data = file.readAll();

        IgotuPoints igotuPoints(data);

        if (variables.count("gpx")) {
            printf("%s", qPrintable(igotuPoints.gpx()));
        } else {
            printf("Log interval: %us\n", igotuPoints.logInterval());
            printf("Interval change: %s\n", igotuPoints.isIntervalChangeEnabled() ?
                    "disabled" : "enabled");
            // TODO
            // 0x0109: 0xa0 -> above 15km/h
            // 0x0109: 0x15 -> above 10km/h
            printf("Above %u?, use %us\n", unsigned(data[0x109]), data[0x10a] + 1);

            unsigned index = 0;
            Q_FOREACH (const IgotuPoint &igotuPoint, igotuPoints.points()) {
                printf("Record %u\n", index++);
                printf("  Unknown %s\n", igotuPoint.unknownDataDump().data());
                if (igotuPoint.isWayPoint())
                    printf("  Waypoint\n");
                printf("  Date %s\n", qPrintable(igotuPoint.dateTime().toString(Qt::ISODate)));
                printf("  Latitude %.6f\n", igotuPoint.latitude());
                printf("  Longitude %.6f\n", igotuPoint.longitude());
                printf("  Elevation %.1f m\n", igotuPoint.elevation());
                printf("  Speed %.1f km/s\n", igotuPoint.speed());
            }
        }
    } catch (const std::exception &e) {
        printf("Exception: %s\n", e.what());
    }

    return 0;
}
