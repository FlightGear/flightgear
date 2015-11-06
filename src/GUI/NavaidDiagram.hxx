// NavaidDiagram.hxx - part of GUI launcher using Qt5
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

#ifndef GUI_NAVAID_DIAGRAM_HXX
#define GUI_NAVAID_DIAGRAM_HXX

#include "BaseDiagram.hxx"
#include <QPainterPath>

#include <Navaids/navrecord.hxx>
#include <simgear/math/sg_geodesy.hxx>

class NavaidDiagram : public BaseDiagram
{
    Q_OBJECT
public:
    NavaidDiagram(QWidget* pr);

    void setNavaid(FGPositionedRef nav);

    void setGeod(const SGGeod& geod);

    bool isOffsetEnabled() const;
    void setOffsetEnabled(bool offset);

    void setOffsetDistanceNm(double distanceNm);
    double offsetDistanceNm() const;

    void setOffsetBearingDeg(int bearingDeg);
    int offsetBearingDeg() const;

    void setHeadingDeg(int headingDeg);
    void headingDeg() const;
protected:
    void paintContents(QPainter *) Q_DECL_OVERRIDE;

    void doComputeBounds() Q_DECL_OVERRIDE;
private:
    FGPositionedRef m_navaid;
    SGGeod m_geod;

    bool m_offsetEnabled;
    double m_offsetDistanceNm;
    int m_offsetBearingDeg;
    int m_headingDeg;
};

#endif // of GUI_NAVAID_DIAGRAM_HXX
