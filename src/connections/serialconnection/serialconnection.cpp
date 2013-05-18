/******************************************************************************
 * Copyright (C) 2009  Michael Hofmann <mh21@mh21.de>                         *
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

#include "dataconnection.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

#include <QCoreApplication>

using namespace igotu;

class SerialConnection : public DataConnection
{
    Q_DECLARE_TR_FUNCTIONS(SerialConnection)
public:
    SerialConnection(const QString &port);
    ~SerialConnection();

    virtual void send(const QByteArray &query);
    virtual QByteArray receive(unsigned expected);
    virtual void purge();

private:
    QByteArray receiveBuffer;
#ifdef Q_OS_WIN32
    HANDLE handle;
#else
    int handle;
#endif
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

#ifdef Q_OS_WIN32
QString errorString(DWORD err)
{
    WCHAR *s;
    if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0,
                reinterpret_cast<WCHAR*>(&s), 0, NULL))
        return QString();
    // wchar_t can be a buildin type, but it is always 16bit
    const QString result = QString::fromUtf16(reinterpret_cast<ushort*>(s))
        .remove(QLatin1Char('\r'));
    LocalFree(s);
    return result;
}
#endif

// Put translations in the right context
//
// TRANSLATOR igotu::Common

// SerialConnection ============================================================

SerialConnection::SerialConnection(const QString &port)
{
    QString device = port;
    bool ok = false;
    unsigned portNumber = port.toUInt(&ok);
#ifdef Q_OS_WIN32
    if (ok)
        device = QString::fromLatin1("\\\\.\\COM%1").arg(portNumber);
    handle = CreateFileA(device.toAscii(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);
    if (handle == INVALID_HANDLE_VALUE)
        throw Exception(Common::tr("Unable to open device '%1': %2")
            .arg(device, errorString(GetLastError())));

    COMMTIMEOUTS Win_CommTimeouts;
    Win_CommTimeouts.ReadIntervalTimeout = 10;
    Win_CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    Win_CommTimeouts.ReadTotalTimeoutConstant = 200;
    Win_CommTimeouts.WriteTotalTimeoutMultiplier = 2;
    Win_CommTimeouts.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts(handle, &Win_CommTimeouts);
#else
    if (ok)
        device = QString::fromLatin1("/dev/ttyUSB%1").arg(portNumber);
    handle = open(device.toAscii(), O_RDWR | O_NOCTTY);
    if (handle == -1)
        throw Exception(Common::tr("Unable to open device '%1': %2")
            .arg(device, QString::fromLocal8Bit(strerror(errno))));
    struct termios options;
    tcgetattr(handle, &options);
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1;
    if (tcsetattr(handle, TCSANOW, &options) != 0)
        throw Exception(Common::tr("Unable to open device '%1': %2")
            .arg(device, QString::fromLocal8Bit(strerror(errno))));
#endif
}

SerialConnection::~SerialConnection()
{
#ifdef Q_OS_WIN32
    CloseHandle(handle);
#else
    close(handle);
#endif
}

void SerialConnection::send(const QByteArray &query)
{
    receiveBuffer.clear();

#ifdef Q_OS_WIN32
    DWORD result;

    if (!WriteFile(handle, query.data(), query.size(), &result, NULL))
        throw Exception(Common::tr("Unable to send data to device: %1")
                .arg(errorString(GetLastError())));
#else
    int result;

    do {
        result = write(handle, query.data(), query.size());
    } while (result == -1 && errno == EINTR);
    if (result == -1)
        throw Exception(Common::tr("Unable to send data to device: %1")
                .arg(QString::fromLocal8Bit(strerror(errno))));
#endif
    if (unsigned(result) != unsigned(query.size()))
        throw Exception(Common::tr("Unable to send data to device: %1")
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
#ifdef Q_OS_WIN32
        DWORD result;
        if (!ReadFile(handle, data.data(), data.size(), &result, NULL))
            throw Exception(Common::tr("Unable to read data from device: %1")
                .arg(errorString(GetLastError())));
#else
        int result;
        do {
            result = read(handle, data.data(), data.size());
        } while (result == -1 && errno == EINTR);
        if (result == -1)
            throw Exception(Common::tr("Unable to read data from device: %1")
                .arg(QString::fromLocal8Bit(strerror(errno))));
#endif
        if (result == 0)
            ++emptyCount;
        receiveBuffer += data.left(result);
    }
    return data;
}

void SerialConnection::purge()
{
#ifdef Q_OS_WIN32
    PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
#else
    tcflush(handle, TCIFLUSH);
#endif
}

// SerialConnectionCreator =====================================================

QString SerialConnectionCreator::dataConnection() const
{
    return QLatin1String("serial");
}

int SerialConnectionCreator::connectionPriority() const
{
    return 300;
}

QString SerialConnectionCreator::defaultConnectionId() const
{
#ifdef Q_OS_WIN32
    return QLatin1String("3");
#else
    return QLatin1String("0");
#endif
}

DataConnection *SerialConnectionCreator::createDataConnection
        (const QString &id) const
{
    return new SerialConnection(id);
}

#include "serialconnection.moc"
