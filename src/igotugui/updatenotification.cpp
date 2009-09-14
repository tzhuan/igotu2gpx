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
#include <QProcess>
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

    void requestReleases(const QString &os);
    void setLastCheck();

public Q_SLOTS:
    void on_http_done(bool error);
    void on_lsbRelease_finished(int exitCode, QProcess::ExitStatus exitStatus);
    void on_lsbRelease_error();

public:
    UpdateNotification *p;
    QHttp *http;
    QProcess *lsbRelease;
    bool lsbReleaseError;
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

void UpdateNotificationPrivate::requestReleases(const QString &os)
{
    QUrl url = releaseUrl();
    url.addQueryItem(QLatin1String("os"), os);
    url.addQueryItem(QLatin1String("version"), QLatin1String(IGOTU_VERSION_STR));
    http->setHost(url.host(), url.scheme().toLower() == QLatin1String("https") ?
            QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp,
            url.port(0));
    if (!url.userName().isEmpty())
        http->setUser(url.userName(), url.password());
    QByteArray path = QUrl::toPercentEncoding(url.path(), "!$&'()*+,;=:@/");
    if (path.isEmpty())
        path = "/";
    if (url.hasQuery())
        path += '?' + url.encodedQuery();

    QHttpRequestHeader header(QLatin1String("GET"), QString::fromAscii(path));
    header.setValue(QLatin1String("User-Agent"), QLatin1String("Igotu2gpx/") +
            QLatin1String(IGOTU_VERSION_STR));
    http->request(header);
}

void UpdateNotificationPrivate::on_http_done(bool error)
{
    if (error) {
        qWarning("Unable to retrieve update information: %s",
                qPrintable(http->errorString()));
        return;
    }

    // TODO: secure cryptographically, e.g. with SSL or an RSA signature
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

    if (type == UpdateNotification::NotifyNever)
        return;

    if (newestVersion == QLatin1String(IGOTU_VERSION_STR) ||
        newestVersion == ignoredVersion())
        return;

    if (url.scheme().toLower() != QLatin1String("http") &&
        url.scheme().toLower() != QLatin1String("https"))
        return;

    emit p->newVersionAvailable(newestVersion,
            settings.value(QLatin1String("name")).toString(), url);
}

void UpdateNotificationPrivate::on_lsbRelease_error()
{
    if (lsbReleaseError)
        return;

    lsbReleaseError = true;
    requestReleases(QLatin1String("unknown"));
}

void UpdateNotificationPrivate::on_lsbRelease_finished(int exitCode,
        QProcess::ExitStatus exitStatus)
{
    if (lsbReleaseError)
        return;

    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        on_lsbRelease_error();
        return;
    }

    QString os = QString::fromLocal8Bit(lsbRelease
            ->readAllStandardOutput()).trimmed();
    os.replace(QRegExp(QLatin1String("^\"(.*)\"$")), QLatin1String("\\1"));

    requestReleases(os);
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

    d->lsbRelease = new QProcess(this);
    d->lsbRelease->setObjectName(QLatin1String("lsbRelease"));

    connectSlotsByNameToPrivate(this, d.get());
}

UpdateNotification::~UpdateNotification()
{
}

void UpdateNotification::runScheduledCheck()
{
    QDateTime lastCheck(d->lastCheck());
    if (lastCheck.isValid() &&
        lastCheck.secsTo(QDateTime::currentDateTime()) < 7 * 24 * 60 * 60)
        return;

    QString version;
#if defined(Q_OS_WIN32)
    d->requestReleases(QString::fromLatin1("win-0x%1")
            .arg(QSysInfo::WindowsVersion, 4, 10, QLatin1Char('0')));
#elif defined(Q_OS_MAC)
    d->requestReleases(QString::fromLatin1("mac-0x%1")
            .arg(QSysInfo::MacintoshVersion, 4, 10, QLatin1Char('0')));
#else
    d->lsbReleaseError = false;
    d->lsbRelease->start(QLatin1String("lsb_release -ds"));
#endif
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
