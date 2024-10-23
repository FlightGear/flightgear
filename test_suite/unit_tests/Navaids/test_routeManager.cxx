#include "config.h"

#include "test_routeManager.hxx"

#include <memory>
#include <cstring>

#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/structure/commands.hxx>

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestPilot.hxx"

#include <Navaids/FlightPlan.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/routePath.hxx>

#include <Main/globals.hxx>
// we need a default GPS instrument, hard to test seperately for now
#include <Instrumentation/gps.hxx>

#include <Autopilot/route_mgr.hxx>

using namespace flightgear;
using namespace std::string_literals;

static bool static_haveProcedures = false;

static FlightPlanRef makeTestFP(const std::string& depICAO, const std::string& depRunway,
                         const std::string& destICAO, const std::string& destRunway,
                         const std::string& waypoints)
{
    FlightPlanRef f = FlightPlan::create();
    FGTestApi::setUp::populateFPWithNasal(f, depICAO, depRunway, destICAO, destRunway, waypoints);
    return f;
}


// Set up function for each test.
void RouteManagerTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("routemanager");
    FGTestApi::setUp::initNavDataCache();
    
    SGPath proceduresPath = SGPath::fromEnv("FG_PROCEDURES_PATH");
    if (proceduresPath.exists()) {
        static_haveProcedures = true;
        globals->append_fg_scenery(proceduresPath);
    }

    globals->get_subsystem_mgr()->add<FGRouteMgr>();

    // setup the default GPS, which is needed for waypoint
    // sequencing to work
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "gps");
    configNode->setIntValue("number", 0);
    GPS* gps(new GPS(configNode, true /* default mode */));
    m_gps = gps;
    
    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation", true)->getChild("gps", 0, true);
  //  node->setBoolValue("serviceable", true);
   // globals->get_props()->setDoubleValue("systems/electrical/outputs/gps", 6.0);
    globals->get_subsystem_mgr()->add("gps", gps);
    
    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    
    FGTestApi::setUp::initStandardNasal();
    globals->get_subsystem_mgr()->postinit();
}


// Clean up after each test.
void RouteManagerTests::tearDown()
{
    m_gps = nullptr;
    FGTestApi::tearDown::shutdownTestGlobals();
}

 void RouteManagerTests::setPositionAndStabilise(const SGGeod& g)
 {
     FGTestApi::setPosition(g);
     for (int i=0; i<60; ++i) {
         globals->get_subsystem_mgr()->update(0.02);
     }
 }

void RouteManagerTests::testBasic()
{
    //FGTestApi::setUp::logPositionToKML("rm_basic");
    
    FlightPlanRef fp1 = makeTestFP("EGLC", "27", "EHAM", "06",
                                   "CLN IDESI RINIS VALKO RIVER RTM EKROS");
    fp1->setIdent("testplan");
    fp1->setCruiseFlightLevel(360);

    CPPUNIT_ASSERT_EQUAL("RIVER"s, fp1->legAtIndex(5)->waypoint()->ident());

    auto rm = globals->get_subsystem<FGRouteMgr>();
    rm->setFlightPlan(fp1);
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps", true);
    CPPUNIT_ASSERT(gpsNode->getStringValue("mode") == "obs");

    rm->activate();
    
    CPPUNIT_ASSERT(fp1->isActive());
    
    // Nasal deleagte should have placed GPS into leg mode
    auto rmNode = globals->get_props()->getNode("autopilot/route-manager", true);
    
    CPPUNIT_ASSERT(gpsNode->getStringValue("mode") == "leg");

    CPPUNIT_ASSERT(rmNode->getStringValue("departure/airport") == "EGLC");
    CPPUNIT_ASSERT(rmNode->getStringValue("departure/runway") == "27");
    CPPUNIT_ASSERT(rmNode->getStringValue("departure/sid") == "");
    CPPUNIT_ASSERT(rmNode->getStringValue("departure/name") == "London City");

    CPPUNIT_ASSERT(rmNode->getStringValue("destination/airport") == "EHAM");
    CPPUNIT_ASSERT(rmNode->getStringValue("destination/runway") == "06");
    
    CPPUNIT_ASSERT_EQUAL(360, rmNode->getIntValue("cruise/flight-level"));
    CPPUNIT_ASSERT_EQUAL(false, rmNode->getBoolValue("airborne"));

    CPPUNIT_ASSERT_EQUAL(0, rmNode->getIntValue("current-wp"));
    auto wp0Node = rmNode->getNode("wp");
    CPPUNIT_ASSERT(wp0Node->getStringValue("id") == "EGLC-27");
    
    auto wp1Node = rmNode->getNode("wp[1]");
    CPPUNIT_ASSERT(wp1Node->getStringValue("id") == "CLN");

    FGPositioned::TypeFilter f{FGPositioned::VOR};
    auto clactonVOR = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("CLN", SGGeod::fromDeg(0.0, 51.0), &f));
    
    // verify hold entry course
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    FGTestApi::setPosition(fp1->departureRunway()->geod());
    pilot->resetAtPosition(fp1->departureRunway()->geod());
    
    pilot->setSpeedKts(220);
    pilot->setCourseTrue(fp1->departureRunway()->headingDeg());
    pilot->setTargetAltitudeFtMSL(10000);
    
    bool ok = FGTestApi::runForTimeWithCheck(30.0, [rmNode] () {
        if (rmNode->getIntValue("current-wp") == 1) return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    
    // continue outbound for some time
    pilot->setSpeedKts(250);
    FGTestApi::runForTime(30.0);
    
    // turn towards Clacton VOR
    pilot->flyDirectTo(clactonVOR->geod());
    pilot->setTargetAltitudeFtMSL(30000);
    pilot->setSpeedKts(280);
    ok = FGTestApi::runForTimeWithCheck(6000.0, [rmNode] () {
        if (rmNode->getIntValue("current-wp") == 2) return true;
        return false;
    });
    CPPUNIT_ASSERT(ok);

    // short straight leg for testing
    pilot->flyHeading(90.0);
    pilot->setSpeedKts(330);
    FGTestApi::runForTime(30.0);

    // let's engage LNAV mode :)
    pilot->flyGPSCourse(m_gps);
    
    ok = FGTestApi::runForTimeWithCheck(6000.0, [rmNode] () {
        if (rmNode->getIntValue("current-wp") == 5) return true;
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    // check where we are - should be heading to RIVER from VALKO
    CPPUNIT_ASSERT_EQUAL(5, rmNode->getIntValue("current-wp"));
    CPPUNIT_ASSERT(wp0Node->getStringValue("id") == "RIVER");
    CPPUNIT_ASSERT(wp1Node->getStringValue("id") == "RTM");
    // slightly rapid descent 
    pilot->setTargetAltitudeFtMSL(3000);

    ok = FGTestApi::runForTimeWithCheck(6000.0, [rmNode] () {
        if (rmNode->getIntValue("current-wp") == 7) return true;
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    // run until the GPS reverts to OBS mode at the end of the flight plan
    ok = FGTestApi::runForTimeWithCheck(6000.0, [gpsNode] () {
        if (gpsNode->getStringValue("mode") == "obs") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(-1, fp1->currentIndex());
    CPPUNIT_ASSERT(!fp1->isActive());
    CPPUNIT_ASSERT_EQUAL(-1, rmNode->getIntValue("current-wp"));
    CPPUNIT_ASSERT_EQUAL(false, rmNode->getBoolValue("active"));
}

void RouteManagerTests::testDefaultSID()
{
    FlightPlanRef fp1 = makeTestFP("EGLC", "27", "EHAM", "24",
                                   "CLN IDRID VALKO");
    fp1->setIdent("testplan");
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    rm->setFlightPlan(fp1);
    
    auto rmNode = globals->get_props()->getNode("autopilot/route-manager", true);
    rmNode->setStringValue("departure/sid", "DEFAULT");
    
    // let's see what we got :)
    
    rm->activate();
    
    CPPUNIT_ASSERT(fp1->isActive());
}

void RouteManagerTests::testDirectToLegOnFlightplanAndResume()
{
 //   FGTestApi::setUp::logPositionToKML("rm_dto_resume_leg");

    // this is very similar to the identiucally name dtest in GPSTests, but relies on the Nasal
    // route manager delegate to perform the same task
    FlightPlanRef fp1 = makeTestFP("EBBR", "07L", "EGGD", "27",
                                   "NIK COA DVR TAWNY WOD");
    auto rm = globals->get_subsystem<FGRouteMgr>();
    rm->setFlightPlan(fp1);
   // FGTestApi::writeFlightPlanToKML(fp1);

    auto gpsNode = globals->get_props()->getNode("instrumentation/gps", true);
    auto rmNode = globals->get_props()->getNode("autopilot/route-manager", true);

    CPPUNIT_ASSERT(gpsNode->getStringValue("mode") == "obs");
    rm->activate();
    
    CPPUNIT_ASSERT(fp1->isActive());

    FGTestApi::setPosition(fp1->departureRunway()->pointOnCenterline(0.0));
    FGTestApi::runForTime(10.0); // let the GPS stabilize

    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, gpsNode->getStringValue("mode"));
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, gpsNode->getStringValue("wp/wp[1]/ID"));
    
    CPPUNIT_ASSERT_EQUAL(0, rmNode->getIntValue("current-wp"));
    auto wp0Node = rmNode->getNode("wp");
    CPPUNIT_ASSERT(wp0Node->getStringValue("id") == "EBBR-07L");
    
    auto wp1Node = rmNode->getNode("wp[1]");
    CPPUNIT_ASSERT(wp1Node->getStringValue("id") == "NIK");

    // initiate a direct to
    SGGeod p2 = fp1->departureRunway()->pointOnCenterline(5.0* SG_NM_TO_METER);
    FGTestApi::setPosition(p2);

    auto doverVOR = fp1->legAtIndex(3)->waypoint()->source();
    
    double distanceToDover = SGGeodesy::distanceNm(p2, doverVOR->geod());
    double bearingToDover = SGGeodesy::courseDeg(p2, doverVOR->geod());
    
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, doverVOR->ident());
    gpsNode->setStringValue("scratch/ident", "DVR");
    gpsNode->setDoubleValue("scratch/longitude-deg", doverVOR->geod().getLongitudeDeg());
    gpsNode->setDoubleValue("scratch/latitude-deg", doverVOR->geod().getLatitudeDeg());
    gpsNode->setStringValue("command", "direct");
    CPPUNIT_ASSERT_EQUAL(std::string{"dto"}, gpsNode->getStringValue("mode"));
    
    // check that upon reaching DOVER, we sequence to TAWNY and resume leg mode
    // this is handled by the default delegate in Nasal
    
    SGGeod posNearDover = SGGeodesy::direct(p2, bearingToDover, (distanceToDover - 8.0) * SG_NM_TO_METER);
    FGTestApi::setPosition(posNearDover);

    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(posNearDover);
    pilot->setSpeedKts(250);
    pilot->flyGPSCourse(m_gps);
    
    bool ok = FGTestApi::runForTimeWithCheck(180.0, [fp1] () {
        if (fp1->currentIndex() == 4) {
            return true;
        }
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, gpsNode->getStringValue("mode"));
}

void RouteManagerTests::testSequenceDiscontinuityAndResume()
{
  //  FGTestApi::setUp::logPositionToKML("rm_seq_discon_resume_leg");

    FlightPlanRef fp1 = makeTestFP("LIRF", "16R", "LEBL", "07L",
                                    "GITRI BALEN MUREN TOSNU");
    
    
    fp1->insertWayptAtIndex(new Discontinuity(fp1), 3);

     auto rm = globals->get_subsystem<FGRouteMgr>();
     rm->setFlightPlan(fp1);

     auto gpsNode = globals->get_props()->getNode("instrumentation/gps", true);
   //  auto rmNode = globals->get_props()->getNode("autopilot/route-manager", true);

    auto balenLeg = fp1->legAtIndex(2);
    auto pos = fp1->pointAlongRoute(2, -8.0); // 8nm before BALEN
    FGTestApi::setPosition(pos);
    FGTestApi::runForTime(10.0); // let the GPS stabilize
    
     CPPUNIT_ASSERT(gpsNode->getStringValue("mode") == "obs");
     rm->activate();
     
     CPPUNIT_ASSERT(fp1->isActive());

    CPPUNIT_ASSERT(gpsNode->getStringValue("mode") == "leg");
    fp1->setCurrentIndex(2);
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(pos);
    pilot->setCourseTrue(270);
    pilot->setSpeedKts(250);
    pilot->flyGPSCourse(m_gps);
    
    bool ok = FGTestApi::runForTimeWithCheck(180.0, [gpsNode] () {
        return (gpsNode->getStringValue("mode") == "obs");
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(2, fp1->currentIndex()); // shouldn't sequence
    
    FGTestApi::runForTime(30.0);
    
    
    // 2nm before MUREN : this should be on the GC course from BALEN
    auto pos2 = fp1->pointAlongRoute(4, -6.0);
    FGTestApi::setPosition(pos2);
    FGTestApi::runForTime(2.0); // let the GPS stabilize

    // initiate a direct-to the next real WP
    const auto murenPos = fp1->legAtIndex(4)->waypoint()->position();
   gpsNode->setStringValue("scratch/ident", "MUREN");
   gpsNode->setDoubleValue("scratch/longitude-deg", murenPos.getLongitudeDeg());
   gpsNode->setDoubleValue("scratch/latitude-deg", murenPos.getLatitudeDeg());
   gpsNode->setStringValue("command", "direct");
   CPPUNIT_ASSERT_EQUAL(std::string{"dto"}, gpsNode->getStringValue("mode"));

    pilot->resetAtPosition(pos2);
    pilot->flyGPSCourse(m_gps);
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp1] () {
        if (fp1->currentIndex() == 5) {
            return true;
        }
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, gpsNode->getStringValue("mode"));
    CPPUNIT_ASSERT_EQUAL(5, fp1->currentIndex());
    
    FGTestApi::runForTime(30.0);

}

void RouteManagerTests::testDefaultApproach()
{
    
}

void RouteManagerTests::testHiddenWaypoints()
{
    FlightPlanRef fp1 = makeTestFP("NZCH", "02", "NZAA", "05L",
                                   "ALADA NS WB WN MAMOD KAPTI OH");
    fp1->setIdent("testplan");
    fp1->setCruiseFlightLevel(360);
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    rm->setFlightPlan(fp1);
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps", true);
    CPPUNIT_ASSERT(gpsNode->getStringValue("mode") == "obs");
    
    // FIXME: use real Nasal test macros soon
    auto testNode = globals->get_props()->getNode("test-data", true);
    
    fp1->legAtIndex(3)->waypoint()->setFlag(WPT_HIDDEN);
    
    // ensure no visual path is generated for hidden waypoints
    RoutePath path(fp1);
    
    // no path should be generated between 2 and 3
    CPPUNIT_ASSERT(path.pathForIndex(3).empty());
    
    // no path should be generated between 3 and 4
    CPPUNIT_ASSERT(path.pathForIndex(4).empty());
    CPPUNIT_ASSERT(!path.pathForIndex(5).empty());
    
    
    bool ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        setprop("/test-data/a", fp.numRemainingWaypoints());
    )");
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(fp1->numLegs(), testNode->getIntValue("a"));
    
    rm->activate();
    fp1->setCurrentIndex(2);
    
    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        setprop("/test-data/a", fp.numRemainingWaypoints());
        setprop("/test-data/b", fp.currentWP(2).id);
    )");
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(7, testNode->getIntValue("a"));
    CPPUNIT_ASSERT_EQUAL(std::string{"WN"}, testNode->getStringValue("b"));
    
    ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        setprop("/test-data/c", fp.currentWP(-1).id);
                                 
        # ensure invalid offset returns nil
        setprop("/test-data/d", fp.currentWP(-100) == nil);
    )");
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"ALADA"}, testNode->getStringValue("c"));
    CPPUNIT_ASSERT_EQUAL(true, testNode->getBoolValue("d"));
}

void RouteManagerTests::testHoldFromNasal()
{
   // FGTestApi::setUp::logPositionToKML("rm_hold_from_nasal");

    // test that Nasal can set a hold-count (implicitly converting a leg
    // to a Hold waypt), configure the hold radial, and exit the hold
    
    FlightPlanRef fp1 = makeTestFP("NZCH", "02", "NZAA", "05L",
                                   "ALADA NS WB WN MAMOD KAPTI OH");
    fp1->setIdent("testplan");
    fp1->setCruiseFlightLevel(360);
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    rm->setFlightPlan(fp1);
  //  FGTestApi::writeFlightPlanToKML(fp1);

    auto gpsNode = globals->get_props()->getNode("instrumentation/gps", true);
    CPPUNIT_ASSERT(gpsNode->getStringValue("mode") == "obs");
    
    rm->activate();
    
    CPPUNIT_ASSERT(fp1->isActive());
    CPPUNIT_ASSERT(gpsNode->getStringValue("mode") == "leg");

    SGGeod posEnrouteToWB = fp1->pointAlongRoute(3, -10.0);
    FGTestApi::setPositionAndStabilise(posEnrouteToWB);
    
    // sequence everything to the correct wp
    fp1->setCurrentIndex(3);

    // setup some hold data from Nasal. To make it more challenging,
    // do it once the wp is already active
    bool ok = FGTestApi::executeNasal(R"(
        var fp = flightplan(); # retrieve the global flightplan
        var leg = fp.getWP(3);
        leg.hold_count = 4;
        leg.hold_heading_radial_deg = 310;
    )");
    CPPUNIT_ASSERT(ok);

    // check the value updated in the leg
    CPPUNIT_ASSERT_EQUAL(4, fp1->legAtIndex(3)->holdCount());

    // check we converted to a hold
    auto wp = fp1->legAtIndex(3)->waypoint();
    auto holdWpt = static_cast<flightgear::Hold*>(wp);

    CPPUNIT_ASSERT_EQUAL(wp->type(), std::string{"hold"});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(310.0, holdWpt->headingRadialDeg(), 0.5);
    
    // establish the test pilot at this position too
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(posEnrouteToWB);
    pilot->setSpeedKts(250);
    pilot->flyGPSCourse(m_gps);
    pilot->setCourseTrue(gpsNode->getDoubleValue("wp/leg-true-course-deg"));
    
    // run for a bit to stabilize everything
    FGTestApi::runForTime(5.0);
    
    // check we upgraded to a hold controller internally, and are flying to it :)
    auto statusNode = gpsNode->getNode("rnav-controller-status");
    CPPUNIT_ASSERT_EQUAL(std::string{"leg-to-hold"}, statusNode->getStringValue());
    
    // check we're on the leg
    CPPUNIT_ASSERT_EQUAL(std::string{"NELSON VOR-DME"}, fp1->legAtIndex(2)->waypoint()->source()->name());

    auto wbPos = fp1->legAtIndex(3)->waypoint()->position();
    auto nsPos = fp1->legAtIndex(2)->waypoint()->position();
    const double crsToWB = SGGeodesy::courseDeg(globals->get_aircraft_position(), wbPos);
    const double crsNSWB = SGGeodesy::courseDeg(nsPos,wbPos);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToWB, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsNSWB, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // fly into the hold, should be teardrop entry
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode] () {
        if (statusNode->getStringValue() == "entry-teardrop") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode] () {
        if (statusNode->getStringValue() == "hold-inbound") return true;
        return false;
    });
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode] () {
        if (statusNode->getStringValue() == "hold-outbound") return true;
        return false;
    });
    CPPUNIT_ASSERT(ok);

    // half way through the outbound turn
    FGTestApi::runForTime(30.0);

    ok = FGTestApi::executeNasal(R"(
                                 setprop("/instrumentation/gps/command", "exit-hold");
                                )");
    CPPUNIT_ASSERT(ok);

    // no change yet
    CPPUNIT_ASSERT_EQUAL(std::string{"hold-outbound"}, statusNode->getStringValue());
    
    // then we fly inbound
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode] () {
        if (statusNode->getStringValue() == "hold-inbound") return true;
        return false;
    });
    CPPUNIT_ASSERT(ok);

    // and then we exit
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp1] () {
        if (fp1->currentIndex() == 4) return true;
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    // get back on course
    FGTestApi::runForTime(60.0);
}

// check that when loading a GPX, airport waypoints are created
// by the default delegate
// https://sourceforge.net/p/flightgear/codetickets/2227/
void RouteManagerTests::loadGPX()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();
    
    FlightPlanRef f = FlightPlan::create();
    rm->setFlightPlan(f);
    
    SGPath gpxPath = simgear::Dir::current().path() / "test_gpx.gpx";
    {
        sg_ofstream s(gpxPath);
        s << R"(<?xml version="1.0" encoding="UTF-8"?>
<gpx xmlns="http://www.topografix.com/GPX/1/1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" creator="SkyVector" version="1.1" xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
  <rte>
    <name>KJFK-KBOS</name>
    <rtept lat="40.639928" lon="-73.778692">
      <name>KJFK</name>
      <overfly>false</overfly>
    </rtept>
    <rtept lat="41.641106" lon="-72.547419">
      <name>HFD</name>
      <overfly>false</overfly>
    </rtept>
    <rtept lat="42.362944" lon="-71.006389">
      <name>KBOS</name>
      <overfly>false</overfly>
    </rtept>

  </rte>
</gpx>
        )";
    }
    
    CPPUNIT_ASSERT(f->load(gpxPath));
    
    auto kbos = FGAirport::getByIdent("KBOS");
    auto kjfk = FGAirport::getByIdent("KJFK");
    CPPUNIT_ASSERT_EQUAL(kjfk, f->departureAirport());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGRunway*>(nullptr), f->departureRunway());
    CPPUNIT_ASSERT_EQUAL(3, f->numLegs());
    CPPUNIT_ASSERT_EQUAL(kbos, f->destinationAirport());

    auto wp1 = f->legAtIndex(1);
    CPPUNIT_ASSERT_EQUAL(std::string{"HFD"}, wp1->waypoint()->ident());
    auto wp2 = f->legAtIndex(2);
    CPPUNIT_ASSERT_EQUAL(std::string{"KBOS"}, wp2->waypoint()->ident());
}

const std::string flightPlanXMLData =
    R"(<?xml version="1.0" encoding="UTF-8"?>
    <PropertyList>
      <version type="int">2</version>
      <departure>
        <airport type="string">EDDM</airport>
        <runway type="string">08R</runway>
      </departure>
      <destination>
        <airport type="string">EDDF</airport>
      </destination>
      <route>
        <wp>
          <type type="string">runway</type>
          <departure type="bool">true</departure>
          <ident type="string">08R</ident>
          <icao type="string">EDDM</icao>
        </wp>
        <wp n="1">
          <type type="string">navaid</type>
          <ident type="string">GIVMI</ident>
          <lon type="double">11.364700</lon>
          <lat type="double">48.701100</lat>
        </wp>
        <wp n="2">
          <type type="string">navaid</type>
          <ident type="string">ERNAS</ident>
          <lon type="double">11.219400</lon>
          <lat type="double">48.844700</lat>
        </wp>
        <wp n="3">
          <type type="string">navaid</type>
          <ident type="string">TALAL</ident>
          <lon type="double">11.085300</lon>
          <lat type="double">49.108300</lat>
        </wp>
        <wp n="4">
          <type type="string">navaid</type>
          <ident type="string">ERMEL</ident>
          <lon type="double">11.044700</lon>
          <lat type="double">49.187800</lat>
        </wp>
        <wp n="5">
          <type type="string">navaid</type>
          <ident type="string">PSA</ident>
          <lon type="double">9.348300</lon>
          <lat type="double">49.862200</lat>
        </wp>
      </route>
    </PropertyList>
)";


// The same test as above, but for a file exported from the route manager or online
void RouteManagerTests::loadFGFP()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();
    
    FlightPlanRef f = FlightPlan::create();
    rm->setFlightPlan(f);
    
    SGPath fgfpPath = simgear::Dir::current().path() / "test_fgfp.fgfp";
    {
        sg_ofstream s(fgfpPath);
        s << flightPlanXMLData;
    }
    
    CPPUNIT_ASSERT(f->load(fgfpPath));
    
    auto eddm = FGAirport::getByIdent("EDDM");
    auto eddf = FGAirport::getByIdent("EDDF");
    CPPUNIT_ASSERT_EQUAL(eddm, f->departureAirport());
    CPPUNIT_ASSERT_EQUAL(eddm->getRunwayByIdent("08R")->ident(), f->departureRunway()->ident());
    CPPUNIT_ASSERT_EQUAL(7, f->numLegs());
    CPPUNIT_ASSERT_EQUAL(eddf, f->destinationAirport());

    auto wp1 = f->legAtIndex(1);
    CPPUNIT_ASSERT_EQUAL(std::string{"GIVMI"}, wp1->waypoint()->ident());
    auto wp2 = f->legAtIndex(6);
    CPPUNIT_ASSERT_EQUAL(std::string{"EDDF"}, wp2->waypoint()->ident());

}

void RouteManagerTests::testRouteWithProcedures()
{
    if (!static_haveProcedures)
        return;
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    
    FlightPlanRef f = FlightPlan::create();
    rm->setFlightPlan(f);
    
    auto kjfk = FGAirport::findByIdent("KJFK");
    auto eham = FGAirport::findByIdent("EHAM");
    f->setDeparture(kjfk->getRunwayByIdent("13L"));
    f->setSID(kjfk->findSIDWithIdent("DEEZZ5.13L"), "CANDR");
    
    f->setDestination(eham->getRunwayByIdent("18R"));
    f->setSTAR(eham->findSTARWithIdent("EEL1A"), "BEDUM");
    
    f->setApproach(eham->findApproachWithIdent("VDM18R"));
    
    auto w = f->waypointFromString("TOMYE");
    f->insertWayptAtIndex(w, f->indexOfFirstNonDepartureWaypoint());
    
    auto w2 = f->waypointFromString("DEVOL");
    f->insertWayptAtIndex(w2, f->indexOfFirstArrivalWaypoint());
    
    // let's check what we got
    
    auto endOfSID = f->legAtIndex(f->indexOfFirstNonDepartureWaypoint() - 1);
    CPPUNIT_ASSERT_EQUAL(endOfSID->waypoint()->ident(), "CANDR"s);

    auto startOfSTAR = f->legAtIndex(f->indexOfFirstArrivalWaypoint());
    CPPUNIT_ASSERT_EQUAL(startOfSTAR->waypoint()->ident(), "BEDUM"s);

    auto endOfSTAR = f->legAtIndex(f->indexOfFirstApproachWaypoint() - 1);
    CPPUNIT_ASSERT_EQUAL(endOfSTAR->waypoint()->ident(), "ARTIP"s);

    auto startOfApproach = f->legAtIndex(f->indexOfFirstApproachWaypoint());
    CPPUNIT_ASSERT_EQUAL(startOfApproach->waypoint()->ident(), "D070O"s);

    auto landingRunway = f->legAtIndex(f->indexOfDestinationRunwayWaypoint());
    CPPUNIT_ASSERT(landingRunway->waypoint()->source() == f->destinationRunway());
    
    auto firstMiss = f->legAtIndex(f->indexOfDestinationRunwayWaypoint() + 1);
    CPPUNIT_ASSERT_EQUAL(firstMiss->waypoint()->ident(), "(461)"s);

    // check it in Nasal too
    bool ok = FGTestApi::executeNasal(
        R"(
        var f = flightplan();
        var depEnd = f.getWP(f.firstNonDepartureLeg - 1);
        var firstArrival = f.getWP(f.firstArrivalLeg);
        var firstApproach = f.getWP(f.firstApproachLeg);
        var destRunway = f.getWP(f.destination_runway_leg);
                                      
        unitTest.assert_equal(depEnd.id, 'CANDR');
                                      
        var firstEnroute = f.getWP(f.firstNonDepartureLeg );
        unitTest.assert_equal(firstEnroute.id, 'TOMYE');
                                      
        unitTest.assert_equal(firstArrival.id, 'BEDUM');
        unitTest.assert_equal(firstApproach.id, 'D070O');
        unitTest.assert_equal(destRunway.id, '18R');
    )");

    CPPUNIT_ASSERT(ok);
}

void RouteManagerTests::testRouteWithApproachProcedures()
{
    if (!static_haveProcedures)
        return;

    auto rm = globals->get_subsystem<FGRouteMgr>();

    FlightPlanRef f = FlightPlan::create();
    rm->setFlightPlan(f);

    auto kjfk = FGAirport::findByIdent("KJFK");
    auto eddm = FGAirport::findByIdent("EDDM");
    f->setDeparture(kjfk->getRunwayByIdent("13L"));

    f->setDestination(eddm->getRunwayByIdent("08R"));
    f->setSTAR(eddm->findSTARWithIdent("ABGA3A.08R"));

    f->setApproach(eddm->findApproachWithIdent("ILS08R"), "NAP08");

    auto w = f->waypointFromString("TOMYE");
    f->insertWayptAtIndex(w, f->indexOfFirstNonDepartureWaypoint());

    auto w2 = f->waypointFromString("DEVOL");
    f->insertWayptAtIndex(w2, f->indexOfFirstArrivalWaypoint());

    // let's check what we got

    auto startOfSTAR = f->legAtIndex(f->indexOfFirstArrivalWaypoint());
    CPPUNIT_ASSERT_EQUAL(startOfSTAR->waypoint()->ident(), "ABGAS"s);

    auto endOfSTAR = f->legAtIndex(f->indexOfFirstApproachWaypoint() - 1);
    CPPUNIT_ASSERT_EQUAL(endOfSTAR->waypoint()->ident(), "MIQ"s);

    auto startOfApproach = f->legAtIndex(f->indexOfFirstApproachWaypoint());
    CPPUNIT_ASSERT_EQUAL(startOfApproach->waypoint()->ident(), "NAPSA"s);

    auto startOfCoreApproach = f->legAtIndex(f->indexOfFirstApproachWaypoint() + 6);
    CPPUNIT_ASSERT_EQUAL(startOfCoreApproach->waypoint()->ident(), "BEGEN"s);

    auto landingRunway = f->legAtIndex(f->indexOfDestinationRunwayWaypoint());
    CPPUNIT_ASSERT(landingRunway->waypoint()->source() == f->destinationRunway());


    //    // check it in Nasal too
    //    bool ok = FGTestApi::executeNasal(
    //        R"(
    //        var f = flightplan();
    //        var depEnd = f.getWP(f.firstNonDepartureLeg - 1);
    //        var firstArrival = f.getWP(f.firstArrivalLeg);
    //        var firstApproach = f.getWP(f.firstApproachLeg);
    //        var destRunway = f.getWP(f.destination_runway_leg);
    //
    //        unitTest.assert_equal(depEnd.id, 'CANDR');
    //
    //        var firstEnroute = f.getWP(f.firstNonDepartureLeg );
    //        unitTest.assert_equal(firstEnroute.id, 'TOMYE');
    //
    //        unitTest.assert_equal(firstArrival.id, 'BEDUM');
    //        unitTest.assert_equal(firstApproach.id, 'D070O');
    //        unitTest.assert_equal(destRunway.id, '18R');
    //    )");
    //
    //    CPPUNIT_ASSERT(ok);
}

void RouteManagerTests::testsSelectNavaid()
{
    // this captures the issue at:
    // https://sourceforge.net/p/flightgear/codetickets/2372/

    auto rm = globals->get_subsystem<FGRouteMgr>();

    FlightPlanRef f = FlightPlan::create();
    rm->setFlightPlan(f);

    auto usss = FGAirport::findByIdent("USSS");
    auto eddh = FGAirport::findByIdent("EDDH");
    f->setDeparture(usss); // Yekaterinberg
    f->setDestination(eddh);

    auto rmNode = globals->get_props()->getNode("autopilot/route-manager", true);
    rmNode->setStringValue("input", "@INSERT1:UUDD");
    rmNode->setStringValue("input", "@INSERT2:UKKM");
    rmNode->setStringValue("input", "@INSERT2:IP");
    rmNode->setStringValue("input", "@INSERT3:OD");

    auto leg = f->legAtIndex(2);
    auto wp1 = leg->waypoint();
    CPPUNIT_ASSERT_EQUAL(wp1->ident(), "IP"s);
    CPPUNIT_ASSERT_EQUAL(wp1->source()->name(), "ZAKHAROVKA NDB"s);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(227, leg->courseDeg(), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(59, leg->distanceNm(), 0.5);

    leg = f->legAtIndex(3);
    auto wp2 = leg->waypoint();
    CPPUNIT_ASSERT_EQUAL(wp2->ident(), "OD"s);
    CPPUNIT_ASSERT_EQUAL(wp2->source()->name(), "BRYANSK NDB"s);
}

void RouteManagerTests::testsSelectWaypoint()
{
    // another part of the issue at
    // https://sourceforge.net/p/flightgear/codetickets/2372/

    auto rm = globals->get_subsystem<FGRouteMgr>();

    FlightPlanRef f = FlightPlan::create();
    rm->setFlightPlan(f);

    auto rmNode = globals->get_props()->getNode("autopilot/route-manager", true);
    rmNode->setStringValue("input", "70N,015E");
    rmNode->setStringValue("input", "55N,015E");
    rmNode->setStringValue("input", "@INSERT1:OSS");
    rmNode->setStringValue("input", "@INSERT2:BOR");

    auto leg = f->legAtIndex(1);
    auto wp1 = leg->waypoint();
    CPPUNIT_ASSERT_EQUAL(wp1->ident(), "OSS"s);
    CPPUNIT_ASSERT_EQUAL(wp1->source()->name(), "OSTERSUND VOR-DME"s);

    //   CPPUNIT_ASSERT_DOUBLES_EQUAL(227, leg->courseDeg(), 0.5);
    //  CPPUNIT_ASSERT_DOUBLES_EQUAL(59, leg->distanceNm(), 0.5);

    leg = f->legAtIndex(2);
    auto wp2 = leg->waypoint();
    CPPUNIT_ASSERT_EQUAL(wp2->ident(), "BOR"s);
    CPPUNIT_ASSERT_EQUAL(wp2->source()->name(), "BORLANGE VOR-DME"s);
}

void RouteManagerTests::testCommandAPI()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();
    SGPath fgfpPath = simgear::Dir::current().path() / "test_fgfp_2.fgfp";
    {
        sg_ofstream s(fgfpPath);
        s << flightPlanXMLData;
    }

    {
        SGPropertyNode_ptr args(new SGPropertyNode);
        args->setStringValue("path", fgfpPath.utf8Str());
        CPPUNIT_ASSERT(globals->get_commands()->execute("load-flightplan", args));
    }

    auto f = rm->flightPlan();
    CPPUNIT_ASSERT_EQUAL(7, f->numLegs());

    CPPUNIT_ASSERT(!f->isActive());

    {
        SGPropertyNode_ptr args(new SGPropertyNode);
        CPPUNIT_ASSERT(globals->get_commands()->execute("activate-flightplan", args));
    }

    CPPUNIT_ASSERT(f->isActive());

    {
        SGPropertyNode_ptr args(new SGPropertyNode);
        args->setIntValue("index", 3);
        CPPUNIT_ASSERT(globals->get_commands()->execute("set-active-waypt", args));
    }

    CPPUNIT_ASSERT_EQUAL(3, f->currentIndex());

    {
        SGPropertyNode_ptr args(new SGPropertyNode);
        args->setIntValue("index", 4);
        args->setStringValue("navaid", "WLD");

        // let's build an offset waypoint for fun
        args->setDoubleValue("offset-nm", 10.0);
        args->setDoubleValue("radial", 30);
        CPPUNIT_ASSERT(globals->get_commands()->execute("insert-waypt", args));
    }

    auto waldaWpt = f->legAtIndex(4)->waypoint();
    auto waldaVOR = waldaWpt->source();
    CPPUNIT_ASSERT_EQUAL("WALDA VOR-DME"s, waldaVOR->name());

    auto d = SGGeodesy::distanceNm(waldaVOR->geod(), waldaWpt->position());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, d, 0.1);
}

void RouteManagerTests::testRMBug2616()
{
    auto edty = FGAirport::findByIdent("EDTY"s);
    edty->testSuiteInjectProceduresXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "EDTY.procedures.xml");

    auto ils28Approach = edty->findApproachWithIdent("ILS28"s);
    CPPUNIT_ASSERT(ils28Approach);
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto f = rm->flightPlan();
    f->clearLegs();
    
    auto edds = FGAirport::findByIdent("EDDS");
    f->setDeparture(edds->getRunwayByIdent("25"));
    f->setDestination(edty->getRunwayByIdent("28"));
    f->setApproach(ils28Approach);
    
      CPPUNIT_ASSERT(f->destinationRunway()->ident() == "28"s);
      CPPUNIT_ASSERT(f->approach()->ident() == "ILS28"s);


    rm->activate();
    CPPUNIT_ASSERT(f->isActive());
}

void RouteManagerTests::testsSelectWaypoint2()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();
    FlightPlanRef f = FlightPlan::create();
    rm->setFlightPlan(f);
    auto rmNode = globals->get_props()->getNode("autopilot/route-manager", true);
    rmNode->setStringValue("input", "UAAA");
    rmNode->setStringValue("input", "EDDH");
    rmNode->setStringValue("input", "@INSERT1:ALM");
    auto leg = f->legAtIndex(1);
    auto wp1 = leg->waypoint();
    CPPUNIT_ASSERT_EQUAL("ALM"s, wp1->ident());
    // CPPUNIT_ASSERT_EQUAL(wp1->source()->name(), string{"ALMATY VOR-DME"});
}

void RouteManagerTests::testAppendWaypoint() {
    // https://sourceforge.net/p/flightgear/codetickets/2829/
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    FlightPlanRef f = FlightPlan::create();
    rm->setFlightPlan(f);
    auto rmNode = globals->get_props()->getNode("autopilot/route-manager", true);
    rmNode->setStringValue("input", "LFOH");
    rmNode->setStringValue("input", "NK");
    rmNode->setStringValue("input", "@INSERT2:NK");

    CPPUNIT_ASSERT_EQUAL("NK"s, f->legAtIndex(1)->waypoint()->source()->name());
    CPPUNIT_ASSERT_EQUAL("NK"s, f->legAtIndex(2)->waypoint()->source()->name());
}
