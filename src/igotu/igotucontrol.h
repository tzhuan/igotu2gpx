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

#ifndef _IGOTU2GPX_SRC_IGOTU_IGOTUCONTROL_H_
#define _IGOTU2GPX_SRC_IGOTU_IGOTUCONTROL_H_

#include "global.h"

#include <boost/scoped_ptr.hpp>

#include <QObject>

namespace igotu
{

class IgotuControlPrivate;

class IGOTU_EXPORT IgotuControl : public QObject
{
    Q_OBJECT
public:
    IgotuControl(QObject *parent = NULL);
    ~IgotuControl();

    // usb:vendor:product, serial:n or image:base64
    QString device() const;
    // default device for the platform
    static QString defaultDevice();

    // may throw
    void info();
    // may throw
    void contents();

    // schedules a slot of an object that will be called when all tasks have
    // been processed
    void notify(QObject *object, const char *method);

    // Returns true if no tasks are pending anymore
    bool queuesEmpty();

public Q_SLOTS:
    void setDevice(const QString &device);

Q_SIGNALS:
    void infoStarted();
    void infoFinished(const QString &info, const QByteArray &contents);
    void infoFailed(const QString &message);

    void contentsStarted();
    // number of blocks finished, from 0 to blocks
    void contentsBlocksFinished(unsigned num, unsigned total);
    void contentsFinished(const QByteArray &contents, unsigned count);
    void contentsFailed(const QString &message);

protected:
    boost::scoped_ptr<IgotuControlPrivate> d;
};

} // namespace igotu

#endif
