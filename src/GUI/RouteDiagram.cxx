// RouteDiagram.cxx - GUI diagram of a route
//
// Written by James Turner, started August 2018.
//
// Copyright (C) 2018 James Turner <james@flightgear.org>
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

#include "RouteDiagram.hxx"


#include <QPainter>
#include <QDebug>
#include <QVector2D>
#include <QMouseEvent>

#include <Navaids/NavDataCache.hxx>

#include "FlightPlanController.hxx"

using namespace flightgear;

RouteDiagram::RouteDiagram(QQuickItem* pr) :
    BaseDiagram(pr)
{
}

void RouteDiagram::setFlightplan(FlightPlanController *fp)
{
    if (fp == m_flightplan)
        return;

    if (m_flightplan) {
        // disconnect from old signal
        disconnect(m_flightplan, nullptr, this, nullptr);
    }

    m_flightplan = fp;
    m_activeLegIndex = 0;

    emit flightplanChanged(fp);

    if (fp) {
        connect(fp, &FlightPlanController::infoChanged, this, &RouteDiagram::fpChanged);
        connect(fp, &FlightPlanController::waypointsChanged, this, &RouteDiagram::fpChanged);
    }

    fpChanged();
    update();
}

int RouteDiagram::numLegs() const
{
    if (!m_flightplan)
        return 0;

    FlightPlanRef fp = m_flightplan->flightplan();
    if (!fp)
        return 0;

    return fp->numLegs();
}

void RouteDiagram::setActiveLegIndex(int activeLegIndex)
{
    if (m_activeLegIndex == activeLegIndex)
        return;

    if ((activeLegIndex < 0) || (activeLegIndex >= numLegs())) {
        qWarning() << Q_FUNC_INFO << "invalid leg index" << activeLegIndex;
        return;
    }

    m_activeLegIndex = activeLegIndex;
    emit legIndexChanged(m_activeLegIndex);

    const double halfLegDistance = m_path->distanceForIndex(m_activeLegIndex) * 0.5;
    m_projectionCenter = m_path->positionForDistanceFrom(m_activeLegIndex, halfLegDistance);
    recomputeBounds(true);
    update();
}

void RouteDiagram::paintContents(QPainter *painter)
{
    if (!m_flightplan)
        return;

    FlightPlanRef fp = m_flightplan->flightplan();
    QVector<QLineF> lines;
    QVector<QLineF> activeLines;
    for (int l=0; l < fp->numLegs(); ++l) {
        QPointF previous;
        bool isFirst = true;
        for (auto g : m_path->pathForIndex(l)) {
            QPointF p = project(g);
            if (isFirst) {
                isFirst = false;
            } else if (l == m_activeLegIndex) {
                activeLines.append(QLineF(previous, p));
            } else {
                lines.append(QLineF(previous, p));
            }
            previous = p;
        }
    }

    QPen linePen(Qt::magenta, 2);
    linePen.setCosmetic(true);
    painter->setPen(linePen);
    painter->drawLines(lines);

    linePen.setColor(Qt::yellow);
    painter->setPen(linePen);
    painter->drawLines(activeLines);
}

void RouteDiagram::doComputeBounds()
{
    FlightPlanRef fp = m_flightplan->flightplan();
    const SGGeodVec gv(m_path->pathForIndex(m_activeLegIndex));
    std::for_each(gv.begin(), gv.end(), [this](const SGGeod& g)
        {this->extendBounds(this->project(g)); }
    );
}

void RouteDiagram::fpChanged()
{
    FlightPlanRef fp = m_flightplan->flightplan();
    m_path.reset(new RoutePath(fp));
    m_activeLegIndex = 0;

    if (fp && (fp->numLegs() > 0)) {
        const double halfLegDistance = m_path->distanceForIndex(m_activeLegIndex) * 0.5;
        m_projectionCenter = m_path->positionForDistanceFrom(m_activeLegIndex, halfLegDistance);
    }
    recomputeBounds(true);
    update();
}
