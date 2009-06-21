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
    void purge();
    void notify(QObject *object, const QByteArray &method);

private:
    void connect();

Q_SIGNALS:
    void infoStarted();
    void infoFinished(const QString &info, const QByteArray &contents);
    void infoFailed(const QString &message);

    void contentsStarted();
    void contentsBlocksFinished(uint num, uint total);
    void contentsFinished(const QByteArray &contents, uint count);
    void contentsFailed(const QString &message);

    void purgeStarted();
    void purgeBlocksFinished(uint num, uint total);
    void purgeFinished();
    void purgeFailed(const QString &message);

private:
    IgotuControlPrivate * const p;

    boost::scoped_ptr<DataConnection> connection;
    QByteArray image;
};

class IgotuControlPrivate : public QObject
{
    Q_OBJECT
    friend class IgotuControl;
public:
    IgotuControlPrivate();

Q_SIGNALS:
    void info();
    void contents();
    void purge();
    void notify(QObject *object, const QByteArray &method);

public:
    unsigned taskCount;
    QSemaphore semaphore;
    EventThread thread;
    IgotuControlPrivateWorker worker;
    QString device;
    int utcOffset;
};

// IgotuControlPrivate =========================================================

IgotuControlPrivate::IgotuControlPrivate() :
    taskCount(1),
    semaphore(taskCount),
    worker(this)
{
}

// IgotuControlPrivateWorker ===================================================

IgotuControlPrivateWorker::IgotuControlPrivateWorker(IgotuControlPrivate *pub) :
    p(pub)
{
}

void IgotuControlPrivateWorker::connect()
{
    connection.reset();
    image.clear();

    const QString protocol = p->device.section(QLatin1Char(':'), 0, 0)
        .toLower();
    const QString name = p->device.section(QLatin1Char(':'), 1);
    if (protocol == QLatin1String ("image")) {
        image = QByteArray::fromBase64(name.toAscii());
#ifdef Q_OS_WIN32
    } else if (protocol == QLatin1String("serial")) {
        connection.reset(new Win32SerialConnection(name.toUint()));
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    } else if (protocol == QLatin1String("usb")) {
        connection.reset(new LibusbConnection(name.section(QLatin1Char(':'), 0,
                        0).toUInt(NULL, 16), name.section(QLatin1Char(':'), 1,
                            1).toUInt(NULL, 16)));
#endif
    } else {
        throw IgotuError(tr("Unable to connect via %1 connection")
                .arg(protocol.toLower()));
    }
}

void IgotuControlPrivateWorker::info()
{
    emit infoStarted();
    try {
        connect();

        QByteArray contents;
        QString status;
        if (connection) {
            NmeaSwitchCommand(connection.get(), false).sendAndReceive();

            IdentificationCommand id(connection.get());
            id.sendAndReceive();
            status += tr("S/N: %1\n").arg(id.serialNumber());
            status += tr("Firmware version: %1\n").arg(id.firmwareVersion());

            ModelCommand model(connection.get());
            model.sendAndReceive();
            if (model.modelId() != ModelCommand::Unknown)
                status += tr("Model: %1\n").arg(model.modelName());
            else
                status += tr("Model: %1, please inform mh21@piware.de\n")
                    .arg(model.modelName());

            CountCommand countCommand(connection.get(),
                    model.modelId() == ModelCommand::Gt120);
            countCommand.sendAndReceive();
            unsigned count = countCommand.trackPointCount();
            status += tr("Number of trackpoints: %1\n").arg(count);
            contents = ReadCommand(connection.get(), 0, 0x1000)
                .sendAndReceive();
            NmeaSwitchCommand(connection.get(), true).sendAndReceive();
        } else {
            contents = image.left(0x1000);
        }

        IgotuPoints igotuPoints(contents, 0);
        if (!igotuPoints.isValid())
            throw IgotuError(tr("Uninitialized device"));

        status += tr("Schedule date: %1").arg(igotuPoints.firstScheduleDate()
                .toString(Qt::SystemLocaleDate)) + QLatin1Char('\n');
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
                        status += tr("Schedule %1:").arg(plan) +
                            QLatin1Char('\n');
                        printed = true;
                    }
                    status += QLatin1String("  ") +
                        tr("Start time: %1").arg(entry.startTime().toString
                                (Qt::SystemLocaleDate)) + QLatin1Char('\n');
                    status += QLatin1String("  ") +
                        tr("End time: %1").arg(entry.endTime().toString
                                (Qt::SystemLocaleDate)) + QLatin1Char('\n');
                    status += QLatin1String("  ") +
                        tr("Log interval: %1 s").arg(entry.logInterval()) +
                        QLatin1Char('\n');
                    if (entry.isIntervalChangeEnabled()) {
                        status += QLatin1String("  ") +
                            tr("Interval change: above %1 km/h, use %2 s")
                            .arg(qRound(entry.intervalChangeSpeed()))
                            .arg(entry.changedLogInterval()) +
                            QLatin1Char('\n');
                    } else {
                        status += QLatin1String("  ") +
                            tr("Interval change: disabled") + QLatin1Char('\n');
                    }
                }
            }
        } else {
            status += tr("Schedule table: disabled") + QLatin1Char('\n');
            ScheduleTableEntry entry = igotuPoints.scheduleTableEntries(1)[0];
            status += tr("Log interval: %1 s").arg(entry.logInterval()) +
                QLatin1Char('\n');
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
        status += tr("Button: %1").arg(igotuPoints.isButtonEnabled() ?
                tr("enabled") : tr("disabled")) + QLatin1Char('\n');

        status += tr("Security version: %1").arg(igotuPoints.securityVersion())
            + QLatin1Char('\n');
        if (igotuPoints.securityVersion() == 0) {
            status += tr("Password: %1, [%2]").arg
                (igotuPoints.isPasswordEnabled() ? tr("enabled") :
                 tr("disabled"), igotuPoints.password()) + QLatin1Char('\n');
        }

        emit infoFinished(status, contents);
    } catch (const std::exception &e) {
        emit infoFailed(QString::fromLocal8Bit(e.what()));
    }
    connection.reset();
    image.clear();

    p->semaphore.release();
}

void IgotuControlPrivateWorker::contents()
{
    emit contentsStarted();
    try {
        connect();

        QByteArray data;
        unsigned count;
        if (connection) {
            NmeaSwitchCommand(connection.get(), false).sendAndReceive();

            ModelCommand model(connection.get());
            model.sendAndReceive();

            CountCommand countCommand(connection.get(),
                    model.modelId() == ModelCommand::Gt120);
            countCommand.sendAndReceive();
            count = countCommand.trackPointCount();
            const unsigned blocks = 1 + (count + 0x7f) / 0x80;

            for (unsigned i = 0; i < blocks; ++i) {
                emit contentsBlocksFinished(i, blocks);
                data += ReadCommand(connection.get(), i * 0x1000,
                        0x1000).sendAndReceive();
            }
            emit contentsBlocksFinished(blocks, blocks);

            NmeaSwitchCommand(connection.get(), true).sendAndReceive();
        } else {
            data = image;
            if (data.size() < 0x1000)
                throw IgotuError(tr("Invalid data"));
            count = (image.size() - 0x1000) / 0x20;
        }

        emit contentsFinished(data, count);
    } catch (const std::exception &e) {
        emit contentsFailed(QString::fromLocal8Bit(e.what()));
    }
    connection.reset();
    image.clear();

    p->semaphore.release();
}

void IgotuControlPrivateWorker::purge()
{
    emit purgeStarted();
    try {
        connect();

        if (!connection)
            throw IgotuError(tr("Need an actual device to purge"));

        NmeaSwitchCommand(connection.get(), false).sendAndReceive();

        ModelCommand model(connection.get());
        model.sendAndReceive();

        switch (model.modelId()) {
        case ModelCommand::Gt120: {
            bool purgeBlocks = false;
            const unsigned blocks = 0x200;
            for (unsigned i = blocks - 1; i > 0; --i) {
                emit purgeBlocksFinished(blocks - i - 1, blocks);
                if (purgeBlocks) {
                    while (UnknownWriteCommand2(connection.get(), 0x0001)
                            .sendAndReceive() != QByteArray(1, '\x00')) {
                        // just wait, TODO: timeout
                    }
                } else {
                    if (ReadCommand(connection.get(), i * 0x1000, 0x10)
                            .sendAndReceive() != QByteArray(0x10, '\xff'))
                        purgeBlocks = true;
                    else
                        continue;
                }
                UnknownWriteCommand1(connection.get(), 0x00).sendAndReceive();
                WriteCommand(connection.get(), 0x20, i * 0x1000, QByteArray())
                    .sendAndReceive();
            }
            if (purgeBlocks) {
                UnknownPurgeCommand1(connection.get(), 0x1e).sendAndReceive();
                UnknownPurgeCommand1(connection.get(), 0x1f).sendAndReceive();
                while (UnknownWriteCommand2(connection.get(), 0x0001)
                        .sendAndReceive() != QByteArray(1, '\x00')) {
                    // just wait, TODO: timeout
                }
            }
            UnknownPurgeCommand1(connection.get(), 0x1e).sendAndReceive();
            UnknownPurgeCommand1(connection.get(), 0x1f).sendAndReceive();
            emit purgeBlocksFinished(blocks, blocks);
            break; }
        case ModelCommand::Gt200: {
            // TODO: better error message
            throw IgotuError(tr("Unable to purge this device model"));
            break; }
        default:
            // TODO: better error message
            throw IgotuError(tr("Unable to purge this device model"));
        }

        NmeaSwitchCommand(connection.get(), true).sendAndReceive();

        emit purgeFinished();
    } catch (const std::exception &e) {
        emit purgeFailed(QString::fromLocal8Bit(e.what()));
    }
    connection.reset();
    image.clear();

    p->semaphore.release();
}

void IgotuControlPrivateWorker::notify(QObject *object,
        const QByteArray &method)
{
    QMetaObject::invokeMethod(object, method);
}

// IgotuControl ================================================================

IgotuControl::IgotuControl(QObject *parent) :
    QObject(parent),
    d(new IgotuControlPrivate)
{
    setDevice(defaultDevice());
    setUtcOffset(defaultUtcOffset());

    connect(&d->worker, SIGNAL(infoStarted()),
             this, SIGNAL(infoStarted()));
    connect(&d->worker, SIGNAL(infoFinished(QString,QByteArray)),
             this, SIGNAL(infoFinished(QString,QByteArray)));
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

    connect(&d->worker, SIGNAL(purgeStarted()),
             this, SIGNAL(purgeStarted()));
    connect(&d->worker, SIGNAL(purgeBlocksFinished(uint,uint)),
             this, SIGNAL(purgeBlocksFinished(uint,uint)));
    connect(&d->worker, SIGNAL(purgeFinished()),
             this, SIGNAL(purgeFinished()));
    connect(&d->worker, SIGNAL(purgeFailed(QString)),
             this, SIGNAL(purgeFailed(QString)));

    connect(d.get(), SIGNAL(info()),
             &d->worker, SLOT(info()));
    connect(d.get(), SIGNAL(contents()),
             &d->worker, SLOT(contents()));
    connect(d.get(), SIGNAL(purge()),
             &d->worker, SLOT(purge()));
    connect(d.get(), SIGNAL(notify(QObject*,QByteArray)),
             &d->worker, SLOT(notify(QObject*,QByteArray)));

    d->worker.moveToThread(&d->thread);
    d->thread.start();
}

IgotuControl::~IgotuControl()
{
    d->semaphore.acquire(d->taskCount);

    // kill the thread
    d->thread.quit();
    d->thread.wait();
}

void IgotuControl::setDevice(const QString &device)
{
    d->device = device;
}

QString IgotuControl::device() const
{
    return d->device;
}

QString IgotuControl::defaultDevice()
{
#ifdef Q_OS_WIN32
    return QLatin1String("serial:3");
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACX)
    return QLatin1String("usb:0df7:0900");
#endif
    return QLatin1String("unknown");
}

void IgotuControl::setUtcOffset(int utcOffset)
{
    d->utcOffset = utcOffset;
}

int IgotuControl::utcOffset() const
{
    return d->utcOffset;
}

int IgotuControl::defaultUtcOffset()
{
    return 0;
}

bool IgotuControl::queuesEmpty()
{
    if (!d->semaphore.tryAcquire(d->taskCount))
        return false;
    d->semaphore.release(d->taskCount);
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

void IgotuControl::purge()
{
    if (!d->semaphore.tryAcquire())
        throw IgotuError(tr("Too many concurrent tasks"));
    emit d->purge();
}

void IgotuControl::notify(QObject *object, const char *method)
{
    emit d->notify(object, method);
}

} // namespace igotu

#include "igotucontrol.moc"
