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

#include "igotu/commonmessages.h"
#include "igotu/exception.h"
#include "igotu/fileexporter.h"
#include "igotu/igotucontrol.h"
#include "igotu/igotudata.h"
#include "igotu/igotupoints.h"
#include "igotu/messages.h"
#include "igotu/pluginloader.h"
#include "igotu/utils.h"

#include "mainobject.h"

#include <QFile>
#include <QMetaMethod>

using namespace igotu;

class MainObjectPrivate : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void on_control_infoStarted();
    void on_control_infoFinished(const QString &info,
            const QByteArray &contents);
    void on_control_infoFailed(const QString &message);

    void on_control_contentsStarted();
    void on_control_contentsBlocksFinished(uint num, uint total);
    void on_control_contentsFinished(const QByteArray &contents, uint count);
    void on_control_contentsFailed(const QString &message);

    void on_control_purgeStarted();
    void on_control_purgeBlocksFinished(uint num, uint total);
    void on_control_purgeFinished();
    void on_control_purgeFailed(const QString &message);

    void on_control_writeStarted();
    void on_control_writeBlocksFinished(uint num, uint total);
    void on_control_writeFinished(const QString &message);
    void on_control_writeFailed(const QString &message);

public:
    MainObject *p;

    IgotuControl *control;
    QByteArray contents;
    QString format;
    QList<FileExporter*> exporters;
};

// Put translations in the right context
//
// TRANSLATOR igotu::Common

// MainObjectPrivate ===========================================================

void MainObjectPrivate::on_control_infoStarted()
{
    Messages::normalMessage(Common::tr("Downloading configuration..."));
}

void MainObjectPrivate::on_control_infoFinished(const QString &info,
        const QByteArray &contents)
{
    if (this->contents.isEmpty()) {
        Messages::textOutput(info);
    } else {
        Messages::textOutput(dump(contents));
        Messages::textOutput(dumpDiff(this->contents, contents));
    }
}

void MainObjectPrivate::on_control_infoFailed(const QString &message)
{
    Messages::errorMessage(Common::tr
                ("Unable to download configuration from GPS tracker: %1").arg(message));
}

void MainObjectPrivate::on_control_contentsStarted()
{
    Messages::normalMessage(Common::tr("Downloading tracks..."));
}

void MainObjectPrivate::on_control_contentsBlocksFinished(uint num, uint total)
{
    Messages::normalMessage(MainObject::tr("Downloaded block %1/%2")
            .arg(num).arg(total));
}

void MainObjectPrivate::on_control_contentsFinished(const QByteArray &contents,
        uint count)
{
    FileExporter *selected = exporters.value(0);
    Q_FOREACH (FileExporter *exporter, exporters) {
        if (format == exporter->formatName()) {
            selected = exporter;
            break;
        }
    }

    if (selected)
        Messages::directOutput(selected->save(IgotuData(contents, count),
                    control->tracksAsSegments(), control->utcOffset()));
    else
        qCritical("No file exporters found");

}

void MainObjectPrivate::on_control_contentsFailed(const QString &message)
{
    Messages::errorMessage(Common::tr("Unable to download trackpoints from "
                "GPS tracker: %1").arg(message));
}

void MainObjectPrivate::on_control_purgeStarted()
{
    Messages::normalMessage(Common::tr("Clearing memory..."));
}

void MainObjectPrivate::on_control_purgeBlocksFinished(uint num, uint total)
{
    Messages::normalMessage(MainObject::tr
            ("Cleared block %1/%2").arg(num).arg(total));
}

void MainObjectPrivate::on_control_purgeFinished()
{
    // do nothing
}

void MainObjectPrivate::on_control_purgeFailed(const QString &message)
{
    Messages::errorMessage(Common::tr
                ("Unable to clear memory of GPS tracker: %1").arg(message));
}

void MainObjectPrivate::on_control_writeStarted()
{
    Messages::normalMessage(Common::tr("Writing configuration..."));
}

void MainObjectPrivate::on_control_writeBlocksFinished(uint num, uint total)
{
    Messages::normalMessage(MainObject::tr
            ("Wrote block %1/%2").arg(num).arg(total));
}

void MainObjectPrivate::on_control_writeFinished(const QString &message)
{
    if (!message.isEmpty())
        Messages::textOutput(message);
}

void MainObjectPrivate::on_control_writeFailed(const QString &message)
{
    Messages::errorMessage(Common::tr
                ("Unable to write configuration to GPS tracker: %1").arg(message));
}

// MainObject ==================================================================

MainObject::MainObject(const QString &device, bool tracksAsSegments, int utcOffset) :
    d(new MainObjectPrivate)
{
    d->p = this;

    QMultiMap<int, FileExporter*> exporterMap;
    Q_FOREACH (FileExporter * const exporter,
            PluginLoader().availablePlugins<FileExporter>())
        exporterMap.insert(exporter->exporterPriority(), exporter);
    d->exporters = exporterMap.values();

    d->control = new IgotuControl(this);
    d->control->setObjectName(QLatin1String("control"));

    connectSlotsByNameToPrivate(this, d.get());

    if (!device.isEmpty())
        d->control->setDevice(device);

    d->control->setUtcOffset(utcOffset);
    d->control->setTracksAsSegments(tracksAsSegments);
}

MainObject::~MainObject()
{
}

void MainObject::info(const QByteArray &contents)
{
    d->contents = contents;

    d->control->info();
    d->control->notify(qApp, "quit");
}

void MainObject::save(const QString &format)
{
    d->format = format;

    d->control->contents();
    d->control->notify(qApp, "quit");
}

void MainObject::purge()
{
    d->control->purge();
    d->control->notify(qApp, "quit");
}

void MainObject::reset()
{
    d->control->reset();
}

#include "mainobject.moc"
