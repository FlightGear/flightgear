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

#include <Main/globals.hxx>

using namespace flightgear;
using namespace std::string_literals;

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
    globals->get_subsystem_mgr()->add<FGRouteMgr>();

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
    FlightPlanRef f = FlightPlan::create();
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

    CPPUNIT_ASSERT_EQUAL("COSTA VOR-DME"s, fp1->legAtIndex(3)->waypoint()->source()->name());

    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan();
        fp.clearAll();
        unitTest.assert_equal(fp.getPlanSize(), 0);
        unitTest.assert_equal(fp.current, -1);
        unitTest.assert_equal(fp.departure, nil);
        unitTest.assert_equal(fp.sid, nil);
        unitTest.assert_equal(fp.cruiseSpeedKt, 0);
    )");
    CPPUNIT_ASSERT(ok);
}

void FPNasalTests::testRestrictions()
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

    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        var leg = fp.getWP(3);
        leg.setAltitude(6000, 'above');
    )");
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(RESTRICT_ABOVE, fp1->legAtIndex(3)->altitudeRestriction());
    CPPUNIT_ASSERT_EQUAL(6000, fp1->legAtIndex(3)->altitudeFt());

    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        var leg = fp.getWP(3);
        leg.setAltitude(6000, 'below');
    )");
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(RESTRICT_BELOW, fp1->legAtIndex(3)->altitudeRestriction());
    CPPUNIT_ASSERT_EQUAL(6000, fp1->legAtIndex(3)->altitudeFt());

    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        var leg = fp.getWP(3);
        leg.setAltitude(6000, 'delete');
    )");
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(RESTRICT_DELETE, fp1->legAtIndex(3)->altitudeRestriction());

    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        var leg = fp.getWP(3);
        leg.setSpeed(250, 'at');
    )");
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(RESTRICT_AT, fp1->legAtIndex(3)->speedRestriction());
    CPPUNIT_ASSERT_EQUAL(250, fp1->legAtIndex(3)->speedKts());

    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        var leg = fp.getWP(3);
        leg.setSpeed(250, 'above');
    )");
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(RESTRICT_ABOVE, fp1->legAtIndex(3)->speedRestriction());
    CPPUNIT_ASSERT_EQUAL(250, fp1->legAtIndex(3)->speedKts());

    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        var leg = fp.getWP(3);
        leg.setSpeed(250, 'below');
    )");
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(RESTRICT_BELOW, fp1->legAtIndex(3)->speedRestriction());
    CPPUNIT_ASSERT_EQUAL(250, fp1->legAtIndex(3)->speedKts());

    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        var leg = fp.getWP(3);
        leg.setSpeed(250, 'delete');
    )");
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(RESTRICT_DELETE, fp1->legAtIndex(3)->speedRestriction());
    
    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        var leg = fp.getWP(3);
        leg.setSpeed(0.8, 'at', "Mach");
        leg.setAltitude(300, 'above', 'FL');
    )");
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(RESTRICT_AT, fp1->legAtIndex(3)->speedRestriction());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, fp1->legAtIndex(3)->speedMach(), 0.01);
    CPPUNIT_ASSERT_EQUAL(RESTRICT_ABOVE, fp1->legAtIndex(3)->altitudeRestriction());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(30000, fp1->legAtIndex(3)->altitudeFt(), 1.0);
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
        unitTest.assert_equal(trans.tp_type, "transition", "Procedure type incorrect");

        fp.sid = trans;
        fp.departure_runway = fp.departure.runway('13L')
    )");

    CPPUNIT_ASSERT(ok);
    
    auto fp = rm->flightPlan();
    
    CPPUNIT_ASSERT(fp->departureRunway());
    CPPUNIT_ASSERT(fp->sid());
    CPPUNIT_ASSERT(fp->sidTransition());

    CPPUNIT_ASSERT_EQUAL(fp->departureRunway()->ident(), "13L"s);
    CPPUNIT_ASSERT_EQUAL(fp->sid()->ident(), "DEEZZ5.13L"s);
    CPPUNIT_ASSERT_EQUAL(fp->sidTransition()->ident(), "CANDR"s);

    // test specify SID via transition in Nasal
    rm->setFlightPlan(FlightPlan::create());
    
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

    CPPUNIT_ASSERT_EQUAL(fp->departureRunway()->ident(), "13L"s);
    CPPUNIT_ASSERT_EQUAL(fp->sid()->ident(), "DEEZZ5.13L"s);
    CPPUNIT_ASSERT_EQUAL(fp->sidTransition()->ident(), "CANDR"s);
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

    CPPUNIT_ASSERT_EQUAL(fp->star()->ident(), "RIXE3A.26L"s);
}

void FPNasalTests::testApproachTransitionAPI()
{
    if (!static_haveProcedures) {
        return;
    }

    auto rm = globals->get_subsystem<FGRouteMgr>();

    bool ok = FGTestApi::executeNasal(R"(
        var fp = flightplan();
        fp.departure = airportinfo("EGLL");
        fp.destination = airportinfo("EDDM");
                                      
        var star = fp.destination.getStar("RIXE3A.08L");
                                      
        unitTest.assert(star != nil, "STAR not found");
        unitTest.assert_equal(star.id, "RIXE3A.08L", "Incorrect STAR loaded");

        fp.star = star;
        fp.destination_runway = fp.destination.runway('08L');
                                      
        var approach = fp.destination.getApproach("ILS08L");
        unitTest.assert(approach != nil, "No approach loaded");
                                      
        var trans = approach.transition('LUL1C');
                    
        unitTest.assert(trans != nil, "approach transition not found");
        unitTest.assert_equal(trans.id, "LUL1C", "Incorrect approach transition loaded");
        unitTest.assert_equal(trans.tp_type, "transition", "Procedure type incorrect");
                                      
        fp.approach = trans;
                  
        unitTest.assert_equal(fp.approach.id, "ILS08L", "Incorrect approach returned");
        unitTest.assert_equal(fp.approach_trans.id, "LUL1C", "Incorrect transition returned");
        unitTest.assert_equal(fp.approach_trans.tp_type, "transition", "Procedure type incorrect");
    )");

    CPPUNIT_ASSERT(ok);

    auto fp = rm->flightPlan();

    CPPUNIT_ASSERT(fp->approach());
    CPPUNIT_ASSERT_EQUAL("LUL1C"s, fp->approachTransition()->ident());
    CPPUNIT_ASSERT_EQUAL("ILS08L"s, fp->approach()->ident());
}

void FPNasalTests::testApproachTransitionAPIWithCloning()
{
    if (!static_haveProcedures) {
        return;
    }

    auto rm = globals->get_subsystem<FGRouteMgr>();

    bool ok = FGTestApi::executeNasal(R"(
        var fp = flightplan();
        fp.departure = airportinfo("EGLL");
        fp.destination = airportinfo("EHAM");
        fp.star = fp.destination.getStar("REDF1A");
        fp.destination_runway = fp.destination.runway('06');
                                      
        var approach = fp.destination.getApproach("ILS06");
        unitTest.assert(approach != nil, "No approach loaded");
                                      
        var trans = approach.transition('SUG2A');
        unitTest.assert(trans != nil, "approach transition not found");
                                      
        fp.approach = trans;
                  
        unitTest.assert_equal(fp.approach.id, "ILS06", "Incorrect approach returned");
        unitTest.assert_equal(fp.approach_trans.id, "SUG2A", "Incorrect transition returned");
        unitTest.assert_equal(fp.approach_trans.tp_type, "transition", "Procedure type incorrect");
    )");

    CPPUNIT_ASSERT(ok);

    auto fp = rm->flightPlan();

    CPPUNIT_ASSERT(fp->approach());
    CPPUNIT_ASSERT_EQUAL("SUG2A"s, fp->approachTransition()->ident());
    CPPUNIT_ASSERT_EQUAL("ILS06"s, fp->approach()->ident());

    auto fp2 = fp->clone("testplan2");
    CPPUNIT_ASSERT_EQUAL("ILS06"s, fp2->approach()->ident());
    CPPUNIT_ASSERT_EQUAL("SUG2A"s, fp2->approachTransition()->ident());
}

void FPNasalTests::testAirwaysAPI()
{
    bool ok = FGTestApi::executeNasal(R"(
        var airwayIdent = "L620";
        var airwayStore = airway(airwayIdent, "low");
        unitTest.assert(airwayStore != nil, "Airway " ~ airwayIdent ~ " not found");
        unitTest.assert(airwayStore.id == airwayIdent, "Incorrect airway found");
        unitTest.assert_equal(airwayStore.level, 'low', "Incorrect airway found");
        unitTest.assert_equal(airwayStore.level_code, Airway.LOW, "Incorrect airway found");

        airwayIdent = "UL620";
        var cln = findNavaidsByID("CLN", "VOR")[0];
        airwayStore = airway(airwayIdent, cln);
        unitTest.assert(airwayStore != nil, "Airway " ~ airwayIdent ~ " not found");
        unitTest.assert(airwayStore.id == airwayIdent, "Incorrect airway found");
        unitTest.assert_equal(airwayStore.level_code, Airway.HIGH, "Incorrect airway found");

        airwayIdent = "J547";
        airwayStore = airway(airwayIdent);
        unitTest.assert(airwayStore != nil, "Airway " ~ airwayIdent ~ " not found");
        unitTest.assert(airwayStore.id == airwayIdent, "Incorrect airway found");
    )");

    CPPUNIT_ASSERT(ok);

    ok = FGTestApi::executeNasal(R"(
    
        var airwayIdent = "L620";
        var airwayStore = airway(airwayIdent, Airway.LOW);
        var cln = findNavaidsByID("CLN", "VOR")[0];

        var v1 = createViaTo(airwayIdent, "CLN");
        unitTest.assert_equal(v1.wp_type, "via");
        unitTest.assert_equal(v1.airway.id, 'L620');
        unitTest.assert_equal(v1.airway.level_code, Airway.LOW);

        var v2 = createViaTo(airwayStore, "TULIP");
        unitTest.assert_equal(v2.wp_type, "via");
        unitTest.assert_equal(v2.airway.id, airwayIdent);

        var v3 = createViaFromTo(cln, "L620", 'low', "TULIP");
        unitTest.assert_equal(v3.airway.id, 'L620');
    
        var v4 = createViaFromTo(cln, "L620", "REDFA");
        unitTest.assert_equal(v4.airway.level_code, Airway.LOW);
    
        # test direct API (no Vias)
        var wps = airwayStore.viaWaypoints(cln, "TULIP");
        unitTest.assert_equal(size(wps), 3);

        unitTest.assert_equal(wps[1].id, 'REDFA');
        unitTest.assert_equal(wps[1].airway.id, 'L620');

    )");

    CPPUNIT_ASSERT(ok);
    
}

void FPNasalTests::testTotalDistanceAPI()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();

    bool ok = FGTestApi::executeNasal(R"(
        var fp = flightplan();
        fp.departure = airportinfo("BIKF");
        fp.destination = airportinfo("EGLL");
        unitTest.assert_doubles_equal(1025.9, fp.totalDistanceNm, 0.1, "Distance assertion failed");
    )");

    CPPUNIT_ASSERT(ok);

    auto fp = rm->flightPlan();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp->totalDistanceNm(), 1025.9, 0.1);
}
