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

#include "qextserialport/qextserialport.h"

#include "exception.h"
#include "serialconnection.h"

namespace igotu
{

class SerialConnectionPrivate
{
public:
    QByteArray receiveBuffer;
    boost::scoped_ptr<QextSerialPort> port;
};

// SerialConnection ============================================================

SerialConnection::SerialConnection(unsigned device) :
    dataPtr(new SerialConnectionPrivate)
{
    D(SerialConnection);

#if defined(Q_OS_WIN32)
    const QString portTemplate(QLatin1String("COM%1"));
#elif defined(Q_OS_LINUX)
    const QString portTemplate(QLatin1String("/dev/ttyUSB%1"));
#else
    #error "Serial port names not known for this OS"
#endif

    d->port.reset(new QextSerialPort(portTemplate.arg(device),
                QextSerialPort::Polling));
    d->port->open(QIODevice::ReadWrite);
    d->port->setTimeout(100);

    if (!d->port || !d->port->isOpen())
        throw IgotuError(tr("Unable to open RS232 port to device"));
}

SerialConnection::~SerialConnection()
{
}

void SerialConnection::send(const QByteArray &query)
{
    D(SerialConnection);

    d->receiveBuffer.clear();

    d->port->flush();
    if (d->port->write(query) != query.size())
        throw IgotuError(tr("Unable to send data to the device"));
}

QByteArray SerialConnection::receive(unsigned expected)
{
    D(SerialConnection);

    unsigned toRead = expected;
    QByteArray result;
    Q_FOREVER {
        unsigned toRemove = qMin(unsigned(d->receiveBuffer.size()), toRead);
        result += d->receiveBuffer.left(toRemove);
        d->receiveBuffer.remove(0, toRemove);
        toRead -= toRemove;
        if (toRead == 0)
            return result;
        QByteArray data(0x2000, 0);
//        QByteArray data(toRead, 0);
        int received = d->port->read(data.data(), data.size());
        if (received < 0)
            throw IgotuError(tr("Unable to read data from the device"));
        d->receiveBuffer += data.left(received);
    }
}

} // namespace
