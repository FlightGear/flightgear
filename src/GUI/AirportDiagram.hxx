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
