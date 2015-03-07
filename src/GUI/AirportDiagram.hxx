// AirportDiagram.hxx - part of GUI launcher using Qt5
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

#include <QWidget>
#include <QPainterPath>

#include <Airports/airports_fwd.hxx>
#include <simgear/math/sg_geodesy.hxx>

class AirportDiagram : public QWidget
{
public:
    AirportDiagram(QWidget* pr);

    void setAirport(FGAirportRef apt);

    void addRunway(FGRunwayRef rwy);
    void addParking(FGParking* park);
protected:
    virtual void paintEvent(QPaintEvent* pe);
    // wheel event for zoom

    // mouse drag for pan


private:
    void extendBounds(const QPointF& p);
    QPointF project(const SGGeod& geod) const;

    void buildTaxiways();
    void buildPavements();

    FGAirportRef m_airport;
    SGGeod m_projectionCenter;
    double m_scale;
    QRectF m_bounds;

    struct RunwayData {
        QPointF p1, p2;
        int widthM;
        FGRunwayRef runway;
    };

    QList<RunwayData> m_runways;

    struct TaxiwayData {
        QPointF p1, p2;
        int widthM;

        bool operator<(const TaxiwayData& other) const
        {
            return widthM < other.widthM;
        }
    };

    QList<TaxiwayData> m_taxiways;
    QList<QPainterPath> m_pavements;
};
