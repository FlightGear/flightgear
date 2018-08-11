// RouteDiagram.hxx - show a route graphically
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

#ifndef GUI_ROUTE_DIAGRAM_HXX
#define GUI_ROUTE_DIAGRAM_HXX

#include "BaseDiagram.hxx"
#include "QmlPositioned.hxx"
#include "UnitsModel.hxx"

#include <Navaids/navrecord.hxx>
#include <Navaids/routePath.hxx>

#include <simgear/math/sg_geodesy.hxx>

class FlightPlanController;

class RouteDiagram : public BaseDiagram
{
    Q_OBJECT

    Q_PROPERTY(FlightPlanController* flightplan READ flightplan WRITE setFlightplan NOTIFY flightplanChanged)

    Q_PROPERTY(int activeLegIndex READ activeLegIndex WRITE setActiveLegIndex NOTIFY legIndexChanged)
    Q_PROPERTY(int numLegs READ numLegs NOTIFY flightplanChanged)
 public:
    RouteDiagram(QQuickItem* pr = nullptr);

    FlightPlanController* flightplan() const
    {
        return m_flightplan;
    }

    void setFlightplan(FlightPlanController* fp);

    int numLegs() const;

    int activeLegIndex() const
    {
        return m_activeLegIndex;
    }

public slots:
    void setActiveLegIndex(int activeLegIndex);

signals:
    void flightplanChanged(FlightPlanController* flightplan);

    void legIndexChanged(int activeLegIndex);

protected:
    void paintContents(QPainter *) override;

    void doComputeBounds() override;
private:
    void fpChanged();

    FlightPlanController* m_flightplan = nullptr;

    std::unique_ptr<RoutePath> m_path;
    int m_activeLegIndex = 0;
};

#endif // of GUI_ROUTE_DIAGRAM_HXX
