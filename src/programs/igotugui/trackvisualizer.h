/******************************************************************************
 * Copyright (C) 2009 Michael Hofmann <mh21@mh21.de>                          *
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

#ifndef _IGOTU2GPX_SRC_IGOTUGUI_TRACKVISUALIZER_H_
#define _IGOTU2GPX_SRC_IGOTUGUI_TRACKVISUALIZER_H_

#include <QWidget>

#include <QtPlugin>

namespace igotu
{
    class IgotuPoint;
    class IgotuPoints;
};

class TrackVisualizer : public QWidget
{
public:
    TrackVisualizer(QWidget *parent = NULL) :
        QWidget(parent)
    {
    }

    enum Flag {
        HasTrackSelection    = 0x01,
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    virtual Flags flags() const = 0;

    virtual void setTracks(const igotu::IgotuPoints &points, int utcOffset) = 0;
    virtual QString tabTitle() const = 0;
    virtual void highlightTrack(const QList<igotu::IgotuPoint> &track) = 0;
    virtual void saveSelectedTracks() = 0;

    // Implementations also need:
    // Q_SIGNALS:
    // void saveTracksRequested(const QList<QList<igotu::IgotuPoint> > &tracks);
    // void trackActivated(const QList<igotu::IgotuPoint> &track);
    // void trackSelectionChanged(bool selected);
};

class TrackVisualizerCreator
{
public:
    virtual ~TrackVisualizerCreator()
    {
    }

    enum AppearanceMode {
        DockWidgetAppearance = 0x01,
        MainWindowAppearance = 0x02,
    };
    Q_DECLARE_FLAGS(AppearanceModes, AppearanceMode)
    virtual AppearanceModes supportedVisualizerAppearances() const = 0;

    virtual QString trackVisualizer() const = 0;
    // lower is better
    virtual int visualizerPriority() const = 0;

    virtual TrackVisualizer *createTrackVisualizer(AppearanceMode mode,
            QWidget *parent = NULL) const = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TrackVisualizerCreator::AppearanceModes)
Q_DECLARE_OPERATORS_FOR_FLAGS(TrackVisualizer::Flags)

Q_DECLARE_INTERFACE(TrackVisualizerCreator,
        "de.mh21.igotu2gpx.trackvisualizer/1.0")

#endif
