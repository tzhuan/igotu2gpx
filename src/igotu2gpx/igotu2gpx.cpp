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

#include "igotu/commands.h"
#include "igotu/exception.h"
#include "igotu/igotupoints.h"
#include "igotu/optionutils.h"
#include "igotu/serialconnection.h"
#include "igotu/verbose.h"

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include <QFile>
#include <QFileInfo>

#include <iostream>

#ifdef Q_OS_LINUX
#include "igotu/libusbconnection.h"
#endif
#ifdef Q_OS_WIN32
#include "igotu/win32serialconnection.h"
#endif

using namespace igotu;

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Command line parsing (uses C++ output)

    QString rawPath, action, imagePath, usb, serial;
    unsigned sectorCount = 0;

    po::options_description options("Options");
    options.add_options()
        ("help", "this help message")

        ("image,i", po::value<QString>(&imagePath),
         "instead of connecting to the gps tracker, use the specified image "
         "file (saved by \"dump --raw\")")
#ifdef Q_OS_LINUX
        ("usb-device,u", po::value<QString>(&usb),
         "connect via usb to the device specified by vendor:product "
         "(this is the default on Linux)")
#endif
        ("serial-device,s", po::value<QString>(&serial),
         "connect via RS232 to the serial port with the specfied number "
         "(this is the default on Windows)")

        ("gpx",
         "output in gpx format (this is the default)")
        ("details",
         "output a detailed representation of the track")
        ("raw", po::value<QString>(&rawPath),
         "output the raw flash contents of the gps tracker")
        ("count", po::value<unsigned>(&sectorCount),
         "limits the number of sectors read (4096 bytes each)")

        ("verbose,v",
         "increase the amount of informative messages")
        ("action", po::value<QString>(&action),
         "dump the trackpoints (dump) or show general info (info)")
    ;
    po::positional_options_description positionalOptions;
    positionalOptions.add("action", 1);

    po::variables_map variables;

    try {
        po::store(po::command_line_parser(arguments())
                .options(options)
                .positional(positionalOptions).run(), variables);
        po::notify(variables);

        if (variables.count("help") || action.isEmpty()) {
            std::cout << "Usage:\n  "
                << qPrintable(QFileInfo(app.applicationFilePath()).fileName())
                << " dump|info [OPTIONS...]\n\n";
            std::cout << options << "\n";
            return 1;
        }
    } catch (const std::exception &e) {
        std::cout << "Unable to parse command line: " << e.what() << "\n";
        return 2;
    }

    Verbose::setVerbose(variables.count("verbose"));
    // Make connection

    boost::scoped_ptr<DataConnection> connection;
    QByteArray contents;

    try {
        if (false) {
            // Dummy
#ifdef Q_OS_WIN32
        } else if (variables.count("serial-device") ||
            (variables.count("usb-device") == 0 &&
             variables.count("image") == 0)) {
            connection.reset(new Win32SerialConnection(serial.isEmpty() ?
                        3 : serial.toUInt()));
#endif
#ifdef Q_OS_LINUX
        } else if (variables.count("usb-device") ||
            (variables.count("serial-device") == 0 &&
             variables.count("image") == 0)) {
            QStringList parts = usb.split(QLatin1Char(':'));
            unsigned vendor = 0, product = 0;
            if (parts.size() > 0)
                vendor = parts[0].toUInt(NULL, 16);
            if (parts.size() > 1)
                product = parts[1].toUInt(NULL, 16);
            connection.reset(new LibusbConnection(vendor, product));
        } else if (variables.count("serial-device")) {
            connection.reset(new SerialConnection(serial.isEmpty() ?
                        0 : serial.toUInt()));
#endif
        } else if (variables.count("image")) {
            QFile file(imagePath);
            if (!file.open(QIODevice::ReadOnly))
                throw IgotuError(QCoreApplication::tr
                        ("Unable to read file '%1'").arg(imagePath));
            contents = file.readAll();
            if (sectorCount > 0)
                contents = contents.left(sectorCount * 0x1000);
        } else {
            throw IgotuError(QCoreApplication::tr("Please specify either "
                        "--usb-device, --serial-device or --image"));
        }
    } catch (const std::exception &e) {
        printf("Unable to create gps tracker connection: %s\n", e.what());
        return 3;
    }

    // Process action

    try {
        if (action == QLatin1String("info")) {
            if (connection) {
                NmeaSwitchCommand(connection.get(), false).sendAndReceive();
                IdentificationCommand id(connection.get());
                id.sendAndReceive();
                printf("S/N: %u\n", id.serialNumber());
                printf("Firmware version: %s\n", qPrintable(id.firmwareVersion()));
                printf("Model: %s\n", qPrintable(id.deviceName()));
                CountCommand count(connection.get());
                count.sendAndReceive();
                printf("Number of trackpoints: %u\n", count.trackPointCount());

                contents += ReadCommand(connection.get(), 0, 0x1000)
                    .sendAndReceive();
                NmeaSwitchCommand(connection.get(), true).sendAndReceive();
            }

            IgotuPoints igotuPoints(contents);
            printf("Log interval: %us\n", igotuPoints.logInterval());
            printf("Interval change: %s\n", igotuPoints
                    .isIntervalChangeEnabled() ? "disabled" : "enabled");
            // TODO
            // 0x0109: 0xa0 -> above 15km/h
            // 0x0109: 0x15 -> above 10km/h
            printf("Above %u? (TODO), use %us\n",
                    unsigned(contents[0x109]), contents[0x10a] + 1);
        } else {
            int count = -1;
            if (connection) {
                NmeaSwitchCommand(connection.get(), false).sendAndReceive();
                CountCommand countCommand(connection.get());
                countCommand.sendAndReceive();
                count = countCommand.trackPointCount();
                for (unsigned i = 0;
                        (sectorCount == 0 && i < 1 + unsigned(count + 0x7f) / 0x80) ||
                        i < sectorCount; ++i) {
                    fprintf(stderr, "Dumping datablock %u...\n", i + 1);
                    QByteArray data = ReadCommand(connection.get(), i * 0x1000,
                            0x1000).sendAndReceive();
                    contents += data;
                }
                NmeaSwitchCommand(connection.get(), true).sendAndReceive();
            }
            if (variables.count("raw")) {
                QFile file(rawPath);
                if (!file.open(QIODevice::WriteOnly))
                    throw IgotuError(QCoreApplication::tr("Unable to write to "
                                "file '%1'").arg(rawPath));
                file.write(contents);
            } else if (variables.count("details")) {
                IgotuPoints igotuPoints(contents, count);
                unsigned index = 0;
                Q_FOREACH (const IgotuPoint &igotuPoint, igotuPoints.points()) {
                    printf("Record %u\n", index++);
                    printf("  Unknown %s\n",
                            igotuPoint.unknownDataDump().data());
                    if (igotuPoint.isWayPoint())
                        printf("  Waypoint\n");
                    printf("  Date %s\n", qPrintable(igotuPoint.dateTime()
                                .toString(Qt::ISODate)));
                    printf("  Latitude %.6f\n", igotuPoint.latitude());
                    printf("  Longitude %.6f\n", igotuPoint.longitude());
                    printf("  Elevation %.1f m\n", igotuPoint.elevation());
                    printf("  Speed %.1f km/s\n", igotuPoint.speed());
                    printf("  Unknown 1 %u\n", igotuPoint.unknown1());
                    printf("  Unknown 2 %u\n", igotuPoint.unknown2());
                }
            } else {
                IgotuPoints igotuPoints(contents, count);
                printf("%s", qPrintable(igotuPoints.gpx()));
            }
        }
    } catch (const std::exception &e) {
        printf("Error: %s\n", e.what());
    }

    return 0;
}
