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

#include <QPixmap>

#include "UnitsModel.hxx"

#include <Airports/parking.hxx>
#include <Airports/runways.hxx>
#include <simgear/math/sg_geodesy.hxx>

// forward decls
class QmlPositioned;

class AirportDiagram : public BaseDiagram
{
    Q_OBJECT

    Q_PROPERTY(QmlPositioned* selection READ selection WRITE setSelection NOTIFY selectionChanged)
    Q_PROPERTY(qlonglong airport READ airportGuid WRITE setAirportGuid NOTIFY airportChanged)

    Q_PROPERTY(bool approachExtensionEnabled READ approachExtensionEnabled WRITE setApproachExtensionEnabled NOTIFY approachExtensionChanged)
    Q_PROPERTY(QuantityValue approachExtension READ approachExtension WRITE setApproachExtension NOTIFY approachExtensionChanged)
public:
    AirportDiagram(QQuickItem* pr = nullptr);
    virtual ~AirportDiagram();

    void setAirport(FGAirportRef apt);

    void addRunway(FGRunwayRef rwy);
    void addParking(FGParkingRef park);
    void addHelipad(FGHelipadRef pad);

    QmlPositioned* selection() const;

    void setSelection(QmlPositioned* pos);

    void setApproachExtension(QuantityValue distance);
    QuantityValue approachExtension() const;

    qlonglong airportGuid() const;
    void setAirportGuid(qlonglong guid);

    bool approachExtensionEnabled() const
    {
        return m_approachExtensionEnabled;
    }

    void setApproachExtensionEnabled(bool e);
Q_SIGNALS:
    void clicked(QmlPositioned* pos);

    void selectionChanged();
    void airportChanged();
    void approachExtensionChanged();

protected:
    
    void mouseReleaseEvent(QMouseEvent* me) override;

    void paintContents(QPainter*) override;

    void doComputeBounds() override;
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

    QPainterPath pathForRunway(const RunwayData &r, const QTransform &t, const double minWidth) const;
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
    QuantityValue m_approachDistance;
    bool m_approachExtensionEnabled = false;

    QPainterPath m_helipadIconPath, m_helipadBoundsPath;
    FGPositionedRef m_selection;
};

#endif // of GUI_AIRPORT_DIAGRAM_HXX
