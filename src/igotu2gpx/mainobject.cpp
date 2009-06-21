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

#include "igotu/exception.h"
#include "igotu/igotucontrol.h"
#include "igotu/igotupoints.h"
#include "igotu/utils.h"
#include "igotu/verbose.h"

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
    QString raw;
};

static void dump(const QByteArray &data)
{
    printf("Complete dump:\n");
    for (unsigned i = 0; i < unsigned(data.size() + 15) / 16; ++i) {
        const QByteArray chunk = data.mid(i * 16, 16);
        printf("%04x  ", i * 16);
        for (unsigned j = 0; j < unsigned(chunk.size()); ++j) {
            printf("%02x ", uchar(chunk[j]));
            if (j % 4 == 3)
                printf(" ");
        }
        printf("\n");
    }
}

static void dumpDiff(const QByteArray &oldData, const QByteArray &newData)
{
    printf("Difference dump:\n");
    for (unsigned i = 0; i < unsigned(oldData.size() + 15) / 16; ++i) {
        const QByteArray oldChunk = oldData.mid(i * 16, 16);
        const QByteArray newChunk = newData.mid(i * 16, 16);
        if (oldChunk == newChunk)
            continue;
        printf("%04x Before  ", i * 16);
        for (unsigned j = 0; j < unsigned(qMin(oldChunk.size(),
                        newChunk.size())); ++j) {
            if (oldChunk[j] == newChunk[j])
                printf("__ ");
            else
                printf("%02x ", uchar(oldChunk[j]));
            if (j % 4 == 3)
                printf(" ");
        }
        printf("\n");
        printf("%04x After   ", i * 16);
        for (unsigned j = 0; j < unsigned(qMin(oldChunk.size(),
                        newChunk.size())); ++j) {
            if (oldChunk[j] == newChunk[j])
                printf("__ ");
            else
                printf("%02x ", uchar(newChunk[j]));
            if (j % 4 == 3)
                printf(" ");
        }
        printf("\n");
    }
}

// MainObjectPrivate ===========================================================

void MainObjectPrivate::on_control_infoStarted()
{
    if (Verbose::verbose() >= 0)
        fprintf(stderr, "%s\n", qPrintable(tr("Retrieving info...")));
}

void MainObjectPrivate::on_control_infoFinished(const QString &info,
        const QByteArray &contents)
{
    if (this->contents.isEmpty()) {
        printf("%s\n", qPrintable(info));
    } else {
        dump(contents);
        dumpDiff(this->contents, contents);
    }
    QCoreApplication::quit();
}

void MainObjectPrivate::on_control_infoFailed(const QString &message)
{
    fprintf(stderr, "%s\n", qPrintable(tr
                ("Unable to connect to gps tracker: %1").arg(message)));
    QCoreApplication::quit();
}

void MainObjectPrivate::on_control_contentsStarted()
{
    if (Verbose::verbose() >= 0)
        fprintf(stderr, "%s\n", qPrintable(tr("Retrieving data...")));
}

void MainObjectPrivate::on_control_contentsBlocksFinished(uint num, uint total)
{
    if (Verbose::verbose() >= 0 && num > 0)
        fprintf(stderr, "%s\n",
                qPrintable(tr("Retrieved block %1/%2").arg(num).arg(total)));
}

void MainObjectPrivate::on_control_contentsFinished(const QByteArray &contents,
        uint count)
{
    try {
        if (!raw.isEmpty()) {
            QFile file(raw);
            if (!file.open(QIODevice::WriteOnly))
                throw IgotuError(QCoreApplication::tr("Unable to write to "
                            "file '%1'").arg(raw));
            file.write(contents);
        } else if (details) {
            IgotuPoints igotuPoints(contents, count);
            unsigned index = 0;
            Q_FOREACH (const IgotuPoint &igotuPoint, igotuPoints.points()) {
                printf("Record %u\n", ++index);
                if (igotuPoint.isWayPoint())
                    printf("  Waypoint\n");
                printf("  Date %s\n", qPrintable(igotuPoint.dateTimeString
                            (control->utcOffset())));
                printf("  Latitude %.6f\n", igotuPoint.latitude());
                printf("  Longitude %.6f\n", igotuPoint.longitude());
                printf("  Elevation %.1f m\n", igotuPoint.elevation());
                printf("  Speed %.1f km/h\n", igotuPoint.speed());
                printf("  Course %.2f degrees\n", igotuPoint.course());
                printf("  EHPE %.2f m\n", igotuPoint.ehpe());
                printf("  Satellites:");
                Q_FOREACH (unsigned satellite, igotuPoint.satellites())
                    printf(" %u", satellite);
                printf("\n");
                printf("  Flags 0x%02x\n", igotuPoint.flags());
                printf("  Timeout %u s\n", igotuPoint.timeout());
                printf("  MSVs_QCN %u\n", igotuPoint.msvsQcn());
                printf("  Weight criteria 0x%02x\n",
                        igotuPoint.weightCriteria());
                printf("  Sleep time %u\n", igotuPoint.sleepTime());
            }
        } else {
            IgotuPoints igotuPoints(contents, count);
            printf("%s", qPrintable(igotuPoints.gpx(control->utcOffset())));
        }
    } catch (const std::exception &e) {
        fprintf(stderr, "%s\n", qPrintable(tr("Unable to save data: %1")
                .arg(QString::fromLocal8Bit(e.what()))));
    }
    QCoreApplication::quit();
}

void MainObjectPrivate::on_control_contentsFailed(const QString &message)
{
    fprintf(stderr, "%s\n", qPrintable(tr
                ("Unable to connect to gps tracker: %1").arg(message)));
    QCoreApplication::quit();
}

void MainObjectPrivate::on_control_purgeStarted()
{
    if (Verbose::verbose() >= 0)
        fprintf(stderr, "%s\n", qPrintable(tr("Purging data...")));
}

void MainObjectPrivate::on_control_purgeBlocksFinished(uint num, uint total)
{
    if (Verbose::verbose() >= 0 && num > 0)
        fprintf(stderr, "%s\n",
                qPrintable(tr("Purged block %1/%2").arg(num).arg(total)));
}

void MainObjectPrivate::on_control_purgeFinished()
{
    QCoreApplication::quit();
}

void MainObjectPrivate::on_control_purgeFailed(const QString &message)
{
    fprintf(stderr, "%s\n", qPrintable(tr
                ("Unable to connect to gps tracker: %1").arg(message)));
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

void MainObject::save(bool details, const QString &raw)
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
