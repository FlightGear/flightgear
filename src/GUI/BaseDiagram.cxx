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

#include <Navaids/navrecord.hxx>
#include <Navaids/positioned.hxx>
#include <Airports/airport.hxx>

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

void BaseDiagram::extendRect(QRectF &r, const QPointF &p)
{
    if (p.x() < r.left()) {
        r.setLeft(p.x());
    } else if (p.x() > r.right()) {
        r.setRight(p.x());
    }

    if (p.y() < r.top()) {
        r.setTop(p.y());
    } else if (p.y() > r.bottom()) {
        r.setBottom(p.y());
    }
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

    paintNavaids(&p);

    paintContents(&p);
}

class MapFilter : public FGPositioned::TypeFilter
{
public:
    MapFilter()
    {
      //  addType(FGPositioned::FIX);
        addType(FGPositioned::AIRPORT);
        addType(FGPositioned::NDB);
        addType(FGPositioned::VOR);
    }

    virtual bool pass(FGPositioned* aPos) const
    {
        bool ok = TypeFilter::pass(aPos);
        if (ok && (aPos->type() == FGPositioned::FIX)) {
            // ignore fixes which end in digits
            if (aPos->ident().length() > 4 && isdigit(aPos->ident()[3]) && isdigit(aPos->ident()[4])) {
                return false;
            }
        }

        return ok;
    }
};


void BaseDiagram::paintNavaids(QPainter* painter)
{
    QTransform xf = painter->transform();
    painter->setTransform(QTransform()); // reset to identity
    QTransform invT = xf.inverted();
    SGGeod topLeft = unproject(invT.map(QPointF(0,0)), m_projectionCenter);

    double minRunwayLengthFt = (16 / m_scale) * SG_METER_TO_FEET;
    // add 10nm fudge factor
    double drawRangeNm = SGGeodesy::distanceNm(m_projectionCenter, topLeft) + 10.0;
    //qDebug() << "draw range computed as:" << drawRangeNm;

    MapFilter f;
    FGPositionedList items = FGPositioned::findWithinRange(m_projectionCenter, drawRangeNm, &f);

    // pass 0 - icons

    FGPositionedList::const_iterator it;
    for (it = items.begin(); it != items.end(); ++it) {
        bool drawAsIcon = true;

        if ((*it)->type() == FGPositioned::AIRPORT) {
            FGAirport* apt = static_cast<FGAirport*>(it->ptr());
            if (apt->hasHardRunwayOfLengthFt(minRunwayLengthFt)) {

                drawAsIcon = false;
                painter->setTransform(xf);
                QVector<QLineF> lines = projectAirportRuwaysWithCenter(apt, m_projectionCenter);

                QPen pen(QColor(0x03, 0x83, 0xbf), 8);
                pen.setCosmetic(true);
                painter->setPen(pen);
                painter->drawLines(lines);

                QPen linePen(Qt::white, 2);
                linePen.setCosmetic(true);
                painter->setPen(linePen);
                painter->drawLines(lines);

                painter->resetTransform();
            }
        }

        if (drawAsIcon) {
            QPixmap pm = iconForPositioned(*it);
            QPointF loc = xf.map(project((*it)->geod()));
            loc -= QPointF(pm.width() >> 1, pm.height() >> 1);
            painter->drawPixmap(loc, pm);
        }
    }

    // restore transform
    painter->setTransform(xf);
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

void BaseDiagram::paintContents(QPainter* painter)
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
    extendRect(m_bounds, p);
}

QPointF BaseDiagram::project(const SGGeod& geod, const SGGeod& center)
{
    double r = earth_radius_lat(geod.getLatitudeRad());
    double ref_lat = center.getLatitudeRad(),
    ref_lon = center.getLongitudeRad(),
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

SGGeod BaseDiagram::unproject(const QPointF& xy, const SGGeod& center)
{
    double r = earth_radius_lat(center.getLatitudeRad());
    double lat = 0,
           lon = 0,
           ref_lat = center.getLatitudeRad(),
           ref_lon = center.getLongitudeRad(),
           rho = QVector2D(xy).length(),
           c = rho/r;

    if (rho == 0) {
        return center;
    }

    double x = xy.x(), y = xy.y();
    lat = asin( cos(c) * sin(ref_lat) + (y * sin(c) * cos(ref_lat)) / rho);

    if (ref_lat == (90 * SG_DEGREES_TO_RADIANS)) // north pole
    {
        lon = ref_lon + atan(-x/y);
    }
    else if (ref_lat == -(90 * SG_DEGREES_TO_RADIANS)) // south pole
    {
        lon = ref_lon + atan(x/y);
    }
    else
    {
        lon = ref_lon + atan(x* sin(c) / (rho * cos(ref_lat) * cos(c) - y * sin(ref_lat) * sin(c)));
    }

    return SGGeod::fromRad(lon, lat);
}

QPointF BaseDiagram::project(const SGGeod& geod) const
{
    return project(geod, m_projectionCenter);
}

QPixmap BaseDiagram::iconForPositioned(const FGPositionedRef& pos)
{
    // if airport type, check towered or untowered

    bool isTowered = false;
    if (FGAirport::isAirportType(pos)) {
        FGAirport* apt = static_cast<FGAirport*>(pos.ptr());
        isTowered = apt->hasTower();
    }

    switch (pos->type()) {
    case FGPositioned::VOR:
        // check for VORTAC

        if (static_cast<FGNavRecord*>(pos.ptr())->hasDME())
            return QPixmap(":/vor-dme-icon");

        return QPixmap(":/vor-icon");

    case FGPositioned::AIRPORT:
        return iconForAirport(static_cast<FGAirport*>(pos.ptr()));

    case FGPositioned::HELIPORT:
        return QPixmap(":/heliport-icon");
    case FGPositioned::SEAPORT:
        return QPixmap(isTowered ? ":/seaport-tower-icon" : ":/seaport-icon");
    case FGPositioned::NDB:
        return QPixmap(":/ndb-icon");
    case FGPositioned::FIX:
        return QPixmap(":/waypoint-icon");

    default:
        break;
    }

    return QPixmap();
}

QPixmap BaseDiagram::iconForAirport(FGAirport* apt)
{
    if (!apt->hasHardRunwayOfLengthFt(1500)) {
        return QPixmap(apt->hasTower() ? ":/airport-tower-icon" : ":/airport-icon");
    }

    if (apt->hasHardRunwayOfLengthFt(8500)) {
        QPixmap result(32, 32);
        result.fill(Qt::transparent);
        {
            QPainter p(&result);
            p.setRenderHint(QPainter::Antialiasing, true);
            QRectF b = result.rect().adjusted(4, 4, -4, -4);
            QVector<QLineF> lines = projectAirportRuwaysIntoRect(apt, b);

            p.setPen(QPen(QColor(0x03, 0x83, 0xbf), 8));
            p.drawLines(lines);

            p.setPen(QPen(Qt::white, 2));
            p.drawLines(lines);
        }
        return result;
    }

    QPixmap result(25, 25);
    result.fill(Qt::transparent);

    {
        QPainter p(&result);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);

        p.setBrush(apt->hasTower() ? QColor(0x03, 0x83, 0xbf) :
                                     QColor(0x9b, 0x5d, 0xa2));
        p.drawEllipse(QPointF(13, 13), 10, 10);

        FGRunwayRef r = apt->longestRunway();

        p.setPen(QPen(Qt::white, 2));
        p.translate(13, 13);
        p.rotate(r->headingDeg());
        p.drawLine(0, -8, 0, 8);
    }

    return result;
}

QVector<QLineF> BaseDiagram::projectAirportRuwaysWithCenter(FGAirportRef apt, const SGGeod& c)
{
    QVector<QLineF> r;

    const FGRunwayList& runways(apt->getRunwaysWithoutReciprocals());
    FGRunwayList::const_iterator it;

    for (it = runways.begin(); it != runways.end(); ++it) {
        FGRunwayRef rwy = *it;
        QPointF p1 = project(rwy->geod(), c);
        QPointF p2 = project(rwy->end(), c);
        r.append(QLineF(p1, p2));
    }

    return r;
}

QVector<QLineF> BaseDiagram::projectAirportRuwaysIntoRect(FGAirportRef apt, const QRectF &bounds)
{
    QVector<QLineF> r = projectAirportRuwaysWithCenter(apt, apt->geod());

    QRectF extent;
    Q_FOREACH(const QLineF& l, r) {
        extendRect(extent, l.p1());
        extendRect(extent, l.p2());
    }

 // find constraining scale factor
    double ratioInX = bounds.width() / extent.width();
    double ratioInY = bounds.height() / extent.height();

    QTransform t;
    t.translate(bounds.left(), bounds.top());
    t.scale(std::min(ratioInX, ratioInY),
            std::min(ratioInX, ratioInY));
    t.translate(-extent.left(), -extent.top()); // move unscaled to 0,0

    for (int i=0; i<r.size(); ++i) {
        r[i] = t.map(r[i]);
    }

    return r;
}
