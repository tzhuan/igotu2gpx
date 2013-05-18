/******************************************************************************
 * Copyright (C) 2009  Michael Hofmann <mh21@mh21.de>                         *
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

#include "igotu/commonmessages.h"
#include "igotu/messages.h"
#include "igotu/optioncontext.h"
#include "igotu/paths.h"

#include "iconstorage.h"
#include "mainwindow.h"

#include <boost/shared_ptr.hpp>

#include <QApplication>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

using namespace igotu;

// Put translations in the right context
//
// TRANSLATOR igotu::Common

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QLatin1String("igotugui"));
    app.setOrganizationName(QLatin1String("mh21.de"));
    app.setOrganizationDomain(QLatin1String("mh21.de"));
    app.setWindowIcon(IconStorage::get(IconStorage::GuiIcon));
#if defined(Q_OS_MAC) && QT_VERSION >= 0x040400
    app.setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

    QTranslator qtTranslator;
    qtTranslator.load(QLatin1String("qt_" )+ QLocale::system().name(),
            QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    // TODO: add marble translations

    QList<boost::shared_ptr<QTranslator> > translators;
    QListIterator<QString> path(Paths::translationDirectories());
    for (path.toBack(); path.hasPrevious();) {
        boost::shared_ptr<QTranslator> translator(new QTranslator);
        if (!translator->load(QLatin1String("igotu2gpx_") +
                    QLocale::system().name(), path.previous()))
            continue;
        translators.append(translator);
        app.installTranslator(translator.get());
    }

    bool version = false;
    int verbose = 0;

    OptionContext context(app.arguments(),
            MainWindow::tr("[OPTION...]"),
            OptionGroup(QString(), Common::tr("Program Options"), QString(),
                QString(), QList<OptionEntry>()
            << OptionEntry(QLatin1String("version"), 0, 0,
                 OptionEntry::NoArgument, &version,
                 Common::tr("output version information and exit"))
            << OptionEntry(QLatin1String("verbose"), 0, 0,
                 OptionEntry::NoArgument, incrementOptionValue(&verbose),
                 Common::tr("increase verbosity"))));

    try {
        QStringList additionalParams = context.parse();

        if (version) {
            Messages::textOutput(Common::tr(
                    "<h3>Igotu2gpx %1</h3><br/>"
                    "Downloads tracks and waypoints from MobileAction i-gotU USB GPS travel loggers.<br/><br/>"
                    "Copyright (C) 2009 Michael Hofmann.<br/>"
                    "License GPLv3+: GNU GPL version 3 or later (http://gnu.org/licenses/gpl.html)<br/>"
                    "This is free software: you are free to change and redistribute it.<br/>"
                    "There is NO WARRANTY, to the extent permitted by law.")
                    .replace(QRegExp(QLatin1String("<br/>")), QLatin1String("\n"))
                    .remove(QRegExp(QLatin1String("<[^>]+>")))
                    .arg(QLatin1String(IGOTU_VERSION_STR)));
            return 0;
        }
    } catch (const std::exception &e) {
        Messages::errorMessage(Common::tr("Unable to parse command line parameters: %1")
                    .arg(QString::fromLocal8Bit(e.what())));
        return 2;
    }

    Messages::setVerbose(verbose);

    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
