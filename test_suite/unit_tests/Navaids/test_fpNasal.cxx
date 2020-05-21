#include "test_fpNasal.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"

#include <simgear/misc/strutils.hxx>

#include <Navaids/FlightPlan.hxx>
#include <Navaids/routePath.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/airways.hxx>
#include <Navaids/fix.hxx>

#include <Airports/airport.hxx>
#include <Autopilot/route_mgr.hxx>

using namespace flightgear;

static bool static_haveProcedures = false;

// Set up function for each test.
void FPNasalTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("flightplan");
    FGTestApi::setUp::initNavDataCache();

    SGPath proceduresPath = SGPath::fromEnv("FG_PROCEDURES_PATH");
    if (proceduresPath.exists()) {
        static_haveProcedures = true;
        globals->append_fg_scenery(proceduresPath);
    }
    
    // flightplan() acces needs the route manager
    globals->add_new_subsystem<FGRouteMgr>();

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    
    FGTestApi::setUp::initStandardNasal();
    globals->get_subsystem_mgr()->postinit();
}


// Clean up after each test.
void FPNasalTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

static FlightPlanRef makeTestFP(const std::string& depICAO, const std::string& depRunway,
                         const std::string& destICAO, const std::string& destRunway,
                         const std::string& waypoints)
{
    FlightPlanRef f = new FlightPlan;
    FGTestApi::setUp::populateFPWithNasal(f, depICAO, depRunway, destICAO, destRunway, waypoints);
    return f;
}

void FPNasalTests::testBasic()
{
    
    FlightPlanRef fp1 = makeTestFP("EGCC", "23L", "EHAM", "24",
                                   "TNT CLN");
    fp1->setIdent("testplan");
    
// setup the FP on the route-manager, so flightplan() call works
    auto rm = globals->get_subsystem<FGRouteMgr>();
    rm->setFlightPlan(fp1);
    rm->activate();

    // modify leg data dfrom Nasal
    bool ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        var leg = fp.getWP(3);
        leg.setAltitude(6000, 'AT');
    )");
    CPPUNIT_ASSERT(ok);

    // check the value updated in the leg
    CPPUNIT_ASSERT_EQUAL(RESTRICT_AT, fp1->legAtIndex(3)->altitudeRestriction());
    CPPUNIT_ASSERT_EQUAL(6000, fp1->legAtIndex(3)->altitudeFt());
    
// insert some waypoints from Nasal
    
    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan();
        var leg = fp.getWP(2);
        var newWP = createWPFrom(navinfo(leg.lat, leg.lon, 'COA')[0]);
        fp.insertWPAfter(newWP, 2);
    )");
    CPPUNIT_ASSERT(ok);
    
    CPPUNIT_ASSERT_EQUAL(string{"COSTA VOR-DME"}, fp1->legAtIndex(3)->waypoint()->source()->name());
}


void FPNasalTests::testSegfaultWaypointGhost() 
{
    // checking for a segfault here, no segfault indicates success. A runtime error in the log is acceptable here.
    bool ok = FGTestApi::executeNasal(R"(
        var fp = createFlightplan();
        fp.departure = airportinfo("BIKF");
        fp.destination = airportinfo("EGLL");
        var wp = fp.getWP(1);
        fp.deleteWP(1);
        print(wp.wp_name);
    )");
    CPPUNIT_ASSERT(ok);
}

void FPNasalTests::testSIDTransitionAPI()
{
    if (!static_haveProcedures) {
        return;
    }

    auto rm = globals->get_subsystem<FGRouteMgr>();
    
    bool ok = FGTestApi::executeNasal(R"(
        var fp = flightplan();
        fp.departure = airportinfo("KJFK");
        fp.destination = airportinfo("EGLL");
                                      
        var sid = fp.departure.getSid("DEEZZ5.13L");
                                      
        unitTest.assert(sid != nil, "SID not found");
        unitTest.assert_equal(sid.id, "DEEZZ5.13L", "Incorrect SID loaded");

        var trans = sid.transition('CANDR');
        unitTest.assert_equal(trans.id, "CANDR", "Couldn't find transition");

        fp.sid = trans;
        fp.departure_runway = fp.departure.runway('13L')
    )");
    
    CPPUNIT_ASSERT(ok);
    
    auto fp = rm->flightPlan();
    
    CPPUNIT_ASSERT(fp->departureRunway());
    CPPUNIT_ASSERT(fp->sid());
    CPPUNIT_ASSERT(fp->sidTransition());
    
    CPPUNIT_ASSERT_EQUAL(fp->departureRunway()->ident(), string{"13L"});
    CPPUNIT_ASSERT_EQUAL(fp->sid()->ident(), string{"DEEZZ5.13L"});
    CPPUNIT_ASSERT_EQUAL(fp->sidTransition()->ident(), string{"CANDR"});
    
// test specify SID via transition in Nasal
    rm->setFlightPlan(new FlightPlan{});
    
    ok = FGTestApi::executeNasal(R"(
         var fp = flightplan();
         fp.departure = airportinfo("KJFK");
         fp.destination = airportinfo("EGLL");
         fp.departure_runway = airportinfo("KJFK").runways["13L"];
         fp.sid = airportinfo("KJFK").getSid("DEEZZ5.13L");
         fp.sid_trans = "CANDR";

    )");
    
    CPPUNIT_ASSERT(ok);
    
    fp = rm->flightPlan();
    
    CPPUNIT_ASSERT(fp->departureRunway());
    CPPUNIT_ASSERT(fp->sid());
    CPPUNIT_ASSERT(fp->sidTransition());
    
    CPPUNIT_ASSERT_EQUAL(fp->departureRunway()->ident(), string{"13L"});
    CPPUNIT_ASSERT_EQUAL(fp->sid()->ident(), string{"DEEZZ5.13L"});
    CPPUNIT_ASSERT_EQUAL(fp->sidTransition()->ident(), string{"CANDR"});
    
    
}

void FPNasalTests::testSTARTransitionAPI()
{
    if (!static_haveProcedures) {
        return;
    }

    auto rm = globals->get_subsystem<FGRouteMgr>();
    
    bool ok = FGTestApi::executeNasal(R"(
        var fp = flightplan();
        fp.departure = airportinfo("EGLL");
        fp.destination = airportinfo("EDDM");
                                      
        var star = fp.destination.getStar("RIXE3A.26L");
                                      
        unitTest.assert(star != nil, "STAR not found");
        unitTest.assert_equal(star.id, "RIXE3A.26L", "Incorrect STAR loaded");


        fp.star = star;
        fp.destination_runway = fp.destination.runway('26L')
    )");
    
    CPPUNIT_ASSERT(ok);
    
    auto fp = rm->flightPlan();
    
    CPPUNIT_ASSERT(fp->star());
    CPPUNIT_ASSERT(fp->starTransition() == nullptr);
    
    CPPUNIT_ASSERT_EQUAL(fp->star()->ident(), string{"RIXE3A.26L"});
}
