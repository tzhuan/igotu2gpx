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

#include "mainobject.h"

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include <QFile>
#include <QFileInfo>
#include <QSet>

#include <iostream>

#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#include "igotu/libusbconnection.h"
#endif
#ifdef Q_OS_WIN32
#include "igotu/win32serialconnection.h"
#endif

using namespace igotu;

namespace po = boost::program_options;

po::variables_map variables;
QString imagePath, usb, serial;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Command line parsing (uses C++ output)

    QString rawPath, action;
    int offset = 0;

    po::options_description options("Options");
    options.add_options()
        ("help", "output this help and exit")
        ("version", "output version information and exit")

        ("image,i", po::value<QString>(&imagePath),
         "instead of connecting to the GPS tracker, use the specified image "
         "file (saved by \"dump --raw\")")
#ifdef Q_OS_WIN32
        ("serial-device,s", po::value<QString>(&serial),
         "connect via RS232 to the serial port with the specfied number")
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACX)
        ("usb-device,u", po::value<QString>(&usb),
         "connect via usb to the device specified by vendor:product")
#endif
        ("gpx",
         "output in GPX format (this is the default)")
        ("details",
         "output a detailed representation of the track")
        ("raw", po::value<QString>(&rawPath),
         "output the raw flash contents of the GPS tracker")

        ("verbose,v",
         "increase the amount of informative messages")
        ("utc-offset", po::value<int>(&offset),
         "time zone offset from UTC in seconds")

        ("action", po::value<QString>(&action),
         "info: show general info\n"
         "dump: dump the trackpoints\n"
         "purge: remove all trackpoints from the GPS tracker\n"
         "diff: show change relative to image file")
    ;
    po::positional_options_description positionalOptions;
    positionalOptions.add("action", 1);

    try {
        po::store(po::command_line_parser(arguments())
                .options(options)
                .positional(positionalOptions).run(), variables);
        po::notify(variables);

        if (variables.count("version")) {
            std::cout
                << "igotu2gpx " << IGOTU_VERSION_STR << "\n"
                << "Copyright (C) 2009 Michael Hofmann\n"
                << "License GPLv3+: GNU GPL version 3 or later "
                   "<http://gnu.org/licenses/gpl.html>\n"
                << "This is free software: "
                   "you are free to change and redistribute it.\n"
                << "There is NO WARRANTY, to the extent permitted by law.\n\n"
                << "Written by Michael Hofmann.\n";
            return 0;
        }
        if (variables.count("help") || action.isEmpty()) {
            std::cout << "Usage:\n  "
                << qPrintable(QFileInfo(app.applicationFilePath()).fileName())
                << " info|dump|purge|diff [OPTIONS...]\n\n";
            std::cout << options << "\n";
            return 1;
        }
    } catch (const std::exception &e) {
        std::cout << "Unable to parse command line: " << e.what() << "\n";
        return 2;
    }

    try {
        QString device;
        if (!serial.isEmpty()) {
            device = QLatin1String("serial:") + serial;
        } else if (!usb.isEmpty()) {
            device = QLatin1String("usb:") + usb;
        } else if (!imagePath.isEmpty() && (action != QLatin1String("diff"))) {
            QFile file(imagePath);
            if (!file.open(QIODevice::ReadOnly))
                throw IgotuError(QCoreApplication::tr
                        ("Unable to read file '%1'").arg(imagePath));
            QByteArray contents = file.readAll();
            device = QLatin1String("image:") +
                QString::fromAscii(contents.toBase64());
        }

        Verbose::setVerbose(variables.count("verbose"));

        MainObject mainObject(device, offset);

        if (action == QLatin1String("info")) {
            mainObject.info();
        } else if (action == QLatin1String("diff")) {
            if (imagePath.isEmpty())
                throw IgotuError(qApp->tr("Please specify --image"));
            QFile file(imagePath);
            if (!file.open(QIODevice::ReadOnly))
                throw IgotuError(qApp->tr("Unable to read file '%1'")
                        .arg(imagePath));
            mainObject.info(file.readAll().left(0x1000));
        } else if (action == QLatin1String("dump")) {
            mainObject.save(variables.count("details"), rawPath);
        } else if (action == QLatin1String("purge")) {
            mainObject.purge();
        } else {
            throw IgotuError(QCoreApplication::tr("Unknown action: %1")
                    .arg(action));
        }

        return app.exec();
    } catch (const std::exception &e) {
        printf("Error: %s\n", e.what());
    }

    return 1;
}
