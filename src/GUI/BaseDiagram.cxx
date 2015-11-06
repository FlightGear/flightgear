// BaseDiagram.cxx - part of GUI launcher using Qt5
//
// Written by James Turner, started December 2014.
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

#include "BaseDiagram.hxx"

#include <limits>

#include <QPainter>
#include <QDebug>
#include <QVector2D>
#include <QMouseEvent>

/* equatorial and polar earth radius */
const float rec  = 6378137;          // earth radius, equator (?)
const float rpol = 6356752.314f;      // earth radius, polar   (?)

//Returns Earth radius at a given latitude (Ellipsoide equation with two equal axis)
static float earth_radius_lat( float lat )
{
    double a = cos(lat)/rec;
    double b = sin(lat)/rpol;
    return 1.0f / sqrt( a * a + b * b );
}

BaseDiagram::BaseDiagram(QWidget* pr) :
    QWidget(pr),
    m_autoScalePan(true),
    m_wheelAngleDeltaAccumulator(0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding,
                  QSizePolicy::MinimumExpanding);
    setMinimumSize(100, 100);
}

QTransform BaseDiagram::transform() const
{
    QTransform t;
    t.translate(width() / 2, height() / 2); // center projection origin in the widget
    t.scale(m_scale, m_scale);

    // apply any pan offset that exists
    t.translate(m_panOffset.x(), m_panOffset.y());
    // center the bounding box (may not be at the origin)
    t.translate(-m_bounds.center().x(), -m_bounds.center().y());
    return t;
}

void BaseDiagram::paintEvent(QPaintEvent* pe)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0x3f, 0x3f, 0x3f));

    if (m_autoScalePan) {
        // fit bounds within our available space, allowing for a margin
        const int MARGIN = 32; // pixels
        double ratioInX = (width() - MARGIN * 2) / m_bounds.width();
        double ratioInY = (height() - MARGIN * 2) / m_bounds.height();
        m_scale = std::min(ratioInX, ratioInY);
    }

    QTransform t(transform());
    p.setTransform(t);

    paintContents(&p);
}

void BaseDiagram::mousePressEvent(QMouseEvent *me)
{
    m_lastMousePos = me->pos();
    m_didPan = false;
}

void BaseDiagram::mouseMoveEvent(QMouseEvent *me)
{
    m_autoScalePan = false;

    QPointF delta = me->pos() - m_lastMousePos;
    m_lastMousePos = me->pos();

    // offset is stored in metres so we don't have to modify it when
    // zooming
    m_panOffset += (delta / m_scale);
    m_didPan = true;

    update();
}

int intSign(int v)
{
    return (v == 0) ? 0 : (v < 0) ? -1 : 1;
}

void BaseDiagram::wheelEvent(QWheelEvent *we)
{
    m_autoScalePan = false;

    int delta = we->angleDelta().y();
    if (delta == 0)
        return;

    if (intSign(m_wheelAngleDeltaAccumulator) != intSign(delta)) {
        m_wheelAngleDeltaAccumulator = 0;
    }

    m_wheelAngleDeltaAccumulator += delta;
    if (m_wheelAngleDeltaAccumulator > 120) {
        m_wheelAngleDeltaAccumulator = 0;
        m_scale *= 2.0;
    } else if (m_wheelAngleDeltaAccumulator < -120) {
        m_wheelAngleDeltaAccumulator = 0;
        m_scale *= 0.5;
    }

    update();
}

void BaseDiagram::paintContents(QPainter*)
{

}

void BaseDiagram::recomputeBounds(bool resetZoom)
{
    m_bounds = QRectF();
    doComputeBounds();

    if (resetZoom) {
        m_autoScalePan = true;
        m_scale = 1.0;
        m_panOffset = QPointF();
    }

    update();
}

void BaseDiagram::doComputeBounds()
{
    // no-op in the base class
}

void BaseDiagram::extendBounds(const QPointF& p)
{
    if (p.x() < m_bounds.left()) {
        m_bounds.setLeft(p.x());
    } else if (p.x() > m_bounds.right()) {
        m_bounds.setRight(p.x());
    }

    if (p.y() < m_bounds.top()) {
        m_bounds.setTop(p.y());
    } else if (p.y() > m_bounds.bottom()) {
        m_bounds.setBottom(p.y());
    }
}

QPointF BaseDiagram::project(const SGGeod& geod) const
{
    double r = earth_radius_lat(geod.getLatitudeRad());
    double ref_lat = m_projectionCenter.getLatitudeRad(),
    ref_lon = m_projectionCenter.getLongitudeRad(),
    lat = geod.getLatitudeRad(),
    lon = geod.getLongitudeRad(),
    lonDiff = lon - ref_lon;

    double c = acos( sin(ref_lat) * sin(lat) + cos(ref_lat) * cos(lat) * cos(lonDiff) );
    if (c == 0.0) {
        // angular distance from center is 0
        return QPointF(0.0, 0.0);
    }

    double k = c / sin(c);
    double x, y;
    if (ref_lat == (90 * SG_DEGREES_TO_RADIANS))
    {
        x = (SGD_PI / 2 - lat) * sin(lonDiff);
        y = -(SGD_PI / 2 - lat) * cos(lonDiff);
    }
    else if (ref_lat == -(90 * SG_DEGREES_TO_RADIANS))
    {
        x = (SGD_PI / 2 + lat) * sin(lonDiff);
        y = (SGD_PI / 2 + lat) * cos(lonDiff);
    }
    else
    {
        x = k * cos(lat) * sin(lonDiff);
        y = k * ( cos(ref_lat) * sin(lat) - sin(ref_lat) * cos(lat) * cos(lonDiff) );
    }

    return QPointF(x, -y) * r;
}
