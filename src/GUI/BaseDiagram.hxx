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

#include <simgear/math/sg_geodesy.hxx>

#include <Navaids/positioned.hxx>
#include <Airports/airport.hxx>

class BaseDiagram : public QWidget
{
    Q_OBJECT
public:
    BaseDiagram(QWidget* pr);

    static QPixmap iconForPositioned(const FGPositionedRef &pos, bool small);
    static QPixmap iconForAirport(FGAirport *apt);

    static QVector<QLineF> projectAirportRuwaysIntoRect(FGAirportRef apt, const QRectF& bounds);
    static QVector<QLineF> projectAirportRuwaysWithCenter(FGAirportRef apt, const SGGeod &c);
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

    static void extendRect(QRectF& r, const QPointF& p);

    static QPointF project(const SGGeod &geod, const SGGeod &center);

    static SGGeod unproject(const QPointF &xy, const SGGeod &center);

    void paintAirplaneIcon(QPainter *painter, const SGGeod &geod, int headingDeg);
private:
    void paintNavaids(QPainter *p);

    bool isNavaidIgnored(const FGPositionedRef& pos) const;

    QVector<FGPositionedRef> m_ignored;
};

#endif // of GUI_BASEDIAGRAM_HXX
