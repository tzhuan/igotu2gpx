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
#include "igotu/igotucontrol.h"
#include "igotu/igotupoints.h"
#include "igotu/messages.h"
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

public:
    MainObject *p;

    IgotuControl *control;
    QByteArray contents;
    bool details;
    bool raw;
};

static QString dump(const QByteArray &data)
{
    QString result;
    result += MainObject::tr("Memory contents:") + QLatin1Char('\n');
    const unsigned chunks = (data.size() + 15) / 16;
    bool firstLine = true;
    for (unsigned i = 0; i < chunks; ++i) {
        const QByteArray chunk = data.mid(i * 16, 16);
        if (firstLine)
            firstLine = false;
        else
            result += QLatin1Char('\n');
        result += QString().sprintf("%04x  ", i * 16);
        for (unsigned j = 0; j < unsigned(chunk.size()); ++j) {
            result += QString().sprintf("%02x ", uchar(chunk[j]));
            if (j % 4 == 3)
                result += QLatin1Char(' ');
        }
    }
    return result;
}

static QString dumpDiff(const QByteArray &oldData, const QByteArray &newData)
{
    QString result;
    result += MainObject::tr("Differences:") + QLatin1Char('\n');
    const unsigned chunks = (oldData.size() + 15) / 16;
    bool firstLine = true;
    for (unsigned i = 0; i < chunks; ++i) {
        const QByteArray oldChunk = oldData.mid(i * 16, 16);
        const QByteArray newChunk = newData.mid(i * 16, 16);
        if (oldChunk == newChunk)
            continue;
        if (firstLine)
            firstLine = false;
        else
            result += QLatin1Char('\n');
        result += QString().sprintf("%04x - ", i * 16);
        for (unsigned j = 0; j < unsigned(qMin(oldChunk.size(),
                        newChunk.size())); ++j) {
            if (oldChunk[j] == newChunk[j])
                result += QLatin1String("__ ");
            else
                result += QString().sprintf("%02x ", uchar(oldChunk[j]));
            if (j % 4 == 3)
                result += QLatin1Char(' ');
        }
        result += QString().sprintf("\n%04x + ", i * 16);
        for (unsigned j = 0; j < unsigned(qMin(oldChunk.size(),
                        newChunk.size())); ++j) {
            if (oldChunk[j] == newChunk[j])
                result += QLatin1String("__ ");
            else
                result += QString().sprintf("%02x ", uchar(newChunk[j]));
            if (j % 4 == 3)
                result += QLatin1Char(' ');
        }
    }
    return result;
}

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
    QCoreApplication::quit();
}

void MainObjectPrivate::on_control_infoFailed(const QString &message)
{
    Messages::errorMessage(Common::tr
                ("Unable to download configuration from GPS tracker: %1").arg(message));
    QCoreApplication::quit();
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
    if (raw) {
        Messages::directOutput(contents);
    } else if (details) {
        IgotuPoints igotuPoints(contents, count);
        unsigned index = 0;
        Q_FOREACH (const IgotuPoint &igotuPoint, igotuPoints.points()) {
            // These shouldn't be localized, just in case somebody wants to
            // parse them?
            Messages::textOutput(QString::fromLatin1("Record %1")
                    .arg(++index));
            if (igotuPoint.isWayPoint())
                Messages::textOutput(QString::fromLatin1("  Waypoint"));
            if (igotuPoint.isTrackStart())
                Messages::textOutput(QString::fromLatin1("  Track start"));
            Messages::textOutput(QString::fromLatin1("  Date %1")
                    .arg(igotuPoint.dateTimeString(control->utcOffset())));
            Messages::textOutput(QString::fromLatin1("  Latitude %1")
                    .arg(igotuPoint.latitude(), 0, 'f', 6));
            Messages::textOutput(QString::fromLatin1("  Longitude %1")
                    .arg(igotuPoint.longitude(), 0, 'f', 6));
            Messages::textOutput(QString::fromLatin1("  Elevation %1 m")
                    .arg(igotuPoint.elevation(), 0, 'f', 1));
            Messages::textOutput(QString::fromLatin1("  Speed %1 km/h")
                    .arg(igotuPoint.speed(), 0, 'f', 1));
            Messages::textOutput(QString::fromLatin1("  Course %1 degrees")
                    .arg(igotuPoint.course(), 0, 'f', 2));
            Messages::textOutput(QString::fromLatin1("  EHPE %1 m")
                    .arg(igotuPoint.ehpe(), 0, 'f', 2));
            QString satellites = QLatin1String("  Satellites:");
            Q_FOREACH (unsigned satellite, igotuPoint.satellites())
                satellites += QString::fromLatin1(" %1").arg(satellite);
            Messages::textOutput(satellites);
            Messages::textOutput(QString::fromLatin1("  Flags 0x%1")
                    .arg(igotuPoint.flags(), 2, 16, QLatin1Char('0')));
            Messages::textOutput(QString::fromLatin1("  Timeout %1 s")
                    .arg(igotuPoint.timeout()));
            Messages::textOutput(QString::fromLatin1("  MSVs_QCN %1")
                    .arg(igotuPoint.msvsQcn()));
            Messages::textOutput(QString::fromLatin1
                    ("  Weight criteria 0x%1").arg
                    (igotuPoint.weightCriteria(), 2, 16, QLatin1Char('0')));
            Messages::textOutput(QString::fromLatin1("  Sleep time %1")
                    .arg(igotuPoint.sleepTime()));
        }
    } else {
        IgotuPoints igotuPoints(contents, count);
        Messages::directOutput(igotuPoints.gpx(control->utcOffset()));
    }
    QCoreApplication::quit();
}

void MainObjectPrivate::on_control_contentsFailed(const QString &message)
{
    Messages::errorMessage(Common::tr("Unable to download trackpoints from "
                "GPS tracker: %1").arg(message));
    QCoreApplication::quit();
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
    QCoreApplication::quit();
}

void MainObjectPrivate::on_control_purgeFailed(const QString &message)
{
    Messages::errorMessage(Common::tr
                ("Unable to clear memory of GPS tracker: %1").arg(message));
    QCoreApplication::quit();
}

// MainObject ==================================================================

MainObject::MainObject(const QString &device, int utcOffset) :
    d(new MainObjectPrivate)
{
    d->p = this;

    d->control = new IgotuControl(this);
    d->control->setObjectName(QLatin1String("control"));

    connectSlotsByNameToPrivate(this, d.get());

    if (!device.isEmpty())
        d->control->setDevice(device);

    d->control->setUtcOffset(utcOffset);
}

MainObject::~MainObject()
{
}

void MainObject::info(const QByteArray &contents)
{
    d->contents = contents;

    d->control->info();
}

void MainObject::save(bool details, bool raw)
{
    d->details = details;
    d->raw = raw;

    d->control->contents();
}

void MainObject::purge()
{
    d->control->purge();
}

#include "mainobject.moc"
