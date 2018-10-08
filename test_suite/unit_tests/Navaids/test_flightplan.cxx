#include "test_flightplan.hxx"

#include "test_suite/FGTestApi/globals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"

#include <simgear/misc/strutils.hxx>

#include <Navaids/FlightPlan.hxx>
#include <Navaids/routePath.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/airways.hxx>

#include <Airports/airport.hxx>

using namespace flightgear;


// Set up function for each test.
void FlightplanTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("flightplan");
    FGTestApi::setUp::initNavDataCache();
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

void FlightplanTests::testBasicAirways()
{
    Airway* awy = Airway::findByIdent("J547", Airway::HighLevel);
    CPPUNIT_ASSERT_EQUAL(awy->ident(), std::string("J547"));
    
    FGAirportRef kord = FGAirport::findByIdent("KORD");
    FlightPlanRef f = new FlightPlan;
    f->setDeparture(kord);
    
    CPPUNIT_ASSERT(awy->findEnroute("KITOK"));
    CPPUNIT_ASSERT(awy->findEnroute("LESUB"));

    auto wpt = awy->findEnroute("FNT");
    CPPUNIT_ASSERT(wpt);
    CPPUNIT_ASSERT(wpt->ident() == "FNT");
    CPPUNIT_ASSERT(wpt->source() == FGNavRecord::findClosestWithIdent("FNT", kord->geod()));

    auto wptKUBBS = f->waypointFromString("KUBBS");
    auto wptFNT = f->waypointFromString("FNT");
    
    CPPUNIT_ASSERT(awy->canVia(wptKUBBS, wptFNT));
    
    WayptVec path = awy->via(wptKUBBS, wptFNT);
    CPPUNIT_ASSERT_EQUAL(4, static_cast<int>(path.size()));
    
    CPPUNIT_ASSERT_EQUAL(std::string("PMM"), path.at(0)->ident());
    CPPUNIT_ASSERT_EQUAL(std::string("HASTE"), path.at(1)->ident());
    CPPUNIT_ASSERT_EQUAL(std::string("DEWIT"), path.at(2)->ident());
    CPPUNIT_ASSERT_EQUAL(std::string("FNT"), path.at(3)->ident());
}

void FlightplanTests::testAirwayNetworkRoute()
{
    FGAirportRef egph = FGAirport::findByIdent("EGPH");
    FlightPlanRef f = new FlightPlan;
    f->setDeparture(egph);
    
    auto highLevelNet = Airway::highLevel();
    
    auto wptTLA = f->waypointFromString("TLA");
    auto wptCNA = f->waypointFromString("CNA");
    
    WayptVec route;
    bool ok = highLevelNet->route(wptTLA, wptCNA, route);
    CPPUNIT_ASSERT(ok);
    
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(route.size()), 18);
}

void FlightplanTests::testParseICAORoute()
{
    FGAirportRef kord = FGAirport::findByIdent("KORD");
    FlightPlanRef f = new FlightPlan;
    f->setDeparture(kord);
    f->setDestination(FGAirport::findByIdent("KSAN"));
    
    const char* route = "DCT JOT J26 IRK J96 SLN J18 GCK J96 CIM J134 GUP J96 KEYKE J134 DRK J78 LANCY J96 PKE";
  //  const char* route = "DCT KUBBS J547 FNT Q824 HOCKE Q905 SIKBO Q907 MIILS N400A TUDEP NATW GISTI DCT SLANY UL9 DIKAS UL18 GAVGO UL9 KONAN UL607 UBIDU Y863 RUDUS T109 HAREM T104 ANORA STAR";
    bool ok = f->parseICAORouteString(route);
    CPPUNIT_ASSERT(ok);
    
    
    
}

void FlightplanTests::testParseICANLowLevelRoute()
{
    const char* route = "DCT DPA V6 IOW V216 LAA V210 GOSIP V83 ACHES V210 BLOKE V83 ALS V210 RSK V95 INW V12 HOXOL V264 OATES V12 JUWSO V264 PKE";
    
    FGAirportRef kord = FGAirport::findByIdent("KORD");
    FlightPlanRef f = new FlightPlan;
    f->setDeparture(kord);
    f->setDestination(FGAirport::findByIdent("KSAN"));
    
    bool ok = f->parseICAORouteString(route);
    CPPUNIT_ASSERT(ok);
}

// https://sourceforge.net/p/flightgear/codetickets/1814/
void FlightplanTests::testBug1814()
{
    const std::string fpXML = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <PropertyList>
                        <version type="int">2</version>
                        <departure>
                            <airport type="string">SAWG</airport>
                            <runway type="string">25</runway>
                        </departure>
                        <destination>
                            <airport type="string">SUMU</airport>
                        </destination>
                        <route>
                              <wp n="6">
                                    <type type="string">navaid</type>
                                    <ident type="string">PUGLI</ident>
                                    <lon type="double">-60.552200</lon>
                                    <lat type="double">-40.490000</lat>
                                  </wp>
                            <wp n="7">
                                <type type="string">navaid</type>
                                <ident type="string">SIGUL</ident>
                                <lon type="double">-59.655800</lon>
                                <lat type="double">-38.312800</lat>
                            </wp>
                            <wp n="8">
                                <type type="string">navaid</type>
                                <ident type="string">MDP</ident>
                                <lon type="double">-57.576400</lon>
                                <lat type="double">-37.929800</lat>
                            </wp>
                        </route>
                    </PropertyList>
  )";

    std::istringstream stream(fpXML);
    FlightPlanRef f = new FlightPlan;
    bool ok = f->load(stream);
    CPPUNIT_ASSERT(ok);

    auto leg = f->legAtIndex(1);
    CPPUNIT_ASSERT_EQUAL(std::string("SIGUL"), leg->waypoint()->ident());

    CPPUNIT_ASSERT_DOUBLES_EQUAL(137, leg->distanceNm(), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(101, f->legAtIndex(2)->distanceNm(), 0.5);
}
