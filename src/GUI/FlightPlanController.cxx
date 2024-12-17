#include "FlightPlanController.hxx"

#include <Main/options.hxx>
#include <QAbstractListModel>
#include <QDebug>
#include <QFileDialog>
#include <QQmlComponent>
#include <QSettings>
#include <QTimer>

#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/airways.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/airways.hxx>

#include "LaunchConfig.hxx"
#include "QmlPositioned.hxx"
#include "SettingsWrapper.hxx"

using namespace flightgear;

const int LegDistanceRole = Qt::UserRole;
const int LegTrackRole = Qt::UserRole + 1;
const int LegTerminatorNavRole = Qt::UserRole + 2;
const int LegAirwayIdentRole = Qt::UserRole + 3;
const int LegTerminatorTypeRole = Qt::UserRole + 4;
const int LegTerminatorNavNameRole = Qt::UserRole + 5;
const int LegTerminatorNavFrequencyRole = Qt::UserRole + 6;
const int LegAltitudeFtRole = Qt::UserRole + 7;
const int LegAltitudeTypeRole = Qt::UserRole + 8;

/////////////////////////////////////////////////////////////////////////////

void LegsModel::setFlightPlan(flightgear::FlightPlanRef f)
{
    beginResetModel();
    _fp = f;
    endResetModel();
    emit numLegsChanged();
}

int LegsModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return _fp->numLegs();
}

QVariant LegsModel::data(const QModelIndex& index, int role) const
{
    const auto leg = _fp->legAtIndex(index.row());
    if (!leg)
        return {};

    const auto wp = leg->waypoint();

    switch (role) {
    case Qt::DisplayRole: {
        if (wp->type() == "via") {
            // we want the end waypoint name
            return QString::fromStdString(wp->source()->ident());
        }

        return QString::fromStdString(leg->waypoint()->ident());
    }

    case LegDistanceRole:
        return QVariant::fromValue(QuantityValue{Units::NauticalMiles, leg->distanceNm()});
    case LegTrackRole:
        return QVariant::fromValue(QuantityValue{Units::DegreesTrue, leg->courseDeg()});

    case LegAirwayIdentRole: {
        AirwayRef awy;
        if (wp->type() == "via") {
            auto via = static_cast<flightgear::Via*>(leg->waypoint());
            awy = via->airway();
        } else if (wp->flag(WPT_VIA)) {
            awy = static_cast<Airway*>(wp->owner());
        }

        return awy ? QString::fromStdString(awy->ident()) : QVariant{};
    }

    case LegTerminatorNavRole: {
        if (leg->waypoint()->source()) {
            return QString::fromStdString(leg->waypoint()->source()->ident());
        }
        break;
    }

    case LegTerminatorNavFrequencyRole: {
        const auto n = fgpositioned_cast<FGNavRecord>(leg->waypoint()->source());
        if (n) {
            const double f = n->get_freq() / 100.0;
            if (n->type() == FGPositioned::NDB) {
                return QVariant::fromValue(QuantityValue(Units::FreqKHz, f));
            }

            return QVariant::fromValue(QuantityValue(Units::FreqMHz, f));
        }
        return QVariant::fromValue(QuantityValue());
    }

    case LegTerminatorNavNameRole: {
        if (leg->waypoint()->source()) {
            return QString::fromStdString(leg->waypoint()->source()->name());
        }
        return QString{}; // avoud undefined-value QML error if we return a null variant
    }

    case LegTerminatorTypeRole:
        return QString::fromStdString(leg->waypoint()->type());

    case LegAltitudeFtRole:
        return leg->altitudeFt();

    case LegAltitudeTypeRole:
        return leg->altitudeRestriction();

    default:
        break;
    }

    return {};
}

void LegsModel::waypointsChanged()
{
    beginResetModel();
    endResetModel();
    numLegsChanged();
}

QHash<int, QByteArray> LegsModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractListModel::roleNames();

    result[Qt::DisplayRole] = "label";
    result[LegDistanceRole] = "distance";
    result[LegTrackRole] = "track";
    result[LegTerminatorNavRole] = "to";
    result[LegTerminatorNavFrequencyRole] = "frequency";
    result[LegAirwayIdentRole] = "via";
    result[LegTerminatorTypeRole] = "wpType";
    result[LegTerminatorNavNameRole] = "toName";
    result[LegAltitudeFtRole] = "altitudeFt";
    result[LegAltitudeTypeRole] = "altitudeType";

    return result;
}

int LegsModel::numLegs() const
{
    return _fp->numLegs();
}

/////////////////////////////////////////////////////////////////////////////

class FPDelegate : public FlightPlan::Delegate
{
public:
    void arrivalChanged() override
    {
        p->infoChanged();
    }

    void departureChanged() override
    {
        p->infoChanged();
    }

    void cruiseChanged() override
    {
        p->infoChanged();
    }

    void waypointsChanged() override
    {
        QTimer::singleShot(0, p->_legs, &LegsModel::waypointsChanged);
        p->waypointsChanged();
        p->infoChanged();
    }

    FlightPlanController* p;
};

/////////////////////////////////////////////////////////////////////////////

FlightPlanController::FlightPlanController(QObject *parent, LaunchConfig* config)
    : QObject(parent)
{
    _config = config;
    connect(_config, &LaunchConfig::collect, this, &FlightPlanController::onCollectConfig);
    connect(_config, &LaunchConfig::save, this, &FlightPlanController::onSave);
    connect(_config, &LaunchConfig::restore, this, &FlightPlanController::onRestore);
    
    _delegate.reset(new FPDelegate);
    _delegate->p = this; // link back to us

    qmlRegisterUncreatableType<LegsModel>("FlightGear", 1, 0, "LegsModel", "singleton");
    _fp.reset(flightgear::FlightPlan::createRoute());
    _fp->addDelegate(_delegate.get());
    _legs = new LegsModel();
    _legs->setFlightPlan(_fp);

    // initial restore
    onRestore();
}

FlightPlanController::~FlightPlanController()
{
    _fp->removeDelegate(_delegate.get());
}

void FlightPlanController::clearPlan()
{
    auto fp = flightgear::FlightPlan::createRoute();
    _fp->removeDelegate(_delegate.get());
    _fp = fp;
    _fp->addDelegate(_delegate.get());
    _legs->setFlightPlan(fp);
    emit infoChanged();

    _enabled = false;
    emit enabledChanged(_enabled);
}

bool FlightPlanController::loadFromPath(QString path)
{
    auto fp = flightgear::FlightPlan::createRoute();
    bool ok = fp->load(SGPath(path.toUtf8().data()));
    if (!ok) {
        qWarning() << "Failed to load flightplan " << path;
        return false;
    }

    _fp->removeDelegate(_delegate.get());
    _fp = fp;
    _fp->addDelegate(_delegate.get());
    _legs->setFlightPlan(fp);

    _enabled = true;
    emit enabledChanged(_enabled);

    // notify that everything changed
    emit infoChanged();
    return true;
}

bool FlightPlanController::saveToPath(QString path) const
{
    SGPath p(path.toUtf8().data());
    return _fp->save(p);
}

void FlightPlanController::onCollectConfig()
{
    if (!_enabled)
        return;

    SGPath p = globals->get_fg_home() / "launcher.fgfp";
    _fp->save(p);
    
    _config->setArg("flight-plan", p.utf8Str());
}

void FlightPlanController::onSave()
{
    std::ostringstream ss;
    _fp->save(ss);
    _config->setValueForKey("", "fp", QString::fromStdString(ss.str()));
}

void FlightPlanController::onRestore()
{
    _enabled = _config->getValueForKey("", "fp-enabled", false).toBool();
    emit enabledChanged(_enabled);

    // if the user specified --flight-plan, load that one
    auto options = flightgear::Options::sharedInstance();
    std::string fpArgPath = options->valueForOption("flight-plan");
    SGPath fp = SGPath::fromUtf8(fpArgPath);
    if (fp.exists()) {
        loadFromPath(QString::fromStdString(fpArgPath));
    } else {
        std::string planXML = _config->getValueForKey("", "fp", QString()).toString().toStdString();
        if (!planXML.empty()) {
            std::istringstream ss(planXML);
            _fp->load(ss);
            emit infoChanged();
        }
    }
}

QuantityValue FlightPlanController::cruiseAltitude() const
{
    if (_fp->cruiseFlightLevel() > 0) {
        return {Units::FlightLevel, _fp->cruiseFlightLevel()};
    }

    if (_fp->cruiseAltitudeM() > 0) {
        return {Units::MetersMSL, _fp->cruiseAltitudeM()};
    }

    return {Units::FeetMSL, _fp->cruiseAltitudeFt()};
}

void FlightPlanController::setCruiseAltitude(QuantityValue alt)
{
    const int ival = static_cast<int>(alt.value);
    switch (alt.unit) {
        case Units::FlightLevel: {
            if (_fp->cruiseFlightLevel() == ival) {
                return;
            }
            _fp->setCruiseFlightLevel(ival);
            break;
        }
        case Units::FeetMSL: {
            if (_fp->cruiseAltitudeFt() == ival) {
                return;
            }
            _fp->setCruiseAltitudeFt(ival);
            break;
        }
        case Units::MetersMSL: {
            if (_fp->cruiseAltitudeM() == ival) {
                return;
            }
            _fp->setCruiseAltitudeM(ival);
            break;
        }
        default:
            qWarning() << "Unsupported cruise altitude units" << alt.unit;
            break;
    }

    emit infoChanged();
}

QString FlightPlanController::description() const
{
    if (_fp->numLegs() == 0) {
        return tr("No flight-plan");
    }

    return tr("From %1 (%2) to %3 (%4)")
        .arg(departure()->ident())
        .arg(departure()->name())
        .arg(destination()->ident())
        .arg(destination()->name());
}

QmlPositioned *FlightPlanController::departure() const
{
    if (!_fp->departureAirport())
        return new QmlPositioned;

    return new QmlPositioned(_fp->departureAirport());
}

QmlPositioned *FlightPlanController::destination() const
{
    if (!_fp->destinationAirport())
        return new QmlPositioned;

    return new QmlPositioned(_fp->destinationAirport());
}

QmlPositioned *FlightPlanController::alternate() const
{
    if (!_fp->alternate())
        return new QmlPositioned;

    return new QmlPositioned(_fp->alternate());
}

QuantityValue FlightPlanController::cruiseSpeed() const
{
    if (_fp->cruiseSpeedMach() > 0.0) {
        return {Units::Mach, _fp->cruiseSpeedMach()};
    }

    if (_fp->cruiseSpeedKPH() > 0) {
        return {Units::KilometersPerHour, _fp->cruiseSpeedKPH()};
    }

    return {Units::Knots, _fp->cruiseSpeedKnots()};
}

FlightPlanController::FlightRules FlightPlanController::flightRules() const
{
    return static_cast<FlightRules>(_fp->flightRules());
}

FlightPlanController::FlightType FlightPlanController::flightType() const
{
    return static_cast<FlightType>(_fp->flightType());
}

void FlightPlanController::setFlightRules(FlightRules r)
{
    _fp->setFlightRules(static_cast<flightgear::ICAOFlightRules>(r));
}

void FlightPlanController::setFlightType(FlightType ty)
{
    _fp->setFlightType(static_cast<flightgear::ICAOFlightType>(ty));
}

QString FlightPlanController::callsign() const
{
    return QString::fromStdString(_fp->callsign());
}

QString FlightPlanController::remarks() const
{
    return QString::fromStdString(_fp->remarks());
}

QString FlightPlanController::aircraftType() const
{
    return QString::fromStdString(_fp->icaoAircraftType());
}

void FlightPlanController::setCallsign(QString s)
{
    const auto stdS = s.toStdString();
    if (_fp->callsign() == stdS)
        return;
    
    _fp->setCallsign(stdS);
    emit infoChanged();
}

void FlightPlanController::setRemarks(QString r)
{
    const auto stdR = r.toStdString();
    if (_fp->remarks() == stdR)
        return;

    _fp->setRemarks(stdR);
    emit infoChanged();
}

void FlightPlanController::setAircraftType(QString ty)
{
    const auto stdT = ty.toStdString();
    if (_fp->icaoAircraftType() == stdT)
        return;

    _fp->setIcaoAircraftType(stdT);
    emit infoChanged();
}

int FlightPlanController::estimatedDurationMinutes() const
{
    return _fp->estimatedDurationMinutes();
}

QuantityValue FlightPlanController::totalDistanceNm() const
{
    return QuantityValue{Units::NauticalMiles, _fp->totalDistanceNm()};
}

bool FlightPlanController::tryParseRoute(QString routeDesc)
{
    bool ok = _fp->parseICAORouteString(routeDesc.toStdString());
    return ok;
}

bool FlightPlanController::tryGenerateRoute()
{
    if (!_fp->departureAirport() || !_fp->destinationAirport()) {
        qWarning() << "departure or destination not set";

        return false;
    }

    auto net = Airway::highLevel();
    auto fromNode = net->findClosestNode(_fp->departureAirport()->geod());
    auto toNode = net->findClosestNode(_fp->destinationAirport()->geod());
    if (!fromNode.first) {
        qWarning() << "Couldn't find airway network transition for "
                   << QString::fromStdString(_fp->departureAirport()->ident());
        return false;
    }

    if (!toNode.first) {
        qWarning() << "Couldn't find airway network transition for "
                   << QString::fromStdString(_fp->destinationAirport()->ident());
        return false;
    }

    WayptRef fromWp = new NavaidWaypoint(fromNode.first, _fp);
    WayptRef toWp = new NavaidWaypoint(toNode.first, _fp);
    WayptVec path;
    bool ok = net->route(fromWp, toWp, path);
    if (!ok) {
        qWarning() << "unable to find a route";
        return false;
    }

    _fp->clearLegs();
    _fp->insertWayptAtIndex(fromWp, -1);
    _fp->insertWayptsAtIndex(path, -1);
    _fp->insertWayptAtIndex(toWp, -1);

    return true;
}

void FlightPlanController::clearRoute()
{
    _fp->clearAll();
}

QString FlightPlanController::icaoRoute() const
{
    return QString::fromStdString(_fp->asICAORouteString());
}

void FlightPlanController::setEstimatedDurationMinutes(int mins)
{
    if (_fp->estimatedDurationMinutes() == mins)
        return;

    _fp->setEstimatedDurationMinutes(mins);
    emit infoChanged();
}

void FlightPlanController::computeDuration()
{
    _fp->computeDurationMinutes();
    emit infoChanged();
}

bool FlightPlanController::loadPlan()
{
    auto settings = flightgear::getQSettings();
    QString lastUsedDir = settings.value("flightplan-lastdir", "").toString();

    QString file = QFileDialog::getOpenFileName(nullptr, tr("Load a flight-plan"),
                                                lastUsedDir, "*.fgfp *.gpx");
    if (file.isEmpty())
        return false;

    QFileInfo fi(file);
    settings.setValue("flightplan-lastdir", fi.absolutePath());

    return loadFromPath(file);
}

void FlightPlanController::savePlan()
{
    auto settings = flightgear::getQSettings();
    QString lastUsedDir = settings.value("flightplan-lastdir", "").toString();

    QString file = QFileDialog::getSaveFileName(nullptr, tr("Save flight-plan"),
                                                lastUsedDir, "*.fgfp");
    if (file.isEmpty())
        return;
    if (!file.endsWith(".fgfp")) {
        file += ".fgfp";
    }

    QFileInfo fi(file);
    settings.setValue("flightplan-lastdir", fi.absolutePath());

    saveToPath(file);
}

void FlightPlanController::setDeparture(QmlPositioned *apt)
{
    if (!apt) {
        _fp->clearDeparture();
    } else {
        if (apt->inner() == _fp->departureAirport())
            return;

        _fp->setDeparture(fgpositioned_cast<FGAirport>(apt->inner()));
    }

    emit infoChanged();
}

void FlightPlanController::setDestination(QmlPositioned *apt)
{
    if (apt) {
        if (apt->inner() == _fp->destinationAirport())
            return;

        _fp->setDestination(fgpositioned_cast<FGAirport>(apt->inner()));
    } else {
        _fp->clearDestination();

    }
    emit infoChanged();
}

void FlightPlanController::setAlternate(QmlPositioned *apt)
{
    if (apt) {
        if (apt->inner() == _fp->alternate())
            return;

        _fp->setAlternate(fgpositioned_cast<FGAirport>(apt->inner()));
    } else {
        _fp->setAlternate(nullptr);

    }
    emit infoChanged();
}

void FlightPlanController::setCruiseSpeed(QuantityValue speed)
{
    switch (speed.unit) {
        case Units::Mach: {
            if (speed == QuantityValue(Units::Mach, _fp->cruiseSpeedMach())) {
                return;
            }

            _fp->setCruiseSpeedMach(speed.value);
            break;
        }
        case Units::Knots: {
            const int knotsVal = static_cast<int>(speed.value);
            if (_fp->cruiseSpeedKnots() == knotsVal) {
                return;
            }

            _fp->setCruiseSpeedKnots(knotsVal);
            break;
        }
        case Units::KilometersPerHour: {
            const int kmhVal = static_cast<int>(speed.value);
            if (_fp->cruiseSpeedKPH() == kmhVal) {
                return;
            }

            _fp->setCruiseSpeedKPH(kmhVal);
            break;
        }
        default:
            qWarning() << "Unsupported cruise speed units" << speed.unit;
            break;
    }

    emit infoChanged();
}
