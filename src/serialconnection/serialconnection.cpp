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

#include "dataconnection.h"

#include <windows.h>

#include <QCoreApplication>

using namespace igotu;

class SerialConnection : public DataConnection
{
    Q_DECLARE_TR_FUNCTIONS(SerialConnection)
public:
    SerialConnection(unsigned port);
    ~SerialConnection();

    virtual void send(const QByteArray &query);
    virtual QByteArray receive(unsigned expected);
    virtual void purge();
    virtual Mode mode() const;

private:
    QByteArray receiveBuffer;
    HANDLE handle;
};

class SerialConnectionCreator :
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

Q_EXPORT_PLUGIN2(serialConnection, SerialConnectionCreator)

QString errorString(DWORD err)
{
    WCHAR *s;
    if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, &s, 0, NULL))
        return QString();
    const QString result = QString::fromUtf16(s).remove(QLatin1Char('\r'));
    LocalFree(s);
    return result;
}

// Put translations in the right context
//
// TRANSLATOR igotu::Common

// SerialConnection ============================================================

SerialConnection::SerialConnection(unsigned port)
{
    QString device = QString::fromLatin1("COM%1").arg(port);
    handle = CreateFileA(device.toAscii(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);
    if (handle == INVALID_HANDLE_VALUE)
        throw IgotuError(Common::tr("Unable to open device %1:").arg(device));

    COMMTIMEOUTS Win_CommTimeouts;
    Win_CommTimeouts.ReadIntervalTimeout = 10;
    Win_CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    Win_CommTimeouts.ReadTotalTimeoutConstant = 200;
    Win_CommTimeouts.WriteTotalTimeoutMultiplier = 2;
    Win_CommTimeouts.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts(handle, &Win_CommTimeouts);
}

SerialConnection::~SerialConnection()
{
    CloseHandle(handle);
}

void SerialConnection::send(const QByteArray &query)
{
    DWORD result;

    receiveBuffer.clear();

    if (!WriteFile(handle, query.data(), query.size(), &result, NULL))
        throw IgotuError(Common::tr("Unable to send data to device: %1")
                .arg(errorString(LastError())));
    if (result != unsigned(query.size()))
        throw IgotuError(Common::tr("Unable to send data to device: %1")
                .arg(Common::tr("Only %1/%2 bytes could be sent"))
                .arg(result).arg(query.size()));
}

QByteArray SerialConnection::receive(unsigned expected)
{
    unsigned toRead = expected;
    QByteArray data;
    unsigned emptyCount = 0;
    while (emptyCount < 3) {
        unsigned toRemove = qMin(unsigned(receiveBuffer.size()), toRead);
        data += receiveBuffer.left(toRemove);
        receiveBuffer.remove(0, toRemove);
        toRead -= toRemove;
        if (toRead == 0)
            break;
        QByteArray data(toRead, 0);
        DWORD result;
        if (!ReadFile(handle, data.data(), data.size(), &result, NULL))
            throw IgotuError(tr("Unable to read data from device"));
        if (result == 0)
            ++emptyCount;
        receiveBuffer += data.left(result);
    }
    return data;
}

void SerialConnection::purge()
{
    PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

DataConnection::Mode SerialConnection::mode() const
{
    return NonBlockingPurge;
}

// SerialConnectionCreator =====================================================

QString SerialConnectionCreator::dataConnection() const
{
    return QLatin1String("serial");
}

int SerialConnectionCreator::connectionPriority() const
{
    return 0;
}

QString SerialConnectionCreator::defaultConnectionId() const
{
    return QLatin1String("3");
}

DataConnection *SerialConnectionCreator::createDataConnection
        (const QString &id) const
{
    return new SerialConnection(id.toUInt());
}

#include "serialconnection.moc"
