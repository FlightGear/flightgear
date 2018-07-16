#include "test_flightplan.hxx"

#include "test_suite/FGTestApi/globals.hxx"

#include <simgear/misc/strutils.hxx>

#include <Navaids/FlightPlan.hxx>
#include <Navaids/routePath.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/navrecord.hxx>

#include <Airports/airport.hxx>

using namespace flightgear;


// Set up function for each test.
void FlightplanTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("flightplan");
}


// Clean up after each test.
void FlightplanTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


FlightPlanRef makeTestFP(const std::string& depICAO, const std::string& depRunway,
                         const std::string& destICAO, const std::string& destRunway,
                         const std::string& waypoints)
{
    FlightPlanRef f = new FlightPlan;

    FGAirportRef depApt = FGAirport::getByIdent(depICAO);
    f->setDeparture(depApt->getRunwayByIdent(depRunway));


    FGAirportRef destApt = FGAirport::getByIdent(destICAO);
    f->setDestination(destApt->getRunwayByIdent(destRunway));

    for (auto ws : simgear::strutils::split(waypoints)) {
        WayptRef wpt = f->waypointFromString(ws);
        f->insertWayptAtIndex(wpt, -1);
    }

    return f;
}

void FlightplanTests::testBasic()
{
    FlightPlanRef fp1 = makeTestFP("EGCC", "23L", "EHAM", "24",
                                   "TNT CLN");
    fp1->setIdent("testplan");

    CPPUNIT_ASSERT(fp1->ident() == "testplan");
    CPPUNIT_ASSERT(fp1->departureAirport()->ident() == "EGCC");
    CPPUNIT_ASSERT(fp1->departureRunway()->ident() == "23L");
    CPPUNIT_ASSERT(fp1->destinationAirport()->ident() == "EHAM");
    CPPUNIT_ASSERT(fp1->destinationRunway()->ident() == "24");

    CPPUNIT_ASSERT_EQUAL(fp1->numLegs(), 2);

    CPPUNIT_ASSERT(fp1->legAtIndex(0)->waypoint()->source()->ident() == "TNT");
    CPPUNIT_ASSERT(fp1->legAtIndex(0)->waypoint()->source()->name() == "TRENT VOR-DME");

    CPPUNIT_ASSERT(fp1->legAtIndex(1)->waypoint()->source()->ident() == "CLN");
    CPPUNIT_ASSERT(fp1->legAtIndex(1)->waypoint()->source()->name() == "CLACTON VOR-DME");
}

void FlightplanTests::testRoutePathBasic()
{
    FlightPlanRef fp1 = makeTestFP("EGHI", "20", "EDDM", "08L",
                                   "SFD LYD BNE CIV ELLX LUX SAA KRH WLD");


    RoutePath rtepath(fp1);
    const int legCount = fp1->numLegs();
    for (int leg = 0; leg < legCount; ++leg) {
        rtepath.trackForIndex(leg);
        rtepath.pathForIndex(leg);
        rtepath.distanceForIndex(leg);
    }

    rtepath.distanceBetweenIndices(2, 5);

    // check some leg parameters

    // BOLOUGNE SUR MER, near LFAY (AMIENS)
    FGNavRecordRef bne = FGNavList::findByFreq(113.8, FGAirport::getByIdent("LFAY")->geod());

    // CHIEVRES
    FGNavRecordRef civ = FGNavList::findByFreq(113.2, FGAirport::getByIdent("EBCI")->geod());

    double distM = SGGeodesy::distanceM(bne->geod(), civ->geod());
    double trackDeg = SGGeodesy::courseDeg(bne->geod(), civ->geod());

    CPPUNIT_ASSERT_DOUBLES_EQUAL(trackDeg, rtepath.trackForIndex(3), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distM, rtepath.distanceForIndex(3), 2000); // 2km precision, allow for turns

}

// https://sourceforge.net/p/flightgear/codetickets/1703/
// https://sourceforge.net/p/flightgear/codetickets/1939/

void FlightplanTests::testRoutePathSkipped()
{
    FlightPlanRef fp1 = makeTestFP("EHAM", "24", "EDDM", "08L",
                                   "EHEH KBO TAU FFM FFM/100/0.01 FFM/120/0.02 WUR WLD");


    RoutePath rtepath(fp1);

    // skipped point uses inbound track
    CPPUNIT_ASSERT_EQUAL(rtepath.trackForIndex(3), rtepath.trackForIndex(4));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, rtepath.distanceForIndex(4), 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, rtepath.distanceForIndex(5), 1e-9);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(101000, rtepath.distanceForIndex(6), 1000);


    // this tests skipping two preceeding points works as it should
    SGGeodVec vec = rtepath.pathForIndex(6);
    CPPUNIT_ASSERT(vec.size() == 9);


}

void FlightplanTests::testRoutePathTrivialFlightPlan()
{
    FlightPlanRef fp1 = makeTestFP("EGPH", "24", "EGPH", "06",
                                   "");


    RoutePath rtepath(fp1);
    const int legCount = fp1->numLegs();
    for (int leg = 0; leg < legCount; ++leg) {
        rtepath.trackForIndex(leg);
        rtepath.pathForIndex(leg);
        rtepath.distanceForIndex(leg);
    }

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, fp1->totalDistanceNm(), 1e-9);
}
