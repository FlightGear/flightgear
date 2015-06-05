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

#include <QPainter>
#include <QDebug>
#include <QVector2D>
#include <QMouseEvent>

#include <Airports/airport.hxx>
#include <Airports/runways.hxx>
#include <Airports/parking.hxx>
#include <Airports/pavement.hxx>

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

AirportDiagram::AirportDiagram(QWidget* pr) :
QWidget(pr)
{
    setSizePolicy(QSizePolicy::MinimumExpanding,
                  QSizePolicy::MinimumExpanding);
    setMinimumSize(100, 100);
}

void AirportDiagram::setAirport(FGAirportRef apt)
{
    m_airport = apt;
    m_projectionCenter = apt ? apt->geod() : SGGeod();
    m_scale = 1.0;
    m_bounds = QRectF(); // clear
    m_runways.clear();

    if (apt) {
        buildTaxiways();
        buildPavements();
    }

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
    
    m_selectedRunway = r;
    update();
}

void AirportDiagram::addRunway(FGRunwayRef rwy)
{
    Q_FOREACH(RunwayData rd, m_runways) {
        if (rd.runway == rwy->reciprocalRunway()) {
            return; // only add one end of reciprocal runways
        }
    }

    QPointF p1 = project(rwy->geod()),
    p2 = project(rwy->end());
    extendBounds(p1);
    extendBounds(p2);

    RunwayData r;
    r.p1 = p1;
    r.p2 = p2;
    r.widthM = qRound(rwy->widthM());
    r.runway = rwy;
    m_runways.append(r);
    update();
}

void AirportDiagram::addParking(FGParking* park)
{
    QPointF p = project(park->geod());
    extendBounds(p);
    update();
}

QTransform AirportDiagram::transform() const
{
    // fit bounds within our available space, allowing for a margin
    const int MARGIN = 32; // pixels
    double ratioInX = (width() - MARGIN * 2) / m_bounds.width();
    double ratioInY = (height() - MARGIN * 2) / m_bounds.height();
    double scale = std::min(ratioInX, ratioInY);
    
    QTransform t;
    t.translate(width() / 2, height() / 2); // center projection origin in the widget
    t.scale(scale, scale);
    // center the bounding box (may not be at the origin)
    t.translate(-m_bounds.center().x(), -m_bounds.center().y());
    return t;
}

void AirportDiagram::paintEvent(QPaintEvent* pe)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0x3f, 0x3f, 0x3f));

    // fit bounds within our available space, allowing for a margin
    const int MARGIN = 32; // pixels
    double ratioInX = (width() - MARGIN * 2) / m_bounds.width();
    double ratioInY = (height() - MARGIN * 2) / m_bounds.height();
    double scale = std::min(ratioInX, ratioInY);

    QTransform t(transform());
    p.setTransform(t);

// pavements
    QBrush brush(QColor(0x9f, 0x9f, 0x9f));
    Q_FOREACH(const QPainterPath& path, m_pavements) {
        p.drawPath(path);
    }

// taxiways
    Q_FOREACH(const TaxiwayData& t, m_taxiways) {
        QPen pen(QColor(0x9f, 0x9f, 0x9f));
        pen.setWidth(t.widthM);
        p.setPen(pen);
        p.drawLine(t.p1, t.p2);
    }

// runways
    QFont f;
    f.setPixelSize(14);
    p.setFont(f);

    Q_FOREACH(const RunwayData& r, m_runways) {
        QColor color(Qt::magenta);
        if ((r.runway == m_selectedRunway) || (r.runway->reciprocalRunway() == m_selectedRunway)) {
            color = Qt::yellow;
        }
        
        p.setTransform(t);

        QPen pen(color);
        pen.setWidth(r.widthM);
        p.setPen(pen);
        
        p.drawLine(r.p1, r.p2);

    // draw idents
        QString ident = QString::fromStdString(r.runway->ident());

        p.translate(r.p1);
        p.rotate(r.runway->headingDeg());
        // invert scaling factor so we can use screen pixel sizes here
        p.scale(1.0 / scale, 1.0/ scale);
        
        p.setPen((r.runway == m_selectedRunway) ? Qt::yellow : Qt::magenta);
        p.drawText(QRect(-100, 5, 200, 200), ident, Qt::AlignHCenter | Qt::AlignTop);

        FGRunway* recip = r.runway->reciprocalRunway();
        QString recipIdent = QString::fromStdString(recip->ident());

        p.setTransform(t);
        p.translate(r.p2);
        p.rotate(recip->headingDeg());
        p.scale(1.0 / scale, 1.0/ scale);

        p.setPen((r.runway->reciprocalRunway() == m_selectedRunway) ? Qt::yellow : Qt::magenta);
        p.drawText(QRect(-100, 5, 200, 200), recipIdent, Qt::AlignHCenter | Qt::AlignTop);
    }
}

void AirportDiagram::mouseReleaseEvent(QMouseEvent* me)
{
    QTransform t(transform());
    double minDist = std::numeric_limits<double>::max();
    FGRunwayRef bestRunway;
    
    Q_FOREACH(const RunwayData& r, m_runways) {
        QPointF p1(t.map(r.p1)), p2(t.map(r.p2));
        double t;
        double d = distanceToLineSegment(QVector2D(me->pos()),
                                         QVector2D(p1),
                                         QVector2D(p2), &t);
        if (d < minDist) {
            if (t > 0.5) {
                bestRunway = r.runway->reciprocalRunway();
            } else {
                bestRunway = r.runway;
            }
            minDist = d;
        }
    }
    
    if (minDist < 16.0) {
        emit clickedRunway(bestRunway);
    }
}

void AirportDiagram::extendBounds(const QPointF& p)
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

QPointF AirportDiagram::project(const SGGeod& geod) const
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

    return QPointF(x, -y) * r * m_scale;
}

void AirportDiagram::buildTaxiways()
{
    m_taxiways.clear();
    for (unsigned int tIndex=0; tIndex < m_airport->numTaxiways(); ++tIndex) {
        FGTaxiwayRef tx = m_airport->getTaxiwayByIndex(tIndex);

        TaxiwayData td;
        td.p1 = project(tx->geod());
        td.p2 = project(tx->pointOnCenterline(tx->lengthM()));
        extendBounds(td.p1);
        extendBounds(td.p2);
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
