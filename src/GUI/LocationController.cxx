// LocationController.cxx - GUI launcher dialog using Qt5
//
// Written by James Turner, started October 2015.
//
// Copyright (C) 2015 James Turner <zakalawe@mac.com>
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

#include "LocationController.hxx"

#include <QSettings>
#include <QAbstractListModel>
#include <QTimer>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>

#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>

#include "AirportDiagram.hxx"
#include "NavaidDiagram.hxx"
#include "LaunchConfig.hxx"
#include "DefaultAircraftLocator.hxx"

#include <Airports/airport.hxx>
#include <Airports/groundnetwork.hxx>

#include <Main/globals.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx> // for fgSetDouble

using namespace flightgear;

const unsigned int MAX_RECENT_LOCATIONS = 64;

QString fixNavaidName(QString s)
{
    // split into words
    QStringList words = s.split(QChar(' '));
    QStringList changedWords;
    Q_FOREACH(QString w, words) {
        QString up = w.toUpper();

        // expand common abbreviations
        // note these are not translated, since they are abbreivations
        // for English-langauge airports, mostly in the US/Canada
        if (up == "FLD") {
            changedWords.append("Field");
            continue;
        }

        if (up == "CO") {
            changedWords.append("County");
            continue;
        }

        if ((up == "MUNI") || (up == "MUN")) {
            changedWords.append("Municipal");
            continue;
        }

        if (up == "MEM") {
            changedWords.append("Memorial");
            continue;
        }

        if (up == "RGNL") {
            changedWords.append("Regional");
            continue;
        }

        if (up == "CTR") {
            changedWords.append("Center");
            continue;
        }

        if (up == "INTL") {
            changedWords.append("International");
            continue;
        }

        // occurs in many Australian airport names in our DB
        if (up == "(NSW)") {
            changedWords.append("(New South Wales)");
            continue;
        }

        if ((up == "VOR") || (up == "NDB")
                || (up == "VOR-DME") || (up == "VORTAC")
                || (up == "NDB-DME")
                || (up == "AFB") || (up == "RAF"))
        {
            changedWords.append(w);
            continue;
        }

        if ((up =="[X]") || (up == "[H]") || (up == "[S]")) {
            continue; // consume
        }

        QChar firstChar = w.at(0).toUpper();
        w = w.mid(1).toLower();
        w.prepend(firstChar);

        changedWords.append(w);
    }

    return changedWords.join(QChar(' '));
}

QVariant savePositionList(const FGPositionedList& posList)
{
    QVariantList vl;
    FGPositionedList::const_iterator it;
    for (it = posList.begin(); it != posList.end(); ++it) {
        QVariantMap vm;
        FGPositionedRef pos = *it;
        vm.insert("ident", QString::fromStdString(pos->ident()));
        vm.insert("type", pos->type());
        vm.insert("lat", pos->geod().getLatitudeDeg());
        vm.insert("lon", pos->geod().getLongitudeDeg());
        vl.append(vm);
    }
    return vl;
}

FGPositionedList loadPositionedList(QVariant v)
{
    QVariantList vl = v.toList();
    FGPositionedList result;
    result.reserve(vl.size());
    NavDataCache* cache = NavDataCache::instance();

    Q_FOREACH(QVariant v, vl) {
        QVariantMap vm = v.toMap();
        std::string ident(vm.value("ident").toString().toStdString());
        double lat = vm.value("lat").toDouble();
        double lon = vm.value("lon").toDouble();
        FGPositioned::Type ty(static_cast<FGPositioned::Type>(vm.value("type").toInt()));
        FGPositioned::TypeFilter filter(ty);
        FGPositionedRef pos = cache->findClosestWithIdent(ident,
                                                          SGGeod::fromDeg(lon, lat),
                                                          &filter);
        if (pos)
            result.push_back(pos);
    }

    return result;
}

class IdentSearchFilter : public FGPositioned::TypeFilter
{
public:
    IdentSearchFilter(LauncherController::AircraftType aircraft)
    {
        addType(FGPositioned::VOR);
        addType(FGPositioned::FIX);
        addType(FGPositioned::NDB);

        if (aircraft == LauncherController::Helicopter) {
            addType(FGPositioned::HELIPAD);
        }

        if (aircraft == LauncherController::Seaplane) {
            addType(FGPositioned::SEAPORT);
        } else {
            addType(FGPositioned::AIRPORT);
        }
    }
};

class NavSearchModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool isSearchActive READ isSearchActive NOTIFY searchActiveChanged)
    Q_PROPERTY(bool haveExistingSearch READ haveExistingSearch NOTIFY haveExistingSearchChanged)

    enum Roles {
        GeodRole = Qt::UserRole + 1,
        GuidRole = Qt::UserRole + 2,
        IdentRole = Qt::UserRole + 3,
        NameRole = Qt::UserRole + 4,
        IconRole = Qt::UserRole + 5,
        TypeRole = Qt::UserRole + 6,
        NavFrequencyRole = Qt::UserRole + 7
    };

public:
    NavSearchModel() { }

    enum AircraftType
    {
        Airplane = LauncherController::Airplane,
        Seaplane = LauncherController::Seaplane,
        Helicopter = LauncherController::Helicopter,
        Airship = LauncherController::Airship
    };

    Q_ENUMS(AircraftType)

    Q_INVOKABLE void setSearch(QString t, AircraftType aircraft)
    {
        beginResetModel();

        m_items.clear();
        m_ids.clear();

        std::string term(t.toUpper().toStdString());

        IdentSearchFilter filter(static_cast<LauncherController::AircraftType>(aircraft));
        FGPositionedList exactMatches = NavDataCache::instance()->findAllWithIdent(term, &filter, true);
        m_ids.reserve(exactMatches.size());
        m_items.reserve(exactMatches.size());
        for (auto match : exactMatches) {
            m_ids.push_back(match->guid());
            m_items.push_back(match);
        }
        endResetModel();

        m_search.reset(new NavDataCache::ThreadedGUISearch(term));
        QTimer::singleShot(100, this, SLOT(onSearchResultsPoll()));
        m_searchActive = true;
        emit searchActiveChanged();
        emit haveExistingSearchChanged();
    }

    bool isSearchActive() const
    {
        return m_searchActive;
    }

    bool haveExistingSearch() const
    {
        return m_searchActive || (!m_items.empty());
    }

    int rowCount(const QModelIndex&) const override
    {
        return m_ids.size();
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        FGPositionedRef pos = itemAtRow(index.row());
        switch (role) {
        case GuidRole: return static_cast<qlonglong>(pos->guid());
        case IdentRole: return QString::fromStdString(pos->ident());
        case NameRole:
            return fixNavaidName(QString::fromStdString(pos->name()));

        case NavFrequencyRole: {
            FGNavRecord* nav = fgpositioned_cast<FGNavRecord>(pos);
            return nav ? nav->get_freq() : 0;
        }

        case TypeRole: return static_cast<QmlPositioned::Type>(pos->type());
        case IconRole:
            return AirportDiagram::iconForPositioned(pos,
                                                     AirportDiagram::SmallIcons | AirportDiagram::LargeAirportPlans);
        }

        return {};
    }

    FGPositionedRef itemAtRow(unsigned int row) const
    {
        FGPositionedRef pos = m_items[row];
        if (!pos.valid()) {
            pos = NavDataCache::instance()->loadById(m_ids[row]);
            m_items[row] = pos;
        }

        return pos;
    }

    void setItems(const FGPositionedList& items)
    {
        beginResetModel();
        m_searchActive = false;
        m_items = items;

        m_ids.clear();
        for (unsigned int i=0; i < items.size(); ++i) {
            m_ids.push_back(m_items[i]->guid());
        }

        endResetModel();
        emit searchActiveChanged();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> result = QAbstractListModel::roleNames();

        result[GeodRole] = "geod";
        result[GuidRole] = "guid";
        result[IdentRole] = "ident";
        result[NameRole] = "name";
        result[IconRole] = "icon";
        result[TypeRole] = "type";
        result[NavFrequencyRole] = "frequency";
        return result;
    }


Q_SIGNALS:
    void searchComplete();
    void searchActiveChanged();
    void haveExistingSearchChanged();

private slots:

    void onSearchResultsPoll()
    {
        if (m_search.isNull()) {
            return;
        }
        
        PositionedIDVec newIds = m_search->results();
        if (!newIds.empty()) {
            m_ids.reserve(newIds.size());
            beginInsertRows(QModelIndex(), m_ids.size(), newIds.size() - 1);
            for (auto id : newIds) {
                m_ids.push_back(id);
                m_items.push_back({}); // null ref
            }
            endInsertRows();
        }

        if (m_search->isComplete()) {
            m_searchActive = false;
            m_search.reset();
            emit searchComplete();
            emit searchActiveChanged();
            emit haveExistingSearchChanged();
        } else {
            QTimer::singleShot(100, this, SLOT(onSearchResultsPoll()));
        }
    }

private:
    PositionedIDVec m_ids;
    mutable FGPositionedList m_items;
    bool m_searchActive = false;
    QScopedPointer<NavDataCache::ThreadedGUISearch> m_search;
};


LocationController::LocationController(QObject *parent) :
    QObject(parent)
{
    qmlRegisterUncreatableType<NavSearchModel>("FlightGear.Launcher", 1, 0, "NavSearchModel", "no");
    m_searchModel = new NavSearchModel;

    m_detailQml = new QmlPositioned(this);
    m_baseQml = new QmlPositioned(this);

    // chain location and offset updated to description
    connect(this, &LocationController::baseLocationChanged,
            this, &LocationController::descriptionChanged);
    connect(this, &LocationController::configChanged,
            this, &LocationController::descriptionChanged);
    connect(this, &LocationController::offsetChanged,
            this, &LocationController::descriptionChanged);
}

LocationController::~LocationController()
{
}

void LocationController::setLaunchConfig(LaunchConfig *config)
{
    m_config = config;
    connect(m_config, &LaunchConfig::collect, this, &LocationController::onCollectConfig);

    connect(m_config, &LaunchConfig::save, this, &LocationController::onSaveCurrentLocation);
    connect(m_config, &LaunchConfig::restore, this, &LocationController::onRestoreCurrentLocation);

}

void LocationController::restoreSearchHistory()
{
    QSettings settings;
    m_recentLocations = loadPositionedList(settings.value("recent-locations"));
}

void LocationController::onRestoreCurrentLocation()
{
    QVariantMap vm = m_config->getValueForKey("", "current-location", QVariantMap()).toMap();
    if (vm.empty())
        return;

    restoreLocation(vm);
}

void LocationController::onSaveCurrentLocation()
{
    m_config->setValueForKey("", "current-location", saveLocation());
}

bool LocationController::isParkedLocation() const
{
    if (m_airportLocation) {
        if (m_detailLocation && (m_detailLocation->type() == FGPositioned::PARKING)) {
            return true;
        }
    }

    // treat all other ground starts as taxi or on runway, i.e engines
    // running if possible
    return false;
}

bool LocationController::isAirborneLocation() const
{
    const bool altIsPositive = (m_altitudeFt > 0);

    if (m_locationIsLatLon) {
        return (m_altitudeType != AltitudeType::Off) && altIsPositive;
    }

    if (m_airportLocation) {
        const bool onRunway =
                (m_detailLocation && (m_detailLocation->type() == FGPositioned::RUNWAY)) ||
                m_useActiveRunway;

        if (onRunway && m_onFinal) {
            // in this case no altitude might be set, but we assume
            // it's still an airborne position
            return true;
        }

        return false;
    }

    // relative to a navaid or fix - base off altitude.
    return (m_altitudeType != AltitudeType::Off) && altIsPositive;
}

int LocationController::offsetRadial() const
{
    return m_offsetRadial;
}

void LocationController::setBaseGeod(QmlGeod geod)
{
    if (m_locationIsLatLon && (m_geodLocation == geod.geod()))
        return;

    m_locationIsLatLon = true;
    m_geodLocation = geod.geod();
    m_location.clear();
    m_airportLocation.clear();
    m_detailLocation.clear();
    emit baseLocationChanged();
}

void LocationController::setBaseLocation(QmlPositioned* pos)
{
    if (!pos) {
        m_location.clear();
        m_detailLocation.clear();
        m_detailQml->setGuid(0);
        m_baseQml->setGuid(0);
        m_airportLocation.clear();
        m_locationIsLatLon = false;
        emit baseLocationChanged();
        return;
    }

    if (pos->inner() == m_location)
        return;

    m_locationIsLatLon = false;
    m_location = pos->inner();
    m_baseQml->setGuid(pos->guid());
    m_detailLocation.clear();
    m_detailQml->setGuid(0);

    if (FGPositioned::isAirportType(m_location.ptr())) {
        m_airportLocation = static_cast<FGAirport*>(m_location.ptr());
        // disable offset when selecting a heliport
        if (m_airportLocation->isHeliport()) {
            qInfo() << "disabling offset";
            m_onFinal = false;
        }
    } else {
        m_airportLocation.clear();
    }

    emit offsetChanged();
    emit baseLocationChanged();
}

void LocationController::setDetailLocation(QmlPositioned* pos)
{
    if (pos && (pos->inner() == m_detailLocation))
        return;

    if (!pos) {
        m_detailLocation.clear();
        m_detailQml->setInner({});
    } else {
        m_detailLocation = pos->inner();
        m_useActiveRunway = false;
        m_detailQml->setInner(pos->inner());
    }

    emit configChanged();
}

QmlGeod LocationController::baseGeod() const
{
    if (m_locationIsLatLon)
        return m_geodLocation;

    if (m_location)
        return QmlGeod(m_location->geod());

    return {};
}

bool LocationController::isAirportLocation() const
{
    return m_airportLocation;
}

void LocationController::setUseActiveRunway(bool b)
{
    if (b == m_useActiveRunway)
        return;

    m_useActiveRunway = b;
    if (m_useActiveRunway) {
        m_detailLocation.clear(); // clear any specific runway
    }
    emit configChanged();
}

void LocationController::addToRecent(QmlPositioned* pos)
{
    addToRecent(pos->inner());
}

QObjectList LocationController::airportRunways() const
{
    if (!m_airportLocation)
        return {};

    QObjectList result;
    if (m_airportLocation->isHeliport()) {
        // helipads
        for (unsigned int r=0; r<m_airportLocation->numHelipads(); ++r) {
            auto p = new QmlPositioned(m_airportLocation->getHelipadByIndex(r).ptr());
            QQmlEngine::setObjectOwnership(p, QQmlEngine::JavaScriptOwnership);
            result.push_back(p);
        }
    } else {
        // regular runways
        for (unsigned int r=0; r<m_airportLocation->numRunways(); ++r) {
            auto p = new QmlPositioned(m_airportLocation->getRunwayByIndex(r).ptr());
            QQmlEngine::setObjectOwnership(p, QQmlEngine::JavaScriptOwnership);
            result.push_back(p);
        }
    }

    return result;
}

QObjectList LocationController::airportParkings() const
{
    if (!m_airportLocation)
        return {};

    QObjectList result;
    for (auto park : m_airportLocation->groundNetwork()->allParkings()) {
        auto p = new QmlPositioned(park);
        QQmlEngine::setObjectOwnership(p, QQmlEngine::JavaScriptOwnership);
        result.push_back(p);
    }
    return result;
}

void LocationController::showHistoryInSearchModel()
{
    // prepend the default location
    FGPositionedList locs = m_recentLocations;
    const std::string defaultICAO = flightgear::defaultAirportICAO();

    auto it = std::find_if(locs.begin(), locs.end(), [defaultICAO](FGPositionedRef pos) {
        return pos->ident() == defaultICAO;
    });

    if (it == locs.end()) {
        FGAirportRef apt = FGAirport::findByIdent(defaultICAO);
        locs.insert(locs.begin(), apt);
    }

    m_searchModel->setItems(locs);
}

QmlGeod LocationController::parseStringAsGeod(QString string) const
{
    SGGeod g;
    if (!simgear::strutils::parseStringAsGeod(string.toStdString(), &g)) {
        return {};
    }

    return QmlGeod(g);
}

QmlPositioned *LocationController::detail() const
{
    return m_detailQml;
}

QmlPositioned *LocationController::baseLocation() const
{
    return m_baseQml;
}

void LocationController::setOffsetRadial(int offsetRadial)
{
    if (m_offsetRadial == offsetRadial)
        return;

    m_offsetRadial = offsetRadial;
    emit offsetChanged();
}

void LocationController::setOffsetNm(double offsetNm)
{
    if (qFuzzyCompare(m_offsetNm, offsetNm))
        return;

    m_offsetNm = offsetNm;
    emit offsetChanged();
}

void LocationController::setOffsetEnabled(bool offsetEnabled)
{
    if (m_offsetEnabled == offsetEnabled)
        return;

    m_offsetEnabled = offsetEnabled;
    emit offsetChanged();
}

void LocationController::setOnFinal(bool onFinal)
{
    if (m_onFinal == onFinal)
        return;

    m_onFinal = onFinal;
    emit configChanged();
}

void LocationController::setTuneNAV1(bool tuneNAV1)
{
    if (m_tuneNAV1 == tuneNAV1)
        return;

    m_tuneNAV1 = tuneNAV1;
    emit configChanged();
}

void LocationController::setUseAvailableParking(bool useAvailableParking)
{
    if (m_useAvailableParking == useAvailableParking)
        return;

    m_useAvailableParking = useAvailableParking;
    if (m_useAvailableParking) {
        m_detailLocation.clear(); // clear any specific runway
    }
    emit configChanged();
}

void LocationController::restoreLocation(QVariantMap l)
{
    try {
        if (l.contains("location-lat")) {
            m_locationIsLatLon = true;
            m_geodLocation = SGGeod::fromDeg(l.value("location-lon").toDouble(),
                                             l.value("location-lat").toDouble());
        } else if (l.contains("location-id")) {
            m_location = NavDataCache::instance()->loadById(l.value("location-id").toULongLong());
            m_locationIsLatLon = false;
            if (FGPositioned::isAirportType(m_location.ptr())) {
                m_airportLocation = static_cast<FGAirport*>(m_location.ptr());
            } else {
                m_airportLocation.clear();
            }
            m_baseQml->setInner(m_location);
        }

        if (l.contains("altitude-type")) {
            m_altitudeFt = l.value("altitude", 6000).toInt();
            m_flightLevel = l.value("flight-level").toInt();
            m_altitudeType = static_cast<AltitudeType>(l.value("altitude-type").toInt());
        } else {
            m_altitudeType = Off;
        }

        m_speedEnabled = l.contains("speed");
        m_headingEnabled = l.contains("heading");

        m_airspeedKnots = l.value("speed", 120).toInt();
        m_headingDeg = l.value("heading").toInt();

        m_offsetEnabled = l.value("offset-enabled").toBool();
        m_offsetRadial = l.value("offset-bearing").toInt();
        m_offsetNm = l.value("offset-distance", 10).toInt();
        m_tuneNAV1 = l.value("tune-nav1-radio").toBool();

        if (m_airportLocation) {
            m_useActiveRunway = false;
            m_detailLocation.clear();

            if (l.contains("location-apt-runway")) {
                QString runway = l.value("location-apt-runway").toString().toUpper();
                if (runway == QStringLiteral("ACTIVE")) {
                    m_useActiveRunway = true;
                } else if (m_airportLocation->isHeliport()) {
                    m_detailLocation = m_airportLocation->getHelipadByIdent(runway.toStdString());
                } else {
                    m_detailLocation = m_airportLocation->getRunwayByIdent(runway.toStdString());
                }
            } else if (l.contains("location-apt-parking")) {
                QString parking = l.value("location-apt-parking").toString();
                m_detailLocation = m_airportLocation->groundNetwork()->findParkingByName(parking.toStdString());
            }

            if (m_detailLocation) {
                m_detailQml->setInner(m_detailLocation);
            }

            m_onFinal = l.value("location-on-final").toBool();
            m_offsetNm = l.value("location-apt-final-distance").toInt();
        } // of location is an airport
    } catch (const sg_exception&) {
        qWarning() << "Errors restoring saved location, clearing";
        m_location.clear();
        m_airportLocation.clear();
        m_baseQml->setInner(nullptr);
        m_offsetEnabled = false;
    }

    baseLocationChanged();
    configChanged();
    offsetChanged();
}

bool LocationController::shouldStartPaused() const
{
    if (!m_location) {
        return false; // defaults to on-ground at the default airport
    }

    if (m_airportLocation) {
        return m_onFinal;
    } else {
        // navaid, start paused
        return true;
    }

    return false;
}

QVariantMap LocationController::saveLocation() const
{
    QVariantMap locationSet;
    if (m_locationIsLatLon) {
        locationSet.insert("location-lat", m_geodLocation.getLatitudeDeg());
        locationSet.insert("location-lon", m_geodLocation.getLongitudeDeg());
    } else if (m_location) {
        locationSet.insert("location-id", static_cast<qlonglong>(m_location->guid()));

        if (m_airportLocation) {
            locationSet.insert("location-on-final", m_onFinal);
            locationSet.insert("location-apt-final-distance", m_offsetNm);
            if (m_useActiveRunway) {
                locationSet.insert("location-apt-runway", "ACTIVE");
            } else if (m_detailLocation) {
                const auto detailType = m_detailLocation->type();
                if (detailType == FGPositioned::RUNWAY) {
                    locationSet.insert("location-apt-runway", QString::fromStdString(m_detailLocation->ident()));
                } else if (detailType == FGPositioned::PARKING) {
                    locationSet.insert("location-apt-parking", QString::fromStdString(m_detailLocation->ident()));
                }
            }
        } // of location is an airport
    } // of m_location is valid

    if (m_altitudeType != Off) {
        locationSet.insert("altitude-type", m_altitudeType);

        if ((m_altitudeType == MSL_Feet) || (m_altitudeType == AGL_Feet)) {
            locationSet.insert("altitude", m_altitudeFt);
        }

        if (m_altitudeType == FlightLevel) {
            locationSet.insert("flight-level", m_flightLevel);
        }
    }

    if (m_speedEnabled) {
        locationSet.insert("speed", m_airspeedKnots);
    }

    if (m_headingEnabled) {
        locationSet.insert("heading", m_headingDeg);
    }

    if (m_offsetEnabled) {
        locationSet.insert("offset-enabled", m_offsetEnabled);
        locationSet.insert("offset-bearing", m_offsetRadial);
        locationSet.insert("offset-distance",m_offsetNm);
    }

    locationSet.insert("text", description());
    locationSet.insert("tune-nav1-radio", m_tuneNAV1);

    return locationSet;
}

void LocationController::setLocationProperties()
{
    SGPropertyNode_ptr presets = fgGetNode("/sim/presets", true);

    QStringList props = QStringList() << "vor-id" << "fix" << "ndb-id" <<
        "runway-requested" << "navaid-id" << "offset-azimuth-deg" <<
        "offset-distance-nm" << "glideslope-deg" <<
        "speed-set" << "on-ground" << "airspeed-kt" <<
        "airport-id" << "runway" << "parkpos";

    Q_FOREACH(QString s, props) {
        SGPropertyNode* c = presets->getChild(s.toStdString());
        if (c) {
            c->clearValue();
        }
    }

    if (m_locationIsLatLon) {
        fgSetDouble("/sim/presets/latitude-deg", m_geodLocation.getLatitudeDeg());
        fgSetDouble("/position/latitude-deg", m_geodLocation.getLatitudeDeg());
        fgSetDouble("/sim/presets/longitude-deg", m_geodLocation.getLongitudeDeg());
        fgSetDouble("/position/longitude-deg", m_geodLocation.getLongitudeDeg());

        applyPositionOffset();
        return;
    }

    fgSetDouble("/sim/presets/latitude-deg", 9999.0);
    fgSetDouble("/sim/presets/longitude-deg", 9999.0);
    fgSetDouble("/sim/presets/altitude-ft", -9999.0);
    fgSetDouble("/sim/presets/heading-deg", 9999.0);

    if (!m_location) {
        return;
    }

    if (m_airportLocation) {
        fgSetString("/sim/presets/airport-id", m_airportLocation->ident());
        fgSetBool("/sim/presets/on-ground", true);
        fgSetBool("/sim/presets/airport-requested", true);

        const bool onRunway = (m_detailLocation && (m_detailLocation->type() == FGPositioned::RUNWAY));
        const bool atParking = (m_detailLocation && (m_detailLocation->type() == FGPositioned::PARKING));
        if (m_useActiveRunway) {
            // automatic runway choice
            // we can't set navaid here
        } else if (onRunway) {
            if (m_airportLocation->type() == FGPositioned::AIRPORT) {
                // explicit runway choice
                fgSetString("/sim/presets/runway", m_detailLocation->ident() );
                fgSetBool("/sim/presets/runway-requested", true );

                // set nav-radio 1 based on selected runway
                FGRunway* runway = static_cast<FGRunway*>(m_detailLocation.ptr());
                if (m_tuneNAV1 && runway->ILS()) {
                    double mhz = runway->ILS()->get_freq() / 100.0;
                    fgSetDouble("/instrumentation/nav[0]/radials/selected-deg", runway->headingDeg());
                    fgSetDouble("/instrumentation/nav[0]/frequencies/selected-mhz", mhz);
                }

                if (m_onFinal) {
                    fgSetDouble("/sim/presets/glideslope-deg", 3.0);
                    fgSetDouble("/sim/presets/offset-distance-nm", m_offsetNm);
                    fgSetBool("/sim/presets/on-ground", false);
                }
            } else if (m_airportLocation->type() == FGPositioned::HELIPORT) {
                // explicit pad choice
                fgSetString("/sim/presets/runway", m_detailLocation->ident() );
                fgSetBool("/sim/presets/runway-requested", true );
            }
        } else if (atParking) {
            // parking selection
            fgSetString("/sim/presets/parkpos", m_detailLocation->ident());
        }
        // of location is an airport
    } else {
        fgSetString("/sim/presets/airport-id", "");

        // location is a navaid
        // note setting the ident here is ambigious, we really only need and
        // want the 'navaid-id' property. However setting the 'real' option
        // gives a better UI experience (eg existing Position in Air dialog)
        FGPositioned::Type ty = m_location->type();
        switch (ty) {
            case FGPositioned::VOR:
                fgSetString("/sim/presets/vor-id", m_location->ident());
                setNavRadioOption();
                break;

            case FGPositioned::NDB:
                fgSetString("/sim/presets/ndb-id", m_location->ident());
                setNavRadioOption();
                break;

            case FGPositioned::FIX:
                fgSetString("/sim/presets/fix", m_location->ident());
                break;
            default:
                break;
        };
        
        // set disambiguation property
        globals->get_props()->setIntValue("/sim/presets/navaid-id",
                                          static_cast<int>(m_location->guid()));
        
        applyPositionOffset();
    } // of navaid location
}

void LocationController::applyPositionOffset()
{
    switch (m_altitudeType) {
    case Off:
        break;
    case MSL_Feet:
        m_config->setArg("altitude", QString::number(m_altitudeFt));
        break;

    case AGL_Feet:
        // fixme - allow the sim to accpet AGL start position
        m_config->setArg("altitude", QString::number(m_altitudeFt));
        break;

    case FlightLevel:
        // FIXME - allow the sim to accept real FlightLevel arguments
        m_config->setArg("altitude", QString::number(m_flightLevel * 100));
        break;
    }

    if (m_speedEnabled) {
        m_config->setArg("vc", QString::number(m_airspeedKnots));
    }

    if (m_headingEnabled) {
        m_config->setArg("heading", QString::number(m_headingDeg));
    }

    if (m_offsetEnabled) {
        // flip direction of azimuth to balance the flip done in fgApplyStartOffset
        // I don't know why that flip exists but changing it there will break
        // command-line compatability so compensating here instead
        int offsetAzimuth = m_offsetRadial - 180;
        m_config->setArg("offset-azimuth", QString::number(offsetAzimuth));
        m_config->setArg("offset-distance", QString::number(m_offsetNm));
    }
}

void LocationController::onCollectConfig()
{
    if (m_skipFromArgs) {
        qInfo() << Q_FUNC_INFO << "skipping argument collection";
        return;
    }

    if (m_locationIsLatLon) {
        m_config->setArg("lat", QString::number(m_geodLocation.getLatitudeDeg()));
        m_config->setArg("lon", QString::number(m_geodLocation.getLongitudeDeg()));
        applyPositionOffset();
        return;
    }

    if (!m_location) {
        return;
    }

    if (m_airportLocation) {
        m_config->setArg("airport", QString::fromStdString(m_airportLocation->ident()));
        const bool onRunway = (m_detailLocation && (m_detailLocation->type() == FGPositioned::RUNWAY));
        const bool atParking = (m_detailLocation && (m_detailLocation->type() == FGPositioned::PARKING));

        if (m_useActiveRunway) {
            // pick by default
        } else if (onRunway) {
            if (m_airportLocation->type() == FGPositioned::AIRPORT) {
                m_config->setArg("runway", QString::fromStdString(m_detailLocation->ident()));

                    // set nav-radio 1 based on selected runway
                FGRunway* runway = static_cast<FGRunway*>(m_detailLocation.ptr());
                if (runway->ILS()) {
                    double mhz = runway->ILS()->get_freq() / 100.0;
                    m_config->setArg("nav1", QString("%1:%2").arg(runway->headingDeg()).arg(mhz));
                }

                if (m_onFinal) {
                    m_config->setArg("glideslope", std::string("3.0"));
                    m_config->setArg("offset-distance", QString::number(m_offsetNm));
                    m_config->setArg("on-ground", std::string("false"));

                    if (m_speedEnabled) {
                        m_config->setArg("vc", QString::number(m_airspeedKnots));
                    }
                }
            } else if (m_airportLocation->type() == FGPositioned::HELIPORT) {
                m_config->setArg("runway", QString::fromStdString(m_detailLocation->ident()));
            }
        } else if (atParking) {
            // parking selection
            m_config->setArg("parkpos", QString::fromStdString(m_detailLocation->ident()));
        }
        // of location is an airport
    } else {
        // location is a navaid
        // note setting the ident here is ambigious, we really only need and
        // want the 'navaid-id' property. However setting the 'real' option
        // gives a better UI experience (eg existing Position in Air dialog)
        FGPositioned::Type ty = m_location->type();
        switch (ty) {
            case FGPositioned::VOR:
                m_config->setArg("vor", m_location->ident());
                setNavRadioOption();
                break;

            case FGPositioned::NDB:
                m_config->setArg("ndb", m_location->ident());
                setNavRadioOption();
                break;

            case FGPositioned::FIX:
                 m_config->setArg("fix", m_location->ident());
                break;
            default:
                break;
        };

        // set disambiguation property
        m_config->setProperty("/sim/presets/navaid-id", QString::number(m_location->guid()));
        applyPositionOffset();
    } // of navaid location
}

void LocationController::setNavRadioOption()
{
    if (!m_tuneNAV1)
        return;

    if (m_location->type() == FGPositioned::VOR) {
        FGNavRecordRef nav(static_cast<FGNavRecord*>(m_location.ptr()));
        double mhz = nav->get_freq() / 100.0;
        int heading = 0; // add heading support
        QString navOpt = QString("%1:%2").arg(heading).arg(mhz);
        m_config->setArg("nav1", navOpt);
    } else {
        FGNavRecordRef nav(static_cast<FGNavRecord*>(m_location.ptr()));
        int khz = nav->get_freq() / 100;
        int heading = 0;
        QString adfOpt = QString("%1:%2").arg(heading).arg(khz);
        m_config->setArg("adf1", adfOpt);
    }
}

void LocationController::onAirportRunwayClicked(FGRunwayRef rwy)
{
    // if (rwy) {
    //     m_ui->runwayRadio->setChecked(true);
    //     int rwyIndex = m_ui->runwayCombo->findText(QString::fromStdString(rwy->ident()));
    //     m_ui->runwayCombo->setCurrentIndex(rwyIndex);
    //     m_ui->airportDiagram->setSelectedRunway(rwy);
    // }

    // updateDescription();
}

void LocationController::onAirportParkingClicked(FGParkingRef park)
{
    // if (park) {
    //     m_ui->parkingRadio->setChecked(true);
    //     int parkingIndex = m_ui->parkingCombo->findData(park->getIndex());
    //     m_ui->parkingCombo->setCurrentIndex(parkingIndex);
    //     m_ui->airportDiagram->setSelectedParking(park);
    // }

    // updateDescription();
}

QString compassPointFromHeading(int heading)
{
    const int labelArc = 360 / 8;
    heading += (labelArc >> 1);
    SG_NORMALIZE_RANGE(heading, 0, 359);

    switch (heading / labelArc) {
    case 0: return "N";
    case 1: return "NE";
    case 2: return "E";
    case 3: return "SE";
    case 4: return "S";
    case 5: return "SW";
    case 6: return "W";
    case 7: return "NW";
    }

    return QString();
}

QString LocationController::description() const
{
    if (!m_location) {
        if (m_locationIsLatLon) {
            const auto s = simgear::strutils::formatGeodAsString(m_geodLocation,
                                                                 simgear::strutils::LatLonFormat::DECIMAL_DEGREES,
                                                                 simgear::strutils::DegreeSymbol::UTF8_DEGREE);
            return tr("at position %1").arg(QString::fromStdString(s));
        }

        return tr("No location selected");
    }

    QString ident = QString::fromStdString(m_location->ident()),
        name = QString::fromStdString(m_location->name());

    name = fixNavaidName(name);

    if (m_airportLocation) {
        const bool onRunway = (m_detailLocation && (m_detailLocation->type() == FGPositioned::RUNWAY));
        const bool atParking = (m_detailLocation && (m_detailLocation->type() == FGPositioned::PARKING));
        QString locationOnAirport;

        if (m_useActiveRunway) {
            if (m_onFinal) {
                locationOnAirport = tr("on %1-mile final to active runway").arg(m_offsetNm);
            } else {
                locationOnAirport = tr("on active runway");
            }
        } if (onRunway) {
            QString runwayName = QString("runway %1").arg(QString::fromStdString(m_detailLocation->ident()));

            if (m_onFinal) {
                locationOnAirport = tr("on %2-mile final to %1").arg(runwayName).arg(m_offsetNm);
            } else {
                locationOnAirport = tr("on %1").arg(runwayName);
            }
        } else if (atParking) {
            locationOnAirport = tr("at parking position %1").arg(QString::fromStdString(m_detailLocation->ident()));
        }

        return tr("%2 (%1): %3").arg(ident).arg(name).arg(locationOnAirport);
    } else {
        QString offsetDesc = tr("at");
        if (m_offsetEnabled) {
            offsetDesc = tr("%1nm %2 of").
                    arg(m_offsetNm, 0, 'f', 1).
                    arg(compassPointFromHeading(m_offsetRadial));
        }

        QString navaidType;
        switch (m_location->type()) {
        case FGPositioned::VOR:
            navaidType = QString("VOR"); break;
        case FGPositioned::NDB:
            navaidType = QString("NDB"); break;
        case FGPositioned::FIX:
            return tr("%2 waypoint %1").arg(ident).arg(offsetDesc);
        default:
            // unsupported type
            break;
        }

        return tr("%4 %1 %2 (%3)").arg(navaidType).arg(ident).arg(name).arg(offsetDesc);
    }

    return tr("No location selected");
}


void LocationController::addToRecent(FGPositionedRef pos)
{
    auto it = std::find(m_recentLocations.begin(),
                        m_recentLocations.end(), pos);
    if (it != m_recentLocations.end()) {
        m_recentLocations.erase(it);
    }

    if (m_recentLocations.size() >= MAX_RECENT_LOCATIONS) {
        m_recentLocations.pop_back();
    }

    m_recentLocations.insert(m_recentLocations.begin(), pos);
    QSettings settings;
    settings.setValue("recent-locations", savePositionList(m_recentLocations));
}

#include "LocationController.moc"
