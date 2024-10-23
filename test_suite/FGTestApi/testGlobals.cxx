/* 
SPDX-Copyright: James Turner
SPDX-License-Identifier: GPL-2.0-or-later 
*/

#include "config.h"

#include "test_suite/dataStore.hxx"
#include <ctime>

#include "TestDataLogger.hxx"
#include "testGlobals.hxx"

#include <simgear/io/iostreams/sgstream.hxx>

#if defined(HAVE_QT)
#include <GUI/QtLauncher.hxx>
#endif

#include <Main/globals.hxx>
#include <Main/options.hxx>
#include <Main/util.hxx>
#include <Main/FGInterpolator.hxx>
#include <Main/locale.hxx>

#include <Time/TimeManager.hxx>

#include <simgear/structure/event_mgr.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/props/props_io.hxx>

#include <Airports/airport.hxx>
#include <Navaids/FlightPlan.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/routePath.hxx>

#include <Scripting/NasalSys.hxx>

using namespace flightgear;

namespace FGTestApi {

bool global_loggingToKML = false;
sg_ofstream global_kmlStream;
bool global_lineStringOpen = false;
    
namespace setUp {

void initTestGlobals(const std::string& testName)
{
    assert(globals == nullptr);
    globals = new FGGlobals;

    DataStore &data = DataStore::get();
    if (!data.getFGRoot().exists()) {
        data.findFGRoot("");
    }
    globals->set_fg_root(data.getFGRoot());

    // current dir
    SGPath homePath = SGPath::fromUtf8(FGBUILDDIR) / "test_home";
    if (!homePath.exists()) {
        (homePath / "dummyFile").create_dir(0755);
    }

    globals->set_fg_home(homePath);
    auto props = globals->get_props();
    props->setStringValue("sim/fg-home", homePath.utf8Str());
    
    // Activate headless mode.
    globals->set_headless(true);

    fgSetDefaults();

    auto t = globals->get_subsystem_mgr()->add<TimeManager>();
    t->bind();
    t->init(); // establish mag-var data

    /**
     * Both the event manager and subsystem manager are initialised by the
     * FGGlobals ctor, but only the subsystem manager is destroyed by the dtor.
     * Here the event manager is added to the subsystem manager so it can be
     * destroyed via the subsystem manager.
     */
    globals->get_subsystem_mgr()->add("events", globals->get_event_mgr());
    
    // necessary to avoid asserts: mark FGLocale as initialized
    globals->get_locale()->selectLanguage({});
}
    
bool logPositionToKML(const std::string& testName)
{
    // clear any previous state
    if (global_loggingToKML) {
        global_kmlStream.close();
        global_lineStringOpen = false;
    }
    
    SGPath p = SGPath::desktop() / (testName + ".kml");
    global_kmlStream.open(p);
    if (!global_kmlStream.is_open()) {
        SG_LOG(SG_GENERAL, SG_WARN, "unable to open:" << p);
        return false;
    }
    
    // pre-amble
    global_kmlStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
        "<Document>\n";
    // need more precision for doubles when specifying lat/lon, see
    // https://xkcd.com/2170/  :)
    global_kmlStream.precision(12);
    
    global_loggingToKML = true;
    return true;
}
    
bool logLinestringsToKML(const std::string& testName)
{
    // clear any previous state
    if (global_loggingToKML) {
        global_kmlStream.close();
        global_lineStringOpen = false;
    }
    
    SGPath p = SGPath::desktop() / (testName + ".kml");
    global_kmlStream.open(p);
    if (!global_kmlStream.is_open()) {
        SG_LOG(SG_GENERAL, SG_WARN, "unable to open:" << p);
        return false;
    }
    
    // pre-amble
    global_kmlStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
        "<Document>\n";
    // need more precision for doubles when specifying lat/lon, see
    // https://xkcd.com/2170/  :)
    global_kmlStream.precision(12);
    
    global_loggingToKML = false;
    return true;
}

void initStandardNasal(bool withCanvas)
{
    fgInitAllowedPaths();
    
    auto nasalNode = globals->get_props()->getNode("nasal", true);
    
// load loadpriority.xml, for default modules load order

    auto nasalLoadPriority = globals->get_props()->getNode("/sim/nasal-load-priority",true);
    readProperties(globals->get_fg_root() / "Nasal/loadpriority.xml", nasalLoadPriority);

// set various props to reduce Nasal errors
    auto props = globals->get_props();
    props->setStringValue("sim/flight-model", "null");
    props->setStringValue("sim/aircraft", "test-suite-aircraft");
    
    props->setDoubleValue("sim/current-view/config/default-field-of-view-deg", 90.0);
    // ensure /sim/view/config exists
    props->setBoolValue("sim/view/config/foo", false);
    
    props->setBoolValue("sim/rendering/precipitation-gui-enable", false);
    props->setBoolValue("sim/rendering/precipitation-aircraft-enable", false);
    
// disable various larger modules
    nasalNode->setBoolValue("canvas/enabled", withCanvas);
    nasalNode->setBoolValue("jetways/enabled", false);
    nasalNode->setBoolValue("jetways_edit/enabled", false);
    nasalNode->setBoolValue("local_weather/enabled", false);

    // Nasal needs the interpolator running
    globals->get_subsystem_mgr()->add<FGInterpolator>();

    // will be inited, since we already did that
    globals->get_subsystem_mgr()->add<FGNasalSys>();
}

void populateFPWithoutNasal(flightgear::FlightPlanRef f,
                 const std::string& depICAO, const std::string& depRunway,
                 const std::string& destICAO, const std::string& destRunway,
                 const std::string& waypoints)
{
    FGAirportRef depApt = FGAirport::getByIdent(depICAO);
    f->setDeparture(depApt->getRunwayByIdent(depRunway));


    FGAirportRef destApt = FGAirport::getByIdent(destICAO);
    f->setDestination(destApt->getRunwayByIdent(destRunway));

    // since we don't have the Nasal route-manager delegate, insert the
    // runway waypoints manually
    
    auto depRwy = new RunwayWaypt(f->departureRunway(), f);
    depRwy->setFlag(WPT_DEPARTURE);
    f->insertWayptAtIndex(depRwy, -1);
    
    for (auto ws : simgear::strutils::split(waypoints)) {
        WayptRef wpt = f->waypointFromString(ws);
        if (!wpt) {
            SG_LOG(SG_NAVAID, SG_ALERT, "No waypoint created for:" << ws);
            continue;
        }
        f->insertWayptAtIndex(wpt, -1);
    }
    
  
    auto destRwy = f->destinationRunway();
    f->insertWayptAtIndex(new BasicWaypt(destRwy->pointOnCenterline(-8 * SG_NM_TO_METER),
                                         destRwy->ident() + "-8", f), -1);
    f->insertWayptAtIndex(new RunwayWaypt(destRwy, f), -1);
}
    
void populateFPWithNasal(flightgear::FlightPlanRef f,
                       const std::string& depICAO, const std::string& depRunway,
                       const std::string& destICAO, const std::string& destRunway,
                       const std::string& waypoints)
{
    FGAirportRef depApt = FGAirport::getByIdent(depICAO);
    f->setDeparture(depApt->getRunwayByIdent(depRunway));
    
    FGAirportRef destApt = FGAirport::getByIdent(destICAO);
    f->setDestination(destApt->getRunwayByIdent(destRunway));
    
    // insert after the last departure waypoint
    int insertIndex = 1;
    
    for (auto ws : simgear::strutils::split(waypoints)) {
        WayptRef wpt = f->waypointFromString(ws);
        f->insertWayptAtIndex(wpt, insertIndex++);
    }
}


}  // End of namespace setUp.
    
    void beginLineString(const std::string& ident)
    {
        global_lineStringOpen = true;
        global_kmlStream << "<Placemark>\n";
        if (!ident.empty()) {
            global_kmlStream << "<name>" << ident << "</name>\n";
        }
        global_kmlStream << "<LineString>\n";
        global_kmlStream << "<tessellate>1</tessellate>\n";
        global_kmlStream << "<coordinates>\n";
    }
    
    void logCoordinate(const SGGeod& pos)
    {
        if (!global_lineStringOpen) {
            beginLineString({});
        }

        global_kmlStream << pos.getLongitudeDeg() << "," << pos.getLatitudeDeg() << " " << std::endl;
    }

    void rawLogCoordinate(const SGGeod& pos)
    {
        global_kmlStream << pos.getLongitudeDeg() << "," << pos.getLatitudeDeg() << " " << std::endl;
    }

    void endCurrentLineString()
    {
        global_lineStringOpen = false;
        global_kmlStream << "</coordinates>\n"
                            "</LineString>\n"
                            "</Placemark>\n"
                         << std::endl;
    }
    
void setPosition(const SGGeod& g)
{
    if (global_loggingToKML) {
        if (global_lineStringOpen) {
            endCurrentLineString();
        }
        
        logCoordinate(g);
    }
    
    globals->get_props()->setDoubleValue("position/latitude-deg", g.getLatitudeDeg());
    globals->get_props()->setDoubleValue("position/longitude-deg", g.getLongitudeDeg());
    globals->get_props()->setDoubleValue("position/altitude-ft", g.getElevationFt());
}

const SGGeod getPosition()
{
    return SGGeod::fromDegFt(    
    globals->get_props()->getDoubleValue("position/latitude-deg"),
    globals->get_props()->getDoubleValue("position/longitude-deg"),
    globals->get_props()->getDoubleValue("position/altitude-ft"));
}
    

    
void setPositionAndStabilise(const SGGeod& g)
{
    setPosition(g);
    for (int i=0; i<60; ++i) {
       globals->get_subsystem_mgr()->update(0.015);
    }
}


void runForTime(double t)
{
    const int tickHz = 30;
    const double tickDuration = 1.0 / tickHz;

    int ticks = static_cast<int>(t * tickHz);
    assert(ticks > 0);

    const int logInterval = 0.5 * tickHz;
    int nextLog = 0;

    long startTime = globals->get_time_params()->get_cur_time();

    for (int t = 0; t < ticks; ++t) {
        globals->inc_sim_time_sec(tickDuration);
        globals->get_time_params()->update(globals->get_view_position(), startTime, t * tickDuration);
        globals->get_subsystem_mgr()->update(tickDuration);

        if (nextLog == 0) {
            if (global_loggingToKML) {
                logCoordinate(globals->get_aircraft_position());
            }

            if (DataLogger::isActive()) {
                DataLogger::instance()->writeRecord();
            }
            nextLog = logInterval;
        } else {
            nextLog--;
        }
    }
}

bool runForTimeWithCheck(double t, RunCheck check)
{
    const int tickHz = 30;
    const double tickDuration = 1.0 / tickHz;

    int ticks = static_cast<int>(t * tickHz);
    assert(ticks > 0);
    const int logInterval = 0.5 * tickHz;
    int nextLog = 0;
    
    for (int t = 0; t < ticks; ++t) {
        globals->inc_sim_time_sec(tickDuration);
        globals->get_subsystem_mgr()->update(tickDuration);

        if (nextLog == 0) {
            if (global_loggingToKML) {
                logCoordinate(globals->get_aircraft_position());
            }

            if (DataLogger::isActive()) {
                DataLogger::instance()->writeRecord();
            }
            nextLog = logInterval;
        } else {
            nextLog--;
        }

        bool done = check();
        if (done) {
            if (global_loggingToKML) {
                endCurrentLineString();
            }
            return true;
        }
    }
    
    return false;
}

void adjustSimulationWorldTime(time_t desiredUnixTime)
{
    int timeOffset = desiredUnixTime - time(nullptr);
    globals->get_props()->setIntValue("/sim/time/cur-time-override", 0);
    globals->get_props()->setIntValue("/sim/time/warp", timeOffset);

    globals->get_subsystem<TimeManager>()->update(0.0);
}

void writeFlightPlanToKML(flightgear::FlightPlanRef fp)
{
    if (!global_loggingToKML)
        return;
    
    RoutePath rpath(fp);
    
    for (int i=0; i<fp->numLegs(); ++i) {
        SGGeodVec legPath = rpath.pathForIndex(i);
        auto wp = fp->legAtIndex(i)->waypoint();

        writeGeodsToKML("FP-leg-" + wp->ident(), legPath);
        
        SGGeod legWPPosition = wp->position();
        writePointToKML("WP " + wp->ident(), legWPPosition);
    }
}
    
void writeGeodsToKML(const std::string &label, const flightgear::SGGeodVec& geods)
{
    if (global_lineStringOpen) {
        endCurrentLineString();
    }
    
    beginLineString(label);
    
    for (const auto& g : geods) {
        logCoordinate(g);
    }
    
    endCurrentLineString();
}

void writePointToKML(const std::string& ident, const SGGeod& pos)
{
    global_kmlStream << "<Placemark>\n";
    global_kmlStream << "<name>"  << ident << "</name>\n";
    global_kmlStream << "<Point>\n";
    global_kmlStream << "<coordinates>\n";
    rawLogCoordinate(pos);
    global_kmlStream << "</coordinates>\n";
    global_kmlStream << "</Point>\n";
    global_kmlStream << "</Placemark>\n";
}

bool executeNasal(const std::string& code)
{
    auto nasal = globals->get_subsystem<FGNasalSys>();
    if (!nasal) {
        throw sg_exception("Nasal not available");
    }

    nasal->getAndClearErrorList();
    std::string output, parseErrors;
    bool ok = nasal->parseAndRunWithOutput(code, output, parseErrors);
    if (!parseErrors.empty()) {
        SG_LOG(SG_NASAL, SG_ALERT, "Errors running Nasal:" << parseErrors);
        return false;
    }

    return ok;
}

SGPropertyNode_ptr propsFromString(const std::string& s)
{
    SGPropertyNode_ptr m = new SGPropertyNode;
    std::istringstream iss(s);
    readProperties(iss, m);
    return m;
}

bool geodsApproximatelyEqual(const SGGeod& a, const SGGeod& b)
{
    return SGGeodesy::distanceM(a, b) < 50.0;
}


namespace tearDown {

void shutdownTestGlobals()
{
    // The QApplication instance must be destroyed before exit() begins, see
    // <https://bugreports.qt.io/browse/QTBUG-48709> (otherwise, segfault).
#if defined(HAVE_QT)
    flightgear::shutdownQtApp();
#endif

    delete globals;
    globals = nullptr;
    
    if (global_kmlStream) {
        if (global_lineStringOpen) {
            endCurrentLineString();
        }
        // post-amble
        global_kmlStream << "</Document>\n"
                            "</kml>"
                         << std::endl;
        global_kmlStream.close();
        global_loggingToKML = false;
    }
}

}  // End of namespace tearDown.

}  // End of namespace FGTestApi.
