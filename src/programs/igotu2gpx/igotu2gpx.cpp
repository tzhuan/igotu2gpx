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
#include "igotu/commonmessages.h"
#include "igotu/exception.h"
#include "igotu/fileexporter.h"
#include "igotu/igotucontrol.h"
#include "igotu/igotupoints.h"
#include "igotu/messages.h"
#include "igotu/optionutils.h"
#include "igotu/paths.h"
#include "igotu/pluginloader.h"

#include "mainobject.h"

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QLocale>
#include <QSet>
#include <QTranslator>

#include <iostream>

using namespace igotu;

namespace po = boost::program_options;

po::variables_map variables;
QString imagePath, device, format;

// Put translations in the right context
//
// TRANSLATOR igotu::Common

static QString parameterList(const QList<QPair<const char*, QString> > &descriptions)
{
    typedef QPair<const char*, QString> QP;
    QString result;
    Q_FOREACH (const QP &description, descriptions)
        result += QLatin1String("  ") + QLatin1String(description.first) +
        QLatin1String(": ") + description.second + QLatin1Char('\n');
    return result.left(result.size() - 1);
}

static QString parameterList(const QMultiMap<int, QString> &exporterMap)
{
    QString result;
    Q_FOREACH (const QString &value, exporterMap)
        result += QLatin1String("  ") + value + QLatin1Char('\n');
    return result.left(result.size() - 1);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTranslator qtTranslator;
    qtTranslator.load(QLatin1String("qt_" )+ QLocale::system().name(),
            QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

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

    QMultiMap<int, QString> exporterMap;
    Q_FOREACH (const FileExporter * const exporter,
            PluginLoader().availablePlugins<FileExporter>())
        exporterMap.insert(exporter->exporterPriority(),
                //: Used for command line help
                //: %1 is the file format name, %2 the description
                MainObject::tr("%1: %2").arg(exporter->formatName(),
                    exporter->formatDescription()));

    QString action;
    QStringList parameters;
    int offset = 0;

    po::options_description options("Options");
    options.add_options()
        ("action",
         po::value<QString>(&action),
         //: Do not translate the word before the colon
         MainObject::tr("info: show GPS tracker configuration")
         .toLocal8Bit() + '\n' +
         //: Do not translate the word before the colon
         MainObject::tr("dump: output trackpoints")
         .toLocal8Bit() + '\n' +
         //: Do not translate the word before the colon
         MainObject::tr("config: change the configuration of the GPS tracker")
         .toLocal8Bit() + '\n' +
         //: Do not translate the word before the colon
         MainObject::tr("clear: clear memory of the GPS tracker")
         .toLocal8Bit() + '\n' +
         //: Do not translate the word before the colon
         MainObject::tr("reset: reset the GPS tracker to factory defaults")
         .toLocal8Bit() + '\n' +
         //: Do not translate the word before the colon
         MainObject::tr("diff: show configuration differences relative to an image file")
         .toLocal8Bit())

        ("device,d",
         po::value<QString>(&device),
         MainObject::tr("connect to the specified device "
             "(usb:<vendor>:<product> (Unix) or serial:<n> "
             "(Windows))").toLocal8Bit())
        ("image,i",
         po::value<QString>(&imagePath),
         MainObject::tr("read memory contents from file "
             "(saved by \"dump -f raw\")").toLocal8Bit())

        ("set,s",
         po::value<QStringList>(&parameters),
         MainObject::tr("change the configuration:").toLocal8Bit() + '\n'
         + parameterList(IgotuControl::configureParameters()).toLocal8Bit())

        ("format,f",
         po::value<QString>(&format),
         MainObject::tr("use the specified output format:").toLocal8Bit() + '\n'
         + parameterList(exporterMap).toLocal8Bit())

        ("segments",
         MainObject::tr("group trackpoints into segments instead of tracks")
         .toLocal8Bit())
        ("utc-offset",
         po::value<int>(&offset),
         MainObject::tr("time zone offset in seconds")
         .toLocal8Bit())

        ("help",
         Common::tr("output this help and exit").toLocal8Bit())
        ("version",
         Common::tr("output version information and exit").toLocal8Bit())
        ("verbose,v",
         Common::tr("increase verbosity").toLocal8Bit())
        ("really-verbose",
         Common::tr("very high verbosity").toLocal8Bit())
    ;
    po::positional_options_description positionalOptions;
    positionalOptions.add("action", 1);
    positionalOptions.add("set", -1);

    try {
        po::store(po::command_line_parser(arguments())
                .options(options)
                .positional(positionalOptions).run(), variables);
        po::notify(variables);

        if (variables.count("version")) {
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
        if (variables.count("help") || action.isEmpty()) {
            Messages::textOutput(Common::tr("Usage: %1")
                    //: Do not translate the actions (info|dump|clear|diff)
                    .arg(MainObject::tr("%1 info|dump|config|clear|reset|diff [OPTIONS...]")
                        .arg(QFileInfo(app.applicationFilePath()).fileName())));
            Messages::normalMessage(QString());
            std::cout << options << "\n";
            return 1;
        }
    } catch (const std::exception &e) {
        Messages::errorMessage(Common::tr("Unable to parse command line parameters: %1")
                    .arg(QString::fromLocal8Bit(e.what())));
        return 2;
    }

    try {
        if (!imagePath.isEmpty() && (action != QLatin1String("diff"))) {
            QFile file(imagePath);
            if (!file.open(QIODevice::ReadOnly))
                throw IgotuError(MainObject::tr
                        ("Unable to read file '%1'").arg(imagePath));
            QByteArray contents = file.readAll();
            device = QLatin1String("image:") +
                QString::fromAscii(contents.toBase64());
        }

        Messages::setVerbose(2 * variables.count("really-verbose") + variables.count("verbose"));

        MainObject mainObject(device, variables.count("segments"), offset);

        if (action == QLatin1String("info")) {
            mainObject.info();
        } else if (action == QLatin1String("diff")) {
            if (imagePath.isEmpty())
                throw IgotuError(MainObject::tr("Diff action requires --image"));
            QFile file(imagePath);
            if (!file.open(QIODevice::ReadOnly))
                throw IgotuError(MainObject::tr("Unable to read file '%1'")
                        .arg(imagePath));
            mainObject.info(file.readAll().left(0x1000));
        } else if (action == QLatin1String("dump")) {
            mainObject.save(format);
        } else if (action == QLatin1String("clear")) {
            mainObject.purge();
        } else if (action == QLatin1String("reset")) {
            mainObject.reset();
        } else if (action == QLatin1String("config")) {
            QVariantMap configParams;
            Q_FOREACH (const QString &param, parameters)
                Q_FOREACH (const QString &part, param.split(QLatin1Char(',')))
                    configParams.insert(part.section(QLatin1Char('='), 0, 0),
                            part.section(QLatin1Char('='), 1));
            mainObject.configure(configParams);
        } else {
            throw IgotuError(MainObject::tr("Unknown action: %1")
                    .arg(action));
        }
        return app.exec();
    } catch (const std::exception &e) {
        Messages::errorMessage(MainObject::tr("Error: %1")
                    .arg(QString::fromLocal8Bit(e.what())));
        return 3;
    }
}
