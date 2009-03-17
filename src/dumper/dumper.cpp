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
#include "igotu/libusbconnection.h"
#include "igotu/optionutils.h"

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include <QFile>
#include <QFileInfo>

#include <iostream>

using namespace igotu;

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString file, usb, serial;

    po::options_description options("Options");
    options.add_options()
        ("help", "this help message")
        ("file,f", po::value<QString>(&file),
         "output file")
        ("usb-device,u", po::value<QString>(&usb),
         "connect via usb to the device specified by [vendor]:[product]")
        ("serial-device,s", po::value<QString>(&serial),
         "connect via RS232 to the serial port with the number [port]")
    ;
    po::positional_options_description positionalOptions;
    positionalOptions.add("file", 1);

    po::variables_map variables;

    try {
        po::store(po::command_line_parser(arguments())
                .options(options)
                .positional(positionalOptions).run(), variables);
        po::notify(variables);

        if (variables.count("help")) {
            std::cout << "Usage:\n  "
                << qPrintable(QFileInfo(app.applicationFilePath()).fileName())
                << " [OPTIONS...] [file]\n\n";
            std::cout << options << "\n";
            return 1;
        }
    } catch (const std::exception &e) {
        printf("Unable to parse command line: %s\n", e.what());
        return 2;
    }

    boost::scoped_ptr<DataConnection> connection;
    try {
        if (variables.count("usb-device") ||
#ifdef Q_OS_LINUX
            variables.count("serial-device") == 0 ||
#endif
            false) {
            QStringList parts = usb.split(QLatin1Char(':'));
            unsigned vendor = 0, product = 0;
            if (parts.size() > 0)
                vendor = parts[0].toUInt(NULL, 16);
            if (parts.size() > 1)
                product = parts[1].toUInt(NULL, 16);
            connection.reset(new LibusbConnection(vendor, product));
        } else if (variables.count("serial-device") ||
#ifdef Q_OS_WIN32
            variables.count("usb-device") == 0 ||
#endif
            false) {
            // TODO: serial port
        } else {
            // TODO: throw exception
        }
    } catch (const std::exception &e) {
        printf("Exception: %s\n", e.what());
        return 3;
    }

    // Just some dummy NMEA read, seems to help?
    try {
        for (unsigned i = 0; i < 10; ++i)
            connection->receive(16).data();
    } catch (const std::exception &e) {
        // ignored
    }

    try {
        NmeaSwitchCommand(connection.get(), false).sendAndReceive(true);

        IdentificationCommand id(connection.get());
        id.sendAndReceive();
        printf("S/N: %u\n", id.serialNumber());
        printf("Model: %u\n", id.model());

        QFile file(QLatin1String("igotu.dump"));
        if (!file.open(QIODevice::WriteOnly))
            throw IgotuError(QCoreApplication::tr("Unable to write to file"));

        for (unsigned i = 0;; ++i) {
            printf("Dumping datablock %u...\n", i + 1);
            QByteArray data = ReadCommand(connection.get(), i * 0x1000, 0x1000)
                .sendAndReceive();
            if (data == QByteArray(0x1000, 0xff))
                break;
            file.write(data);
        }

        NmeaSwitchCommand(connection.get(), true).sendAndReceive();
    } catch (const std::exception &e) {
        printf("Exception: %s\n", e.what());
    }

    return 0;
}
