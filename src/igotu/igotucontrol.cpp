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
#include "igotuconfig.h"
#include "igotucontrol.h"
#include "igotudata.h"
#include "igotupoints.h"
#include "pluginloader.h"
#include "threadutils.h"
#include "utils.h"

#include <QDir>
#include <QLocale>
#include <QMutex>
#include <QSemaphore>
#include <QSet>
#include <QStringList>

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
    void write(const IgotuConfig &config);
    void reset();

    void notify(QObject *object, const QByteArray &method);
    void disconnectQuietly();
    void endTask();

private:
    void connect();
    void disconnect();
    void waitForWrite();

Q_SIGNALS:
    void commandStarted(const QString &message);
    void commandRunning(uint num, uint total);
    void commandFailed(const QString &message);
    void commandSucceeded(const QString &message = QString());

    void infoRetrieved(const QString &info, const QByteArray &contents);
    void contentsRetrieved(const QByteArray &contents, uint count);

private:
    IgotuControlPrivate * const p;

    boost::scoped_ptr<DataConnection> connection;
    QString connectedDevice;
    QByteArray image;
};

class IgotuControlPrivate : public QObject
{
    Q_OBJECT
    friend class IgotuControl;
public:
    IgotuControlPrivate();

    // returns false if there are already too many tasks running
    bool startTask();
    void requestCancel();
    bool cancelRequested() const;

    static QList<DataConnectionCreator*> creators();

Q_SIGNALS:
    void info();
    void contents();
    void purge();
    void write(const IgotuConfig &config);
    void reset();

    void notify(QObject *object, const QByteArray &method);
    void disconnectQuietly();
    void endTask();

public:
    unsigned taskCount;
    QSemaphore semaphore;
    mutable QMutex cancelLock;
    bool cancel;
    EventThread thread;
    IgotuControlPrivateWorker worker;
    QString device;
    int utcOffset;
    bool tracksAsSegments;
};

// Put translations in the right context
//
// TRANSLATOR igotu::IgotuControl

// IgotuControlPrivate =========================================================

IgotuControlPrivate::IgotuControlPrivate() :
    taskCount(10),
    semaphore(taskCount),
    worker(this)
{
}

bool IgotuControlPrivate::startTask()
{
    if (!semaphore.tryAcquire())
        return false;

    QMutexLocker locker(&cancelLock);
    cancel = false;
    return true;
}

bool IgotuControlPrivate::cancelRequested() const
{
    QMutexLocker locker(&cancelLock);

    return cancel;
}

void IgotuControlPrivate::requestCancel()
{
    QMutexLocker locker(&cancelLock);

    cancel = true;
}

QList<DataConnectionCreator*> IgotuControlPrivate::creators()
{
    static QList<DataConnectionCreator*> result;
    if (result.isEmpty()) {
        QMultiMap<int, DataConnectionCreator*> creatorMap;
        Q_FOREACH (DataConnectionCreator * const creator,
                PluginLoader().availablePlugins<DataConnectionCreator>())
            creatorMap.insert(creator->connectionPriority(), creator);
        result = creatorMap.values();
    }
    return result;
}

// IgotuControlPrivateWorker ===================================================

IgotuControlPrivateWorker::IgotuControlPrivateWorker(IgotuControlPrivate *pub) :
    p(pub)
{
}

void IgotuControlPrivateWorker::connect()
{
    if (!connectedDevice.isEmpty() && p->device == connectedDevice)
        return;

    disconnect();

    const QString protocol = p->device.section(QLatin1Char(':'), 0, 0)
        .toLower();
    const QString name = p->device.section(QLatin1Char(':'), 1);
    if (protocol == QLatin1String("image")) {
        image = QByteArray::fromBase64(name.toAscii());
        connectedDevice = p->device;
    } else {
        Q_FOREACH (DataConnectionCreator *creator, p->creators()) {
            if (creator->dataConnection() != protocol)
                continue;
            try {
                connection.reset(creator->createDataConnection(name));
                connectedDevice = p->device;
                NmeaSwitchCommand(connection.get(), false).sendAndReceive();
            } catch (...) {
                connection.reset();
                connectedDevice.clear();
                throw;
            }
            break;
        }
        if (!connection)
            throw IgotuError(IgotuControl::tr("Unable to connect via '%1'")
                    .arg(p->device));
    }
}

void IgotuControlPrivateWorker::disconnectQuietly()
{
    try {
        disconnect();
    } catch (...) {
        // ignored
    }
}

void IgotuControlPrivateWorker::disconnect()
{
    image.clear();
    connectedDevice.clear();
    if (connection) {
        try {
            NmeaSwitchCommand(connection.get(), true).sendAndReceive();
        } catch (...) {
            // the next connect should not come here again
            connection.reset();
            throw;
        }
        connection.reset();
    }
}

void IgotuControlPrivateWorker::endTask()
{
    p->semaphore.release();
}

void IgotuControlPrivateWorker::waitForWrite()
{
    for (unsigned retries = 10; retries > 0; --retries) {
        if (UnknownWriteCommand2(connection.get(), 0x0001)
                .sendAndReceive() == QByteArray(1, '\x00'))
            break;
        if (retries == 1)
            throw IgotuError(IgotuControl::tr("Command timeout"));
    }
}

void IgotuControlPrivateWorker::info()
{
    if (p->cancelRequested())
        return;

    emit commandStarted(tr("Downloading configuration..."));
    try {
        connect();

        QByteArray contents;
        QString status;
        if (connection) {
            IdentificationCommand id(connection.get());
            id.sendAndReceive();
            status += IgotuControl::tr("Serial number: %1").arg(id.serialNumber()) +
                QLatin1Char('\n');
            status += IgotuControl::tr("Firmware version: %1")
                .arg(id.firmwareVersionString()) + QLatin1Char('\n');

            ModelCommand model(connection.get());
            model.sendAndReceive();
            if (model.modelId() != ModelCommand::Unknown)
                status += IgotuControl::tr("Model: %1").arg(model.modelName()) +
                    QLatin1Char('\n');
            else
                status += IgotuControl::tr("Model: %1, please file a bug at "
                        "http://bugs.launchpad.net/igotu2gpx/+filebug")
                    .arg(model.modelName()) + QLatin1Char('\n');

            // Workaround necessary for:
            //   GT120 3.03
            //   GT100 2.24
            // Not necessary for:
            //   GT100 1.39
            // Can be forced with env variables:
            //   IGOTU2GPX_WORKAROUND and IGOTU2GPX_NOWORKAROUND
            // There seems to be a major firmware rework around 2.15 (debug
            // mode was introduced), so until proven otherwise we just use this
            // version)
            CountCommand countCommand(connection.get(),
                    id.firmwareVersion() >= 0x0215);
            countCommand.sendAndReceive();
            unsigned count = countCommand.trackPointCount();
            status += IgotuControl::tr("Number of points: %1")
                .arg(count) + QLatin1Char('\n');
            contents = ReadCommand(connection.get(), 0, 0x1000)
                .sendAndReceive();
        } else {
            contents = image.left(0x1000);
        }

        IgotuConfig igotuPoints(contents);
        if (!igotuPoints.isValid())
            throw IgotuError(IgotuControl::tr("Uninitialized GPS tracker"));

        status += IgotuControl::tr("Schedule date: %1")
            .arg(QLocale::system().toString(igotuPoints.firstScheduleDate() ,
                        QLocale::LongFormat)) + QLatin1Char('\n');
        status += IgotuControl::tr("Schedule date offset: %n day(s)", "",
                igotuPoints.dateOffset()) + QLatin1Char('\n');
        QList<unsigned> tablePlans = igotuPoints.scheduleTablePlans();
        QSet<unsigned> tablePlanSet = QSet<unsigned>::fromList(tablePlans);
        if (igotuPoints.isScheduleTableEnabled()) {
            status += IgotuControl::tr("Schedule plans: %1")
                .arg(IgotuControl::tr("enabled")) + QLatin1Char('\n');
            status += IgotuControl::tr("Schedule plans used:");
            Q_FOREACH (unsigned plan, tablePlanSet)
                status += QLatin1Char(' ') + QString::number(plan);
            status += QLatin1Char('\n');
            status += IgotuControl::tr("Schedule plan order:") +
                QLatin1Char(' ');
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
                        status += IgotuControl::tr("Schedule plan %1:").arg(plan) +
                            QLatin1Char('\n');
                        printed = true;
                    }
                    status += QLatin1String("  ") + IgotuControl::tr
                        ("Start time: %1")
                        .arg(QLocale::system().toString(entry.startTime(),
                                    QLocale::LongFormat)) + QLatin1Char('\n');
                    status += QLatin1String("  ") + IgotuControl::tr
                        ("End time: %1")
                        .arg(QLocale::system().toString(entry.endTime(),
                                    (QLocale::LongFormat))) + QLatin1Char('\n');
                    status += QLatin1String("  ") + IgotuControl::tr
                        ("Log interval: %1 s")
                        .arg(entry.logInterval()) + QLatin1Char('\n');
                    if (entry.intervalChangeSpeed() != 0.0) {
                        status += QLatin1String("  ") + IgotuControl::tr
                            ("Interval change: %1")
                            .arg(IgotuControl::tr("above %1 km/h, use %2 s"))
                            .arg(qRound(entry.intervalChangeSpeed()))
                            .arg(entry.changedLogInterval()) +
                            QLatin1Char('\n');
                    } else {
                        status += QLatin1String("  ") + IgotuControl::tr
                            ("Interval change: %1")
                            .arg(IgotuControl::tr("disabled")) + QLatin1Char('\n');
                    }
                }
            }
        } else {
            status += IgotuControl::tr("Schedule table: %1")
                .arg(IgotuControl::tr("disabled")) + QLatin1Char('\n');
            ScheduleTableEntry entry = igotuPoints.scheduleTableEntries(1)[0];
            status += IgotuControl::tr("Log interval: %1 s")
                .arg(entry.logInterval()) + QLatin1Char('\n');
            if (entry.intervalChangeSpeed() != 0.0) {
                status += IgotuControl::tr
                    ("Interval change: %1")
                    .arg(IgotuControl::tr("above %1 km/h, use %2 s"))
                    .arg(qRound(entry.intervalChangeSpeed()))
                    .arg(entry.changedLogInterval()) + QLatin1Char('\n');
            } else {
                status += IgotuControl::tr("Interval change: %1")
                    .arg(IgotuControl::tr("disabled")) + QLatin1Char('\n');
            }
        }

        status += IgotuControl::tr("LEDs: %1").arg(igotuPoints.ledsEnabled() ?
                IgotuControl::tr("enabled") : IgotuControl::tr("disabled")) +
            QLatin1Char('\n');
        status += IgotuControl::tr("Button: %1")
            .arg(igotuPoints.isButtonEnabled() ?
                    IgotuControl::tr("enabled") : IgotuControl::tr("disabled"))
            + QLatin1Char('\n');

        status += IgotuControl::tr("Security version: %1")
            .arg(igotuPoints.securityVersion()) + QLatin1Char('\n');
        if (igotuPoints.securityVersion() == 0) {
            status += IgotuControl::tr("Password: %1, [%2]")
                .arg(igotuPoints.isPasswordEnabled() ?
                        IgotuControl::tr("enabled") :
                        IgotuControl::tr("disabled"),
                        igotuPoints.password()) + QLatin1Char('\n');
        }

        emit commandSucceeded();
        emit infoRetrieved(status, contents);
    } catch (const std::exception &e) {
        disconnectQuietly();
        emit commandFailed(tr
                ("Unable to download configuration from GPS tracker: %1")
                .arg(QString::fromLocal8Bit(e.what())));
    }
}

void IgotuControlPrivateWorker::contents()
{
    if (p->cancelRequested())
        return;

    emit commandStarted(tr("Downloading tracks..."));
    try {
        connect();

        QByteArray data;
        unsigned count;
        if (connection) {
            IdentificationCommand id(connection.get());
            id.sendAndReceive();

            CountCommand countCommand(connection.get(),
                    id.firmwareVersion() >= 0x0215);
            countCommand.sendAndReceive();
            count = countCommand.trackPointCount();
            const unsigned blocks = 1 + (count + 0x7f) / 0x80;

            for (unsigned i = 0; i < blocks; ++i) {
                emit commandRunning(i, blocks);
                if (p->cancelRequested())
                    throw IgotuError(IgotuControl::tr("Cancelled"));
                data += ReadCommand(connection.get(), i * 0x1000,
                        0x1000).sendAndReceive();
            }
            emit commandRunning(blocks, blocks);
        } else {
            data = image;
            if (data.size() < 0x1000)
                throw IgotuError(IgotuControl::tr("Invalid data"));
            count = (data.size() - 0x1000) / 0x20;
        }

        emit commandSucceeded();
        emit contentsRetrieved(data, count);
    } catch (const std::exception &e) {
        disconnectQuietly();
        emit commandFailed(tr
                ("Unable to download trackpoints from GPS tracker: %1")
                .arg(QString::fromLocal8Bit(e.what())));
    }
}

void IgotuControlPrivateWorker::purge()
{
    if (p->cancelRequested())
        return;

    emit commandStarted(tr("Clearing memory..."));
    try {
        connect();

        if (!connection)
            throw IgotuError(IgotuControl::tr("No device specified"));

        IdentificationCommand id(connection.get());
        id.sendAndReceive();

        ModelCommand model(connection.get());
        model.sendAndReceive();

        unsigned blocks = 1;

        switch (model.modelId()) {
        case ModelCommand::Gt100:
            blocks = 0x080;
            break;
        case ModelCommand::Gt120:
            blocks = 0x200;
            break;
        case ModelCommand::Gt200:
            blocks = 0x100;
            break;
        default:
            throw IgotuError(IgotuControl::tr
                    ("%1: Unable to clear memory of this GPS tracker model. "
                     "Instructions how to help with this can be found at "
                     "https://answers.launchpad.net/igotu2gpx/+faq/480.")
                    .arg(model.modelName()));
        }

        if (id.firmwareVersion() >= 0x0215) {
            bool purgeBlocks = false;
            for (unsigned i = blocks - 1; i > 0; --i) {
                emit commandRunning(blocks - i - 1, blocks);
                if (p->cancelRequested())
                    throw IgotuError(IgotuControl::tr("Cancelled"));
                if (purgeBlocks) {
                    waitForWrite();
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
                waitForWrite();
            }
            UnknownPurgeCommand1(connection.get(), 0x1e).sendAndReceive();
            UnknownPurgeCommand1(connection.get(), 0x1f).sendAndReceive();
            emit commandRunning(blocks, blocks);
        } else {
            bool purgeBlocks = false;
            for (unsigned i = blocks - 1; i > 0; --i) {
                emit commandRunning(blocks - i - 1, blocks);
                if (p->cancelRequested())
                    throw IgotuError(IgotuControl::tr("Cancelled"));
                if (purgeBlocks) {
                    waitForWrite();
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
                UnknownPurgeCommand2(connection.get()).sendAndReceive();
                waitForWrite();
            }
            UnknownPurgeCommand2(connection.get()).sendAndReceive();
            emit commandRunning(blocks, blocks);
        }

        emit commandSucceeded();
    } catch (const std::exception &e) {
        disconnectQuietly();
        emit commandFailed(tr
            ("Unable to clear memory of GPS tracker: %1")
            .arg(QString::fromLocal8Bit(e.what())));
    }
}

void IgotuControlPrivateWorker::write(const IgotuConfig &config)
{
    if (p->cancelRequested())
        return;

    emit commandStarted(tr("Writing configuration..."));
    try {
        connect();

        QString message;

        if (connection) {
            IdentificationCommand id(connection.get());
            id.sendAndReceive();

            ModelCommand model(connection.get());
            model.sendAndReceive();

            UnknownWriteCommand1(connection.get(), 0x00).sendAndReceive();
            WriteCommand(connection.get(), 0x20, 0x0000, QByteArray())
                .sendAndReceive();
            waitForWrite();

            unsigned blocks = 0x10;
            QByteArray configData = config.memoryDump();
            for (unsigned i = 0; i < blocks; ++i) {
                emit commandRunning(i, blocks + 1);
                if (p->cancelRequested())
                    throw IgotuError(IgotuControl::tr("Cancelled"));
                UnknownWriteCommand1(connection.get(), 0x00).sendAndReceive();
                WriteCommand(connection.get(), 0x02, i * 0x0100,
                        configData.mid(i * 0x0100, 0x100)).sendAndReceive();
                waitForWrite();
            }
            emit commandRunning(blocks, blocks + 1);
            TimeCommand(connection.get(), QDateTime::currentDateTime()
                    .toUTC().time()).sendAndReceive();
            ReadCommand(connection.get(), 0, 0x1000).sendAndReceive();
            UnknownWriteCommand3(connection.get()).sendAndReceive();
            emit commandRunning(blocks + 1, blocks + 1);
        } else {
            message = dumpDiff(IgotuData(image, 0).config().memoryDump(),
                    config.memoryDump());
        }

        emit commandSucceeded(message);
    } catch (const std::exception &e) {
        disconnectQuietly();
        emit commandFailed(tr
            ("Unable to write configuration to GPS tracker: %1")
            .arg(QString::fromLocal8Bit(e.what())));
    }
}

void IgotuControlPrivateWorker::reset()
{
    // TODO
    write(IgotuConfig::gt120DefaultConfig());
    // TODO There are some more commands missing here...
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
    qRegisterMetaType<IgotuConfig>("IgotuConfig");

    setDevice(defaultDevice());
    setUtcOffset(defaultUtcOffset());
    setTracksAsSegments(defaultTracksAsSegments());

    connect(&d->worker, SIGNAL(commandStarted(QString)),
             this, SIGNAL(commandStarted(QString)));
    connect(&d->worker, SIGNAL(commandRunning(uint,uint)),
             this, SIGNAL(commandRunning(uint,uint)));
    connect(&d->worker, SIGNAL(commandFailed(QString)),
             this, SIGNAL(commandFailed(QString)));
    connect(&d->worker, SIGNAL(commandSucceeded(QString)),
             this, SIGNAL(commandSucceeded(QString)));

    connect(&d->worker, SIGNAL(infoRetrieved(QString,QByteArray)),
             this, SIGNAL(infoRetrieved(QString,QByteArray)));
    connect(&d->worker, SIGNAL(contentsRetrieved(QByteArray,uint)),
             this, SIGNAL(contentsRetrieved(QByteArray,uint)));

    connect(d.get(), SIGNAL(info()),
             &d->worker, SLOT(info()));
    connect(d.get(), SIGNAL(contents()),
             &d->worker, SLOT(contents()));
    connect(d.get(), SIGNAL(purge()),
             &d->worker, SLOT(purge()));
    connect(d.get(), SIGNAL(write(IgotuConfig)),
             &d->worker, SLOT(write(IgotuConfig)));
    connect(d.get(), SIGNAL(reset()),
             &d->worker, SLOT(reset()));

    connect(d.get(), SIGNAL(notify(QObject*,QByteArray)),
             &d->worker, SLOT(notify(QObject*,QByteArray)));
    connect(d.get(), SIGNAL(disconnectQuietly()),
             &d->worker, SLOT(disconnectQuietly()));
    connect(d.get(), SIGNAL(endTask()),
             &d->worker, SLOT(endTask()));

    d->worker.moveToThread(&d->thread);
    d->thread.start();
}

IgotuControl::~IgotuControl()
{
    d->semaphore.acquire(d->taskCount);
    QMetaObject::invokeMethod(&d->worker, "disconnectQuietly", Qt::BlockingQueuedConnection);

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
    const DataConnectionCreator * const creator =
        IgotuControlPrivate::creators().value(0);
    if (!creator)
        return QString();
    return creator->dataConnection() + QLatin1Char(':') +
        creator->defaultConnectionId();
}

void IgotuControl::setUtcOffset(int utcOffset)
{
    d->utcOffset = utcOffset;
}

int IgotuControl::utcOffset() const
{
    return d->utcOffset;
}

void IgotuControl::setTracksAsSegments(bool tracksAsSegments)
{
    d->tracksAsSegments = tracksAsSegments;
}

bool IgotuControl::tracksAsSegments() const
{
    return d->tracksAsSegments;
}

int IgotuControl::defaultUtcOffset()
{
    return 0;
}

bool IgotuControl::defaultTracksAsSegments()
{
    return false;
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
    if (!d->startTask())
        return;
    emit d->info();
    emit d->endTask();
}

void IgotuControl::contents()
{
    if (!d->startTask())
        return;
    emit d->contents();
    emit d->endTask();
}

void IgotuControl::purge()
{
    if (!d->startTask())
        return;
    emit d->purge();
    emit d->endTask();
}

void IgotuControl::write(const IgotuConfig &config)
{
    if (!d->startTask())
        return;
    emit d->write(config);
    emit d->endTask();
}

void IgotuControl::reset()
{
    if (!d->startTask())
        return;
    emit d->reset();
    emit d->endTask();
}

void IgotuControl::cancel()
{
    d->requestCancel();
}

void IgotuControl::notify(QObject *object, const char *method)
{
    emit d->notify(object, method);
}

} // namespace igotu

#include "igotucontrol.moc"
