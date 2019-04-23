#include "config.h"

#include "test_suite/dataStore.hxx"

#include "globals.hxx"

#if defined(HAVE_QT)
#include <GUI/QtLauncher.hxx>
#endif

#include <Main/globals.hxx>
#include <Main/options.hxx>

#include <Time/TimeManager.hxx>

#include <simgear/structure/event_mgr.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <Airports/airport.hxx>
#include <Navaids/FlightPlan.hxx>
#include <Navaids/waypoint.hxx>

using namespace flightgear;

namespace FGTestApi {

namespace setUp {

void initTestGlobals(const std::string& testName)
{
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

    // Activate headless mode.
    globals->set_headless(true);

    fgSetDefaults();

    std::unique_ptr<TimeManager> t;
    t.reset(new TimeManager);
    t->init(); // establish mag-var data

    /**
     * Both the event manager and subsystem manager are initialised by the
     * FGGlobals ctor, but only the subsystem manager is destroyed by the dtor.
     * Here the event manager is added to the subsystem manager so it can be
     * destroyed via the subsystem manager.
     */
    globals->add_subsystem("events", globals->get_event_mgr(), SGSubsystemMgr::DISPLAY);
}

void populateFP(flightgear::FlightPlanRef f,
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
    
    f->insertWayptAtIndex(new RunwayWaypt(f->departureRunway(), f), -1);
    
    for (auto ws : simgear::strutils::split(waypoints)) {
        WayptRef wpt = f->waypointFromString(ws);
        f->insertWayptAtIndex(wpt, -1);
    }
    
  
    auto destRwy = f->destinationRunway();
    f->insertWayptAtIndex(new BasicWaypt(destRwy->pointOnCenterline(-8 * SG_NM_TO_METER),
                                         destRwy->ident() + "-8", f), -1);
    f->insertWayptAtIndex(new RunwayWaypt(destRwy, f), -1);
}


}  // End of namespace setUp.

void setPosition(const SGGeod& g)
{
    globals->get_props()->setDoubleValue("position/latitude-deg", g.getLatitudeDeg());
    globals->get_props()->setDoubleValue("position/longitude-deg", g.getLongitudeDeg());
    globals->get_props()->setDoubleValue("position/altitude-ft", g.getElevationFt());
}

void runForTime(double t)
{
    int ticks = t * 120.0;
    assert(ticks > 0);

    for (int t = 0; t < ticks; ++t) {
        globals->get_subsystem_mgr()->update(1 / 120.0);
    }
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
}

}  // End of namespace tearDown.

}  // End of namespace FGTestApi.
