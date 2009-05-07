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

#include "mainwindow.h"

#include <boost/program_options.hpp>

#include <QApplication>
#include <QFileInfo>

#include <iostream>

using namespace igotu;

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Command line parsing (uses C++ output)

    po::variables_map variables;

    po::options_description options("Options");
    options.add_options()
        ("help", "this help message")
        ("verbose,v",
         "increase the amount of informative messages")
    ;

    try {
        po::store(po::command_line_parser(arguments())
                .options(options).run(), variables);
        po::notify(variables);

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
