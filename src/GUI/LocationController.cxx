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
#include "CarrierDiagram.hxx"
#include "NavaidDiagram.hxx"
#include "LaunchConfig.hxx"
#include "DefaultAircraftLocator.hxx"
#include "NavaidSearchModel.hxx"
#include "CarriersLocationModel.hxx"

#include <Airports/airport.hxx>
#include <Airports/groundnetwork.hxx>

#include <Main/globals.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx> // for fgSetDouble

using namespace flightgear;

const unsigned int MAX_RECENT_LOCATIONS = 20;

QVariant savePositionList(const FGPositionedList& posList)
{
    QVariantList vl;
    for (const auto& pos : posList) {
        QVariantMap vm;
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
    result.reserve(static_cast<size_t>(vl.size()));
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

LocationController::LocationController(QObject *parent) :
    QObject(parent)
{
    m_searchModel = new NavaidSearchModel(this);
    m_detailQml = new QmlPositioned(this);
    m_baseQml = new QmlPositioned(this);
    m_carriersModel = new CarriersLocationModel(this);

    m_defaultAltitude = QuantityValue{Units::FeetMSL, 6000};
    m_defaultAirspeed = QuantityValue{Units::Knots, 120};
    m_defaultHeading = QuantityValue{Units::DegreesTrue, 0};
    m_defaultOffsetDistance = QuantityValue{Units::NauticalMiles, 1.0};
    m_defaultOffsetRadial = QuantityValue{Units::DegreesTrue, 90};

    // chain location and offset updated to description
    connect(this, &LocationController::baseLocationChanged,
            this, &LocationController::descriptionChanged);
    connect(this, &LocationController::configChanged,
            this, &LocationController::descriptionChanged);
    connect(this, &LocationController::offsetChanged,
            this, &LocationController::descriptionChanged);
}

LocationController::~LocationController() = default;

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
        if (m_useAvailableParking)
            return true;

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
    const bool altIsPositive = (m_altitude.value > 0);

    if (m_locationIsLatLon) {
        return m_altitudeEnabled && altIsPositive;
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
    return m_altitudeEnabled && altIsPositive;
}

QuantityValue LocationController::offsetRadial() const
{
    return m_offsetRadial;
}

void LocationController::setBaseGeod(QmlGeod geod)
{
    if (m_locationIsLatLon && (m_geodLocation == geod.geod()))
        return;

    clearLocation();
    m_locationIsLatLon = true;
    m_geodLocation = geod.geod();
    emit baseLocationChanged();
}

QString LocationController::carrierName() const
{
    return m_carrierName;
}

void LocationController::setCarrierLocation(QString name)
{
    const auto cIndex = m_carriersModel->indexOf(name);
    clearLocation();
    if (cIndex < 0) {
        qWarning() << "invalid carrier name:" << name;
        return;
    }

    m_locationIsCarrier = true;
    m_carrierName = name;
    m_geodLocation = m_carriersModel->geodForIndex(cIndex);
    m_carrierParkings = m_carriersModel->parkingsForIndex(cIndex);

    emit baseLocationChanged();
}

void LocationController::clearLocation()
{
    m_locationIsLatLon = false;
    m_locationIsCarrier = false;
    m_abeam = false;
    m_location.clear();
    m_carrierName.clear();
    m_airportLocation.clear();
    m_detailLocation.clear();
    m_detailQml->setGuid(0);
    m_baseQml->setGuid(0);
    m_carrierParkings.clear();
    m_carrierParking.clear();
    emit baseLocationChanged();
}

void LocationController::setBaseLocation(QmlPositioned* pos)
{
    if (!pos) {
        clearLocation();
        return;
    }

    if (pos->inner() == m_location)
        return;

    clearLocation();
    m_location = pos->inner();
    m_baseQml->setGuid(pos->guid());

    if (FGPositioned::isAirportType(m_location.ptr())) {
        m_airportLocation = static_cast<FGAirport*>(m_location.ptr());
        // disable offset when selecting a heliport
        if (m_airportLocation->isHeliport()) {
            m_onFinal = false;
            m_useActiveRunway = false;
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
        m_useAvailableParking = false;
        m_detailQml->setInner(pos->inner());
    }

    emit configChanged();
}

QmlGeod LocationController::baseGeod() const
{
    if (m_locationIsLatLon || m_locationIsCarrier)
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
        m_useAvailableParking = false;
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
    for (unsigned int r = 0; r < m_airportLocation->numRunways(); ++r) {
        auto p = new QmlPositioned(m_airportLocation->getRunwayByIndex(r).ptr());
        QQmlEngine::setObjectOwnership(p, QQmlEngine::JavaScriptOwnership);
        result.push_back(p);
    }

    return result;
}

QObjectList LocationController::airportHelipads() const
{
    if (!m_airportLocation)
        return {};

    QObjectList result;
    for (unsigned int r = 0; r < m_airportLocation->numHelipads(); ++r) {
        auto p = new QmlPositioned(m_airportLocation->getHelipadByIndex(r).ptr());
        QQmlEngine::setObjectOwnership(p, QQmlEngine::JavaScriptOwnership);
        result.push_back(p);
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
    // prepend the default location and tutorial airport

	FGPositionedList locs = m_recentLocations;
    const std::string defaultICAO = flightgear::defaultAirportICAO();
	const std::string tutorialICAO = "PHTO"; // C172P tutorial aiurport

	// remove them from the recent locations
    auto it = std::remove_if(locs.begin(), locs.end(),
		[defaultICAO, tutorialICAO](FGPositionedRef pos)
	{
        return (pos->ident() == defaultICAO) || (pos->ident() == tutorialICAO);
    });
	locs.erase(it, locs.end());

	// prepend them
	FGAirportRef apt = FGAirport::findByIdent(tutorialICAO);
	locs.insert(locs.begin(), apt);

	apt = FGAirport::findByIdent(defaultICAO);
	locs.insert(locs.begin(), apt);

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

QString LocationController::carrierParking() const
{
    if (!m_locationIsCarrier)
        return {};
    return m_carrierParking;
}

void LocationController::setCarrierParking(QString name)
{
    if (!m_locationIsCarrier) {
        qWarning() << "active location is not a carrier";
        return;
    }

    if (m_carrierParking == name)
        return;

    if (!m_carrierParkings.contains(name)) {
        qWarning() << "parking '" << name << "' not found in carrier parking list";
        return;
    }

    m_carrierParking = name;
    m_useCarrierFLOLS = false;
    emit configChanged();
}

QmlPositioned *LocationController::detail() const
{
    return m_detailQml;
}

QmlPositioned *LocationController::baseLocation() const
{
    return m_baseQml;
}

QStringList LocationController::carrierParkings() const
{
    return m_carrierParkings;
}

void LocationController::setOffsetRadial(QuantityValue offsetRadial)
{
    if (m_offsetRadial == offsetRadial)
        return;

    m_offsetRadial = offsetRadial;
    emit offsetChanged();
}

void LocationController::setOffsetDistance(QuantityValue d)
{
    if (m_offsetDistance == d)
        return;

    m_offsetDistance = d;
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
        m_useActiveRunway = false;
    }
    emit configChanged();
}

void LocationController::setUseCarrierFLOLS(bool useCarrierFLOLS)
{
    if (!m_locationIsCarrier) {
        qWarning() << "location is not a carrier";
        return;
    }

    if (m_useCarrierFLOLS == useCarrierFLOLS)
        return;

    m_useCarrierFLOLS = useCarrierFLOLS;
    m_carrierParking.clear();
    emit configChanged();
}

void LocationController::setAbeam(bool abeam)
{
  m_abeam = abeam;
  emit configChanged();
}

void LocationController::restoreLocation(QVariantMap l)
{
    clearLocation();

    try {
        m_altitudeEnabled = l.contains("altitude");
        m_speedEnabled = l.contains("speed");
        m_headingEnabled = l.contains("heading");

        m_altitude = l.value("altitude", QVariant::fromValue(m_defaultAltitude)).value<QuantityValue>();
        m_airspeed = l.value("speed", QVariant::fromValue(m_defaultAirspeed)).value<QuantityValue>();
        m_heading = l.value("heading", QVariant::fromValue(m_defaultHeading)).value<QuantityValue>();

        m_offsetEnabled = l.value("offset-enabled").toBool();
        m_offsetRadial = l.value("offset-bearing", QVariant::fromValue(m_defaultOffsetRadial)).value<QuantityValue>();
        m_offsetDistance = l.value("offset-distance", QVariant::fromValue(m_defaultOffsetDistance)).value<QuantityValue>();
        m_tuneNAV1 = l.value("tune-nav1-radio").toBool();

        if (l.contains("location-lat")) {
            m_locationIsLatLon = true;
            m_geodLocation = SGGeod::fromDeg(l.value("location-lon").toDouble(),
                                             l.value("location-lat").toDouble());
        } else if (l.contains("carrier")) {
            setCarrierLocation(l.value("carrier").toString());
            if (l.contains("carrier-flols")) {
                setUseCarrierFLOLS(l.value("carrier-flols").toBool());
                setAbeam(l.value("abeam").toBool());
                // overwrite value form above, intentionally
                m_offsetDistance = l.value("location-carrier-flols-distance", QVariant::fromValue(m_defaultOffsetDistance)).value<QuantityValue>();
            } else if (l.contains("carrier-parking")) {
                setCarrierParking(l.value("carrier-parking").toString());
            }
        } else if (l.contains("location-id")) {
            m_location = NavDataCache::instance()->loadById(l.value("location-id").toLongLong());
            m_locationIsLatLon = false;
            if (FGPositioned::isAirportType(m_location.ptr())) {
                m_airportLocation = static_cast<FGAirport*>(m_location.ptr());
            } else {
                m_airportLocation.clear();
            }
            m_baseQml->setInner(m_location);
        }

        if (m_airportLocation) {
            m_useActiveRunway = false;
            m_useAvailableParking = false;

            if (l.contains("location-apt-runway")) {
                QString runway = l.value("location-apt-runway").toString().toUpper();
                const auto runwayStr = runway.toStdString();
                if (runway == QStringLiteral("ACTIVE")) {
                    m_useActiveRunway = true;
                } else if (m_airportLocation->isHeliport()) {
                    m_detailLocation = m_airportLocation->getHelipadByIdent(runwayStr);
                } else {
                    // could be a helipad at a regular airport
                    if (m_airportLocation->hasHelipadWithIdent(runwayStr)) {
                        m_detailLocation = m_airportLocation->getHelipadByIdent(runwayStr);
                    } else {
                        m_detailLocation = m_airportLocation->getRunwayByIdent(runwayStr);
                    }
                }
            } else if (l.contains("location-apt-parking")) {
                QString parking = l.value("location-apt-parking").toString();
                if (parking == QStringLiteral("AVAILABLE")) {
                    m_useAvailableParking = true;
                } else {
                    m_detailLocation = m_airportLocation->groundNetwork()->findParkingByName(parking.toStdString());
                }
            }

            if (m_detailLocation) {
                m_detailQml->setInner(m_detailLocation);
            }

            m_onFinal = l.value("location-on-final").toBool();
            setAbeam(l.value("abeam").toBool());
            m_offsetDistance = l.value("location-apt-final-distance", QVariant::fromValue(m_defaultOffsetDistance)).value<QuantityValue>();
        } // of location is an airport
    } catch (const sg_exception&) {
        qWarning() << "Errors restoring saved location, clearing";
        clearLocation();
    }

    baseLocationChanged();
    configChanged();
    offsetChanged();
}

bool LocationController::shouldStartPaused() const
{
    if (m_useCarrierFLOLS) {
        return true;
    }

    if (!m_location) {
        return false; // defaults to on-ground at the default airport
    }

    if (m_airportLocation) {
        return m_onFinal;
    } else {
        // navaid, start paused
        return true;
    }
}

QVariantMap LocationController::saveLocation() const
{
    QVariantMap locationSet;
    if (m_locationIsLatLon) {
        locationSet.insert("location-lat", m_geodLocation.getLatitudeDeg());
        locationSet.insert("location-lon", m_geodLocation.getLongitudeDeg());
    } else if (m_locationIsCarrier) {
        locationSet.insert("carrier", m_carrierName);
        if (m_useCarrierFLOLS) {
            locationSet.insert("carrier-flols", true);
            locationSet.insert("location-carrier-flols-distance", QVariant::fromValue(m_offsetDistance));
            locationSet.insert("abeam", m_abeam);
        } else if (!m_carrierParking.isEmpty()) {
            locationSet.insert("carrier-parking", m_carrierParking);
        }
    } else if (m_location) {
        locationSet.insert("location-id", static_cast<qlonglong>(m_location->guid()));

        if (m_airportLocation) {
            locationSet.insert("location-on-final", m_onFinal);
            locationSet.insert("location-apt-final-distance", QVariant::fromValue(m_offsetDistance));
            locationSet.insert("abeam", m_abeam);
            if (m_useActiveRunway) {
                locationSet.insert("location-apt-runway", "ACTIVE");
            } else if (m_useAvailableParking) {
                locationSet.insert("location-apt-parking", "AVAILABLE");
            } else if (m_detailLocation) {
                const auto detailType = m_detailLocation->type();
                if ((detailType == FGPositioned::RUNWAY) || (detailType == FGPositioned::HELIPAD)) {
                    locationSet.insert("location-apt-runway", QString::fromStdString(m_detailLocation->ident()));
                } else if (detailType == FGPositioned::PARKING) {
                    locationSet.insert("location-apt-parking", QString::fromStdString(m_detailLocation->ident()));
                }
            }
        } // of location is an airport
    } // of m_location is valid

    if (m_altitudeEnabled) {
        locationSet.insert("altitude", QVariant::fromValue(m_altitude));
    }

    if (m_speedEnabled) {
        locationSet.insert("speed", QVariant::fromValue(m_airspeed));
    }

    if (m_headingEnabled) {
        locationSet.insert("heading", QVariant::fromValue(m_heading));
    }

    if (m_offsetEnabled) {
        locationSet.insert("offset-enabled", m_offsetEnabled);
        locationSet.insert("offset-bearing", QVariant::fromValue(m_offsetRadial));
        locationSet.insert("offset-distance", QVariant::fromValue(m_offsetDistance));
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
        "airport-id" << "runway" << "parkpos" << "carrier" << "carrier-position";

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
        applyAltitude();
        applyAirspeed();
        return;
    }

    fgSetDouble("/sim/presets/latitude-deg", 9999.0);
    fgSetDouble("/sim/presets/longitude-deg", 9999.0);
    fgSetDouble("/sim/presets/altitude-ft", -9999.0);
    fgSetDouble("/sim/presets/heading-deg", 9999.0);

    if (m_locationIsCarrier) {
        fgSetString("/sim/presets/carrier", m_carrierName.toStdString());

        if (m_useCarrierFLOLS) {
            if (m_abeam) {
              fgSetString("/sim/presets/carrier-position", "abeam");
            } else {
              fgSetString("/sim/presets/carrier-position", "FLOLS");
            }
            fgSetDouble("/sim/presets/offset-distance-nm", m_offsetDistance.convertToUnit(Units::NauticalMiles).value);
            applyAltitude();
            applyAirspeed();
        } else if (!m_carrierParking.isEmpty()) {
            fgSetString("/sim/presets/carrier-position", m_carrierParking.toStdString());
        }

        if (m_tuneNAV1) {
            // tune TACAN to the carrier
            qInfo() << "Implement TACAN tuning";
        }
        return;
    }

    if (!m_location) {
        return;
    }

    if (m_airportLocation) {
        fgSetString("/sim/presets/airport-id", m_airportLocation->ident());
        fgSetBool("/sim/presets/on-ground", true);
        fgSetBool("/sim/presets/airport-requested", true);
        fgSetBool("/sim/presets/abeam", m_abeam);

        const bool onRunway = (m_detailLocation && (m_detailLocation->type() == FGPositioned::RUNWAY));
        const bool atParking = (m_detailLocation && (m_detailLocation->type() == FGPositioned::PARKING));
        if (m_useActiveRunway) {
            // automatic runway choice
            // we can't set navaid here
        } else if (m_useAvailableParking) {
            fgSetString("/sim/presets/parkpos", "AVAILABLE");
        } else if (onRunway) {
            if (m_airportLocation->type() == FGPositioned::AIRPORT) {
                // explicit runway choice (this also works for helipads)
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
                    fgSetDouble("/sim/presets/offset-distance-nm", m_offsetDistance.convertToUnit(Units::NauticalMiles).value);
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
        }

        // set disambiguation property
        globals->get_props()->setIntValue("/sim/presets/navaid-id",
                                          static_cast<int>(m_location->guid()));

        applyPositionOffset();
        applyAltitude();
        applyAirspeed();
    } // of navaid location
}

void LocationController::applyAirspeed()
{
    if (m_speedEnabled && (m_airspeed.unit != Units::NoUnits)) {
        if (m_airspeed.unit == Units::Knots) {
            m_config->setArg("vc", QString::number(m_airspeed.value));
        } else if (m_airspeed.unit == Units::KilometersPerHour) {
            const double vc = m_airspeed.convertToUnit(Units::Knots).value;
            m_config->setArg("vc", QString::number(vc));
        } else if (m_airspeed.unit == Units::Mach) {
            m_config->setArg("mach", QString::number(m_airspeed.value));
        } else {
            qWarning() << Q_FUNC_INFO << "unsupported airpseed unit" << m_airspeed.unit;
        }
    }
}

void LocationController::applyPositionOffset()
{
    if (m_headingEnabled && (m_heading.unit != Units::NoUnits)) {
        if (m_heading.unit == Units::DegreesTrue) {
            m_config->setArg("heading", QString::number(m_heading.value));
        } else {
            qWarning() << Q_FUNC_INFO << "unsupported heading unit" << m_heading.unit;
        }
    }

    if (m_offsetEnabled) {
        // flip direction of azimuth to balance the flip done in fgApplyStartOffset
        // I don't know why that flip exists but changing it there will break
        // command-line compatability so compensating here instead
        int offsetAzimuth = static_cast<int>(m_offsetRadial.value) - 180;
        m_config->setArg("offset-azimuth", QString::number(offsetAzimuth));
        const double offsetNm = m_offsetDistance.convertToUnit(Units::NauticalMiles).value;
        m_config->setArg("offset-distance", QString::number(offsetNm));
    }
}

void LocationController::applyAltitude()
{
    if (!m_altitudeEnabled)
        return;

    switch (m_altitude.unit) {
    default:
        qWarning() << Q_FUNC_INFO << "unsupported altitude unit";
        break;
    case Units::FeetMSL:
        m_config->setArg("altitude", QString::number(m_altitude.value));
        break;

    case Units::FeetAGL:
        // fixme - allow the sim to accpet AGL start position
        m_config->setArg("altitude", QString::number(m_altitude.value));
        break;

    case Units::FlightLevel:
        // FIXME - allow the sim to accept real FlightLevel arguments
        m_config->setArg("altitude", QString::number(m_altitude.value * 100));
        break;

    case Units::FeetAboveFieldElevation:
        m_config->setArg("altitude", QString::number(m_altitude.value));
        break;

    case Units::MetersMSL:
        const double ftMSL = m_altitude.convertToUnit(Units::FeetMSL).value;
        m_config->setArg("altitude", QString::number(ftMSL));
        break;
    }
}

void LocationController::applyOnFinal()
{
    if (m_onFinal) {
        if (!m_altitudeEnabled) {
            m_config->setArg("glideslope", std::string("3.0"));
        }

        const double offsetNm = m_offsetDistance.convertToUnit(Units::NauticalMiles).value;
        m_config->setArg("offset-distance", QString::number(offsetNm));
        m_config->setArg("on-ground", std::string("false"));

        applyAirspeed();
        applyAltitude();
    }
}

void LocationController::onCollectConfig()
{
    if (m_skipFromArgs) {
        qWarning() << Q_FUNC_INFO << "skipping argument collection";
        return;
    }

    if (m_locationIsLatLon) {
        m_config->setArg("lat", QString::number(m_geodLocation.getLatitudeDeg(), 'f', 8));
        m_config->setArg("lon", QString::number(m_geodLocation.getLongitudeDeg(), 'f', 8));
        applyPositionOffset();
        applyAltitude();
        applyAirspeed();
        return;
    }

    if (m_locationIsCarrier) {
        m_config->setArg("carrier", m_carrierName);

        if (!m_carrierParking.isEmpty()) {
            m_config->setArg("carrier-position", m_carrierParking);
        } else if (m_useCarrierFLOLS) {
            if (m_abeam) {
              m_config->setArg("carrier-position", QStringLiteral("abeam"));
            } else {
              m_config->setArg("carrier-position", QStringLiteral("FLOLS"));
            }
            const double offsetNm = m_offsetDistance.convertToUnit(Units::NauticalMiles).value;
            m_config->setArg("offset-distance", QString::number(offsetNm));

            applyAltitude();
            applyAirspeed();
        }

        return;
    }

    if (!m_location) {
        return;
    }

    if (m_airportLocation) {
        m_config->setArg("airport", QString::fromStdString(m_airportLocation->ident()));
        const auto ty = m_detailLocation ? m_detailLocation->type() : FGPositioned::INVALID;
        const bool onRunway = (ty == FGPositioned::RUNWAY) || (ty == FGPositioned::HELIPAD);
        const bool atParking = ty == FGPositioned::PARKING;

        if (m_useActiveRunway) {
            // pick by default
            applyOnFinal();
        } else if (m_useAvailableParking) {
             m_config->setArg("parking-id", QStringLiteral("AVAILABLE"));
        } else if (onRunway) {
            if (m_airportLocation->type() == FGPositioned::AIRPORT) {
                m_config->setArg("runway", QString::fromStdString(m_detailLocation->ident()));

                if (ty == FGPositioned::RUNWAY) {
                    // set nav-radio 1 based on selected runway
                    auto runway = fgpositioned_cast<FGRunway>(m_detailLocation);
                    if (runway->ILS()) {
                        double mhz = runway->ILS()->get_freq() / 100.0;
                        m_config->setArg("nav1", QString("%1:%2").arg(runway->headingDeg()).arg(mhz));
                    }

                    applyOnFinal();
                }

            } else if (m_airportLocation->type() == FGPositioned::HELIPORT) {
                m_config->setArg("runway", QString::fromStdString(m_detailLocation->ident()));
            }
        } else if (atParking) {
            // parking selection
            m_config->setArg("parking-id", QString::fromStdString(m_detailLocation->ident()));
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
        }

        // set disambiguation property
        m_config->setProperty("/sim/presets/navaid-id", QString::number(m_location->guid()));
        applyPositionOffset();
        applyAltitude();
        applyAirspeed();
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

QString LocationController::compassPointFromHeading(int heading) const
{
    const int labelArc = 360 / 8;
    heading += (labelArc >> 1);
    SG_NORMALIZE_RANGE(heading, 0, 359);

    //
    switch (heading / labelArc) {
    case 0: return tr("N");
    case 1: return tr("NE");
    case 2: return tr("E");
    case 3: return tr("SE");
    case 4: return tr("S");
    case 5: return tr("SW");
    case 6: return tr("W");
    case 7: return tr("NW");
    }

    return {};
}

QString LocationController::description() const
{
    const double offsetNm = m_offsetDistance.convertToUnit(Units::NauticalMiles).value;

    if (!m_location) {
        if (m_locationIsLatLon) {
            const auto s = simgear::strutils::formatGeodAsString(m_geodLocation,
                                                                 simgear::strutils::LatLonFormat::DECIMAL_DEGREES,
                                                                 simgear::strutils::DegreeSymbol::UTF8_DEGREE);
            return tr("at position %1").arg(QString::fromStdString(s));
        }

        if (m_locationIsCarrier) {
            QString pennant = m_carriersModel->pennantForIndex(m_carriersModel->indexOf(m_carrierName));
            QString locationToCarrier;
            if (m_abeam) {
              locationToCarrier = tr("%1nm abeam").arg(offsetNm);
            } else if (m_useCarrierFLOLS) {
              locationToCarrier = tr("on %1nm final to").arg(offsetNm);
            } else {
              locationToCarrier = tr("on deck at %1 on").arg(m_carrierParking);
            }
            return tr("%1 carrier %2 (%3)").arg(locationToCarrier).arg(m_carrierName).arg(pennant);
        }

        return tr("No location selected");
    }

    QString ident = QString::fromStdString(m_location->ident()),
        name = QString::fromStdString(m_location->name());

    name = fixNavaidName(name);

    if (m_airportLocation) {
        const auto ty = m_detailLocation ? m_detailLocation->type() : FGPositioned::INVALID;
        const bool onRunway = ty == FGPositioned::RUNWAY;
        const bool onPad = ty == FGPositioned::HELIPAD;
        const bool atParking = ty == FGPositioned::PARKING;

        QString locationOnAirport;

        if (m_useActiveRunway) {
            if (m_onFinal) {
                locationOnAirport = tr("on %1-mile final to active runway").arg(offsetNm);
            } else {
                locationOnAirport = tr("on active runway");
            }
        } else if (m_useAvailableParking) {
            locationOnAirport = tr("at an available parking position");
        } else if (onRunway) {
            QString runwayName = tr("runway %1").arg(QString::fromStdString(m_detailLocation->ident()));

            if (m_onFinal) {
                locationOnAirport = tr("on %2-mile final to %1").arg(runwayName).arg(offsetNm);
            } else {
                locationOnAirport = tr("on %1").arg(runwayName);
            }
        } else if (onPad) {
            locationOnAirport = tr("on pad %1").arg(QString::fromStdString(m_detailLocation->ident()));
        } else if (atParking) {
            locationOnAirport = tr("at parking position %1").arg(QString::fromStdString(m_detailLocation->ident()));
        }

        return tr("%2 (%1): %3").arg(ident).arg(name).arg(locationOnAirport);
    } else {
        QString offsetDesc = tr("at");
        if (m_offsetEnabled) {
            offsetDesc = tr("%1nm %2 of").
                    arg(offsetNm, 0, 'f', 1).
                    arg(compassPointFromHeading(static_cast<int>(m_offsetRadial.value)));
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
