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

#include "igotu/utils.h"

#include "updatenotification.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QHttp>
#include <QProcess>
#include <QSettings>
#include <QSslError>
#include <QSslSocket>
#include <QTemporaryFile>
#include <QUrl>
#include <QUuid>

using namespace igotu;

class UpdateNotificationPrivate : public QObject
{
    Q_OBJECT
public:
    QString releaseUrl() const;
    QDateTime lastCheck() const;
    QString ignoredVersion() const;
    QString uuid() const;
    bool newerVersion(const QString &oldVersion, const QString &newVersion) const;

    void requestReleases(const QString &os);
    void setLastCheck();

public Q_SLOTS:
    void on_http_sslErrors(const QList<QSslError> &errors);
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
#define NOTIFY_UUID QLatin1String("Updates/uuid")

#define DEFAULT_RELEASE_URL QLatin1String("https://mh21.de/igotu2gpx/releases.txt")

// CAcert is not included per default, but the certificate at mh21.de
// is signed by it

const char rootCaCert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIHPTCCBSWgAwIBAgIBADANBgkqhkiG9w0BAQQFADB5MRAwDgYDVQQKEwdSb290\n"
    "IENBMR4wHAYDVQQLExVodHRwOi8vd3d3LmNhY2VydC5vcmcxIjAgBgNVBAMTGUNB\n"
    "IENlcnQgU2lnbmluZyBBdXRob3JpdHkxITAfBgkqhkiG9w0BCQEWEnN1cHBvcnRA\n"
    "Y2FjZXJ0Lm9yZzAeFw0wMzAzMzAxMjI5NDlaFw0zMzAzMjkxMjI5NDlaMHkxEDAO\n"
    "BgNVBAoTB1Jvb3QgQ0ExHjAcBgNVBAsTFWh0dHA6Ly93d3cuY2FjZXJ0Lm9yZzEi\n"
    "MCAGA1UEAxMZQ0EgQ2VydCBTaWduaW5nIEF1dGhvcml0eTEhMB8GCSqGSIb3DQEJ\n"
    "ARYSc3VwcG9ydEBjYWNlcnQub3JnMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIIC\n"
    "CgKCAgEAziLA4kZ97DYoB1CW8qAzQIxL8TtmPzHlawI229Z89vGIj053NgVBlfkJ\n"
    "8BLPRoZzYLdufujAWGSuzbCtRRcMY/pnCujW0r8+55jE8Ez64AO7NV1sId6eINm6\n"
    "zWYyN3L69wj1x81YyY7nDl7qPv4coRQKFWyGhFtkZip6qUtTefWIonvuLwphK42y\n"
    "fk1WpRPs6tqSnqxEQR5YYGUFZvjARL3LlPdCfgv3ZWiYUQXw8wWRBB0bF4LsyFe7\n"
    "w2t6iPGwcswlWyCR7BYCEo8y6RcYSNDHBS4CMEK4JZwFaz+qOqfrU0j36NK2B5jc\n"
    "G8Y0f3/JHIJ6BVgrCFvzOKKrF11myZjXnhCLotLddJr3cQxyYN/Nb5gznZY0dj4k\n"
    "epKwDpUeb+agRThHqtdB7Uq3EvbXG4OKDy7YCbZZ16oE/9KTfWgu3YtLq1i6L43q\n"
    "laegw1SJpfvbi1EinbLDvhG+LJGGi5Z4rSDTii8aP8bQUWWHIbEZAWV/RRyH9XzQ\n"
    "QUxPKZgh/TMfdQwEUfoZd9vUFBzugcMd9Zi3aQaRIt0AUMyBMawSB3s42mhb5ivU\n"
    "fslfrejrckzzAeVLIL+aplfKkQABi6F1ITe1Yw1nPkZPcCBnzsXWWdsC4PDSy826\n"
    "YreQQejdIOQpvGQpQsgi3Hia/0PsmBsJUUtaWsJx8cTLc6nloQsCAwEAAaOCAc4w\n"
    "ggHKMB0GA1UdDgQWBBQWtTIb1Mfz4OaO873SsDrusjkY0TCBowYDVR0jBIGbMIGY\n"
    "gBQWtTIb1Mfz4OaO873SsDrusjkY0aF9pHsweTEQMA4GA1UEChMHUm9vdCBDQTEe\n"
    "MBwGA1UECxMVaHR0cDovL3d3dy5jYWNlcnQub3JnMSIwIAYDVQQDExlDQSBDZXJ0\n"
    "IFNpZ25pbmcgQXV0aG9yaXR5MSEwHwYJKoZIhvcNAQkBFhJzdXBwb3J0QGNhY2Vy\n"
    "dC5vcmeCAQAwDwYDVR0TAQH/BAUwAwEB/zAyBgNVHR8EKzApMCegJaAjhiFodHRw\n"
    "czovL3d3dy5jYWNlcnQub3JnL3Jldm9rZS5jcmwwMAYJYIZIAYb4QgEEBCMWIWh0\n"
    "dHBzOi8vd3d3LmNhY2VydC5vcmcvcmV2b2tlLmNybDA0BglghkgBhvhCAQgEJxYl\n"
    "aHR0cDovL3d3dy5jYWNlcnQub3JnL2luZGV4LnBocD9pZD0xMDBWBglghkgBhvhC\n"
    "AQ0ESRZHVG8gZ2V0IHlvdXIgb3duIGNlcnRpZmljYXRlIGZvciBGUkVFIGhlYWQg\n"
    "b3ZlciB0byBodHRwOi8vd3d3LmNhY2VydC5vcmcwDQYJKoZIhvcNAQEEBQADggIB\n"
    "ACjH7pyCArpcgBLKNQodgW+JapnM8mgPf6fhjViVPr3yBsOQWqy1YPaZQwGjiHCc\n"
    "nWKdpIevZ1gNMDY75q1I08t0AoZxPuIrA2jxNGJARjtT6ij0rPtmlVOKTV39O9lg\n"
    "18p5aTuxZZKmxoGCXJzN600BiqXfEVWqFcofN8CCmHBh22p8lqOOLlQ+TyGpkO/c\n"
    "gr/c6EWtTZBzCDyUZbAEmXZ/4rzCahWqlwQ3JNgelE5tDlG+1sSPypZt90Pf6DBl\n"
    "Jzt7u0NDY8RD97LsaMzhGY4i+5jhe1o+ATc7iwiwovOVThrLm82asduycPAtStvY\n"
    "sONvRUgzEv/+PDIqVPfE94rwiCPCR/5kenHA0R6mY7AHfqQv0wGP3J8rtsYIqQ+T\n"
    "SCX8Ev2fQtzzxD72V7DX3WnRBnc0CkvSyqD/HMaMyRa+xMwyN2hzXwj7UfdJUzYF\n"
    "CpUCTPJ5GhD22Dp1nPMd8aINcGeGG7MW9S/lpOt5hvk9C8JzC6WZrG/8Z7jlLwum\n"
    "GCSNe9FINSkYQKyTYOGWhlC0elnYjyELn8+CkcY7v2vcB5G5l1YjqrZslMZIBjzk\n"
    "zk6q5PYvCdxTby78dOs6Y5nCpqyJvKeyRKANihDjbPIky/qbn3BHLt4Ui9SyIAmW\n"
    "omTxJBzcoTWcFbLUvFUufQb1nA5V9FrWk9p2rSVzTMVD\n"
    "-----END CERTIFICATE-----\n";

const char class3CaCert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIGCDCCA/CgAwIBAgIBATANBgkqhkiG9w0BAQQFADB5MRAwDgYDVQQKEwdSb290\n"
    "IENBMR4wHAYDVQQLExVodHRwOi8vd3d3LmNhY2VydC5vcmcxIjAgBgNVBAMTGUNB\n"
    "IENlcnQgU2lnbmluZyBBdXRob3JpdHkxITAfBgkqhkiG9w0BCQEWEnN1cHBvcnRA\n"
    "Y2FjZXJ0Lm9yZzAeFw0wNTEwMTQwNzM2NTVaFw0zMzAzMjgwNzM2NTVaMFQxFDAS\n"
    "BgNVBAoTC0NBY2VydCBJbmMuMR4wHAYDVQQLExVodHRwOi8vd3d3LkNBY2VydC5v\n"
    "cmcxHDAaBgNVBAMTE0NBY2VydCBDbGFzcyAzIFJvb3QwggIiMA0GCSqGSIb3DQEB\n"
    "AQUAA4ICDwAwggIKAoICAQCrSTURSHzSJn5TlM9Dqd0o10Iqi/OHeBlYfA+e2ol9\n"
    "4fvrcpANdKGWZKufoCSZc9riVXbHF3v1BKxGuMO+f2SNEGwk82GcwPKQ+lHm9WkB\n"
    "Y8MPVuJKQs/iRIwlKKjFeQl9RrmK8+nzNCkIReQcn8uUBByBqBSzmGXEQ+xOgo0J\n"
    "0b2qW42S0OzekMV/CsLj6+YxWl50PpczWejDAz1gM7/30W9HxM3uYoNSbi4ImqTZ\n"
    "FRiRpoWSR7CuSOtttyHshRpocjWr//AQXcD0lKdq1TuSfkyQBX6TwSyLpI5idBVx\n"
    "bgtxA+qvFTia1NIFcm+M+SvrWnIl+TlG43IbPgTDZCciECqKT1inA62+tC4T7V2q\n"
    "SNfVfdQqe1z6RgRQ5MwOQluM7dvyz/yWk+DbETZUYjQ4jwxgmzuXVjit89Jbi6Bb\n"
    "6k6WuHzX1aCGcEDTkSm3ojyt9Yy7zxqSiuQ0e8DYbF/pCsLDpyCaWt8sXVJcukfV\n"
    "m+8kKHA4IC/VfynAskEDaJLM4JzMl0tF7zoQCqtwOpiVcK01seqFK6QcgCExqa5g\n"
    "eoAmSAC4AcCTY1UikTxW56/bOiXzjzFU6iaLgVn5odFTEcV7nQP2dBHgbbEsPyyG\n"
    "kZlxmqZ3izRg0RS0LKydr4wQ05/EavhvE/xzWfdmQnQeiuP43NJvmJzLR5iVQAX7\n"
    "6QIDAQABo4G/MIG8MA8GA1UdEwEB/wQFMAMBAf8wXQYIKwYBBQUHAQEEUTBPMCMG\n"
    "CCsGAQUFBzABhhdodHRwOi8vb2NzcC5DQWNlcnQub3JnLzAoBggrBgEFBQcwAoYc\n"
    "aHR0cDovL3d3dy5DQWNlcnQub3JnL2NhLmNydDBKBgNVHSAEQzBBMD8GCCsGAQQB\n"
    "gZBKMDMwMQYIKwYBBQUHAgEWJWh0dHA6Ly93d3cuQ0FjZXJ0Lm9yZy9pbmRleC5w\n"
    "aHA/aWQ9MTAwDQYJKoZIhvcNAQEEBQADggIBAH8IiKHaGlBJ2on7oQhy84r3HsQ6\n"
    "tHlbIDCxRd7CXdNlafHCXVRUPIVfuXtCkcKZ/RtRm6tGpaEQU55tiKxzbiwzpvD0\n"
    "nuB1wT6IRanhZkP+VlrRekF490DaSjrxC1uluxYG5sLnk7mFTZdPsR44Q4Dvmw2M\n"
    "77inYACHV30eRBzLI++bPJmdr7UpHEV5FpZNJ23xHGzDwlVks7wU4vOkHx4y/CcV\n"
    "Bc/dLq4+gmF78CEQGPZE6lM5+dzQmiDgxrvgu1pPxJnIB721vaLbLmINQjRBvP+L\n"
    "ivVRIqqIMADisNS8vmW61QNXeZvo3MhN+FDtkaVSKKKs+zZYPumUK5FQhxvWXtaM\n"
    "zPcPEAxSTtAWYeXlCmy/F8dyRlecmPVsYGN6b165Ti/Iubm7aoW8mA3t+T6XhDSU\n"
    "rgCvoeXnkm5OvfPi2RSLXNLrAWygF6UtEOucekq9ve7O/e0iQKtwOIj1CodqwqsF\n"
    "YMlIBdpTwd5Ed2qz8zw87YC8pjhKKSRf/lk7myV6VmMAZLldpGJ9VzZPrYPvH5JT\n"
    "oI53V93lYRE9IwCQTDz6o2CTBKOvNfYOao9PSmCnhQVsRqGP9Md246FZV/dxssRu\n"
    "FFxtbUFm3xuTsdQAw+7Lzzw9IYCpX2Nl/N3gX6T0K/CFcUHUZyX7GrGXrtaZghNB\n"
    "0m6lG5kngOcLqagA\n"
    "-----END CERTIFICATE-----\n";

// UpdateNotificationPrivate ===================================================

QString UpdateNotificationPrivate::releaseUrl() const
{
    return QSettings().value(NOTIFY_URL, DEFAULT_RELEASE_URL).toString();
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

QString UpdateNotificationPrivate::uuid() const
{
    QSettings settings;
    if (!settings.contains(NOTIFY_UUID))
        settings.setValue(NOTIFY_UUID, QUuid::createUuid().toString().mid(1, 36));
    return settings.value(NOTIFY_UUID).toString();
}

bool UpdateNotificationPrivate::newerVersion(const QString &oldVersion,
        const QString &newVersion) const
{
    QStringList oldParts = oldVersion.split(QLatin1Char('.'));
    QStringList newParts = newVersion.split(QLatin1Char('.'));
    if (oldParts.value(0).toUInt() < newParts.value(0).toUInt())
        return true;
    if (oldParts.value(0).toUInt() > newParts.value(0).toUInt())
        return false;
    if (oldParts.value(1).toUInt() < newParts.value(1).toUInt())
        return true;
    if (oldParts.value(1).toUInt() > newParts.value(1).toUInt())
        return false;
    oldParts = oldParts.value(2).split(QLatin1Char('+'));
    newParts = newParts.value(2).split(QLatin1Char('+'));
    if (oldParts.value(0).toUInt() < newParts.value(0).toUInt())
        return true;
    if (oldParts.value(0).toUInt() > newParts.value(0).toUInt())
        return false;
    if (oldParts.value(1) < newParts.value(1))
        return true;
    if (oldParts.value(1) > newParts.value(1))
        return false;
    return false;
}

void UpdateNotificationPrivate::requestReleases(const QString &os)
{
    QUrl url = releaseUrl();

    RETURN_IF_FAIL(url.scheme().toLower() == QLatin1String("https") ||
                   url.scheme().toLower() == QLatin1String("http"));

    // SSL certificate validation crashes on Ubuntu 8.04
#if QT_VERSION < 0x040400
    url.setScheme(QLatin1String("http"));
#endif

    url.addQueryItem(QLatin1String("os"), os);
    url.addQueryItem(QLatin1String("version"), QLatin1String(IGOTU_VERSION_STR));
    url.addQueryItem(QLatin1String("uuid"), uuid());

    http->setHost(url.host(),
            url.scheme().toLower() == QLatin1String("https") ?
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
    header.setValue(QLatin1String("Host"), url.host() + QLatin1Char(':') +
            QString::number(url.port
                (url.scheme().toLower() == QLatin1String("https") ? 443 : 80)));
    header.setValue(QLatin1String("User-Agent"), QLatin1String("Igotu2gpx/") +
            QLatin1String(IGOTU_VERSION_STR));
    http->request(header);
}

void UpdateNotificationPrivate::on_http_sslErrors(const QList<QSslError> &errors)
{
    Q_FOREACH (const QSslError &error, errors)
        qCritical("Unable to retrieve update information: %s",
                qPrintable(error.errorString()));
}

void UpdateNotificationPrivate::on_http_done(bool error)
{
    if (error) {
        qWarning("Unable to retrieve update information: %s",
                qPrintable(http->errorString()));
        return;
    }

    QTemporaryFile iniFile;
    RETURN_IF_FAIL(iniFile.open());

    iniFile.write(http->readAll());
    iniFile.flush();
    QSettings settings(iniFile.fileName(), QSettings::IniFormat);

    QString newestVersion = QLatin1String(IGOTU_VERSION_STR);
    Q_FOREACH (const QString group, settings.childGroups()) {
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

    if (url.scheme().toLower() != QLatin1String("https"))
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

    QSslSocket::addDefaultCaCertificate(QSslCertificate(QByteArray(rootCaCert)));
    QSslSocket::addDefaultCaCertificate(QSslCertificate(QByteArray(class3CaCert)));

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
