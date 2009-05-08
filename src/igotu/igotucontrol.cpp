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

#include "igotucontrol.h"
#include "exception.h"
#include "threadutils.h"

#include <QDir>
#include <QSemaphore>
#include <QStringList>

namespace igotu
{

class IgotuControlPrivateProxy : public QObject
{
    Q_OBJECT
public:
    IgotuControlPrivateProxy(IgotuControlPrivate *pub);

public Q_SLOTS:
    void info();
    void notify(QObject *object, const QByteArray &method);

Q_SIGNALS:
    void infoStarted();
    void infoFinished(const QString &info);
    void infoFailed(const QString &message);

private:
    IgotuControlPrivate * const p;
};

class IgotuControlPrivate : public QObject
{
    Q_OBJECT
    friend class IgotuControl;
public:
    IgotuControlPrivate();

    unsigned count;
    QSemaphore semaphore;
    EventThread thread;
    IgotuControlPrivateProxy proxy;

Q_SIGNALS:
    void info();
    void notify(QObject *object, const QByteArray &method);
};

// IgotuControlPrivate =========================================================

IgotuControlPrivate::IgotuControlPrivate() :
    count(1),
    semaphore(count),
    proxy(this)
{
}

// IgotuControlPrivateProxy ====================================================

IgotuControlPrivateProxy::IgotuControlPrivateProxy(IgotuControlPrivate *pub) :
    p(pub)
{
}

void IgotuControlPrivateProxy::info()
{
    emit infoStarted();
    try {
        // Calculate result
        QString result;
        emit infoFinished(result);
    } catch (const std::exception &e) {
        emit infoFailed(QString::fromLocal8Bit(e.what()));
    }
    p->semaphore.release();
}

void IgotuControlPrivateProxy::notify(QObject *object, const QByteArray &method)
{
    QMetaObject::invokeMethod(object, method);
}

// IgotuControl ================================================================

IgotuControl::IgotuControl(QObject *parent) :
    QObject(parent),
    d(new IgotuControlPrivate)
{
//    qRegisterMetaType<DataCollection>();

    connect(&d->proxy, SIGNAL(infoStarted()),
             this, SIGNAL(infoStarted()));
    connect(&d->proxy, SIGNAL(infoFinished(QString)),
             this, SIGNAL(infoFinished(QString)));
    connect(&d->proxy, SIGNAL(infoFailed(QString)),
             this, SIGNAL(infoFailed(QString)));

    connect(d.get(), SIGNAL(info()),
             &d->proxy, SLOT(info()));
    connect(d.get(), SIGNAL(notify(QObject*,QByteArray)),
             &d->proxy, SLOT(notify(QObject*,QByteArray)));

    d->proxy.moveToThread(&d->thread);
    d->thread.start();
}

IgotuControl::~IgotuControl()
{
    d->semaphore.acquire(d->count);

    // kill the thread
    d->thread.quit();
    d->thread.wait();
}

bool IgotuControl::queuesEmpty()
{
    if (!d->semaphore.tryAcquire(d->count))
        return false;
    d->semaphore.release(d->count);
    return true;
}

void IgotuControl::info()
{
    if (!d->semaphore.tryAcquire())
        throw IgotuError(tr("Too many concurrent tasks"));
    emit d->info();
}

void IgotuControl::notify(QObject *object, const char *method)
{
    emit d->notify(object, method);
}

} // namespace igotu

#include "igotucontrol.moc"
