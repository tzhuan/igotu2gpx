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
#include "igotu/verbose.h"

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include <QFile>
#include <QFileInfo>
#include <QSet>

#include <iostream>

#ifdef Q_OS_LINUX
#include "igotu/libusbconnection.h"
#endif
#ifdef Q_OS_WIN32
#include "igotu/win32serialconnection.h"
#endif

using namespace igotu;

namespace po = boost::program_options;

po::variables_map variables;
QByteArray contents;
boost::scoped_ptr<DataConnection> connection;
QString imagePath, usb, serial;
unsigned sectorCount = 0;

void makeConnection(bool forceImage = false, bool forceDevice = false)
{
    try {
        if (false) {
            // Dummy
#ifdef Q_OS_WIN32
        } else if (!forceImage &&
                (variables.count("serial-device") ||
                 (variables.count("usb-device") == 0 && forceDevice) ||
                 (variables.count("usb-device") == 0 &&
                  variables.count("image") == 0))) {
            connection.reset(new Win32SerialConnection(serial.isEmpty() ?
                        3 : serial.toUInt()));
#endif
#ifdef Q_OS_LINUX
        } else if (!forceImage &&
                (variables.count("usb-device") ||
                 (variables.count("serial-device") == 0 && forceDevice) ||
                 (variables.count("serial-device") == 0 &&
                  variables.count("image") == 0))) {
            QStringList parts = usb.split(QLatin1Char(':'));
            unsigned vendor = 0, product = 0;
            if (parts.size() > 0)
                vendor = parts[0].toUInt(NULL, 16);
            if (parts.size() > 1)
                product = parts[1].toUInt(NULL, 16);
            connection.reset(new LibusbConnection(vendor, product));
#endif
        } else if (forceImage && !variables.count("image")) {
            throw IgotuError(QCoreApplication::tr("Please specify --image"));
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
        exit(3);
    }
}

void dump(const QByteArray &data)
{
    printf("Complete dump:\n");
    for (unsigned i = 0; i < unsigned(data.size() + 15) / 16; ++i) {
        const QByteArray chunk = data.mid(i * 16, 16);
        printf("%04x  ", i * 16);
        for (unsigned j = 0; j < unsigned(chunk.size()); ++j) {
            printf("%02x ", uchar(chunk[j]));
            if (j % 4 == 3)
                printf(" ");
        }
        printf("\n");
    }
}

void dumpDiff(const QByteArray &oldData, const QByteArray &newData)
{
    printf("Difference dump:\n");
    for (unsigned i = 0; i < unsigned(oldData.size() + 15) / 16; ++i) {
        const QByteArray oldChunk = oldData.mid(i * 16, 16);
        const QByteArray newChunk = newData.mid(i * 16, 16);
        if (oldChunk == newChunk)
            continue;
        printf("%04x Before  ", i * 16);
        for (unsigned j = 0; j < unsigned(oldChunk.size()); ++j) {
            if (oldChunk[j] == newChunk[j])
                printf("__ ");
            else
                printf("%02x ", uchar(oldChunk[j]));
            if (j % 4 == 3)
                printf(" ");
        }
        printf("\n");
        printf("%04x After   ", i * 16);
        for (unsigned j = 0; j < unsigned(newChunk.size()); ++j) {
            if (oldChunk[j] == newChunk[j])
                printf("__ ");
            else
                printf("%02x ", uchar(newChunk[j]));
            if (j % 4 == 3)
                printf(" ");
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Command line parsing (uses C++ output)

    QString rawPath, action;

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
         "dump: dump the trackpoints\n"
         "info: show general info\n"
         "diff: show change relative to image file")
    ;
    po::positional_options_description positionalOptions;
    positionalOptions.add("action", 1);

    try {
        po::store(po::command_line_parser(arguments())
                .options(options)
                .positional(positionalOptions).run(), variables);
        po::notify(variables);

        if (variables.count("help") || action.isEmpty()) {
            std::cout << "Usage:\n  "
                << qPrintable(QFileInfo(app.applicationFilePath()).fileName())
                << " dump|info|diff [OPTIONS...]\n\n";
            std::cout << options << "\n";
            return 1;
        }
    } catch (const std::exception &e) {
        std::cout << "Unable to parse command line: " << e.what() << "\n";
        return 2;
    }

    Verbose::setVerbose(variables.count("verbose"));

    makeConnection(action == QLatin1String("diff"));

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
            if (!igotuPoints.isValid())
                throw IgotuError(QCoreApplication::tr("Uninitialized device"));

            printf("Schedule date: %s\n",
                    qPrintable(igotuPoints.firstScheduleDate().toString()));
            printf("Schedule date offset: %u days\n", igotuPoints.dateOffset());
            QList<unsigned> tablePlans = igotuPoints.scheduleTablePlans();
            QSet<unsigned> tablePlanSet = QSet<unsigned>::fromList(tablePlans);
            if (igotuPoints.isScheduleTableEnabled()) {
                printf("Schedule table: enabled\n");
                printf("Schedule table plans used:");
                Q_FOREACH (unsigned plan, tablePlanSet)
                    printf(" %u", plan);
                printf("\n");
                printf("Schedule table plan order: ");
                if (tablePlans.size() > 1)
                    printf("\n  ");
                for (unsigned i = 0; i < unsigned(tablePlans.size()); ++i) {
                    if (i && i % 7 == 0)
                        printf(" ");
                    if (i && i % (7 * 7) == 0)
                        printf("\n  ");
                    printf("%u", tablePlans[i]);
                }
                printf("\n");
                Q_FOREACH (unsigned plan, tablePlanSet) {
                    bool printed = false;
                    Q_FOREACH (const ScheduleTableEntry &entry,
                            igotuPoints.scheduleTableEntries(plan)) {
                        if (!entry.isValid())
                            continue;
                        if (!printed) {
                            printf("Schedule %u:\n", plan);
                            printed = true;
                        }
                        printf("  Start time: %s\n",
                                qPrintable(entry.startTime().toString()));
                        printf("  End time: %s\n",
                                qPrintable(entry.endTime().toString()));
                        printf("  Log interval: %u s\n", entry.logInterval());
                        if (entry.isIntervalChangeEnabled()) {
                            printf("  Interval change: above %.0f km/h, use %u s\n",
                                    entry.intervalChangeSpeed(),
                                    entry.changedLogInterval());
                        } else {
                            printf("  Interval change: disabled\n");
                        }
                    }
                }
            } else {
                printf("Schedule table: disabled\n");
                ScheduleTableEntry entry = igotuPoints.scheduleTableEntries(1)[0];
                printf("Log interval: %u s\n", entry.logInterval());
                if (entry.isIntervalChangeEnabled()) {
                    printf("Interval change: above %.0f km/h, use %u s\n",
                            entry.intervalChangeSpeed(),
                            entry.changedLogInterval());
                } else {
                    printf("Interval change: disabled\n");
                }
            }

            printf("LEDs: %s\n", igotuPoints.ledsEnabled() ? "enabled" : "disabled");
            printf("Button: %s\n", igotuPoints.isButtonEnabled() ? "enabled" : "disabled");

            printf("Security version: %u\n", igotuPoints.securityVersion());
            if (igotuPoints.securityVersion() == 0) {
                printf("Password: %s, [%s]\n",
                    igotuPoints.isPasswordEnabled() ? "enabled" : "disabled",
                    qPrintable(igotuPoints.password()));
            }
        } else if (action == QLatin1String("diff")) {
            QByteArray oldData = contents.left(0x1000);
            makeConnection(false, true);
            if (!connection)
                throw IgotuError(QCoreApplication::tr("Unable to connect to device"));

            fprintf(stderr, "Dumping datablock %u...\n", 1);
            NmeaSwitchCommand(connection.get(), false).sendAndReceive();
            QByteArray newData = ReadCommand(connection.get(), 0, 0x1000)
                .sendAndReceive();
            NmeaSwitchCommand(connection.get(), true).sendAndReceive();

            dump(newData);
            dumpDiff(oldData, newData);
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
                    printf("Record %u\n", ++index);
                    if (igotuPoint.isWayPoint())
                        printf("  Waypoint\n");
                    printf("  Date %s\n", qPrintable(igotuPoint.dateTime()
                                .toString(Qt::ISODate)));
                    printf("  Latitude %.6f\n", igotuPoint.latitude());
                    printf("  Longitude %.6f\n", igotuPoint.longitude());
                    printf("  Elevation %.1f m\n", igotuPoint.elevation());
                    printf("  Speed %.1f km/s\n", igotuPoint.speed());
                    printf("  Course %.2f degrees\n", igotuPoint.course());
                    printf("  EHPE %.2f m\n", igotuPoint.ehpe());
                    printf("  Satellites:");
                    Q_FOREACH (unsigned satellite, igotuPoint.satellites())
                        printf(" %u", satellite);
                    printf("\n");
                    printf("  Flags 0x%02x\n", igotuPoint.flags());
                    printf("  Timeout %u s\n", igotuPoint.timeout());
                    printf("  MSVs_QCN %u\n", igotuPoint.msvsQcn());
                    printf("  Weight criteria 0x%02x\n", igotuPoint.weightCriteria());
                    printf("  Sleep time %u\n", igotuPoint.sleepTime());
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
