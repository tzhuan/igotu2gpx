/******************************************************************************
 * Copyright (C) 2008  Michael Hofmann <mh21@piware.de>                       *
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

#include "paths.h"

#include <QCoreApplication>
#include <QDir>

#include <cstdlib>

// On Unix, 4 file paths are searched in the following order:
// 1. the home direcory(XDG_DATA_HOME/igotu2gpx, .local/lib/igotu2gpx)
// 2. the directory where igotu2gpx is installed (../share/igotu2gpx,
//    ../lib/igotu2gpx)
// 3  or the directory where igotu2gpx is located
// 4. the default directories(XDG_DATA_DIRS, /usr/local/lib:/usr/lib)
//
// On Windows, 4 file paths are searched in the following order:
// 1. the home direcory(APPDATA/igotu2gpx)
// 2. the common home direcory(COMMON_APPDATA/igotu2gpx)
// 3. the directory where igotu2gpx is installed(../share, ../lib)
// 4. or the directory where igotu2gpx is located

namespace igotu
{

#ifdef Q_OS_WIN

#define _WIN32_IE 0x0400
#include <shlobj.h>

// TODO: Qt 4.4 has support to retrieve these folders
static QString windowsConfigPath(int type)
{
    QString result;

    TCHAR path[MAX_PATH];
    SHGetSpecialFolderPathW(0, path, type, FALSE);
    result = QString::fromUtf16 ((ushort*) path);

    if (result.isEmpty()) {
        switch(type) {
        case CSIDL_COMMON_APPDATA:
            result = QLatin1String("C:\\temp\\qt-common");
            break;
        case CSIDL_APPDATA:
            result = QLatin1String("C:\\temp\\qt-user");
            break;
        default:
            ;
        }
    }

    return QDir::fromNativeSeparators(result);
}

#endif // Q_OS_WIN

#ifdef Q_OS_UNIX

static QStringList directoriesFromEnvironment(const char *name,
        const QString &defaultPath, const QString &suffix)
{
    QString path = QFile::decodeName(getenv(name));
    if (path.isEmpty())
        path = defaultPath;
    const QStringList dirs = QDir::fromNativeSeparators(path).split
        (QLatin1Char(':'));
    QStringList result;
    Q_FOREACH (const QString &dir, dirs)
        result.append(dir + suffix);
    return result;
}

#endif // Q_OS_UNIX

// If installed returns the base directory(/usr) plus installedSubDirs.
// If not installed, returns the source root directory plus uninstalledSubDirs.
// If not installed and uninstalledSubDirs is empty, returns applicationDirPath.
static QStringList relativeToBaseDirectory(const QStringList &installedSubDirs,
        const QStringList &uninstalledSubDirs)
{
    QStringList result;
    QDir applicationDir(QCoreApplication::applicationDirPath());
    if (applicationDir.dirName().compare(QLatin1String("bin"),
                Qt::CaseInsensitive) == 0 ||
        applicationDir.dirName().compare(QLatin1String("MacOS"),
                Qt::CaseInsensitive) == 0) {
        if (applicationDir.cdUp())
            Q_FOREACH (const QString &installedSubDir, installedSubDirs)
                result.append(applicationDir.path() + installedSubDir);
    } else {
        if (!uninstalledSubDirs.isEmpty()) {
            if (applicationDir.cdUp() && applicationDir.cdUp())
                Q_FOREACH (const QString &uninstalledSubDir, uninstalledSubDirs)
                    result.append(applicationDir.path() + uninstalledSubDir);
        } else {
            result.append(applicationDir.path());
        }
    }
    return result;
}

static QStringList uniqueDirectoryList(const QStringList &list)
{
    QStringList result;
    Q_FOREACH (const QString &dir, list)
        if (!dir.isEmpty() && !result.contains(dir))
            result.append(dir);
    return result;
}

#define DIRECTORY QLatin1String("/igotu2gpx")

QStringList Paths::iconDirectories()
{
    QStringList result;
#if defined(Q_OS_LINUX)
    result << directoriesFromEnvironment("XDG_DATA_HOME",
            QDir::homePath() + QLatin1String("/.local/share"),
            QLatin1String("/icons/hicolor"));
    result << relativeToBaseDirectory
           (QStringList() << QLatin1String("/share/icons/hicolor"),
            QStringList() << QLatin1String("/data/icons")
                          << QLatin1String("/contrib/tango/icons"));
    result << directoriesFromEnvironment("XDG_DATA_DIRS",
            QLatin1String("/usr/local/share:/usr/share"),
            QLatin1String("/icons/hicolor"));
#elif defined(Q_OS_MACX)
    // TODO: for the MacOS X native case, user config is missing
    result << directoriesFromEnvironment("XDG_DATA_HOME",
            QDir::homePath() + QLatin1String("/.local/share"),
            QLatin1String("/icons/hicolor"));
    result << relativeToBaseDirectory
           (QStringList() << QLatin1String("/share/icons/hicolor")
                          << QLatin1String("/Resources/icons"),
            QStringList() << QLatin1String("/data/icons")
                          << QLatin1String("/contrib/tango/icons"));
    result << directoriesFromEnvironment("XDG_DATA_DIRS",
            QLatin1String("/usr/local/share:/usr/share"),
            QLatin1String("/icons/hicolor"));
#elif defined(Q_OS_WIN)
    result <<  windowsConfigPath(CSIDL_APPDATA) + DIRECTORY +
            QLatin1String("/icons");
    result << windowsConfigPath(CSIDL_COMMON_APPDATA) + DIRECTORY +
            QLatin1String("/icons");
    result << relativeToBaseDirectory
           (QStringList() << QLatin1String("/icons"),
            QStringList() << QLatin1String("/data/icons")
                          << QLatin1String("/contrib/tango/icons"));
#else
#error No idea where to find icon directories on this platform
#endif
    return uniqueDirectoryList(result);
}

QStringList Paths::translationDirectories()
{
    QStringList result;
#if defined(Q_OS_LINUX)
    result << directoriesFromEnvironment("XDG_DATA_HOME",
            QDir::homePath() + QLatin1String("/.local/share"),
            QLatin1String("/locale"));
    result << relativeToBaseDirectory
           (QStringList() << QLatin1String("/share/locale"),
            QStringList() << QLatin1String("/translations"));
    result << directoriesFromEnvironment("XDG_DATA_DIRS",
            QLatin1String("/usr/local/share:/usr/share"),
            QLatin1String("/locale"));
#elif defined(Q_OS_MACX)
    // TODO: for the MacOS X native case, user config is missing
    result << directoriesFromEnvironment("XDG_DATA_HOME",
            QDir::homePath() + QLatin1String("/.local/share"),
            QLatin1String("/locale"));
    result << relativeToBaseDirectory
           (QStringList() << QLatin1String("/share/locale")
                          << QLatin1String("/Resources/locale"),
            QStringList() << QLatin1String("/translations"));
    result << directoriesFromEnvironment("XDG_DATA_DIRS",
            QLatin1String("/usr/local/share:/usr/share"),
            QLatin1String("/locale"));
#elif defined(Q_OS_WIN)
    result <<  windowsConfigPath(CSIDL_APPDATA) + DIRECTORY +
            QLatin1String("/locale");
    result << windowsConfigPath(CSIDL_COMMON_APPDATA) + DIRECTORY +
            QLatin1String("/locale");
    result << relativeToBaseDirectory
           (QStringList() << QLatin1String("/locale"),
            QStringList() << QLatin1String("/translations"));
#else
#error No idea where to find translation directories on this platform
#endif
    return uniqueDirectoryList(result);
}

QString Paths::mainPluginDirectory()
{
    QString result;
#if defined(Q_OS_MACX)
    result = relativeToBaseDirectory(QStringList(QLatin1String("/PlugIns")),
            QStringList()).value(0);
#elif defined(Q_OS_WIN32)
    result = relativeToBaseDirectory(QStringList(QLatin1String("/bin")),
            QStringList()).value(0);
#endif
    return result;
}

QString Paths::mainDataDirectory()
{
    QString result;
#if defined(Q_OS_MACX)
    result = relativeToBaseDirectory(QStringList(QLatin1String("/Resources")),
            QStringList()).value(0);
#elif defined(Q_OS_WIN32)
    result = relativeToBaseDirectory(QStringList(QLatin1String("/share")),
            QStringList()).value(0);
#endif
    return result;
}

QStringList Paths::pluginDirectories()
{
    QStringList result;
#if defined(Q_OS_LINUX)
    result << QDir::homePath() + QLatin1String("/.local/lib") + DIRECTORY;
    result << relativeToBaseDirectory
           (QStringList() << QLatin1String("/lib") + DIRECTORY,
            QStringList());
    result << QLatin1String("/usr/local/lib") + DIRECTORY;
    result << QLatin1String("/usr/lib") + DIRECTORY;
#elif defined(Q_OS_MACX)
    // TODO: for the MacOS X native case, user config is missing
    result << QDir::homePath() + QLatin1String("/.local/lib") + DIRECTORY;
    result << relativeToBaseDirectory
           (QStringList() << QLatin1String("/lib") + DIRECTORY
                          << QLatin1String("/PlugIns"),
            QStringList());
    result << QLatin1String("/usr/local/lib") + DIRECTORY;
    result << QLatin1String("/usr/lib") + DIRECTORY;
#elif defined(Q_OS_WIN)
    result << windowsConfigPath(CSIDL_APPDATA) + DIRECTORY +
            QLatin1String("/lib");
    result << windowsConfigPath(CSIDL_COMMON_APPDATA) + DIRECTORY +
            QLatin1String("/lib");
    result << relativeToBaseDirectory
           (QStringList() << QLatin1String("/lib"),
            QStringList());
#else
#error No idea where to find plugin directories on this platform
#endif
    return uniqueDirectoryList(result);
}

} // namespace igotu
