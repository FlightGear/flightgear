#include "test_flightplan.hxx"

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

using namespace flightgear;


// Set up function for each test.
void FlightplanTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("flightplan");
    FGTestApi::setUp::initNavDataCache();
    
    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();
}


// Clean up after each test.
void FlightplanTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

static FlightPlanRef makeTestFP(const std::string& depICAO, const std::string& depRunway,
                         const std::string& destICAO, const std::string& destRunway,
                         const std::string& waypoints)
{
    FlightPlanRef f = new FlightPlan;
    FGTestApi::setUp::populateFPWithoutNasal(f, depICAO, depRunway, destICAO, destRunway, waypoints);
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

    CPPUNIT_ASSERT_EQUAL(5, fp1->numLegs());

    CPPUNIT_ASSERT(fp1->legAtIndex(0)->waypoint()->source()->ident() == "23L");

    CPPUNIT_ASSERT(fp1->legAtIndex(1)->waypoint()->source()->ident() == "TNT");
    CPPUNIT_ASSERT(fp1->legAtIndex(1)->waypoint()->source()->name() == "TRENT VOR-DME");

    CPPUNIT_ASSERT(fp1->legAtIndex(2)->waypoint()->source()->ident() == "CLN");
    CPPUNIT_ASSERT(fp1->legAtIndex(2)->waypoint()->source()->name() == "CLACTON VOR-DME");

    CPPUNIT_ASSERT(fp1->legAtIndex(4)->waypoint()->source()->ident() == "24");

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

    CPPUNIT_ASSERT_DOUBLES_EQUAL(trackDeg, rtepath.trackForIndex(4), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distM, rtepath.distanceForIndex(4), 2000); // 2km precision, allow for turns

}

// https://sourceforge.net/p/flightgear/codetickets/1703/
// https://sourceforge.net/p/flightgear/codetickets/1939/

void FlightplanTests::testRoutePathSkipped()
{
    FlightPlanRef fp1 = makeTestFP("EHAM", "24", "EDDM", "08L",
                                   "EHEH KBO TAU FFM FFM/100/0.01 FFM/120/0.02 WUR WLD");


    RoutePath rtepath(fp1);

    // skipped point uses inbound track
    CPPUNIT_ASSERT_EQUAL(rtepath.trackForIndex(4), rtepath.trackForIndex(5));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, rtepath.distanceForIndex(5), 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, rtepath.distanceForIndex(6), 1e-9);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(101000, rtepath.distanceForIndex(7), 1000);


    // this tests skipping two preceeding points works as it should
    SGGeodVec vec = rtepath.pathForIndex(7);
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

    CPPUNIT_ASSERT_DOUBLES_EQUAL(19.68, fp1->totalDistanceNm(), 0.1);
}

void FlightplanTests::testRoutePathVec()
{
    FlightPlanRef fp1 = makeTestFP("KNUQ", "14L", "PHNL", "22R",
                                   "ROKME WOVAB");
    RoutePath rtepath(fp1);

    SGGeodVec vec = rtepath.pathForIndex(0);

    FGAirportRef ksfo = FGAirport::findByIdent("KSFO");
    FGFixRef rokme = fgpositioned_cast<FGFix>(FGPositioned::findClosestWithIdent("ROKME", ksfo->geod()));
    auto depRwy = fp1->departureRunway();

    CPPUNIT_ASSERT_DOUBLES_EQUAL(depRwy->geod().getLongitudeDeg(), vec.front().getLongitudeDeg(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(depRwy->geod().getLatitudeDeg(), vec.front().getLatitudeDeg(), 0.01);

    SGGeodVec vec1 = rtepath.pathForIndex(1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLongitudeDeg(), vec1.back().getLongitudeDeg(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLatitudeDeg(), vec1.back().getLatitudeDeg(), 0.01);

    SGGeodVec vec2 = rtepath.pathForIndex(2);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLongitudeDeg(), vec2.front().getLongitudeDeg(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLatitudeDeg(), vec2.front().getLatitudeDeg(), 0.01);

    //CPPUNIT_ASSERT(vec.front()
}

void FlightplanTests::testRoutPathWpt0Midflight()
{
    // test behaviour of RoutePath when WP0 is not a runway
    // happens for the Airbus ND which removes past wpts when sequencing

    FlightPlanRef fp1 = makeTestFP("KNUQ", "14L", "PHNL", "22R",
                                   "ROKME WOVAB");
    // actually delete leg 0 so we start at ROKME
    fp1->deleteIndex(0);

    RoutePath rtepath(fp1);

    SGGeodVec vec = rtepath.pathForIndex(0);

    FGAirportRef ksfo = FGAirport::findByIdent("KSFO");
    FGFixRef rokme = fgpositioned_cast<FGFix>(FGPositioned::findClosestWithIdent("ROKME", ksfo->geod()));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLongitudeDeg(), vec.front().getLongitudeDeg(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLatitudeDeg(), vec.front().getLatitudeDeg(), 0.01);

    SGGeodVec vec2 = rtepath.pathForIndex(1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLongitudeDeg(), vec2.front().getLongitudeDeg(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLatitudeDeg(), vec2.front().getLatitudeDeg(), 0.01);

    //CPPUNIT_ASSERT(vec.front()
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

void FlightplanTests::testLoadSaveMachRestriction()
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
                <wp n="0">
                  <type type="string">navaid</type>
                  <ident type="string">PUGLI</ident>
                  <lon type="double">-60.552200</lon>
                  <lat type="double">-40.490000</lat>
                </wp>
                <wp n="1">
                  <type type="string">basic</type>
                  <alt-restrict type="string">at</alt-restrict>
                  <altitude-ft type="double">36000</altitude-ft>
                  <speed-restrict type="string">mach</speed-restrict>
                  <speed type="double">1.24</speed>
                  <ident type="string">SV002</ident>
                  <lon type="double">-115.50531</lon>
                  <lat type="double">37.89523</lat>
                </wp>
                <wp n="2">
                     <type type="string">navaid</type>
                     <ident type="string">SIGUL</ident>
                     <lon type="double">-60.552200</lon>
                     <lat type="double">-40.490000</lat>
                   </wp>
           </route>
       </PropertyList>
     )";
    
     std::istringstream stream(fpXML);
     FlightPlanRef f = new FlightPlan;
     bool ok = f->load(stream);
     CPPUNIT_ASSERT(ok);

     auto leg = f->legAtIndex(1);
    CPPUNIT_ASSERT_EQUAL(SPEED_RESTRICT_MACH, leg->speedRestriction());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.24, leg->speedMach(), 0.01);
    
    auto firstLeg = f->legAtIndex(0);
    firstLeg->setSpeed(SPEED_RESTRICT_MACH, 1.56);
    
    // upgrade to a hold and set the count
    f->legAtIndex(2)->setHoldCount(8);
    
    // round trip through XML to check :)
    std::ostringstream ss;
    f->save(ss);
    
    std::istringstream iss(ss.str());
    FlightPlanRef f2 = new FlightPlan;
    ok = f2->load(iss);
    CPPUNIT_ASSERT(ok);
    
    auto leg3 = f2->legAtIndex(0);
    CPPUNIT_ASSERT_EQUAL(SPEED_RESTRICT_MACH, leg3->speedRestriction());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.56, leg3->speedMach(), 0.01);
    
    CPPUNIT_ASSERT_EQUAL(8, f2->legAtIndex(2)->holdCount());
}

void FlightplanTests::testBasicDiscontinuity()
{
    FlightPlanRef fp1 = makeTestFP("LIRF", "34L", "LEBL", "25R",
                                   "ESINO GITRI BALEN MUREN TOSNU");

    const auto tdBefore = fp1->totalDistanceNm();
    
    
    const SGGeod balenPos = fp1->legAtIndex(3)->waypoint()->position();
    const SGGeod murenPos = fp1->legAtIndex(4)->waypoint()->position();
    const auto crs = SGGeodesy::courseDeg(balenPos, murenPos);
    
    // total distance should not change
    fp1->insertWayptAtIndex(new Discontinuity(fp1), 4); // betwee BALEN and MUREN
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(tdBefore, fp1->totalDistanceNm(), 1.0);

    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(4)->courseDeg(),
                                 SGGeodesy::courseDeg(balenPos, murenPos), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(4)->distanceNm(), 0.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(5)->courseDeg(),
                                 SGGeodesy::courseDeg(balenPos, murenPos), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(5)->distanceNm(),
                                 SGGeodesy::distanceNm(balenPos, murenPos), 0.1);
    
    // ensure that pointAlongRoute works correctly going into the DISCON
    
    
    const auto pos1 = fp1->pointAlongRoute(3, 20.0);
    const auto validP1 = SGGeodesy::direct(balenPos, crs, 20.0 * SG_NM_TO_METER);
    
    CPPUNIT_ASSERT(SGGeodesy::distanceM(pos1, validP1) < 500.0);
    
    const auto pos2 = fp1->pointAlongRoute(5, -10.0);
    const auto crs2 = SGGeodesy::courseDeg(murenPos, balenPos);
    const auto validP2 = SGGeodesy::direct(murenPos, crs2, 10.0 * SG_NM_TO_METER);
    
    CPPUNIT_ASSERT(SGGeodesy::distanceM(pos2, validP2) < 500.0);
    
    // remove the discontinuity
    fp1->deleteIndex(4);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(4)->courseDeg(),
                                 SGGeodesy::courseDeg(balenPos, murenPos), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(4)->distanceNm(),
                                 SGGeodesy::distanceNm(balenPos, murenPos), 0.1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(tdBefore, fp1->totalDistanceNm(), 1.0);

}

void FlightplanTests::testOnlyDiscontinuityRoute()
{
    FlightPlanRef f = new FlightPlan;
    FGAirportRef depApt = FGAirport::getByIdent("LFBD");
    f->setDeparture(depApt);


    FGAirportRef destApt = FGAirport::getByIdent("EHAM");
    f->setDestination(destApt);

    f->insertWayptAtIndex(new Discontinuity(f), 0);
    
    RoutePath rp1(f);
    
    // discontinuity should act like a straight segment between preceeding and following
    const double d = SGGeodesy::distanceNm(depApt->geod(), destApt->geod());
   // CPPUNIT_ASSERT_DOUBLES_EQUAL(rp1.distanceForIndex(0), d, 0.5);
    
    // start inserting waypoints ahead of the DISCON, Boeing FPL style
    WayptRef wpt = f->waypointFromString("LMG");
    f->insertWayptAtIndex(wpt, 0);
    
    wpt = f->waypointFromString("KUKOR");
    f->insertWayptAtIndex(wpt, 1);
    
    wpt = f->waypointFromString("EPL");
    f->insertWayptAtIndex(wpt, 2);
}

void FlightplanTests::testLeadingWPDynamic()
{
    FlightPlanRef f = new FlightPlan;
    // plan has no departure, so this discon is floating
    f->insertWayptAtIndex(new Discontinuity(f), 0);
    
    auto ha = new HeadingToAltitude(f, "TO_3000", 90);
    ha->setAltitude(3000, RESTRICT_AT);
    f->insertWayptAtIndex(ha, 1);
    
    RoutePath rp1(f);
    // distance will be invalid, but shouldn;t assert or crash :)
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, f->totalDistanceNm(), 0.001);
}

void FlightplanTests::testRadialIntercept()
{
    // replicate AJO1R departure
    FlightPlanRef f = makeTestFP("LFKC", "36", "LIRF", "25", "BUNAX BEBEV AJO");
    
    f->insertWayptAtIndex(new BasicWaypt(SGGeod::fromDeg(8.78333, 42.566), "KC502", f), 1);

    SGGeod pos = SGGeod::fromDeg(8.445556,42.216944);
    auto intc = new RadialIntercept(f, "INTC", pos, 230, 5);
    f->insertWayptAtIndex(intc, 3); // between BUNAX and BEBEV
    
    RoutePath rtepath(f);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(232, f->legAtIndex(3)->courseDeg(), 1.0);
}
