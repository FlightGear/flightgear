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
#include "UnitsModel.hxx"

#include <Navaids/navrecord.hxx>
#include <simgear/math/sg_geodesy.hxx>

class NavaidDiagram : public BaseDiagram
{
    Q_OBJECT

    Q_PROPERTY(qlonglong navaid READ navaid WRITE setNavaid NOTIFY locationChanged)
    Q_PROPERTY(QmlGeod geod READ geod WRITE setGeod NOTIFY locationChanged)

    Q_PROPERTY(QuantityValue heading READ heading WRITE setHeading NOTIFY offsetChanged)
    Q_PROPERTY(QuantityValue offsetBearing READ offsetBearing WRITE setOffsetBearing NOTIFY offsetChanged)
    Q_PROPERTY(bool offsetEnabled READ isOffsetEnabled WRITE setOffsetEnabled NOTIFY offsetChanged)
    Q_PROPERTY(QuantityValue offsetDistance READ offsetDistance WRITE setOffsetDistance NOTIFY offsetChanged)
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

    void setOffsetDistance(QuantityValue distance);
    QuantityValue offsetDistance() const
    { return m_offsetDistance; }

    void setOffsetBearing(QuantityValue bearing);
    QuantityValue offsetBearing() const
    { return m_offsetBearing; }

    void setHeading(QuantityValue heading);
    QuantityValue heading() const
    { return m_heading; }

signals:
    void locationChanged();
    void offsetChanged();

protected:
    void paintContents(QPainter *) override;

    void doComputeBounds() override;
private:
    FGPositionedRef m_navaid;
    SGGeod m_geod;

    bool m_offsetEnabled = false;
    QuantityValue m_offsetDistance;
    QuantityValue m_offsetBearing;
    QuantityValue m_heading;
};

#endif // of GUI_NAVAID_DIAGRAM_HXX
