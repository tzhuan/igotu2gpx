/******************************************************************************
 * Copyright (C) 2007  Michael Hofmann <mh21@piware.de>                       *
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

#include "commands.h"
#include "dataconnection.h"
#include "exception.h"
#include "igotucontrol.h"
#include "igotupoints.h"
#include "threadutils.h"

#include <QDir>
#include <QSemaphore>
#include <QStringList>

#if defined(Q_OS_LINUX) || defined(Q_OS_MACX)
#include "libusbconnection.h"
#endif
#ifdef Q_OS_WIN32
#include "win32serialconnection.h"
#endif

namespace igotu
{

class IgotuControlPrivateWorker : public QObject
{
    Q_OBJECT
public:
    IgotuControlPrivateWorker(IgotuControlPrivate *pub);

public Q_SLOTS:
    void info();
    void contents();
    void notify(QObject *object, const QByteArray &method);

private:
    void connect();

Q_SIGNALS:
    void infoStarted();
    void infoFinished(const QString &info);
    void infoFailed(const QString &message);

    void contentsStarted();
    void contentsBlocksFinished(unsigned num, unsigned total);
    void contentsFinished(const QByteArray &contents, unsigned count);
    void contentsFailed(const QString &message);

private:
    IgotuControlPrivate * const p;

    boost::scoped_ptr<DataConnection> connection;
};

class IgotuControlPrivate : public QObject
{
    Q_OBJECT
    friend class IgotuControl;
public:
    IgotuControlPrivate();

    unsigned count;
    QSemaphore semaphore;
    EventThread thread;
    IgotuControlPrivateWorker worker;

Q_SIGNALS:
    void info();
    void contents();
    void notify(QObject *object, const QByteArray &method);
};

// IgotuControlPrivate =========================================================

IgotuControlPrivate::IgotuControlPrivate() :
    count(1),
    semaphore(count),
    worker(this)
{
}

// IgotuControlPrivateWorker ====================================================

IgotuControlPrivateWorker::IgotuControlPrivateWorker(IgotuControlPrivate *pub) :
    p(pub)
{
}

void IgotuControlPrivateWorker::connect()
{
    connection.reset();
#ifdef Q_OS_WIN32
    connection.reset(new Win32SerialConnection);
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    connection.reset(new LibusbConnection);
#endif
}

void IgotuControlPrivateWorker::info()
{
    emit infoStarted();
    try {
        connect();

        NmeaSwitchCommand(connection.get(), false).sendAndReceive();
        QString status;
        IdentificationCommand id(connection.get());
        id.sendAndReceive();
        status += tr("S/N: %1\n").arg(id.serialNumber());
        status += tr("Firmware version: %1\n").arg(id.firmwareVersion());
        status += tr("Model: %1\n").arg(id.deviceName());
        CountCommand countCommand(connection.get());
        countCommand.sendAndReceive();
        unsigned count = countCommand.trackPointCount();
        status += tr("Number of trackpoints: %1\n").arg(count);
        const QByteArray contents = ReadCommand(connection.get(), 0, 0x1000)
            .sendAndReceive();
        NmeaSwitchCommand(connection.get(), true).sendAndReceive();

        IgotuPoints igotuPoints(contents);
        if (!igotuPoints.isValid())
            throw IgotuError(tr("Uninitialized device"));

        status += tr("Schedule date: %1").arg(igotuPoints.firstScheduleDate()
                .toString(Qt::DefaultLocaleLongDate)) + QLatin1Char('\n');
        status += tr("Schedule date offset: %1 days").arg(igotuPoints
                .dateOffset()) + QLatin1Char('\n');
        QList<unsigned> tablePlans = igotuPoints.scheduleTablePlans();
        QSet<unsigned> tablePlanSet = QSet<unsigned>::fromList(tablePlans);
        if (igotuPoints.isScheduleTableEnabled()) {
            status += tr("Schedule table: enabled") + QLatin1Char('\n');
            status += tr("Schedule table plans used:");
            Q_FOREACH (unsigned plan, tablePlanSet)
                status += QLatin1Char(' ') + QString::number(plan);
            status += QLatin1Char('\n');
            status += tr("Schedule table plan order:") + QLatin1Char(' ');
            if (tablePlans.size() > 1)
                status += QLatin1String("\n  ");
            for (unsigned i = 0; i < unsigned(tablePlans.size()); ++i) {
                if (i && i % 7 == 0)
                    status += QLatin1Char(' ');
                if (i && i % (7 * 7) == 0)
                    status += QLatin1String("\n  ");
                status += QString::number(tablePlans[i]);
            }
            status += QLatin1Char('\n');
            Q_FOREACH (unsigned plan, tablePlanSet) {
                bool printed = false;
                Q_FOREACH (const ScheduleTableEntry &entry,
                        igotuPoints.scheduleTableEntries(plan)) {
                    if (!entry.isValid())
                        continue;
                    if (!printed) {
                        status += tr("Schedule %1:").arg(plan) + QLatin1Char('\n');
                        printed = true;
                    }
                    status += QLatin1String("  ") +
                        tr("Start time: %1").arg(entry.startTime().toString(Qt::DefaultLocaleLongDate)) +
                        QLatin1Char('\n');
                    status += QLatin1String("  ") +
                        tr("End time: %1").arg(entry.endTime().toString(Qt::DefaultLocaleLongDate)) +
                        QLatin1Char('\n');
                    status += QLatin1String("  ") +
                        tr("Log interval: %1 s").arg(entry.logInterval()) +
                        QLatin1Char('\n');
                    if (entry.isIntervalChangeEnabled()) {
                        status += QLatin1String("  ") +
                            tr("Interval change: above %1 km/h, use %2 s")
                            .arg(qRound(entry.intervalChangeSpeed()))
                            .arg(entry.changedLogInterval()) + QLatin1Char('\n');
                    } else {
                        status += QLatin1String("  ") +
                            tr("Interval change: disabled") + QLatin1Char('\n');
                    }
                }
            }
        } else {
            status += tr("Schedule table: disabled") + QLatin1Char('\n');
            ScheduleTableEntry entry = igotuPoints.scheduleTableEntries(1)[0];
            status += tr("Log interval: %1 s").arg(entry.logInterval()) + QLatin1Char('\n');
            if (entry.isIntervalChangeEnabled()) {
                status += tr("Interval change: above %1 km/h, use %2 s")
                        .arg(qRound(entry.intervalChangeSpeed()))
                        .arg(entry.changedLogInterval()) + QLatin1Char('\n');
            } else {
                status += tr("Interval change: disabled") + QLatin1Char('\n');
            }
        }

        status += tr("LEDs: %1").arg(igotuPoints.ledsEnabled() ? tr("enabled") :
                tr("disabled")) + QLatin1Char('\n');
        status += tr("Button: %1").arg(igotuPoints.isButtonEnabled() ? tr("enabled")
                : tr("disabled")) + QLatin1Char('\n');

        status += tr("Security version: %1").arg(igotuPoints.securityVersion()) +
            QLatin1Char('\n');
        if (igotuPoints.securityVersion() == 0) {
            status += tr("Password: %1, [%2]")
                .arg(igotuPoints.isPasswordEnabled() ? tr("enabled") : tr("disabled"),
                igotuPoints.password()) + QLatin1Char('\n');
        }

        emit infoFinished(status);
    } catch (const std::exception &e) {
        emit infoFailed(QString::fromLocal8Bit(e.what()));
    }
    connection.reset();

    p->semaphore.release();
}

void IgotuControlPrivateWorker::contents()
{
    emit contentsStarted();
    try {
        connect();

        NmeaSwitchCommand(connection.get(), false).sendAndReceive();
        CountCommand countCommand(connection.get());
        countCommand.sendAndReceive();
        const unsigned count = countCommand.trackPointCount();
        const unsigned blocks = 1 + (count + 0x7f) / 0x80;
        QByteArray data;
        for (unsigned i = 0; i < blocks; ++i) {
            emit contentsBlocksFinished(i, blocks);
            data += ReadCommand(connection.get(), i * 0x1000,
                    0x1000).sendAndReceive();
        }
        NmeaSwitchCommand(connection.get(), true).sendAndReceive();

        emit contentsBlocksFinished(blocks, blocks);
        emit contentsFinished(data, count);
    } catch (const std::exception &e) {
        emit contentsFailed(QString::fromLocal8Bit(e.what()));
    }
    connection.reset();

    p->semaphore.release();
}

void IgotuControlPrivateWorker::notify(QObject *object, const QByteArray &method)
{
    QMetaObject::invokeMethod(object, method);
}

// IgotuControl ================================================================

IgotuControl::IgotuControl(QObject *parent) :
    QObject(parent),
    d(new IgotuControlPrivate)
{
    connect(&d->worker, SIGNAL(infoStarted()),
             this, SIGNAL(infoStarted()));
    connect(&d->worker, SIGNAL(infoFinished(QString)),
             this, SIGNAL(infoFinished(QString)));
    connect(&d->worker, SIGNAL(infoFailed(QString)),
             this, SIGNAL(infoFailed(QString)));

    connect(&d->worker, SIGNAL(contentsStarted()),
             this, SIGNAL(contentsStarted()));
    connect(&d->worker, SIGNAL(contentsBlocksFinished(uint,uint)),
             this, SIGNAL(contentsBlocksFinished(uint,uint)));
    connect(&d->worker, SIGNAL(contentsFinished(QByteArray,uint)),
             this, SIGNAL(contentsFinished(QByteArray,uint)));
    connect(&d->worker, SIGNAL(contentsFailed(QString)),
             this, SIGNAL(contentsFailed(QString)));

    connect(d.get(), SIGNAL(info()),
             &d->worker, SLOT(info()));
    connect(d.get(), SIGNAL(contents()),
             &d->worker, SLOT(contents()));
    connect(d.get(), SIGNAL(notify(QObject*,QByteArray)),
             &d->worker, SLOT(notify(QObject*,QByteArray)));

    d->worker.moveToThread(&d->thread);
    d->thread.start();
}

IgotuControl::~IgotuControl()
{
    d->semaphore.acquire(d->count);

    // kill the thread
    d->thread.quit();
    d->thread.wait();
}

bool IgotuControl::queuesEmpty()
{
    if (!d->semaphore.tryAcquire(d->count))
        return false;
    d->semaphore.release(d->count);
    return true;
}

void IgotuControl::info()
{
    if (!d->semaphore.tryAcquire())
        throw IgotuError(tr("Too many concurrent tasks"));
    emit d->info();
}

void IgotuControl::contents()
{
    if (!d->semaphore.tryAcquire())
        throw IgotuError(tr("Too many concurrent tasks"));
    emit d->contents();
}

void IgotuControl::notify(QObject *object, const char *method)
{
    emit d->notify(object, method);
}

} // namespace igotu

#include "igotucontrol.moc"
