// BaseDiagram.hxx - part of GUI launcher using Qt5
//
// Written by James Turner, started October 2015.
//
// Copyright (C) 2014 James Turner <zakalawe@mac.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef GUI_BASEDIAGRAM_HXX
#define GUI_BASEDIAGRAM_HXX

#include <QWidget>
#include <QPainterPath>
#include <QHash>

#include <simgear/math/sg_geodesy.hxx>

#include <Navaids/positioned.hxx>
#include <Airports/airport.hxx>
#include <Navaids/PolyLine.hxx>

#include "QtLauncher_fwd.hxx"

class BaseDiagram : public QWidget
{
    Q_OBJECT
public:
    BaseDiagram(QWidget* pr);

    enum IconOption
    {
        NoOptions = 0,
        SmallIcons = 0x1,
        LargeAirportPlans = 0x2
    };

    Q_DECLARE_FLAGS(IconOptions, IconOption)

    static QPixmap iconForPositioned(const FGPositionedRef &pos, const IconOptions& options = NoOptions);
    static QPixmap iconForAirport(FGAirport *apt, const IconOptions& options = NoOptions);

    static QVector<QLineF> projectAirportRuwaysIntoRect(FGAirportRef apt, const QRectF& bounds);
    static QVector<QLineF> projectAirportRuwaysWithCenter(FGAirportRef apt, const SGGeod &c);

    void setAircraftType(LauncherAircraftType type);
protected:
    virtual void paintEvent(QPaintEvent* pe);

    virtual void mousePressEvent(QMouseEvent* me);
    virtual void mouseMoveEvent(QMouseEvent* me);

    virtual void wheelEvent(QWheelEvent* we);

    virtual void paintContents(QPainter*);


protected:
    void recomputeBounds(bool resetZoom);

    virtual void doComputeBounds();

    void extendBounds(const QPointF& p);
    QPointF project(const SGGeod& geod) const;
    QTransform transform() const;
    
    void clearIgnoredNavaids();
    void addIgnoredNavaid(FGPositionedRef pos);

    SGGeod m_projectionCenter;
    double m_scale;
    QRectF m_bounds;
    bool m_autoScalePan;
    QPointF m_panOffset, m_lastMousePos;
    int m_wheelAngleDeltaAccumulator;
    bool m_didPan;
    LauncherAircraftType m_aircraftType;

    static void extendRect(QRectF& r, const QPointF& p);

    static QPointF project(const SGGeod &geod, const SGGeod &center);

    static SGGeod unproject(const QPointF &xy, const SGGeod &center);

    void paintAirplaneIcon(QPainter *painter, const SGGeod &geod, int headingDeg);
private:
    enum LabelPosition
    {
        LABEL_RIGHT = 0,
        LABEL_ABOVE,
        LABEL_BELOW,
        LABEL_LEFT,
        LABEL_NE,
        LABEL_SE,
        LABEL_SW,
        LABEL_NW,
        LAST_POSITION // marker value
    };

    void paintNavaids(QPainter *p);

    bool isNavaidIgnored(const FGPositionedRef& pos) const;

    bool isLabelRectAvailable(const QRect& r) const;
    QRect rectAndFlagsForLabel(PositionedID guid, const QRect &item,
                               const QSize &bounds,
                               int & flags /* out parameter */) const;
    QRect labelPositioned(const QRect &itemRect, const QSize &bounds, LabelPosition lp) const;

    QVector<FGPositionedRef> m_ignored;

    mutable QHash<PositionedID, LabelPosition> m_labelPositions;
    mutable QVector<QRect> m_labelRects;

    static int textFlagsForLabelPosition(LabelPosition pos);

    void splitItems(const FGPositionedList &in, FGPositionedList &navaids, FGPositionedList &ports);
    void paintNavaid(QPainter *painter,
                     const QTransform& t,
                     const FGPositionedRef &pos);
    void paintPolygonData(QPainter *painter);
    void paintGeodVec(QPainter *painter, const flightgear::SGGeodVec &vec);
    void fillClosedGeodVec(QPainter *painter, const QColor &color, const flightgear::SGGeodVec &vec);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(BaseDiagram::IconOptions)

#endif // of GUI_BASEDIAGRAM_HXX
