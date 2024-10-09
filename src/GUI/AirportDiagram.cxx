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


#include <simgear/sg_inlines.h>

#include <QPainter>
#include <QDebug>
#include <QVector2D>
#include <QMouseEvent>

#include <Airports/airport.hxx>
#include <Airports/runways.hxx>
#include <Airports/parking.hxx>
#include <Airports/pavement.hxx>
#include <Airports/groundnetwork.hxx>

#include <Navaids/navrecord.hxx>
#include <Navaids/NavDataCache.hxx>

#include "QmlPositioned.hxx"

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
}

static double unitLengthAfterMapping(const QTransform& t)
{
    const QPointF tVec = t.map(QPointF(1.0, 0.0)) - t.map(QPointF(0.0, 0.0));
    return QVector2D(tVec).length();
}

AirportDiagram::AirportDiagram(QQuickItem* pr) :
    BaseDiagram(pr)
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

    m_helipadBoundsPath.moveTo(0, 0);
    m_helipadBoundsPath.addEllipse(QPointF(0, 0), 16.0, 16.0);

    m_helipadIconPath.moveTo(0,0);
    m_helipadIconPath.addEllipse(QPointF(0, 0), 16.0, 16.0);
    m_helipadIconPath.addEllipse(QPointF(0, 0), 13.0, 13.0);

    QFont f;
    f.setPixelSize(24.0);
    f.setBold(true);
    QFontMetrics metrics(f);
    qreal xOffset = metrics.horizontalAdvance("H") * 0.5;
    qreal yOffset = metrics.capHeight() * 0.5;
    m_helipadIconPath.addText(-xOffset, yOffset, f, "H");
}

AirportDiagram::~AirportDiagram()
{

}

void AirportDiagram::setAirport(FGAirportRef apt)
{
    m_airport = apt;
    m_projectionCenter = apt ? apt->geod() : SGGeod();
    m_runways.clear();
    m_parking.clear();
    m_helipads.clear();

    if (apt) {
        if (apt->type() == FGPositioned::HELIPORT) {
            for (unsigned int r=0; r<apt->numHelipads(); ++r) {
                FGHelipadRef pad = apt->getHelipadByIndex(r);
                // add pad with index as data role
                addHelipad(pad);
            }
        } else {
            for (unsigned int r = 0; r < apt->numHelipads(); ++r) {
                FGHelipadRef pad = apt->getHelipadByIndex(r);
                addHelipad(pad);
            }

            for (unsigned int r=0; r<apt->numRunways(); ++r) {
                addRunway(apt->getRunwayByIndex(r));
            }
        }

       FGGroundNetwork* ground = apt->groundNetwork();
       if (ground && ground->exists()) {
           for (auto park : ground->allParkings()) {
               addParking(park);
           }
       } // of was able to get ground-network

        buildTaxiways();
        buildPavements();
    }

    clearIgnoredNavaids();
    addIgnoredNavaid(apt);
    recomputeBounds(true);
    update();
}

void AirportDiagram::setSelection(QmlPositioned* pos)
{
    if (pos && (m_selection == pos->inner())) {
        return;
    }

    if (!pos) {
        m_selection.clear();
    } else {
        m_selection = pos->inner();
    }
    emit selectionChanged();
    recomputeBounds(false);
    update();
}

void AirportDiagram::setApproachExtension(QuantityValue distance)
{
    if (m_approachDistance == distance) {
        return;
    }

    m_approachDistance = distance;
    recomputeBounds(false);
    update();
    emit approachExtensionChanged();
}

QuantityValue AirportDiagram::approachExtension() const
{
    return m_approachDistance;
}

QmlPositioned* AirportDiagram::selection() const
{
    if (!m_selection)
        return nullptr;

    return new QmlPositioned{m_selection};
}

qlonglong AirportDiagram::airportGuid() const
{
    if (!m_airport)
        return 0;
    return m_airport->guid();
}

void AirportDiagram::setAirportGuid(qlonglong guid)
{
    if (guid == -1) {
        m_airport.clear();
    } else {
        m_airport = fgpositioned_cast<FGAirport>(flightgear::NavDataCache::instance()->loadById(guid));
    }
    setAirport(m_airport);
    emit airportChanged();
}

void AirportDiagram::setApproachExtensionEnabled(bool e)
{
    if (m_approachExtensionEnabled == e)
        return;
    m_approachExtensionEnabled = e;
    recomputeBounds(true);
    update();
    emit approachExtensionChanged();
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

    extendBounds(r.p1);
    extendBounds(r.p2);
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
        extendBounds(p.pt, 10.0);
    }

    Q_FOREACH(const HelipadData& p, m_helipads) {
        extendBounds(p.pt, 20.0);
    }

    FGRunway* runwaySelection = fgpositioned_cast<FGRunway>(m_selection);
    if (runwaySelection && m_approachExtensionEnabled) {
        double d = m_approachDistance.convertToUnit(Units::Kilometers).value * 1000;
        QPointF pt = project(runwaySelection->pointOnCenterline(-d));
        extendBounds(pt);
    }
}

void AirportDiagram::addParking(FGParkingRef park)
{
    ParkingData pd = { project(park->geod()), park };
    m_parking.push_back(pd);
    extendBounds(pd.pt);
    update();
}

void AirportDiagram::addHelipad(FGHelipadRef pad)
{
    HelipadData pd = { project(pad->geod()), pad };
    m_helipads.push_back(pd);
    extendBounds(pd.pt);
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

    FGRunway* runwaySelection = fgpositioned_cast<FGRunway>(m_selection);

    // now draw the runways for real
    Q_FOREACH(const RunwayData& r, m_runways) {

        QColor color(Qt::magenta);
        if ((r.runway == runwaySelection) || (r.runway->reciprocalRunway() == runwaySelection)) {
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

        p->setPen((r.runway == runwaySelection) ? Qt::yellow : Qt::magenta);
        p->drawText(QRect(-100, 5, 200, 200), ident, Qt::AlignHCenter | Qt::AlignTop);

        FGRunway* recip = r.runway->reciprocalRunway();
        QString recipIdent = QString::fromStdString(recip->ident());

        p->setTransform(t);
        p->translate(r.p2);
        p->rotate(recip->headingDeg());
        p->scale(1.0 / m_scale, 1.0/ m_scale);

        p->setPen((r.runway->reciprocalRunway() == runwaySelection) ? Qt::yellow : Qt::magenta);
        p->drawText(QRect(-100, 5, 200, 200), recipIdent, Qt::AlignHCenter | Qt::AlignTop);
    }

    if (runwaySelection) {
        drawAircraft = true;
        aircraftPos = runwaySelection->geod();
        headingDeg = runwaySelection->headingDeg();
    }

    if (runwaySelection && m_approachExtensionEnabled) {
        p->setTransform(t);
        // draw approach extension point
        double d = m_approachDistance.convertToUnit(Units::Kilometers).value * 1000;
        QPointF pt = project(runwaySelection->pointOnCenterline(-d));
        QPointF pt2 = project(runwaySelection->geod());
        QPen pen(Qt::yellow);
        pen.setWidth(2.0 / m_scale);
        p->setPen(pen);
        p->drawLine(pt, pt2);

        aircraftPos = runwaySelection->pointOnCenterline(-d);
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
    FGHelipad* selectedHelipad = fgpositioned_cast<FGHelipad>(m_selection);

    Q_FOREACH(const HelipadData& p, m_helipads) {
        painter->save();
        painter->translate(p.pt);

        if (p.helipad == selectedHelipad) {
            painter->setBrush(Qt::yellow);
        } else {
            painter->setBrush(Qt::magenta);
        }

        painter->drawPath(m_helipadIconPath);
        painter->restore();
    }
}

void AirportDiagram::drawParking(QPainter* painter, const ParkingData& p) const
{
    painter->save();
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
    painter->setPen(Qt::NoPen);

    FGParking* selectedParking = fgpositioned_cast<FGParking>(m_selection);
    if (p.parking == selectedParking) {
        painter->setBrush(Qt::yellow);
    } else {
        painter->setBrush(QColor(255, 196, 196)); // kind of pink
    }

    painter->drawPath(useLeftIcon ? m_parkingIconLeftPath : m_parkingIconPath);

    // ensure the selection colour is quite visible, by not filling
    // with white when selected
    if (p.parking != selectedParking) {
        painter->fillRect(labelRect, Qt::white);
    }

    QFont f = painter->font();
    f.setPixelSize(20);
    painter->setFont(f);

    QString parkingName = QString::fromStdString(p.parking->name());
    int textFlags = Qt::AlignVCenter | Qt::AlignHCenter | Qt::TextWordWrap;
    QRectF bounds = painter->boundingRect(labelRect, textFlags, parkingName);
    if (bounds.height() > labelRect.height()) {
        f.setPixelSize(10);
        painter->setFont(f);
    }

    // draw text
    painter->setPen(Qt::black);
    painter->drawText(labelRect, textFlags, parkingName);
    painter->restore();
}

AirportDiagram::ParkingData AirportDiagram::findParkingData(const FGParkingRef &pk) const
{
    FGParking* selectedParking = fgpositioned_cast<FGParking>(m_selection);
    if (!selectedParking)
        return {};

    Q_FOREACH(const ParkingData& p, m_parking) {
        if (p.parking == selectedParking) {
            return p;
        }
    }

    return {};
}

void AirportDiagram::drawParkings(QPainter* painter) const
{
    FGParking* selectedParking = fgpositioned_cast<FGParking>(m_selection);

    Q_FOREACH(const ParkingData& p, m_parking) {
        if (p.parking == selectedParking) {
            continue; // skip and draw last
        }

        drawParking(painter, p);
    }

    if (selectedParking) {
        drawParking(painter, findParkingData(selectedParking));
    }
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
    me->accept();
    QTransform t(transform());
    double minWidth = 8.0 * unitLengthAfterMapping(t.inverted());

    Q_FOREACH (const HelipadData& pad, m_helipads) {
        QPainterPath pp = pathForHelipad(pad, t);
        //imgPaint.drawPath(pp);
        if (pp.contains(me->pos())) {
            emit clicked(new QmlPositioned{pad.helipad});
            return;
        }
    }

    Q_FOREACH(const RunwayData& r, m_runways) {
        QPainterPath pp = pathForRunway(r, t, minWidth);
        if (pp.contains(me->pos())) {
            // check which end was clicked
            QPointF p1(t.map(r.p1)), p2(t.map(r.p2));
            double param;
            distanceToLineSegment(QVector2D(me->pos()), QVector2D(p1), QVector2D(p2), &param);
            const FGRunwayRef clickedRunway = (param > 0.5) ? FGRunwayRef{r.runway->reciprocalRunway()} : r.runway;
            emit clicked(new QmlPositioned{clickedRunway});
            return;
        }
    } // of runways iteration

    Q_FOREACH(const ParkingData& parking, m_parking) {
        QPainterPath pp = pathForParking(parking, t);
        if (pp.contains(me->pos())) {
            emit clicked(new QmlPositioned{parking.parking});
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
    QTransform x = t;
    x.translate(h.pt.x(), h.pt.y());
    return x.map(m_helipadBoundsPath);
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
    auto pavementlist = m_airport->getPavements();
    for (auto pvtiter = pavementlist.begin(); pvtiter != pavementlist.end(); ++pvtiter)
    {
        FGPavementRef pave = *pvtiter;
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
