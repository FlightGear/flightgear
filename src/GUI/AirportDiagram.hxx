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

#ifndef GUI_AIRPORT_DIAGRAM_HXX
#define GUI_AIRPORT_DIAGRAM_HXX

#include "BaseDiagram.hxx"

#include <Airports/parking.hxx>
#include <Airports/runways.hxx>
#include <simgear/math/sg_geodesy.hxx>

class AirportDiagram : public BaseDiagram
{
    Q_OBJECT
public:
    AirportDiagram(QWidget* pr);
    virtual ~AirportDiagram();

    void setAirport(FGAirportRef apt);

    void addRunway(FGRunwayRef rwy);
    void addParking(FGParkingRef park);
    void addHelipad(FGHelipadRef pad);

    FGRunwayRef selectedRunway() const;
    void setSelectedRunway(FGRunwayRef r);

    void setSelectedHelipad(FGHelipadRef pad);
    void setSelectedParking(FGParkingRef park);

    void setApproachExtensionDistance(double distanceNm);
Q_SIGNALS:
    void clickedRunway(FGRunwayRef rwy);
    void clickedHelipad(FGHelipadRef pad);
    void clickedParking(FGParkingRef park);
protected:
    
    virtual void mouseReleaseEvent(QMouseEvent* me);

    void paintContents(QPainter*) Q_DECL_OVERRIDE;

    void doComputeBounds() Q_DECL_OVERRIDE;
private:
    struct RunwayData {
        QPointF p1, p2;
        int widthM;
        FGRunwayRef runway;
    };

    struct TaxiwayData {
        QPointF p1, p2;
        int widthM;

        bool operator<(const TaxiwayData& other) const
        {
            return widthM < other.widthM;
        }
    };

    struct ParkingData
    {
        QPointF pt;
        FGParkingRef parking;
    };

    struct HelipadData
    {
        QPointF pt;
        FGHelipadRef helipad;
    };

    void buildTaxiways();
    void buildPavements();

    void drawILS(QPainter *painter, FGRunwayRef runway) const;

    void drawParkings(QPainter *p) const;
    void drawParking(QPainter *painter, const ParkingData &p) const;

    ParkingData findParkingData(const FGParkingRef& pk) const;

    void drawHelipads(QPainter *painter);

    QPainterPath pathForRunway(const RunwayData &r, const QTransform &t) const;
    QPainterPath pathForHelipad(const HelipadData &h, const QTransform &t) const;
    QPainterPath pathForParking(const ParkingData &p, const QTransform &t) const;

private:
    FGAirportRef m_airport;

    QVector<RunwayData> m_runways;
    QVector<TaxiwayData> m_taxiways;
    QVector<QPainterPath> m_pavements;
    QVector<ParkingData> m_parking;
    QVector<HelipadData> m_helipads;

    QPainterPath m_parkingIconPath, // arrow points right
        m_parkingIconLeftPath; // arrow points left
    double m_approachDistanceNm;
    FGRunwayRef m_selectedRunway;
    FGParkingRef m_selectedParking;

    QPixmap m_helipadIcon;
};

#endif // of GUI_AIRPORT_DIAGRAM_HXX
