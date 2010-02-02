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
    void on_control_commandStarted(const QString &message);
    void on_control_commandRunning(uint num, uint total);
    void on_control_commandSucceeded();
    void on_control_commandFailed(const QString &failed);

    void on_control_infoRetrieved(const QString &info, const QByteArray &contents);
    void on_control_contentsRetrieved(const QByteArray &contents, uint count);

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

void MainObjectPrivate::on_control_commandStarted(const QString &message)
{
    Messages::normalMessagePart(message);
}

void MainObjectPrivate::on_control_commandRunning(uint num, uint total)
{
    Q_UNUSED(num);
    Q_UNUSED(total);
    Messages::normalMessagePart(QLatin1String("."));
}

void MainObjectPrivate::on_control_commandFailed(const QString &message)
{
    Messages::normalMessage(QString());
    Messages::errorMessage(message);
}

void MainObjectPrivate::on_control_commandSucceeded()
{
    Messages::normalMessage(QString());
}

void MainObjectPrivate::on_control_infoRetrieved(const QString &info,
        const QByteArray &contents)
{
    if (this->contents.isEmpty()) {
        Messages::textOutput(info);
    } else {
        Messages::textOutput(dump(contents));
        Messages::textOutput(dumpDiff(this->contents, contents));
    }
}

void MainObjectPrivate::on_control_contentsRetrieved(const QByteArray &contents,
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

    connectSlotsByNameToPrivate(this, d);

    if (!device.isEmpty())
        d->control->setDevice(device);

    d->control->setUtcOffset(utcOffset);
    d->control->setTracksAsSegments(tracksAsSegments);
}

MainObject::~MainObject()
{
    delete d;
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
    d->control->notify(qApp, "quit");
}

void MainObject::configure(const QVariantMap &config)
{
    d->control->configure(config);
    d->control->notify(qApp, "quit");
}

#include "mainobject.moc"
