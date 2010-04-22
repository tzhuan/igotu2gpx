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

#include "messages.h"

#include <QMutex>
#include <QString>

#include <iostream>

namespace igotu
{

class MessagesPrivate
{
public:
    MessagesPrivate() :
        level(0)
    {
    }

    int verbose()
    {
        QMutexLocker locker(&lock);
        return level;
    }

    void setVerbose(int value)
    {
        QMutexLocker locker(&lock);
        level = value;
    }

private:
    int level;
    QMutex lock;
};

Q_GLOBAL_STATIC(MessagesPrivate, messagesPrivate)

int Messages::verbose()
{
    return messagesPrivate()->verbose();
}

void Messages::setVerbose(int value)
{
    messagesPrivate()->setVerbose(value);
}

void Messages::errorMessage(const QString &message)
{
    std::cerr << qPrintable(message) << std::endl;
}

void Messages::normalMessage(const QString &message)
{
    if (messagesPrivate()->verbose() >= 0)
        std::cerr << qPrintable(message) << std::endl;
}

void Messages::verboseMessage(const QString &message)
{
    if (messagesPrivate()->verbose() >= 1)
        std::cerr << qPrintable(message) << std::endl;
}

void Messages::textOutput(const QString &message)
{
    std::cout << qPrintable(message) << std::endl;
}

void Messages::directOutput(const QByteArray &data)
{
    std::cout << std::string(data.data(), data.length());
}

void Messages::normalMessagePart(const QString &message)
{
    if (messagesPrivate()->verbose() >= 0)
        std::cerr << qPrintable(message) << std::flush;
}

} // namespace igotu
