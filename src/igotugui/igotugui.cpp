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

#include "igotu/optionutils.h"
#include "igotu/verbose.h"

#include "iconstorage.h"
#include "mainwindow.h"

#include <boost/program_options.hpp>

#include <QApplication>
#include <QFileInfo>
#include <QLocale>

#include <iostream>

using namespace igotu;

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
#if QT_VERSION >= 0x040500
    // This crashes libmarble, at least up to 0.7 and on nvidia
    // can be tested with "-graphicssystem opengl" command line parameters
    // QApplication::setGraphicsSystem(QLatin1String("opengl"));
#endif
    QApplication app(argc, argv);
    app.setApplicationName(QLatin1String("igotugui"));
    app.setOrganizationName(QLatin1String("mh21.de"));
    app.setOrganizationDomain(QLatin1String("mh21.de"));
    app.setWindowIcon(IconStorage::get(IconStorage::GuiIcon));
#if defined(Q_OS_MACX) && QT_VERSION >= 0x040400
    app.setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

    // Command line parsing (uses C++ output)

    po::variables_map variables;

    po::options_description options("Options");
    options.add_options()
        ("help", "output this help and exit")
        ("version", "output version information and exit")

        ("verbose,v",
         "increase the amount of informative messages")
    ;

    try {
        po::store(po::command_line_parser(arguments())
                .options(options).run(), variables);
        po::notify(variables);

        if (variables.count("version")) {
            std::cout
                << "igotugui (igotu2gpx) " << IGOTU_VERSION_STR << "\n"
                << "Copyright (C) 2009 Michael Hofmann\n"
                << "License GPLv3+: GNU GPL version 3 or later "
                   "<http://gnu.org/licenses/gpl.html>\n"
                << "This is free software: "
                   "you are free to change and redistribute it.\n"
                << "There is NO WARRANTY, to the extent permitted by law.\n\n"
                << "Written by Michael Hofmann.\n";
            return 0;
        }
        if (variables.count("help")) {
            std::cout << "Usage:\n  "
                << qPrintable(QFileInfo(app.applicationFilePath()).fileName())
                << " [OPTIONS...]\n\n";
            std::cout << options << "\n";
            return 1;
        }
    } catch (const std::exception &e) {
        std::cout << "Unable to parse command line: " << e.what() << "\n";
        return 2;
    }

    Verbose::setVerbose(variables.count("verbose"));

    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
