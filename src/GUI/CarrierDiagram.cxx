// CarrierDiagram.cxx - part of GUI launcher using Qt5
//
// Written by Stuart Buchanan, started April 2020.
//
// Copyright (C) 2022 Stuart Buchanan <stuart13@gmail.com>
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

#include "CarrierDiagram.hxx"

#include <limits>

#include <QPainter>
#include <QDebug>
#include <QVector2D>
#include <QMouseEvent>

#include <Navaids/NavDataCache.hxx>

CarrierDiagram::CarrierDiagram(QQuickItem* pr) :
    BaseDiagram(pr)
{
}

void CarrierDiagram::setGeod(QmlGeod geod)
{
    setGeod(geod.geod());
}

QmlGeod CarrierDiagram::geod() const
{
    return QmlGeod(m_geod);
}

void CarrierDiagram::setGeod(const SGGeod &geod)
{
    m_Carrier.clear();
    m_geod = geod;
    m_projectionCenter = m_geod;
    recomputeBounds(true);
    emit locationChanged();
}

void CarrierDiagram::setOffsetEnabled(bool offset)
{
    if (m_offsetEnabled == offset)
        return;
    m_offsetEnabled = offset;
    recomputeBounds(true);
    emit offsetChanged();
}

void CarrierDiagram::setAbeam(bool abeam)
{
    if (m_abeam == abeam)
        return;
    m_abeam = abeam;
    recomputeBounds(true);
    emit offsetChanged();
}

void CarrierDiagram::setOffsetDistance(QuantityValue distanceNm)
{
    if (distanceNm == m_offsetDistance)
        return;

    m_offsetDistance = distanceNm;
    update();
    emit offsetChanged();
}

void CarrierDiagram::paintContents(QPainter *painter)
{
    QPointF base = project(m_geod);

    SGGeod carrierPos = m_geod;

    SGGeod aircraftPos = m_geod;
    float aircraft_heading = 0;
    if (m_offsetEnabled) {
        // We don't actually know the eventual orientation of the carrier,
        // so we place the aircraft relative to the aircraft carrier icon,
        // which has the angled flight deck at 80 degrees (e.g. just N or E)
        //
        //  There are two case:
        //  - On finals, which will be at -100 degrees, aircraft heading 80
        //  - Abeam on a left hand circuit, so offset 0 degrees,
        //    aircraft heading downwind -90  (note that this is parallel to the
        //    _carrier_, not the angled flightdeck)

        float offset_heading = -100;
        aircraft_heading = 80;

        if (m_abeam) {
          offset_heading = 0;
          aircraft_heading = -90;
        }

        double d = m_offsetDistance.convertToUnit(Units::Kilometers).value * 1000;
        SGGeod offsetGeod = SGGeodesy::direct(m_geod, offset_heading, d);
        QPointF offset = project(offsetGeod);

        QPen pen(Qt::green);
        pen.setCosmetic(true);
        painter->setPen(pen);
        painter->drawLine(base, offset);

        aircraftPos = offsetGeod;
    } else {
      // We're at a parking position or on the catapults, so simply rotate to
      // match the carrier heading - E
      aircraft_heading = 90;
    }

    paintCarrierIcon(painter, carrierPos, 0.0);
    paintAirplaneIcon(painter, aircraftPos, aircraft_heading);
}

void CarrierDiagram::doComputeBounds()
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
        float offset_heading = m_abeam ? 0 : -100; // See above for explanation
        SGGeod offsetPos = SGGeodesy::direct(m_geod, offset_heading, d);
        extendBounds(project(offsetPos));
    }
}
