/******************************************************************************
 * Copyright (C) 2007  Michael Hofmann <mh21@mh21.de>                         *
 * Portions Copyright (C) 1992-2007 Trolltech ASA.                            *
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

#include "commonmessages.h"
#include "utils.h"

#include <QMetaProperty>
#include <QTextStream>

#include <cmath>

namespace igotu
{

// Put translations in the right context
//
// TRANSLATOR igotu::Common

QString dump(const QByteArray &data)
{
    QString result;
    result += Common::tr("Memory contents:") + QLatin1Char('\n');
    const unsigned chunks = (data.size() + 15) / 16;
    bool firstLine = true;
    for (unsigned i = 0; i < chunks; ++i) {
        const QByteArray chunk = data.mid(i * 16, 16);
        if (firstLine)
            firstLine = false;
        else
            result += QLatin1Char('\n');
        result += QString().sprintf("%04x  ", i * 16);
        for (unsigned j = 0; j < unsigned(chunk.size()); ++j) {
            result += QString().sprintf("%02x ", uchar(chunk[j]));
            if (j % 4 == 3)
                result += QLatin1Char(' ');
        }
    }
    return result;
}

QString dumpDiff(const QByteArray &oldData, const QByteArray &newData)
{
    QString result;
    result += Common::tr("Differences:") + QLatin1Char('\n');
    const unsigned chunks = (oldData.size() + 15) / 16;
    bool firstLine = true;
    for (unsigned i = 0; i < chunks; ++i) {
        const QByteArray oldChunk = oldData.mid(i * 16, 16);
        const QByteArray newChunk = newData.mid(i * 16, 16);
        if (oldChunk == newChunk)
            continue;
        if (firstLine)
            firstLine = false;
        else
            result += QLatin1Char('\n');
        result += QString().sprintf("%04x - ", i * 16);
        for (unsigned j = 0; j < unsigned(qMin(oldChunk.size(),
                        newChunk.size())); ++j) {
            if (oldChunk[j] == newChunk[j])
                result += QLatin1String("__ ");
            else
                result += QString().sprintf("%02x ", uchar(oldChunk[j]));
            if (j % 4 == 3)
                result += QLatin1Char(' ');
        }
        result += QString().sprintf("\n%04x + ", i * 16);
        for (unsigned j = 0; j < unsigned(qMin(oldChunk.size(),
                        newChunk.size())); ++j) {
            if (oldChunk[j] == newChunk[j])
                result += QLatin1String("__ ");
            else
                result += QString().sprintf("%02x ", uchar(newChunk[j]));
            if (j % 4 == 3)
                result += QLatin1Char(' ');
        }
    }
    return result;
}

// copied and modified from qobject.cpp
void connectSlotsByNameToPrivate(QObject *publicObject, QObject *privateObject)
{
    if (!publicObject)
        return;
    const QMetaObject *mo = privateObject->metaObject();
    RETURN_IF_FAIL(mo);
    const QObjectList list = qFindChildren<QObject*>(publicObject, QString());
    for (int i = 0; i < mo->methodCount(); ++i) {
        const char *slot = mo->method(i).signature();
        RETURN_IF_FAIL(slot);
        if (slot[0] != 'o' || slot[1] != 'n' || slot[2] != '_')
            continue;
        bool foundIt = false;
        for(int j = 0; j < list.count(); ++j) {
            const QObject *co = list.at(j);
            QByteArray objName = co->objectName().toAscii();
            int len = objName.length();
            if (!len || qstrncmp(slot + 3, objName.data(), len) ||
                    slot[len+3] != '_')
                continue;
            const QMetaObject *smo = co->metaObject();
            int sigIndex = smo->indexOfMethod(slot + len + 4);
            if (sigIndex < 0) { // search for compatible signals
                int slotlen = qstrlen(slot + len + 4) - 1;
                for (int k = 0; k < co->metaObject()->methodCount(); ++k) {
                    if (smo->method(k).methodType() != QMetaMethod::Signal)
                        continue;

                    if (!qstrncmp(smo->method(k).signature(), slot + len + 4,
                                slotlen)) {
                        sigIndex = k;
                        break;
                    }
                }
            }
            if (sigIndex < 0)
                continue;
            if (QMetaObject::connect(co, sigIndex, privateObject, i)) {
                foundIt = true;
                break;
            }
        }
        if (foundIt) {
            // we found our slot, now skip all overloads
            while (mo->method(i + 1).attributes() & QMetaMethod::Cloned)
                  ++i;
        } else if (!(mo->method(i).attributes() & QMetaMethod::Cloned)) {
            qCritical("connectSlotsByName: No matching signal for %s", slot);
        }
    }
}

void connectWorker(QObject *workerObject, QObject *publicObject, QObject *privateObject)
{
    if (!workerObject)
        return;
    const QMetaObject *mo = workerObject->metaObject();
    const QMetaObject *pubMo = publicObject->metaObject();
    const QMetaObject *privMo = privateObject->metaObject();
    RETURN_IF_FAIL(mo);
    for (unsigned i = mo->methodOffset(); i < unsigned(mo->methodCount()); ++i) {
        const QMetaMethod m = mo->method(i);
        const QMetaMethod::MethodType t = m.methodType();
        const char * const signature = m.signature();
        if (t == QMetaMethod::Signal) {
            int index = pubMo->indexOfMethod(signature);
            if (index < 0 ||
                pubMo->method(index).methodType() != QMetaMethod::Signal) {
                qCritical("Unable to find public signal for worker signal %s",
                        signature);
                continue;
            }
            if (!QMetaObject::connect(workerObject, i, publicObject, index)) {
                qCritical("Unable to connect public signal for worker signal %s",
                        signature);
            }
        } else if (t == QMetaMethod::Slot) {
            int index = privMo->indexOfMethod(signature);
            if (index < 0 ||
                privMo->method(index).methodType() != QMetaMethod::Signal) {
                qCritical("Unable to find private signal for worker slot %s",
                        signature);
                continue;
            }
            if (!QMetaObject::connect(privateObject, index, workerObject, i)) {
                qCritical("Unable to connect private signal for worker slot %s",
                        signature);
            }
        }
    }
}

int enumKeyToValue(const QMetaObject &metaObject,
        const char *type, const char *key)
{
    const int metaEnumIndex = metaObject.indexOfEnumerator(type);
    if (metaEnumIndex < 0)
        return 0;
    const QMetaEnum metaEnum = metaObject.enumerator(metaEnumIndex);
    const int enumValue = metaEnum.keyToValue(key);
    if (enumValue < 0)
        return 0;
    return enumValue;
}

const char *enumValueToKey(const QMetaObject &metaObject,
        const char *type, int value)
{
    const int metaEnumIndex = metaObject.indexOfEnumerator(type);
    if (metaEnumIndex < 0)
        return "";
    const QMetaEnum metaEnum = metaObject.enumerator(metaEnumIndex);
    const char * const key = metaEnum.valueToKey(value);
    if (!key)
        return metaEnum.key(0);
    return key;
}

// QColor/QRgb is in QtGui
static unsigned ahsv(double hue, double s, double v, double a)
{
    double r, g, b;

    if (s == 0) {
        // achromatic (grey)
        r = g = b = v;
    } else {
        hue *= 6;
        const unsigned sector = unsigned(floor(hue));
        const double f = hue - sector;
        const double p = v * (1 - s);
        if (sector & 1) {
            const double q = v * (1 - s * f);
            switch (sector) {
            case 1:
                r = q;
                g = v;
                b = p;
                break;
            case 3:
                r = p;
                g = q;
                b = v;
                break;
            default: // 5
                r = v;
                g = p;
                b = q;
                break;
            }
        } else {
            const double t = v * (1 - s * (1 - f));
            switch (sector) {
            case 0:
                r = v;
                g = t;
                b = p;
                break;
            case 2:
                r = p;
                g = v;
                b = t;
                break;
            default: // 4
                r = t;
                g = p;
                b = v;
                break;
            }
        }
    }
    return (qRound(a * 0xff) << 24) +
           (qRound(r * 0xff) << 16) +
           (qRound(g * 0xff) << 8) +
            qRound(b * 0xff);
}

static QByteArray kmlColor(unsigned color)
{
    const unsigned kmlColor =
        (color & 0xff000000) +
       ((color & 0x00ff0000) >> 16) +
        (color & 0x0000ff00) +
       ((color & 0x000000ff) << 16);
    return QByteArray::number(kmlColor, 16).rightJustified(8, '0');
}

unsigned colorTableEntry(unsigned index)
{
    const static unsigned hueBases[4] = { 16, 18, 17, 19 };
    double hue = ((hueBases[(index / 6) % 4] + (index % 6) * 4) % 24) / 24.0;
    return ahsv(hue, 1.0, 1.0, .65);
}

QByteArray pointsToKml(const QList<QList<IgotuPoint> > &tracks, bool tracksAsSegments)
{
    QByteArray result;
    QTextStream out(&result);
    out.setCodec("UTF-8");
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
           "<Document>\n";

    const unsigned styleCount = tracksAsSegments ? 1 : tracks.count();
    for (unsigned i = 0; i < styleCount; ++i) {
        out << "<Style id=\"line" << i + 1 << "\">\n"
               "    <LineStyle>\n"
               "    <color>" << kmlColor(colorTableEntry(i)) << "</color>\n"
               "    <width>5</width>\n"
               "    </LineStyle>\n"
               "</Style>\n";
    }

    unsigned counter = 1;
//    out << "<Folder>\n"; Bug in Marble? Points are not displayed if placed in folders
    Q_FOREACH (const QList<IgotuPoint> &track, tracks) {
        Q_FOREACH (const IgotuPoint &point, track) {
            if (!point.isWayPoint())
                continue;
            out << "<Placemark>\n"
                "<name>"
                << Common::tr("Waypoint %1").arg(counter)
                << "</name>\n"
                "<Point>\n"
                "<coordinates>\n";
            out << point.longitude() << ',' << point.latitude() << '\n';
            out << "</coordinates>\n"
                "</Point>\n"
                "</Placemark>\n";
            ++counter;
        }
    }
//    out << "</Folder>\n";

    counter = 1;
//    out << "<Folder>\n";
    Q_FOREACH (const QList<IgotuPoint> &track, tracks) {
        if (track.isEmpty())
            continue;
        out << "<Placemark>\n"
               "<name>" << Common::tr("Track %1").arg(counter) << "</name>\n"
               "<Point>\n"
               "<coordinates>\n";
        out << track.at(0).longitude() << ',' << track.at(0).latitude() << '\n';
        out << "</coordinates>\n"
               "</Point>\n"
               "</Placemark>\n";
        if (tracksAsSegments)
            break;
        ++counter;
    }
//    out << "</Folder>\n";

    counter = 1;
    out << "<Folder>\n";
    if (tracksAsSegments)
        out << "<Placemark>\n"
               "<name>" << Common::tr("Track %1").arg(counter) << "</name>\n"
               "<styleUrl>#line" << counter << "</styleUrl>\n"
               "<MultiGeometry>\n";

    Q_FOREACH (const QList<IgotuPoint> &track, tracks) {
        if (track.isEmpty())
            continue;
        if (!tracksAsSegments)
            out << "<Placemark>\n"
                   "<name>" << Common::tr("Track %1").arg(counter) << "</name>\n"
                   "<styleUrl>#line" << counter << "</styleUrl>\n";

        out << "<LineString>\n"
               "<tessellate>1</tessellate>\n"
               "<coordinates>\n";
        Q_FOREACH (const IgotuPoint &point, track)
            out << point.longitude() << ',' << point.latitude() << '\n';
        out << "</coordinates>\n"
               "</LineString>\n";

        if (!tracksAsSegments)
            out << "</Placemark>\n";

        ++counter;
    }

    if (tracksAsSegments)
        out << "</MultiGeometry>\n"
               "</Placemark>\n";
    out << "</Folder>\n";

    out << "</Document>\n"
           "</kml>\n";

    out.flush();

    return result;
}

} // namespace igotu
