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

NavaidDiagram::NavaidDiagram(QWidget* pr) :
    BaseDiagram(pr),
    m_offsetEnabled(false),
    m_offsetDistanceNm(5.0),
    m_offsetBearingDeg(0),
    m_headingDeg(0)
{

}

void NavaidDiagram::setNavaid(FGPositionedRef nav)
{
    m_navaid = nav;
    m_projectionCenter = nav ? nav->geod() : SGGeod();
    m_scale = 1.0;
    m_bounds = QRectF(); // clear
    update();
}

void NavaidDiagram::paintContents(QPainter *)
{

}
