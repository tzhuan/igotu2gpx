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
    virtual void run();

    void waitForReadTimeout();
    QByteArray fetchReadBuffer(unsigned count);
    void purgeReadBuffer();

    void synchronousWriteTransfer(const QByteArray &data);

private:
    void scheduleReadTransfer();
    static void handleReadTransfer(libusb_transfer *transfer);

    void scheduleWriteTransfer(const QByteArray &data);
    void waitForWriteTransferCompleted();
    static void handleWriteTransfer(libusb_transfer *transfer);

    libusb_context *context;
    libusb_device_handle *handle;

    QByteArray readBuffer;
    boost::shared_ptr<libusb_transfer> readTransfer;
    QByteArray readData;
    QWaitCondition readCompleted;
    QMutex readGuard;

    QByteArray writeBuffer;
    boost::shared_ptr<libusb_transfer> writeTransfer;
    QWaitCondition writeCompleted;
    QMutex writeGuard;
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
    handle(handle)
{
    writeTransfer.reset(libusb_alloc_transfer(0), libusb_free_transfer);

    readBuffer.resize(0x10);
    readTransfer.reset(libusb_alloc_transfer(0), libusb_free_transfer);
    libusb_fill_interrupt_transfer(readTransfer.get(), handle, 0x81,
            reinterpret_cast<unsigned char*>(readBuffer.data()),
            readBuffer.size(), &handleReadTransfer, this, 0x20);
    scheduleReadTransfer();

    start();
}

void WorkerThread::run()
{
    while (!libusb_handle_events(context)) {
        // just loop, TODO: how to break
    }
}

void WorkerThread::waitForReadTimeout()
{
    QMutexLocker locker(&readGuard);
    readCompleted.wait(&readGuard);
}

QByteArray WorkerThread::fetchReadBuffer(unsigned count)
{
    QMutexLocker locker(&readGuard);
    const QByteArray result = readData.left(count);
    readData = readData.mid(count);
    return result;
}

void WorkerThread::purgeReadBuffer()
{
    QMutexLocker locker(&readGuard);
    readData.clear();
}

void WorkerThread::synchronousWriteTransfer(const QByteArray &data)
{
    QMutexLocker locker(&writeGuard);
    scheduleWriteTransfer(data);
    writeCompleted.wait(&writeGuard);
    writeGuard.unlock();
    // TODO: return value
}

void WorkerThread::scheduleReadTransfer()
{
   bool result = libusb_submit_transfer(readTransfer.get());
   // TODO: somebody needs to handle error conditions here
   Q_UNUSED(result);
}

void WorkerThread::handleReadTransfer(libusb_transfer *transfer)
{
    WorkerThread * const This =
        reinterpret_cast<WorkerThread*>(transfer->user_data);
    const QByteArray data = This->readBuffer.left(transfer->actual_length);
    This->scheduleReadTransfer();
    // TODO: somebody needs to handle error conditions here

    This->readGuard.lock();
    This->readData = This->readData + data;
    This->readGuard.unlock();
//    qDebug() << "Read" << data.toHex();
    This->readCompleted.wakeOne();
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
    This->writeCompleted.wakeOne();
}

// Libusb10Connection ==========================================================

Libusb10Connection::Libusb10Connection(unsigned vendorId, unsigned productId)
{
    libusb_context *contextPtr;
    if (int result = libusb_init(&contextPtr))
        throw IgotuError(tr("Unable to initialize libusb: %1").arg(result));
    context.reset(contextPtr, libusb_exit);

    if (Messages::verbose() > 0)
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
//        throw IgotuError(tr("Unable to send data to the device: %1")
//                .arg(QString::fromLocal8Bit(strerror(-result))));
//    if (result != query.size())
//        throw IgotuError(Common::tr("Unable to send data to the device: Only "
//                    "%1/%2 bytes could be sent")
//                .arg(result).arg(query.size()));
}

QByteArray Libusb10Connection::receive(unsigned expected)
{
    QByteArray data;
    unsigned emptyCount = 0;
    while (emptyCount < 3) {
        QByteArray newData = thread->fetchReadBuffer(expected - data.size());
        data += newData;
        if (unsigned(data.size()) == expected)
            break;
        if (newData.isEmpty())
            ++emptyCount;
        usleep(20 * 1000);
//        if (result < 0)
//            throw IgotuError(tr("Unable to read data from the device: %1")
//                    .arg(result));
    }
    return data;
}

void Libusb10Connection::purge()
{
//    thread->waitForReadTimeout();
    thread->purgeReadBuffer();
}

DataConnection::Mode Libusb10Connection::mode() const
{
    return NonBlockingPurge;
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
