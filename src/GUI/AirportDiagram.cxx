// AirportDiagram.cxx - part of GUI launcher using Qt5
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

#include "AirportDiagram.hxx"

#include <limits>

#include <simgear/sg_inlines.h>

#include <QPainter>
#include <QDebug>
#include <QVector2D>
#include <QMouseEvent>

#include <Airports/airport.hxx>
#include <Airports/runways.hxx>
#include <Airports/parking.hxx>
#include <Airports/pavement.hxx>

#include <Navaids/navrecord.hxx>

static double distanceToLineSegment(const QVector2D& p, const QVector2D& a,
                                    const QVector2D& b, double* outT = NULL)
{
    QVector2D ab(b - a);
    QVector2D ac(p - a);
    
    // Squared length, to avoid a sqrt
    const qreal len2 = ab.lengthSquared();
    
    // Line null, the projection can't exist, we return the first point
    if (qIsNull(len2)) {
        if (outT) {
            *outT = 0.0;
        }
        return (p - a).length();
    }
    
    // Parametric value of the projection on the line
    const qreal t = (ac.x() * ab.x() + ac.y() * ab.y()) / len2;
    
    if (t < 0.0) {
        // Point is before the first point
        if (outT) {
            *outT = 0.0;
        }
        return (p - a).length();
    } else if (t > 1.0) {
        // Point is after the second point
        if (outT) {
            *outT = 1.0;
        }
        return (p - b).length();
    } else {
        if (outT) {
            *outT = t;
        }
        
        const QVector2D proj = a + t * ab;
        return (proj - p).length();
    }
    
    return 0.0;
}

static double unitLengthAfterMapping(const QTransform& t)
{
    const QPointF tVec = t.map(QPointF(1.0, 0.0)) - t.map(QPointF(0.0, 0.0));
    return QVector2D(tVec).length();
}

AirportDiagram::AirportDiagram(QWidget* pr) :
    BaseDiagram(pr),
    m_approachDistanceNm(-1.0),
    m_helipadIcon(":/heliport-icon")
{
    m_parkingIconPath.moveTo(0,0);
    m_parkingIconPath.lineTo(-16, -16);
    m_parkingIconPath.lineTo(-64, -16);
    m_parkingIconPath.lineTo(-64, 16);
    m_parkingIconPath.lineTo(-16, 16);
    m_parkingIconPath.lineTo(0, 0);

    m_parkingIconLeftPath.moveTo(0,0);
    m_parkingIconLeftPath.lineTo(16, -16);
    m_parkingIconLeftPath.lineTo(64, -16);
    m_parkingIconLeftPath.lineTo(64, 16);
    m_parkingIconLeftPath.lineTo(16, 16);
    m_parkingIconLeftPath.lineTo(0, 0);
}

AirportDiagram::~AirportDiagram()
{

}

void AirportDiagram::setAirport(FGAirportRef apt)
{
    m_airport = apt;
    m_projectionCenter = apt ? apt->geod() : SGGeod();
    m_runways.clear();
    m_approachDistanceNm = -1.0;    m_parking.clear();
    m_helipads.clear();

    if (apt) {
        buildTaxiways();
        buildPavements();
    }

    clearIgnoredNavaids();
    addIgnoredNavaid(apt);

    recomputeBounds(true);
    update();
}

FGRunwayRef AirportDiagram::selectedRunway() const
{
    return m_selectedRunway;
}

void AirportDiagram::setSelectedRunway(FGRunwayRef r)
{
    if (r == m_selectedRunway) {
        return;
    }

    m_selectedParking.clear();
    m_selectedRunway = r;
    update();
}

void AirportDiagram::setSelectedHelipad(FGHelipadRef pad)
{
    qWarning() << Q_FUNC_INFO << "implement me";
}

void AirportDiagram::setSelectedParking(FGParkingRef park)
{
    if (m_selectedParking == park) {
        return;
    }

    m_selectedRunway.clear();
    m_selectedParking = park;
    update();
}

void AirportDiagram::setApproachExtensionDistance(double distanceNm)
{
    if (m_approachDistanceNm == distanceNm) {
        return;
    }

    m_approachDistanceNm = distanceNm;
    recomputeBounds(true);
    update();
}

void AirportDiagram::addRunway(FGRunwayRef rwy)
{
    Q_FOREACH(RunwayData rd, m_runways) {
        if (rd.runway == rwy->reciprocalRunway()) {
            return; // only add one end of reciprocal runways
        }
    }

    RunwayData r;
    r.p1 = project(rwy->geod());
    r.p2 = project(rwy->end());
    r.widthM = qRound(rwy->widthM());
    r.runway = rwy;
    m_runways.append(r);

    recomputeBounds(false);
    update();
}

void AirportDiagram::doComputeBounds()
{
    Q_FOREACH(const RunwayData& r, m_runways) {
        extendBounds(r.p1);
        extendBounds(r.p2);
    }

    Q_FOREACH(const TaxiwayData& t, m_taxiways) {
        extendBounds(t.p1);
        extendBounds(t.p2);
    }

    Q_FOREACH(const ParkingData& p, m_parking) {
        extendBounds(p.pt);
    }

    Q_FOREACH(const HelipadData& p, m_helipads) {
        extendBounds(p.pt);
    }

    if (m_selectedRunway && (m_approachDistanceNm > 0.0)) {
        double d = SG_NM_TO_METER * m_approachDistanceNm;
        QPointF pt = project(m_selectedRunway->pointOnCenterline(-d));
        extendBounds(pt);
    }
}

void AirportDiagram::addParking(FGParkingRef park)
{
    ParkingData pd = { project(park->geod()), park };
    m_parking.push_back(pd);
    recomputeBounds(false);
    update();
}

void AirportDiagram::addHelipad(FGHelipadRef pad)
{
    HelipadData pd = { project(pad->geod()), pad };
    m_helipads.push_back(pd);
    recomputeBounds(false);
    update();
}


void AirportDiagram::paintContents(QPainter* p)
{
    QTransform t = p->transform();

// pavements
    QBrush brush(QColor(0x9f, 0x9f, 0x9f));
    Q_FOREACH(const QPainterPath& path, m_pavements) {
        p->drawPath(path);
    }

// taxiways
    Q_FOREACH(const TaxiwayData& t, m_taxiways) {
        QPen pen(QColor(0x9f, 0x9f, 0x9f));
        pen.setWidth(t.widthM);
        p->setPen(pen);
        p->drawLine(t.p1, t.p2);
    }

    drawHelipads(p);
    drawParkings(p);

// runways
    QFont f;
    f.setPixelSize(14);
    p->setFont(f);

    // draw ILS first so underneath all runways
    QPen pen(QColor(0x5f, 0x5f, 0x5f));
    pen.setWidth(1);
    pen.setCosmetic(true);
    p->setPen(pen);

    Q_FOREACH(const RunwayData& r, m_runways) {
        drawILS(p, r.runway);
        drawILS(p, r.runway->reciprocalRunway());
    }

    bool drawAircraft = false;
    SGGeod aircraftPos;
    int headingDeg;

    // now draw the runways for real
    Q_FOREACH(const RunwayData& r, m_runways) {

        QColor color(Qt::magenta);
        if ((r.runway == m_selectedRunway) || (r.runway->reciprocalRunway() == m_selectedRunway)) {
            color = Qt::yellow;
        }
        
        p->setTransform(t);

        QPen pen(color);
        pen.setWidth(r.widthM);
        p->setPen(pen);
        
        p->drawLine(r.p1, r.p2);

    // draw idents
        QString ident = QString::fromStdString(r.runway->ident());

        p->translate(r.p1);
        p->rotate(r.runway->headingDeg());
        // invert scaling factor so we can use screen pixel sizes here
        p->scale(1.0 / m_scale, 1.0/ m_scale);
        
        p->setPen((r.runway == m_selectedRunway) ? Qt::yellow : Qt::magenta);
        p->drawText(QRect(-100, 5, 200, 200), ident, Qt::AlignHCenter | Qt::AlignTop);

        FGRunway* recip = r.runway->reciprocalRunway();
        QString recipIdent = QString::fromStdString(recip->ident());

        p->setTransform(t);
        p->translate(r.p2);
        p->rotate(recip->headingDeg());
        p->scale(1.0 / m_scale, 1.0/ m_scale);

        p->setPen((r.runway->reciprocalRunway() == m_selectedRunway) ? Qt::yellow : Qt::magenta);
        p->drawText(QRect(-100, 5, 200, 200), recipIdent, Qt::AlignHCenter | Qt::AlignTop);
    }

    if (m_selectedRunway) {
        drawAircraft = true;
        aircraftPos = m_selectedRunway->geod();
        headingDeg = m_selectedRunway->headingDeg();
    }

    if (m_selectedRunway && (m_approachDistanceNm > 0.0)) {
        p->setTransform(t);
        // draw approach extension point
        double d = SG_NM_TO_METER * m_approachDistanceNm;
        QPointF pt = project(m_selectedRunway->pointOnCenterline(-d));
        QPointF pt2 = project(m_selectedRunway->geod());
        QPen pen(Qt::yellow);
        pen.setWidth(2.0 / m_scale);
        p->setPen(pen);
        p->drawLine(pt, pt2);

        aircraftPos = m_selectedRunway->pointOnCenterline(-d);
    }

    if (drawAircraft) {
        p->setTransform(t);
        paintAirplaneIcon(p, aircraftPos, headingDeg);
    }

 #if 0
    p->resetTransform();
    QPen testPen(Qt::cyan);
    testPen.setWidth(1);
    testPen.setCosmetic(true);
    p->setPen(testPen);
    p->setBrush(Qt::NoBrush);

    double minWidth = 8.0 * unitLengthAfterMapping(t.inverted());

    Q_FOREACH(const RunwayData& r, m_runways) {
        QPainterPath pp = pathForRunway(r, t, minWidth);
        p->drawPath(pp);
    } // of runways iteration
#endif
}

void AirportDiagram::drawHelipads(QPainter* painter)
{
    QTransform t = painter->transform();

    QRect r = m_helipadIcon.rect();
    r.moveCenter(QPoint(0, 0));

    Q_FOREACH(const HelipadData& p, m_helipads) {
        painter->setTransform(t);
        painter->translate(p.pt);
        painter->drawPixmap(r, m_helipadIcon);
    }
}

void AirportDiagram::drawParking(QPainter* painter, const ParkingData& p) const
{
    painter->translate(p.pt);

    double hdg = p.parking->getHeading();
    bool useLeftIcon = false;
    QRect labelRect(-62, -14, 40, 28);

    if (hdg > 180.0) {
        hdg += 90;
        useLeftIcon = true;
        labelRect = QRect(22, -14, 40, 28);
    } else {
        hdg -= 90;
    }

    painter->rotate(hdg);

    if (p.parking == m_selectedParking) {
        painter->setBrush(Qt::yellow);
    } else {
        painter->setBrush(QColor(255, 196, 196)); // kind of pink
    }

    painter->drawPath(useLeftIcon ? m_parkingIconLeftPath : m_parkingIconPath);

    painter->fillRect(labelRect, Qt::white);

    // draw text
    painter->setPen(Qt::black);
    painter->drawText(labelRect,
                      Qt::AlignVCenter | Qt::AlignHCenter,
                      QString::fromStdString(p.parking->name()));
}

AirportDiagram::ParkingData AirportDiagram::findParkingData(const FGParkingRef &pk) const
{
    Q_FOREACH(const ParkingData& p, m_parking) {
        if (p.parking == m_selectedParking) {
            return p;
        }
    }

    return ParkingData();
}

void AirportDiagram::drawParkings(QPainter* painter) const
{
    painter->save();
    QTransform t = painter->transform();

    QFont f = painter->font();
    f.setPixelSize(16);
    painter->setFont(f);

    Q_FOREACH(const ParkingData& p, m_parking) {
        if (p.parking == m_selectedParking) {
            continue; // skip and draw last
        }

        painter->setTransform(t);
        drawParking(painter, p);
    }

    if (m_selectedParking) {
        painter->setTransform(t);
        drawParking(painter, findParkingData(m_selectedParking));
    }

    painter->restore();
}

void AirportDiagram::drawILS(QPainter* painter, FGRunwayRef runway) const
{
    if (!runway)
        return;

    FGNavRecord* loc = runway->ILS();
    if (!loc)
        return;

    double halfBeamWidth = loc->localizerWidth() * 0.5;
    QPointF threshold = project(runway->threshold());
    double rangeM = loc->get_range() * SG_NM_TO_METER;
    double radial = loc->get_multiuse();
    SG_NORMALIZE_RANGE(radial, 0.0, 360.0);

// compute the three end points at the wide end of the arrow
    QPointF endCentre = project(SGGeodesy::direct(loc->geod(), radial, -rangeM));
    QPointF endR = project(SGGeodesy::direct(loc->geod(), radial + halfBeamWidth, -rangeM * 1.1));
    QPointF endL = project(SGGeodesy::direct(loc->geod(), radial - halfBeamWidth, -rangeM * 1.1));

    painter->drawLine(threshold, endCentre);
    painter->drawLine(threshold, endL);
    painter->drawLine(threshold, endR);
    painter->drawLine(endL, endCentre);
    painter->drawLine(endR, endCentre);
}

void AirportDiagram::mouseReleaseEvent(QMouseEvent* me)
{
    if (me->button() != Qt::LeftButton) {
        return;
    }

    QTransform t(transform());
    double minWidth = 8.0 * unitLengthAfterMapping(t.inverted());

    Q_FOREACH(const RunwayData& r, m_runways) {
        QPainterPath pp = pathForRunway(r, t, minWidth);
        if (pp.contains(me->pos())) {
            // check which end was clicked
            QPointF p1(t.map(r.p1)), p2(t.map(r.p2));
            double param;
            distanceToLineSegment(QVector2D(me->pos()), QVector2D(p1), QVector2D(p2), &param);
            if (param > 0.5) {
                emit clickedRunway(r.runway->reciprocalRunway());
            } else {
                emit clickedRunway(r.runway);
            }
            return;
        }
    } // of runways iteration

    Q_FOREACH(const ParkingData& parking, m_parking) {
        QPainterPath pp = pathForParking(parking, t);
        if (pp.contains(me->pos())) {
            emit clickedParking(parking.parking);
            return;
        }
    }


    Q_FOREACH(const HelipadData& pad, m_helipads) {
        QPainterPath pp = pathForHelipad(pad, t);
        if (pp.contains(me->pos())) {
            emit clickedHelipad(pad.helipad);
            return;
        }
    }
}

QPainterPath AirportDiagram::pathForRunway(const RunwayData& r, const QTransform& t,
                                           const double minWidth) const
{
    QPainterPath pp;
    double width = qMax(static_cast<double>(r.widthM), minWidth);
    double halfWidth = width * 0.5;
    QVector2D v = QVector2D(r.p2 - r.p1);
    v.normalize();
    QVector2D halfVec = QVector2D(v.y(), -v.x()) * halfWidth;

    pp.moveTo(r.p1 - halfVec.toPointF());
    pp.lineTo(r.p1 + halfVec.toPointF());
    pp.lineTo(r.p2 + halfVec.toPointF());
    pp.lineTo(r.p2 - halfVec.toPointF());
    pp.closeSubpath();

    return t.map(pp);
}

QPainterPath AirportDiagram::pathForParking(const ParkingData& p, const QTransform& t) const
{
    bool useLeftIcon = false;
    double hdg = p.parking->getHeading();

    if (hdg > 180.0) {
        hdg += 90;
        useLeftIcon = true;
    } else {
        hdg -= 90;
    }

    QTransform x = t;
    x.translate(p.pt.x(), p.pt.y());
    x.rotate(hdg);
    return x.map(useLeftIcon ? m_parkingIconLeftPath : m_parkingIconPath);
}

QPainterPath AirportDiagram::pathForHelipad(const HelipadData& h, const QTransform& t) const
{
    QPainterPath pp;
    QRect r = m_helipadIcon.rect();
    r.moveCenter(QPoint(0, 0));
    pp.addEllipse(r);
    return t.map(pp);
}

void AirportDiagram::buildTaxiways()
{
    m_taxiways.clear();
    for (unsigned int tIndex=0; tIndex < m_airport->numTaxiways(); ++tIndex) {
        FGTaxiwayRef tx = m_airport->getTaxiwayByIndex(tIndex);

        TaxiwayData td;
        td.p1 = project(tx->geod());
        td.p2 = project(tx->pointOnCenterline(tx->lengthM()));

        td.widthM = tx->widthM();
        m_taxiways.append(td);
    }
}

void AirportDiagram::buildPavements()
{
    m_pavements.clear();
    for (unsigned int pIndex=0; pIndex < m_airport->numPavements(); ++pIndex) {
        FGPavementRef pave = m_airport->getPavementByIndex(pIndex);
        if (pave->getNodeList().empty()) {
            continue;
        }

        QPainterPath pp;
        QPointF startPoint;
        bool closed = true;
        QPointF p0 = project(pave->getNodeList().front()->mPos);

        FGPavement::NodeList::const_iterator it;
        for (it = pave->getNodeList().begin(); it != pave->getNodeList().end(); ) {
            const FGPavement::BezierNode *bn = dynamic_cast<const FGPavement::BezierNode *>(it->get());
            bool close = (*it)->mClose;

            // increment iterator so we can look at the next point
            ++it;
            QPointF nextPoint = (it == pave->getNodeList().end()) ? startPoint : project((*it)->mPos);

            if (bn) {
                QPointF control = project(bn->mControl);
                QPointF endPoint = close ? startPoint : nextPoint;
                pp.quadTo(control, endPoint);
            } else {
                // straight line segment
                if (closed) {
                    pp.moveTo(p0);
                    closed = false;
                    startPoint = p0;
                } else
                    pp.lineTo(p0);
            }

            if (close) {
                closed = true;
                pp.closeSubpath();
                startPoint = QPointF();
            }

            p0 = nextPoint;
        } // of nodes iteration

        if (!closed) {
            pp.closeSubpath();
        }

        m_pavements.append(pp);
    } // of pavements iteration
}
