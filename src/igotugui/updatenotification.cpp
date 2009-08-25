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

#include "igotu/utils.h"

#include "updatenotification.h"

#include <QHttp>
#include <QSettings>
#include <QTemporaryFile>
#include <QUrl>

using namespace igotu;

class UpdateNotificationPrivate : public QObject
{
    Q_OBJECT
public:
    QString releaseUrl() const;
    bool newerVersion(const QString &oldVersion, const QString &newVersion);

public Q_SLOTS:
    void on_http_done(bool error);

public:
    UpdateNotification *p;
    QHttp *http;
};

// UpdateNotificationPrivate ===================================================

QString UpdateNotificationPrivate::releaseUrl() const
{
    return QSettings().contains(QLatin1String("releaseUrl")) ?
        QSettings().value(QLatin1String("releaseUrl")).toString() :
        QLatin1String("http://mh21.de/igotu2gpx/releases.txt");
}

bool UpdateNotificationPrivate::newerVersion(const QString &oldVersion,
        const QString &newVersion)
{
    QStringList oldParts = oldVersion.split(QLatin1Char('.'));
    QStringList newParts = newVersion.split(QLatin1Char('.'));
    if (oldParts.value(0).toUInt() < newParts.value(0).toUInt())
        return true;
    if (oldParts.value(1).toUInt() < newParts.value(1).toUInt())
        return true;
    oldParts = oldParts.value(2).split(QLatin1Char('+'));
    newParts = newParts.value(2).split(QLatin1Char('+'));
    if (oldParts.value(0).toUInt() < newParts.value(0).toUInt())
        return true;
    if (oldParts.value(1) < newParts.value(1))
        return true;
    return false;
}

void UpdateNotificationPrivate::on_http_done(bool error)
{
    if (error) {
        qDebug("Unable to retrieve update information: %s",
                qPrintable(http->errorString()));
        return;
    }

    QTemporaryFile iniFile;
    if (!iniFile.open()) {
        qDebug("Unable to create temporary update file: %s",
                qPrintable(iniFile.errorString()));
        return;
    }
    iniFile.write(http->readAll());
    iniFile.flush();
    QSettings settings(iniFile.fileName(), QSettings::IniFormat);

    QString newestVersion = QLatin1String(IGOTU_VERSION_STR);
    Q_FOREACH(const QString group, settings.childGroups()) {
        settings.beginGroup(group);
        if (settings.contains(QLatin1String("version")) &&
            settings.contains(QLatin1String("name")) &&
            settings.contains(QLatin1String("url")) &&
            settings.contains(QLatin1String("status"))) {
            const QString version =
                settings.value(QLatin1String("version")).toString();
            // TODO check for devel/stable in status
            if (newerVersion(newestVersion, version))
                newestVersion = version;
        }
        settings.endGroup();
    }

    if (newestVersion == QLatin1String(IGOTU_VERSION_STR))
        return;

    // TODO: check for ignored versions

    settings.beginGroup(newestVersion);
    // only accept http and https TODO
    emit p->newVersionAvailable(settings.value(QLatin1String("name")).toString(),
                QUrl(settings.value(QLatin1String("url")).toString()));
}

// UpdateNotification ==========================================================

UpdateNotification::UpdateNotification(QObject *parent) :
    QObject(parent),
    d(new UpdateNotificationPrivate)
{
    d->p = this;

    d->http = new QHttp(this);
    d->http->setObjectName(QLatin1String("http"));

    connectSlotsByNameToPrivate(this, d.get());

    // TODO: check update interval

    const QUrl url = d->releaseUrl();
    d->http->setHost(url.host(), url.scheme() == QLatin1String("https") ?
            QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp, url.port(0));
    d->http->get(QString::fromAscii(QUrl(d->releaseUrl()).toEncoded()));
}

UpdateNotification::~UpdateNotification()
{
}

void UpdateNotification::scheduleNewCheck()
{
    // TODO: schedule new check (+7 days)
}

void UpdateNotification::ignoreVersion()
{
    // TODO: mark the current version as ignored
}

#include "updatenotification.moc"
