/******************************************************************************
 * Copyright (C) 2009 Michael Hofmann <mh21@piware.de>                        *
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

#include "igotu/exception.h"
#include "igotu/igotupoints.h"

#include "trackvisualizer.h"

#include <QHeaderView>
#include <QLayout>
#include <QTreeWidget>

class ListVisualizer: public TrackVisualizer
{
    Q_OBJECT
public:
    ListVisualizer(QWidget *parent = NULL);

    virtual void setTracks(const igotu::IgotuPoints &points, int utcOffset);
    virtual QString tabTitle() const;
    virtual int priority() const;

private:
    QTreeWidget *tracks;
};

class ListVisualizerCreator :
    public QObject,
    public TrackVisualizerCreator
{
    Q_OBJECT
    Q_INTERFACES(TrackVisualizerCreator)
public:
    virtual QStringList trackVisualizers() const;
    virtual TrackVisualizer *createTrackVisualizer(const QString &name,
            QWidget *parent = NULL) const;
};

Q_EXPORT_PLUGIN2(listVisualizer, ListVisualizerCreator)

using namespace igotu;

static QString formatCoordinates(const IgotuPoint &point)
{
    const double longitude = point.longitude();
    const int longitudeSeconds = qRound(qAbs(longitude * 3600));
    const double latitude = point.latitude();
    const int latitudeSeconds = qRound(qAbs(latitude * 3600));
    return QString::fromUtf8("%1\xc2\xb0%2'%3\"%4, %5\xc2\xb0%6'%7\"%8")
        .arg(longitudeSeconds / 3600)
        .arg((longitudeSeconds / 60) % 60)
        .arg(longitudeSeconds % 60)
        .arg(longitude < 0 ? ListVisualizer::tr("W") : ListVisualizer::tr("E"))
        .arg(latitudeSeconds / 3600)
        .arg((latitudeSeconds / 60) % 60)
        .arg(latitudeSeconds % 60)
        .arg(latitude < 0 ? ListVisualizer::tr("S") : ListVisualizer::tr("N"));

}

// ListVisualizer ==============================================================

ListVisualizer::ListVisualizer(QWidget *parent) :
    TrackVisualizer(parent)
{
    QLayout * const verticalLayout = new QVBoxLayout(this);
    verticalLayout->setMargin(0);

    tracks = new QTreeWidget(this);
    tracks->setObjectName(QLatin1String("tracks"));
    tracks->setAllColumnsShowFocus(true);
    tracks->setRootIsDecorated(false);
    tracks->setColumnCount(3);
    tracks->setHeaderLabels(QStringList()
            << tr("Date")
            << tr("Position")
            << tr("Number of trackpoints"));

    verticalLayout->addWidget(tracks);
}

void ListVisualizer::setTracks(const igotu::IgotuPoints &points, int utcOffset)
{
    QList<QTreeWidgetItem*> items;
    Q_FOREACH (const QList<IgotuPoint> &track, points.tracks()) {
        QString date = track.at(0).humanDateTimeString(utcOffset);
        items.append(new QTreeWidgetItem((QTreeWidget*)NULL, QStringList()
                    << date
                    << formatCoordinates(track.at(0))
                    << tr("%n point(s)", NULL, track.count())));
    }

    tracks->clear();
    tracks->insertTopLevelItems(0, items);
    for (unsigned i = 0; i < 3; ++i)
        tracks->resizeColumnToContents(i);
}

QString ListVisualizer::tabTitle() const
{
    return tr("List");
}

int ListVisualizer::priority() const
{
    return 100;
}

// ListVisualizerCreator =======================================================

QStringList ListVisualizerCreator::trackVisualizers() const
{
    return QStringList() << QLatin1String("list");
}

TrackVisualizer *ListVisualizerCreator::createTrackVisualizer
    (const QString &name, QWidget *parent) const
{
    if (name == QLatin1String("list"))
        return new ListVisualizer(parent);

    return NULL;
}

#include "listvisualizer.moc"
