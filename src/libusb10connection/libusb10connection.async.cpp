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
#include "igotu/messages.h"

#include "dataconnection.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <libusb.h>

#include <QCoreApplication>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

using namespace igotu;

// TODO: better error messages with libusb error codes

class WorkerThread: public QThread
{
public:
    WorkerThread(libusb_context *context, libusb_device_handle *handle);
    ~WorkerThread();
    virtual void run();

//    void waitForReadTimeout();
//    QByteArray fetchReadBuffer(unsigned count);
//    void purgeReadBuffer();
//    void waitForWriteTransferCompleted();

    void synchronousWriteTransfer(const QByteArray &data);
    void synchronousReadTransfer();

    QByteArray fetchReadBuffer();
    void purgeReadBuffer();

private:
    void scheduleReadTransfer();
    static void handleReadTransfer(libusb_transfer *transfer);

    void scheduleWriteTransfer(const QByteArray &data);
    static void handleWriteTransfer(libusb_transfer *transfer);

    libusb_context *context;
    libusb_device_handle *handle;

    QByteArray readBuffer;
    boost::shared_ptr<libusb_transfer> readTransfer;
    QWaitCondition readCompleted;
    QMutex readGuard;

    QByteArray readData;

    QByteArray writeBuffer;
    boost::shared_ptr<libusb_transfer> writeTransfer;
    QWaitCondition writeCompleted;
    QMutex writeGuard;

    QMutex terminateGuard;
    bool terminate;

    QMutex readUrbGuard;
    bool readUrbPending;
};

class Libusb10Connection : public DataConnection
{
    Q_DECLARE_TR_FUNCTIONS(Libusb10Connection)
public:
    Libusb10Connection(unsigned vendorId, unsigned productId);
    ~Libusb10Connection();

    virtual void send(const QByteArray &query);
    virtual QByteArray receive(unsigned expected);
    virtual void purge();
    virtual Mode mode() const;

private:
    typedef boost::shared_ptr<libusb_device> Device;
    typedef QList<Device> DeviceList;
    DeviceList find_devices(unsigned vendor, unsigned product);

    boost::shared_ptr<libusb_context> context;
    boost::shared_ptr<libusb_device_handle> handle;
    boost::scoped_ptr<WorkerThread> thread;

    // TODO: this shouldn't be needed
    QByteArray receiveBuffer;
};

class Libusb10ConnectionCreator :
    public QObject,
    public DataConnectionCreator
{
    Q_OBJECT
    Q_INTERFACES(igotu::DataConnectionCreator)
public:
    virtual QString dataConnection() const;
    virtual int connectionPriority() const;
    virtual QString defaultConnectionId() const;
    virtual DataConnection *createDataConnection(const QString &id) const;
};

Q_EXPORT_PLUGIN2(libusb10Connection, Libusb10ConnectionCreator)

// Put translations in the right context
//
// TRANSLATOR igotu::Common

// WorkerThread ================================================================

WorkerThread::WorkerThread(libusb_context *context,
        libusb_device_handle *handle) :
    context(context),
    handle(handle),
    terminate(false),
    readUrbPending(false)
{
    writeTransfer.reset(libusb_alloc_transfer(0), libusb_free_transfer);

    readBuffer.resize(0x10);
    readTransfer.reset(libusb_alloc_transfer(0), libusb_free_transfer);
    libusb_fill_interrupt_transfer(readTransfer.get(), handle, 0x81,
            reinterpret_cast<unsigned char*>(readBuffer.data()),
            readBuffer.size(), &handleReadTransfer, this, 20);
    //scheduleReadTransfer();

    setTerminationEnabled();
    start();
}

WorkerThread::~WorkerThread()
{
    {
        QMutexLocker locker(&terminateGuard);
        terminate = true;
    }
    wait();
}

void WorkerThread::run()
{
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10;
    Q_FOREVER {
        // without a timeout, handle_events does not return early if there is a
        // callback coming
        int result = libusb_handle_events_timeout(context, &tv);
        // just loop, TODO: how to break
        if (result)
            break;
        QMutexLocker locker(&terminateGuard);
        if (terminate)
            break;
    }
}

//void WorkerThread::waitForReadTimeout()
//{
//    QMutexLocker locker(&readGuard);
//    readCompleted.wait(&readGuard);
//}

QByteArray WorkerThread::fetchReadBuffer()
{
    QMutexLocker locker(&readGuard);
    const QByteArray result = readData;
    readData.clear();
    return result;
}

void WorkerThread::purgeReadBuffer()
{
    QMutexLocker locker(&readGuard);
    readData.clear();
}

void WorkerThread::synchronousWriteTransfer(const QByteArray &data)
{
    qDebug("syncWrite");
    QMutexLocker locker(&writeGuard);
    scheduleWriteTransfer(data);
    writeCompleted.wait(&writeGuard);
    writeGuard.unlock();
    scheduleReadTransfer();
    // TODO: this is a hack
//    purgeReadBuffer();
    // TODO: return value
}

void WorkerThread::synchronousReadTransfer()
{
    qDebug("syncRead");
    QMutexLocker locker(&readGuard);
    scheduleReadTransfer();
    readCompleted.wait(&readGuard);
//    int size = 0;
//    libusb_interrupt_transfer(handle, 0x81, reinterpret_cast<unsigned char*>(readBuffer.data()), 0x10, &size,
//            20);
//    QByteArray result = readBuffer.left(size);
//    QByteArray result = readData;
//    readData.clear();
//    return result;
}

void WorkerThread::scheduleReadTransfer()
{
    QMutexLocker locker(&readUrbGuard);

    if (readUrbPending)
        return;
    readUrbPending = true;

    qDebug("schedRead");
    bool result = libusb_submit_transfer(readTransfer.get());
    // TODO: somebody needs to handle error conditions here
    Q_UNUSED(result);
//    usleep(5000);
}

void WorkerThread::handleReadTransfer(libusb_transfer *transfer)
{
    WorkerThread * const This =
        reinterpret_cast<WorkerThread*>(transfer->user_data);
    const QByteArray data = This->readBuffer.left(transfer->actual_length);

    This->readGuard.lock();
    This->readData = This->readData + data;
    This->readGuard.unlock();
//    qDebug() << "Read" << data.toHex();

    This->readUrbGuard.lock();
    This->readUrbPending = false;
    This->readUrbGuard.unlock();

    This->scheduleReadTransfer();

    This->readCompleted.wakeOne();
    // TODO: somebody needs to handle error conditions here
}

void WorkerThread::scheduleWriteTransfer(const QByteArray &data)
{
    writeBuffer = QByteArray(8, 0x00) + data;
    libusb_fill_control_setup(reinterpret_cast<unsigned char*>
            (writeBuffer.data()), 0x21, 0x09, 0x0200, 0x0000, data.size());
    libusb_fill_control_transfer(writeTransfer.get(), handle,
            reinterpret_cast<unsigned char*>(writeBuffer.data()),
            &handleWriteTransfer, this, 0x1000);
    libusb_submit_transfer(writeTransfer.get());
}

void WorkerThread::handleWriteTransfer(libusb_transfer *transfer)
{
    WorkerThread * const This =
        reinterpret_cast<WorkerThread*>(transfer->user_data);
    // TODO handle errors
    // TODO purge read buffer
    This->writeCompleted.wakeOne();
}

// Libusb10Connection ==========================================================

Libusb10Connection::Libusb10Connection(unsigned vendorId, unsigned productId)
{
    libusb_context *contextPtr;
    if (int result = libusb_init(&contextPtr))
        throw IgotuError(tr("Unable to initialize libusb: %1").arg(result));
    context.reset(contextPtr, libusb_exit);

    if (Messages::verbose() > 1)
        libusb_set_debug(contextPtr, 3);

    if (vendorId == 0)
        vendorId = 0x0df7;
    if (productId == 0)
        productId = 0x0900;

    DeviceList devs = find_devices(vendorId, productId);
    // Just in case, try without a product id
    if (devs.isEmpty())
        devs = find_devices(vendorId, 0);
    if (devs.isEmpty())
        throw IgotuError(Common::tr("Unable to find device '%1'")
                .arg(QString().sprintf("%04x:%04x", vendorId, productId)));

    Q_FOREACH (const Device &dev, devs) {
        libusb_device_handle *handlePtr;
        if (libusb_open(dev.get(), &handlePtr))
            continue;
        handle.reset(handlePtr, libusb_close);
        break;
    }

    if (!handle)
        throw IgotuError(Common::tr("Unable to open device '%1'")
                .arg(QString().sprintf("%04x:%04x", vendorId, productId)));

#ifdef Q_OS_LINUX
    if (libusb_kernel_driver_active(handle.get(), 0) == 1) {
        Messages::verboseMessage(Common::tr
                ("Interface 0 already claimed by kernel driver, detaching"));

        if (int result = libusb_detach_kernel_driver(handle.get(), 0))
            throw IgotuError(Common::tr
                    ("Unable to detach kernel driver from device '%1': %2")
                    .arg(QString().sprintf("%04x:%04x", vendorId, productId))
                    .arg(result));
    }
#endif

    if (libusb_claim_interface(handle.get(), 0) != 0)
        throw IgotuError(Common::tr("Unable to claim interface 0 on device '%1'")
                .arg(QString().sprintf("%04x:%04x", vendorId, productId)));

    thread.reset(new WorkerThread(context.get(), handle.get()));
}

Libusb10Connection::~Libusb10Connection()
{
    libusb_release_interface(handle.get(), 0);
}

Libusb10Connection::DeviceList Libusb10Connection::find_devices
        (unsigned vendor, unsigned product)
{
    DeviceList result;

    libusb_device **list;
    ssize_t count = libusb_get_device_list(context.get(), &list);
    if (count < 0)
        throw IgotuError(Libusb10Connection::tr
                ("Unable to enumerate usb devices: %1").arg(count));

    for (ssize_t i = 0; i < count; ++i) {
        libusb_device *device = list[i];
        libusb_device_descriptor descriptor;
        if (libusb_get_device_descriptor(device, &descriptor))
            continue;
        if (descriptor.idVendor == vendor
            && (product == 0 || descriptor.idProduct == product))
            result.append(Device(libusb_ref_device(device),
                        libusb_unref_device));
    }

    libusb_free_device_list(list, 1);

    return result;
}

void Libusb10Connection::send(const QByteArray &query)
{
    thread->synchronousWriteTransfer(query);

// TODO
//    if (result < 0)
//        throw IgotuError(Common::tr("Unable to send data to device: %1")
//                .arg(QString::fromLocal8Bit(strerror(-result))));
//    if (result != query.size())
//        throw IgotuError(Common::tr("Unable to send data to device: %1")
//                .arg(Common::tr("Only %1/%2 bytes could be sent"))
//                .arg(result).arg(query.size()));
}

QByteArray Libusb10Connection::receive(unsigned expected)
{
    unsigned toRead = expected;
    QByteArray data;
    unsigned emptyCount = 0;
    // The first empty one is maybe because there was no read scheduled
    while (emptyCount < 4) {
        QByteArray newData = thread->fetchReadBuffer();
        if (newData.isEmpty())
            ++emptyCount;
        receiveBuffer += newData;

        unsigned toRemove = qMin(unsigned(receiveBuffer.size()), toRead);
        data += receiveBuffer.left(toRemove);
        receiveBuffer.remove(0, toRemove);
        toRead -= toRemove;
        if (toRead == 0)
            break;

        thread->synchronousReadTransfer();
        qDebug("More read: %s", newData.toHex().data());
        // There is something weird going on, without the next line, data is
        // reset to 0xff....
        // valgrind does not find anything
        data.data();
        //usleep(1000); // TODO
//        if (result < 0)
//            throw IgotuError(Common::tr("Unable to read data from device: %1")
//                    .arg(result));
    }
    return data;
}

void Libusb10Connection::purge()
{
    qDebug("Purge");
    thread->synchronousReadTransfer();
//    thread->waitForReadTimeout();
    thread->purgeReadBuffer();
}

DataConnection::Mode Libusb10Connection::mode() const
{
    return 0;
//    return NonBlockingPurge;
}

// Libusb10ConnectionCreator ===================================================

QString Libusb10ConnectionCreator::dataConnection() const
{
    return QLatin1String("usb10");
}

int Libusb10ConnectionCreator::connectionPriority() const
{
    return 200;
}

QString Libusb10ConnectionCreator::defaultConnectionId() const
{
    return QLatin1String("0df7:0900");
}

DataConnection *Libusb10ConnectionCreator::createDataConnection
        (const QString &id) const
{
    return new Libusb10Connection
        (id.section(QLatin1Char(':'), 0, 0).toUInt(NULL, 16),
         id.section(QLatin1Char(':'), 1, 1).toUInt(NULL, 16));
}

#include "libusb10connection.moc"
