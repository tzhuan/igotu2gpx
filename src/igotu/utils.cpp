/******************************************************************************
 * Copyright (C) 2007  Michael Hofmann <mh21@piware.de>                       *
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

namespace igotu
{

// Put translations in the right context
//
// TRANSLATOR igotu::Common

// copied and modified from qobject.cpp
void connectSlotsByNameToPrivate(QObject *publicObject, QObject *privateObject)
{
    if (!publicObject)
        return;
    const QMetaObject *mo = privateObject->metaObject();
    Q_ASSERT(mo);
    const QObjectList list = qFindChildren<QObject*>(publicObject, QString());
    for (int i = 0; i < mo->methodCount(); ++i) {
        const char *slot = mo->method(i).signature();
        Q_ASSERT(slot);
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
            qWarning("connectSlotsByName: No matching signal for %s", slot);
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

QByteArray pointsToKml(const QList<QList<IgotuPoint> > &tracks, bool tracksAsSegments)
{
    QByteArray result;
    QTextStream out(&result);
    out.setCodec("UTF-8");
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
           "<Document>\n"
           "<Style id=\"line\">\n"
           "    <LineStyle>\n"
           "    <color>73FF0000</color>\n"
           "    <width>5</width>\n"
           "    </LineStyle>\n"
           "</Style>\n";

    unsigned counter = 1;
//    out << "<Folder>\n"; Bug in Marble? Points are not displayed if placed in folders
    Q_FOREACH (const QList<IgotuPoint> &track, tracks) {
        Q_FOREACH (const IgotuPoint &point, track) {
            if (!point.isWayPoint())
                continue;
            out << "<Placemark>\n"
                "<name>"
                << Common::tr("Waypoint %1").arg(counter++)
                << "</name>\n"
                "<Point>\n"
                "<coordinates>\n";
            out << point.longitude() << ',' << point.latitude() << '\n';
            out << "</coordinates>\n"
                "</Point>\n"
                "</Placemark>\n";
        }
    }
//    out << "</Folder>\n";

    counter = 1;
//    out << "<Folder>\n";
    Q_FOREACH (const QList<IgotuPoint> &track, tracks) {
        if (track.isEmpty())
            continue;
        out << "<Placemark>\n"
               "<name>" << Common::tr("Track %1").arg(counter++) << "</name>\n"
               "<Point>\n"
               "<coordinates>\n";
        out << track.at(0).longitude() << ',' << track.at(0).latitude() << '\n';
        out << "</coordinates>\n"
               "</Point>\n"
               "</Placemark>\n";
        if (tracksAsSegments)
            break;
    }
//    out << "</Folder>\n";

    counter = 1;
    out << "<Folder>\n";
    if (tracksAsSegments)
        out << "<Placemark>\n"
               "<name>" << Common::tr("Track %1").arg(counter++) << "</name>\n"
               "<styleUrl>#line</styleUrl>\n"
               "<MultiGeometry>\n";

    Q_FOREACH (const QList<IgotuPoint> &track, tracks) {
        if (track.isEmpty())
            continue;
        if (!tracksAsSegments)
            out << "<Placemark>\n"
                   "<name>" << Common::tr("Track %1").arg(counter++) << "</name>\n"
                   "<styleUrl>#line</styleUrl>\n";

        out << "<LineString>\n"
               "<tessellate>1</tessellate>\n"
               "<coordinates>\n";
        Q_FOREACH (const IgotuPoint &point, track)
            out << point.longitude() << ',' << point.latitude() << '\n';
        out << "</coordinates>\n"
               "</LineString>\n";

        if (!tracksAsSegments)
            out << "</Placemark>\n";
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
