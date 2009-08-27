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
#include "igotu/messages.h"
#include "igotu/commonmessages.h"

#include "dataconnection.h"

#include <boost/shared_ptr.hpp>

#include <usb.h>

#include <QCoreApplication>

using namespace igotu;

class LibusbConnection : public DataConnection
{
    Q_DECLARE_TR_FUNCTIONS(LibusbConnection)
public:
    LibusbConnection(unsigned vendorId, unsigned productId);
    ~LibusbConnection();

    virtual void send(const QByteArray &query);
    virtual QByteArray receive(unsigned expected);
    virtual void purge();
    virtual Mode mode() const;

private:
    static QList<struct usb_device*> find_devices(unsigned vendor,
            unsigned product);

    QByteArray receiveBuffer;
    boost::shared_ptr<struct usb_dev_handle> handle;
};

class LibusbConnectionCreator :
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

Q_EXPORT_PLUGIN2(libusbConnection, LibusbConnectionCreator)

// Put translations in the right context
//
// TRANSLATOR igotu::Common

// LibusbConnection ============================================================

LibusbConnection::LibusbConnection(unsigned vendorId, unsigned productId)
{
    usb_init();
    if (Messages::verbose() > 0)
        usb_set_debug(255);
    usb_find_busses();
    usb_find_devices();

    if (vendorId == 0)
        vendorId = 0x0df7;
    if (productId == 0)
        productId = 0x0900;

    QList<struct usb_device*> devs = find_devices(vendorId, productId);
    // Just in case, try without a product id
    if (devs.isEmpty())
        devs = find_devices(vendorId, 0);
    if (devs.isEmpty())
        throw IgotuError(Common::tr("Unable to find device %1")
                .arg(QString().sprintf("%04x:%04x", vendorId, productId)));

    Q_FOREACH (struct usb_device *dev, devs) {
        handle.reset(usb_open(dev), usb_close);
        if (handle)
            break;
    }

    if (!handle)
        throw IgotuError(Common::tr("Unable to open device %1")
                .arg(QString().sprintf("%04x:%04x", vendorId, productId)));

#ifdef Q_OS_LINUX
    char buf[256];
    if (usb_get_driver_np(handle.get(), 0, buf, sizeof(buf)) == 0) {
        Messages::verboseMessage(tr
                ("Interface 0 already claimed by driver '%1', detaching")
                .arg(QString::fromAscii(buf)));

        int result = usb_detach_kernel_driver_np(handle.get(), 0);
        if (result < 0)
            throw IgotuError(tr
                    ("Unable to detach kernel driver '%1' from device: %1")
                    .arg(QString::fromAscii(buf),
                        QString::fromLocal8Bit(strerror(-result))));
    }
#endif

    if (usb_claim_interface(handle.get(), 0) != 0)
        throw IgotuError(Common::tr("Unable to claim interface 0 on device %1")
                .arg(QString().sprintf("%04x:%04x", vendorId, productId)));
}

LibusbConnection::~LibusbConnection()
{
    usb_release_interface(handle.get(), 0);
}

QList<struct usb_device*> LibusbConnection::find_devices(unsigned vendor,
        unsigned product)
{
    QList<struct usb_device*> result;

    for (struct usb_bus *bus = usb_get_busses(); bus; bus = bus->next) {
        for (struct usb_device *dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == vendor
                && (product == 0 || dev->descriptor.idProduct == product))
                result.append(dev);
        }
    }

    return result;
}

void LibusbConnection::send(const QByteArray &query)
{
    receiveBuffer.clear();

    if (query.isEmpty())
        return;

    int result = usb_control_msg(handle.get(), 0x21, 0x09, 0x0200, 0x0000,
            const_cast<char*>(query.data()), query.size(), 1000);

    if (result < 0)
        throw IgotuError(tr("Unable to send data to the device: %1")
                .arg(QString::fromLocal8Bit(strerror(-result))));
    if (result != query.size())
        throw IgotuError(Common::tr("Unable to send data to the device: Tried "
                    "to send %1 bytes, but only succeeded sending %2 bytes")
                .arg(query.size(), result));
}

QByteArray LibusbConnection::receive(unsigned expected)
{
    unsigned toRead = expected;
    QByteArray data;
    unsigned emptyCount = 0;
    while (emptyCount < 3) {
        // This is not ideal, the receive needs to be initiated before the
        // actual send(), and the receives need to stick closely together,
        // otherwise data gets lost
        unsigned toRemove = qMin(unsigned(receiveBuffer.size()), toRead);
        data += receiveBuffer.left(toRemove);
        receiveBuffer.remove(0, toRemove);
        toRead -= toRemove;
        if (toRead == 0)
            break;
        QByteArray data(0x10, 0);
        int result = usb_interrupt_read(handle.get(), 0x81, data.data(), 0x10,
                20);
        if (result < 0)
            throw IgotuError(tr("Unable to read data from the device: %1")
                .arg(QString::fromLocal8Bit(strerror(-result))));
        if (result == 0)
            ++emptyCount;
        receiveBuffer += data.left(result);
    }
    return data;
}

void LibusbConnection::purge()
{
    usb_interrupt_read(handle.get(), 0x81, QByteArray(0x10, '\0').data(), 0x10,
            20);
}

DataConnection::Mode LibusbConnection::mode() const
{
#ifdef Q_OS_MACX
    return 0;
#else
    return NonBlockingPurge;
#endif
}

// LibusbConnectionCreator =====================================================

QString LibusbConnectionCreator::dataConnection() const
{
    return QLatin1String("usb");
}

int LibusbConnectionCreator::connectionPriority() const
{
    return 100;
}

QString LibusbConnectionCreator::defaultConnectionId() const
{
    return QLatin1String("0df7:0900");
}

DataConnection *LibusbConnectionCreator::createDataConnection
        (const QString &id) const
{
    return new LibusbConnection
        (id.section(QLatin1Char(':'), 0, 0).toUInt(NULL, 16),
         id.section(QLatin1Char(':'), 1, 1).toUInt(NULL, 16));
}

#include "libusbconnection.moc"
