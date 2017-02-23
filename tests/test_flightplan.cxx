#include "config.h"

#include "unitTestHelpers.hxx"

#include <simgear/misc/test_macros.hxx>
#include <simgear/misc/strutils.hxx>

#include <Navaids/FlightPlan.hxx>
#include <Navaids/routePath.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/navrecord.hxx>

#include <Airports/airport.hxx>

using namespace flightgear;

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

void testBasic()
{
    FlightPlanRef fp1 = makeTestFP("EGCC", "23L", "EHAM", "24",
                                   "TNT CLN");
    fp1->setIdent("testplan");

    SG_CHECK_EQUAL(fp1->ident(), "testplan");
    SG_CHECK_EQUAL(fp1->departureAirport()->ident(), "EGCC");
    SG_CHECK_EQUAL(fp1->departureRunway()->ident(), "23L");
    SG_CHECK_EQUAL(fp1->destinationAirport()->ident(), "EHAM");
    SG_CHECK_EQUAL(fp1->destinationRunway()->ident(), "24");

    SG_CHECK_EQUAL(fp1->numLegs(), 2);

    SG_CHECK_EQUAL(fp1->legAtIndex(0)->waypoint()->source()->ident(), "TNT");
    SG_CHECK_EQUAL(fp1->legAtIndex(0)->waypoint()->source()->name(), "TRENT VOR-DME");

    SG_CHECK_EQUAL(fp1->legAtIndex(1)->waypoint()->source()->ident(), "CLN");
    SG_CHECK_EQUAL(fp1->legAtIndex(1)->waypoint()->source()->name(), "CLACTON VOR-DME");
}

void testRoutePathBasic()
{
    FlightPlanRef fp1 = makeTestFP("EGHI", "20", "EDDM", "08L",
                                   "SFD LYD BNE CIV ELLX LUX SAA KRH WLD");


    RoutePath rtepath(fp1);
    const unsigned int legCount = fp1->numLegs();
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

    SG_CHECK_EQUAL_EP2(trackDeg, rtepath.trackForIndex(3), 0.5);
    SG_CHECK_EQUAL_EP2(distM, rtepath.distanceForIndex(3), 2000); // 2km precision, allow for turns

}

// https://sourceforge.net/p/flightgear/codetickets/1703/
// https://sourceforge.net/p/flightgear/codetickets/1939/

void testRoutePathSkipped()
{
    FlightPlanRef fp1 = makeTestFP("EHAM", "24", "EDDM", "08L",
                                   "EHEH KBO TAU FFM FFM/100/0.01 FFM/120/0.02 WUR WLD");


    RoutePath rtepath(fp1);

    // skipped point uses inbound track
    SG_CHECK_EQUAL_EP(rtepath.trackForIndex(3), rtepath.trackForIndex(4));

    SG_CHECK_EQUAL_EP(0.0, rtepath.distanceForIndex(4));
    SG_CHECK_EQUAL_EP(0.0, rtepath.distanceForIndex(5));

    SG_CHECK_EQUAL_EP2(101000, rtepath.distanceForIndex(6), 1000);


    // this tests skipping two preceeding points works as it should
    SGGeodVec vec = rtepath.pathForIndex(6);
    SG_CHECK_EQUAL(vec.size(), 9);


}

void testRoutePathTrivialFlightPlan()
{
    FlightPlanRef fp1 = makeTestFP("EGPH", "24", "EGPH", "06",
                                   "");


    RoutePath rtepath(fp1);
    const unsigned int legCount = fp1->numLegs();
    for (int leg = 0; leg < legCount; ++leg) {
        rtepath.trackForIndex(leg);
        rtepath.pathForIndex(leg);
        rtepath.distanceForIndex(leg);
    }

    SG_CHECK_EQUAL_EP(0.0, fp1->totalDistanceNm());
}

int main(int argc, char* argv[])
{
    fgtest::initTestGlobals("flightplan");

    testBasic();
    testRoutePathBasic();
    testRoutePathSkipped();
    testRoutePathTrivialFlightPlan();
    
    fgtest::shutdownTestGlobals();
}
