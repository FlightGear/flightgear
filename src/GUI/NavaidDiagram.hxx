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
#include "QmlPositioned.hxx"

#include <Navaids/navrecord.hxx>
#include <simgear/math/sg_geodesy.hxx>

class NavaidDiagram : public BaseDiagram
{
    Q_OBJECT

    Q_PROPERTY(qlonglong navaid READ navaid WRITE setNavaid NOTIFY locationChanged)
    Q_PROPERTY(QmlGeod geod READ geod WRITE setGeod NOTIFY locationChanged)

    Q_PROPERTY(int headingDeg READ headingDeg WRITE setHeadingDeg NOTIFY offsetChanged)
    Q_PROPERTY(int offsetBearingDeg READ offsetBearingDeg WRITE setOffsetBearingDeg NOTIFY offsetChanged)
    Q_PROPERTY(bool offsetEnabled READ isOffsetEnabled WRITE setOffsetEnabled NOTIFY offsetChanged)
    Q_PROPERTY(double offsetDistanceNm READ offsetDistanceNm WRITE setOffsetDistanceNm NOTIFY offsetChanged)
public:
    NavaidDiagram(QQuickItem* pr = nullptr);

    void setNavaid(qlonglong nav);
    qlonglong navaid() const;

    void setGeod(QmlGeod geod);
    QmlGeod geod() const;

    void setGeod(const SGGeod& geod);

    bool isOffsetEnabled() const
    { return m_offsetEnabled; }

    void setOffsetEnabled(bool offset);

    void setOffsetDistanceNm(double distanceNm);
    double offsetDistanceNm() const
    { return m_offsetDistanceNm; }

    void setOffsetBearingDeg(int bearingDeg);
    int offsetBearingDeg() const
    { return m_offsetBearingDeg; }

    void setHeadingDeg(int headingDeg);
    int headingDeg() const
    { return m_headingDeg; }

signals:
    void locationChanged();
    void offsetChanged();

protected:
    void paintContents(QPainter *) override;

    void doComputeBounds() override;
private:
    FGPositionedRef m_navaid;
    SGGeod m_geod;

    bool m_offsetEnabled;
    double m_offsetDistanceNm;
    int m_offsetBearingDeg;
    int m_headingDeg;
};

#endif // of GUI_NAVAID_DIAGRAM_HXX
