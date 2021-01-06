#ifndef FLIGHTPLANCONTROLLER_HXX
#define FLIGHTPLANCONTROLLER_HXX

#include <memory>

#include <QObject>

#include <Navaids/FlightPlan.hxx>

#include "UnitsModel.hxx"

class QmlPositioned;
class LegsModel;
class FPDelegate;
class LaunchConfig;

class FlightPlanController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool enabled MEMBER _enabled NOTIFY enabledChanged)
    Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)

    Q_PROPERTY(QString callsign READ callsign WRITE setCallsign NOTIFY infoChanged)
    Q_PROPERTY(QString remarks READ remarks WRITE setRemarks NOTIFY infoChanged)
    Q_PROPERTY(QString aircraftType READ aircraftType WRITE setAircraftType NOTIFY infoChanged)

    Q_PROPERTY(LegsModel* legs READ legs CONSTANT)

    Q_PROPERTY(QString icaoRoute READ icaoRoute NOTIFY waypointsChanged)
    
    Q_ENUMS(FlightRules)
    Q_ENUMS(FlightType)

    Q_PROPERTY(FlightRules flightRules READ flightRules WRITE setFlightRules NOTIFY infoChanged)
    Q_PROPERTY(FlightType flightType READ flightType WRITE setFlightType NOTIFY infoChanged)

    // planned departure date + time

    Q_PROPERTY(QuantityValue totalDistanceNm READ totalDistanceNm NOTIFY infoChanged)

    Q_PROPERTY(int estimatedDurationMinutes READ estimatedDurationMinutes WRITE setEstimatedDurationMinutes NOTIFY infoChanged)

    Q_PROPERTY(QuantityValue cruiseAltitude READ cruiseAltitude WRITE setCruiseAltitude NOTIFY infoChanged)
    Q_PROPERTY(QuantityValue cruiseSpeed READ cruiseSpeed WRITE setCruiseSpeed NOTIFY infoChanged)

    Q_PROPERTY(QmlPositioned* departure READ departure WRITE setDeparture NOTIFY infoChanged)
    Q_PROPERTY(QmlPositioned* destination READ destination WRITE setDestination NOTIFY infoChanged)
    Q_PROPERTY(QmlPositioned* alternate READ alternate WRITE setAlternate NOTIFY infoChanged)

    // equipment
public:
    virtual ~FlightPlanController();

    // alias these enums to QML
    enum FlightRules
    {
        VFR = 0,
        IFR,
        IFR_VFR,
        VFR_IFR
    };

    enum FlightType
    {
        Scheduled = 0,
        NonScheduled,
        GeneralAviation,
        Military,
        Other
    };

    explicit FlightPlanController(QObject *parent,
                                  LaunchConfig* config);

    bool loadFromPath(QString path);
    bool saveToPath(QString path) const;

    QuantityValue cruiseAltitude() const;
    void setCruiseAltitude(QuantityValue alt);

    QString description() const;

    QmlPositioned* departure() const;
    QmlPositioned* destination() const;
    QmlPositioned* alternate() const;

    QuantityValue cruiseSpeed() const;

    FlightRules flightRules() const;
    FlightType flightType() const;

    QString callsign() const;
    QString remarks() const;
    QString aircraftType() const;

    int estimatedDurationMinutes() const;
    QuantityValue totalDistanceNm() const;

    Q_INVOKABLE bool tryParseRoute(QString routeDesc);

    Q_INVOKABLE bool tryGenerateRoute();
    Q_INVOKABLE void clearRoute();

    LegsModel* legs() const
    { return _legs; }

    QString icaoRoute() const;

    flightgear::FlightPlanRef flightplan() const
    { return _fp; }

    Q_INVOKABLE bool loadPlan();
signals:
    void infoChanged();
    void waypointsChanged();
    
    void enabledChanged(bool enabled);
    void descriptionChanged(QString description);

public slots:

    void setFlightType(FlightType ty);
    void setFlightRules(FlightRules r);

    void setCallsign(QString s);
    void setRemarks(QString r);
    void setAircraftType(QString ty);

    void setDeparture(QmlPositioned* destinationAirport);
    void setDestination(QmlPositioned* destinationAirport);
    void setAlternate(QmlPositioned* apt);

    void setCruiseSpeed(QuantityValue cruiseSpeed);

    void setEstimatedDurationMinutes(int mins);
    
    void computeDuration();

    void clearPlan();
    void savePlan();
private slots:
    void onCollectConfig();
    void onSave();
    void onRestore();
    
private:
    friend class FPDelegate;

    flightgear::FlightPlanRef _fp;
    LegsModel* _legs = nullptr;
    std::unique_ptr<FPDelegate> _delegate;
    LaunchConfig* _config = nullptr;
    bool _enabled = false;
};

#endif // FLIGHTPLANCONTROLLER_HXX
