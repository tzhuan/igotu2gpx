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

#include <QCoreApplication>
#include <QDateTime>
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
    QDateTime lastCheck() const;
    QString ignoredVersion() const;
    bool newerVersion(const QString &oldVersion, const QString &newVersion) const;

    void setLastCheck();

public Q_SLOTS:
    void on_http_done(bool error);

public:
    UpdateNotification *p;
    QHttp *http;
    UpdateNotification::Type type;
};

#define NOTIFY_URL QLatin1String("Updates/releaseUrl")
#define NOTIFY_LAST QLatin1String("Updates/lastCheck")
#define NOTIFY_IGNORE QLatin1String("Updates/ignoreVersion")

// UpdateNotificationPrivate ===================================================

QString UpdateNotificationPrivate::releaseUrl() const
{
    return QSettings().value(NOTIFY_URL,
            QLatin1String("http://mh21.de/igotu2gpx/releases.txt")).toString();

}

QDateTime UpdateNotificationPrivate::lastCheck() const
{
    return QDateTime::fromString(QSettings().value(NOTIFY_LAST).toString(),
            Qt::ISODate);
}

void UpdateNotificationPrivate::setLastCheck()
{
    QSettings().setValue(NOTIFY_LAST,
            QDateTime::currentDateTime().toString(Qt::ISODate));
}

QString UpdateNotificationPrivate::ignoredVersion() const
{
    return QSettings().value(NOTIFY_IGNORE).toString();
}

bool UpdateNotificationPrivate::newerVersion(const QString &oldVersion,
        const QString &newVersion) const
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
        qWarning("Unable to retrieve update information: %s",
                qPrintable(http->errorString()));
        return;
    }

    QTemporaryFile iniFile;
    if (!iniFile.open()) {
        qWarning("Unable to create temporary update file: %s",
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
            if (newerVersion(newestVersion, version) &&
                    (type == UpdateNotification::DevelopmentSnapshots ||
                     settings.value(QLatin1String("status")).toString() ==
                     QLatin1String("stable")))
                newestVersion = version;
        }
        settings.endGroup();
    }

    settings.beginGroup(newestVersion);
    const QUrl url = QUrl(settings.value(QLatin1String("url")).toString());

    setLastCheck();

    if (newestVersion == QLatin1String(IGOTU_VERSION_STR) ||
        newestVersion == ignoredVersion())
        return;

    if (url.scheme() != QLatin1String("http") &&
        url.scheme() != QLatin1String("https"))
        return;

    emit p->newVersionAvailable(newestVersion,
            settings.value(QLatin1String("name")).toString(), url);
}

// UpdateNotification ==========================================================

UpdateNotification::UpdateNotification(QObject *parent) :
    QObject(parent),
    d(new UpdateNotificationPrivate)
{
    d->p = this;

    setUpdateNotification(defaultUpdateNotification());

    d->http = new QHttp(this);
    d->http->setObjectName(QLatin1String("http"));

    connectSlotsByNameToPrivate(this, d.get());
}

UpdateNotification::~UpdateNotification()
{
}

void UpdateNotification::runScheduledCheck()
{
    if (d->type == NotifyNever)
        return;

    QDateTime lastCheck(d->lastCheck());
    if (lastCheck.isValid() &&
        lastCheck.secsTo(QDateTime::currentDateTime()) < 7 * 24 * 60 * 60)
        return;

    const QUrl url = d->releaseUrl();
    d->http->setHost(url.host(), url.scheme() == QLatin1String("https") ?
            QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp,
            url.port(0));
    d->http->get(QString::fromAscii(QUrl(d->releaseUrl()).toEncoded()));
}

void UpdateNotification::setIgnoredVersion(const QString &version)
{
    if (!version.isEmpty())
        QSettings().setValue(NOTIFY_IGNORE, version);
    else
        QSettings().remove(NOTIFY_IGNORE);
}

UpdateNotification::Type UpdateNotification::defaultUpdateNotification()
{
#ifdef Q_OS_LINUX
    if (QCoreApplication::applicationDirPath() == QLatin1String("/usr/bin"))
        return NotifyNever;
#endif
    const QStringList parts = QString::fromLatin1(IGOTU_VERSION_STR)
        .split(QLatin1Char('.'));
    const QStringList patchParts = parts.value(2).split(QLatin1Char('+'));
    if (patchParts.value(0).toUInt() >= 90)
        return DevelopmentSnapshots;
    return StableReleases;
}

void UpdateNotification::setUpdateNotification(Type type)
{
    d->type = type;
}

#include "updatenotification.moc"
