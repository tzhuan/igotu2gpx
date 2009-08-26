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

#ifndef _IGOTU2GPX_SRC_IGOTUGUI_UPDATENOTIFICATION_H_
#define _IGOTU2GPX_SRC_IGOTUGUI_UPDATENOTIFICATION_H_

#include <boost/scoped_ptr.hpp>

#include <QMessageBox>

class QUrl;

class UpdateNotificationPrivate;

class UpdateNotification : public QObject
{
    Q_OBJECT
    Q_ENUMS(Type)
    friend class UpdateNotificationPrivate;
public:
    UpdateNotification(QObject *parent = NULL);
    ~UpdateNotification();

    enum Type {
        NotifyNever,
        StableReleases,
        DevelopmentSnapshots
    };

    void scheduleNewCheck();
    void ignoreVersion();

    static Type defaultUpdateNotification();
    void setUpdateNotification(Type type);

public Q_SLOTS:
    void runScheduledCheck();

Q_SIGNALS:
    void newVersionAvailable(const QString &name, const QUrl &url);

protected:
    boost::scoped_ptr<UpdateNotificationPrivate> d;
};

#endif

