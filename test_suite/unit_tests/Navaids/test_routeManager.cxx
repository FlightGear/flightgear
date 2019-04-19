#include "test_routeManager.hxx"

#include <memory>
#include <cstring>

#include "test_suite/FGTestApi/globals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestPilot.hxx"

#include <Navaids/FlightPlan.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/navlist.hxx>

// we need a default GPS instrument, hard to test seperately for now
#include <Instrumentation/gps.hxx>

#include <Autopilot/route_mgr.hxx>

using namespace flightgear;

static FlightPlanRef makeTestFP(const std::string& depICAO, const std::string& depRunway,
                         const std::string& destICAO, const std::string& destRunway,
                         const std::string& waypoints)
{
    FlightPlanRef f = new FlightPlan;
    FGTestApi::setUp::populateFPWithNasal(f, depICAO, depRunway, destICAO, destRunway, waypoints);
    return f;
}


// Set up function for each test.
void RouteManagerTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("routemanager");
    FGTestApi::setUp::initNavDataCache();
    
    globals->add_new_subsystem<FGRouteMgr>();
    
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
    globals->add_subsystem("gps", gps, SGSubsystemMgr::POST_FDM);
    
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
    FGTestApi::setUp::logPositionToKML("rm_basic");
    
    FlightPlanRef fp1 = makeTestFP("EGLC", "27", "EHAM", "06",
                                   "CLN IDESI RINIS VALKO RIVER RTM EKROS");
    fp1->setIdent("testplan");
    fp1->setCruiseFlightLevel(360);
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    rm->setFlightPlan(fp1);
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps", true);
    CPPUNIT_ASSERT(!strcmp("obs", gpsNode->getStringValue("mode")));

    rm->activate();
    
    CPPUNIT_ASSERT(fp1->isActive());
    
    // Nasal deleagte should have placed GPS into leg mode
    auto rmNode = globals->get_props()->getNode("autopilot/route-manager", true);
    
    CPPUNIT_ASSERT(!strcmp("leg", gpsNode->getStringValue("mode")));

    CPPUNIT_ASSERT(!strcmp("EGLC", rmNode->getStringValue("departure/airport")));
    CPPUNIT_ASSERT(!strcmp("27", rmNode->getStringValue("departure/runway")));
    CPPUNIT_ASSERT(!strcmp("", rmNode->getStringValue("departure/sid")));
    CPPUNIT_ASSERT(!strcmp("London City", rmNode->getStringValue("departure/name")));

    CPPUNIT_ASSERT(!strcmp("EHAM", rmNode->getStringValue("destination/airport")));
    CPPUNIT_ASSERT(!strcmp("06", rmNode->getStringValue("destination/runway")));
    
    CPPUNIT_ASSERT_EQUAL(360, rmNode->getIntValue("cruise/flight-level"));
    CPPUNIT_ASSERT_EQUAL(false, rmNode->getBoolValue("airborne"));

    CPPUNIT_ASSERT_EQUAL(0, rmNode->getIntValue("current-wp"));
    auto wp0Node = rmNode->getNode("wp");
    CPPUNIT_ASSERT(!strcmp("EGLC-27", wp0Node->getStringValue("id")));
    
    auto wp1Node = rmNode->getNode("wp[1]");
    CPPUNIT_ASSERT(!strcmp("CLN", wp1Node->getStringValue("id")));

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
    CPPUNIT_ASSERT(!strcmp("RIVER", wp0Node->getStringValue("id")));
    CPPUNIT_ASSERT(!strcmp("RTM", wp1Node->getStringValue("id")));
    // slightly rapid descent 
    pilot->setTargetAltitudeFtMSL(3000);

    // temporary until GPS course / deviation fixes are merged
    return;
    
    ok = FGTestApi::runForTimeWithCheck(6000.0, [rmNode] () {
        if (rmNode->getIntValue("current-wp") == 7) return true;
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    // run until the GPS reverts to OBS mode at the end of the flight plan
    ok = FGTestApi::runForTimeWithCheck(6000.0, [gpsNode] () {
        std::string mode = gpsNode->getStringValue("mode");
        if (mode == "obs") return true;
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
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps", true);
    auto rmNode = globals->get_props()->getNode("autopilot/route-manager", true);
    rmNode->setStringValue("departure/sid", "DEFAULT");
    
    // let's see what we got :)
    
    rm->activate();
    
    CPPUNIT_ASSERT(fp1->isActive());
}

void RouteManagerTests::testDefaultApproach()
{
    
}

