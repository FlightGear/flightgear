// NavaidDiagram.cxx - part of GUI launcher using Qt5
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

#include "NavaidDiagram.hxx"

#include <limits>

#include <QPainter>
#include <QDebug>
#include <QVector2D>
#include <QMouseEvent>

#include <Navaids/NavDataCache.hxx>

NavaidDiagram::NavaidDiagram(QQuickItem* pr) :
    BaseDiagram(pr)
{
}

void NavaidDiagram::setNavaid(qlonglong nav)
{
    m_navaid = fgpositioned_cast<FGNavRecord>(flightgear::NavDataCache::instance()->loadById(nav));
    m_projectionCenter = m_navaid ? m_navaid->geod() : SGGeod();
    m_geod = m_projectionCenter;
    recomputeBounds(true);
    emit locationChanged();
}

qlonglong NavaidDiagram::navaid() const
{
    return m_navaid->guid();
}

void NavaidDiagram::setGeod(QmlGeod geod)
{
    setGeod(geod.geod());
}

QmlGeod NavaidDiagram::geod() const
{
    return QmlGeod(m_geod);
}

void NavaidDiagram::setGeod(const SGGeod &geod)
{
    m_navaid.clear();
    m_geod = geod;
    m_projectionCenter = m_geod;
    recomputeBounds(true);
    emit locationChanged();
}

void NavaidDiagram::setOffsetEnabled(bool offset)
{
    if (m_offsetEnabled == offset)
        return;
    m_offsetEnabled = offset;
    recomputeBounds(true);
    emit offsetChanged();
}

void NavaidDiagram::setOffsetDistance(QuantityValue distanceNm)
{
    if (distanceNm == m_offsetDistance)
        return;

    m_offsetDistance = distanceNm;
    update();
    emit offsetChanged();
}

void NavaidDiagram::setOffsetBearing(QuantityValue bearing)
{
    m_offsetBearing = bearing;
    update();
    emit offsetChanged();
}

void NavaidDiagram::setHeading(QuantityValue headingDeg)
{
    m_heading = headingDeg;
    update();
    emit offsetChanged();
}

void NavaidDiagram::paintContents(QPainter *painter)
{
    QPointF base = project(m_geod);

    SGGeod aircraftPos = m_geod;
    if (m_offsetEnabled) {

        double d = m_offsetDistance.convertToUnit(Units::Kilometers).value * 1000;
        SGGeod offsetGeod = SGGeodesy::direct(m_geod, m_offsetBearing.value, d);
        QPointF offset = project(offsetGeod);

        QPen pen(Qt::green);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->drawLine(base, offset);

        aircraftPos = offsetGeod;
    }

    paintAirplaneIcon(painter, aircraftPos, m_heading.value);
}

void NavaidDiagram::doComputeBounds()
{
    extendBounds(project(m_geod));

// project four points around the base location at 20nm to give some
// coverage
    for (int i=0; i<4; ++i) {
        SGGeod pt = SGGeodesy::direct(m_geod, i * 90, SG_NM_TO_METER * 20.0);
        extendBounds(project(pt));
    }

    if (m_offsetEnabled) {
        double d = m_offsetDistance.convertToUnit(Units::Kilometers).value * 1000;
        SGGeod offsetPos = SGGeodesy::direct(m_geod, m_offsetBearing.value, d);
        extendBounds(project(offsetPos));
    }
}
