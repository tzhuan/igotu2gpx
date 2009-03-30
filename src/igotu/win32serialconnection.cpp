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

#include "exception.h"
#include "win32serialconnection.h"

#include <windows.h>

namespace igotu
{

class Win32SerialConnectionPrivate
{
public:
    QByteArray receiveBuffer;
    HANDLE handle;
};

// Win32SerialConnectionPrivate =====================================================

// Win32SerialConnection ============================================================

Win32SerialConnection::Win32SerialConnection(unsigned port) :
    dataPtr(new Win32SerialConnectionPrivate)
{
    D(Win32SerialConnection);

    QString device = QString::fromLatin1("COM%1").arg(port);
    d->handle = CreateFileA(device.toAscii(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);
    if (d->handle == INVALID_HANDLE_VALUE)
        throw IgotuError(tr("Unable to open device %1").arg(device));

    COMMTIMEOUTS Win_CommTimeouts;
    Win_CommTimeouts.ReadIntervalTimeout = 10;
    Win_CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    Win_CommTimeouts.ReadTotalTimeoutConstant = 200;
    Win_CommTimeouts.WriteTotalTimeoutMultiplier = 2;
    Win_CommTimeouts.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts(d->handle, &Win_CommTimeouts);
}

Win32SerialConnection::~Win32SerialConnection()
{
    D(Win32SerialConnection);

    CloseHandle(d->handle);
}

void Win32SerialConnection::send(const QByteArray &query)
{
    D(Win32SerialConnection);

    DWORD result;

    d->receiveBuffer.clear();
    char dummy[0x10];
    ReadFile(d->handle, dummy, sizeof(dummy), &result, NULL);

    if (!WriteFile(d->handle, query.data(), query.size(), &result, NULL))
        throw IgotuError(tr("Unable to send data to the device"));
    if (result != query.size())
        throw IgotuError(tr("Unable to send data to the device: Tried "
                    "to send %1 bytes, but only succeeded sending %2 bytes")
                .arg(query.size(), result));
}

QByteArray Win32SerialConnection::receive(unsigned expected)
{
    D(Win32SerialConnection);

    unsigned toRead = expected;
    QByteArray data;
    unsigned emptyCount = 0;
    while (emptyCount < 3) {
        unsigned toRemove = qMin(unsigned(d->receiveBuffer.size()), toRead);
        data += d->receiveBuffer.left(toRemove);
        d->receiveBuffer.remove(0, toRemove);
        toRead -= toRemove;
        if (toRead == 0)
            break;
        QByteArray data(toRead, 0);
        DWORD result;
        if (!ReadFile(d->handle, data.data(), data.size(), &result, NULL))
            throw IgotuError(tr("Unable to read data from the device"));
        if (result == 0)
            ++emptyCount;
        d->receiveBuffer += data.left(result);
    }
    return data;
}
} // namespace igotu

