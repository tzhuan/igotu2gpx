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
#include "igotu/optioncontext.h"
#include "igotu/paths.h"
#include "igotu/pluginloader.h"

#include "mainobject.h"

#include <boost/shared_ptr.hpp>

#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QLocale>
#include <QSet>
#include <QTranslator>

using namespace igotu;

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
    QMap<QString, QString> parameters;
    bool segments = false;
    bool version = false;
    int verbose = 0;
    int offset = 0;

    OptionContext context(app.arguments(),
            MainObject::tr("info|dump|config|clear|reset|diff [OPTION...]"),
            OptionGroup(QString(), Common::tr("Program Options"), QString(),
                QString(), QList<OptionEntry>()
             << OptionEntry(QLatin1String("action"), 0, 0,
                 OptionEntry::RequiredArgument, &action,
                 //: Do not translate the word before the colon
                 MainObject::tr("info: show GPS tracker configuration")
                 + QLatin1Char('\n') +
                 //: Do not translate the word before the colon
                 MainObject::tr("dump: output trackpoints")
                 + QLatin1Char('\n') +
                 //: Do not translate the word before the colon
                 MainObject::tr("config: change the configuration of the GPS tracker")
                 + QLatin1Char('\n') +
                 //: Do not translate the word before the colon
                 MainObject::tr("clear: clear memory of the GPS tracker")
                 + QLatin1Char('\n') +
                 //: Do not translate the word before the colon
                 MainObject::tr("reset: reset the GPS tracker to factory defaults")
                 + QLatin1Char('\n') +
                 //: Do not translate the word before the colon
                 MainObject::tr("diff: show configuration differences relative to an image file"),
                 MainObject::tr("ACTION"))
             << OptionEntry(QLatin1String("device"), QLatin1Char('d'), 0,
                 OptionEntry::RequiredArgument, &device,
                 MainObject::tr("connect to the specified device "
                     "(usb:<vendor>:<product> (Unix) or serial:<n> "
                     "(Windows))"),
                 MainObject::tr("DEVICE"))
             << OptionEntry(QLatin1String("image"), QLatin1Char('i'), 0,
                 OptionEntry::RequiredArgument, &imagePath,
                 MainObject::tr("read memory contents from file "
                     "(saved by \"dump -f raw\")"),
                 MainObject::tr("FILE"))
             << OptionEntry(QLatin1String("set"), QLatin1Char('s'), 0,
                 OptionEntry::RequiredArgument, mapOptionValue(&parameters),
                 MainObject::tr("change the configuration:") + QLatin1Char('\n')
                 + parameterList(IgotuControl::configureParameters()),
                 MainObject::tr("PARAM=VALUE"))
             << OptionEntry(QLatin1String("format"), QLatin1Char('f'), 0,
                 OptionEntry::RequiredArgument, &format,
                 MainObject::tr("use the specified output format:") + QLatin1Char('\n')
                 + parameterList(exporterMap),
                 MainObject::tr("FORMAT"))
             << OptionEntry(QLatin1String("segments"), 0, 0,
                 OptionEntry::NoArgument, &segments,
                 MainObject::tr("group trackpoints into segments instead of tracks"))
             << OptionEntry(QLatin1String("utc-offset"), 0, 0,
                 OptionEntry::RequiredArgument, &offset,
                 MainObject::tr("time zone offset in seconds"),
                 MainObject::tr("SECONDS"))
            << OptionEntry(QLatin1String("version"), 0, 0,
                 OptionEntry::NoArgument, &version,
                 Common::tr("output version information and exit"))
            << OptionEntry(QLatin1String("verbose"), 0, 0,
                 OptionEntry::NoArgument, incrementOptionValue(&verbose),
                 Common::tr("increase verbosity"))));

    try {
        QStringList additionalParams = context.parse();

        for (unsigned i = 0; i < unsigned(additionalParams.size()); ++i) {
            switch (i) {
            case 0:
                action = additionalParams[i];
                break;
            default:
                mapOptionValue(&parameters).setValue(additionalParams[i]);
            }
        }

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
        if (action.isEmpty())
            context.printShortHelp();
    } catch (const std::exception &e) {
        Messages::errorMessage(Common::tr("Unable to parse command line parameters: %1")
                    .arg(QString::fromLocal8Bit(e.what())));
        return 2;
    }

    try {
        if (!imagePath.isEmpty() && (action != QLatin1String("diff"))) {
            QFile file(imagePath);
            if (!file.open(QIODevice::ReadOnly))
                throw Exception(MainObject::tr
                        ("Unable to read file '%1'").arg(imagePath));
            QByteArray contents = file.readAll();
            device = QLatin1String("image:") +
                QString::fromAscii(contents.toBase64());
        }

        Messages::setVerbose(verbose);

        MainObject mainObject(device, segments, offset);

        if (action == QLatin1String("info")) {
            mainObject.info();
        } else if (action == QLatin1String("diff")) {
            if (imagePath.isEmpty())
                throw Exception(MainObject::tr("Diff action requires --image"));
            QFile file(imagePath);
            if (!file.open(QIODevice::ReadOnly))
                throw Exception(MainObject::tr("Unable to read file '%1'")
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
            throw Exception(MainObject::tr("Unknown action: %1")
                    .arg(action));
        }
        return app.exec();
    } catch (const std::exception &e) {
        Messages::errorMessage(MainObject::tr("Error: %1")
                    .arg(QString::fromLocal8Bit(e.what())));
        return 3;
    }
}
