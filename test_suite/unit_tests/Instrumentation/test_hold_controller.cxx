/*
 * Copyright (C) 2019 James Turner
 *
 * This file is part of the program FlightGear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "test_hold_controller.hxx"

#include <memory>
#include <cstring>

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestPilot.hxx"

#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/FlightPlan.hxx>

#include <Instrumentation/gps.hxx>
#include <Instrumentation/navradio.hxx>

#include <Autopilot/route_mgr.hxx>

using namespace flightgear;

/////////////////////////////////////////////////////////////////////////////

class TestFPDelegate : public FlightPlan::Delegate
{
public:
    FlightPlanRef thePlan;
    int sequenceCount = 0;

    void sequence() override
    {
        
        ++sequenceCount;
        int newIndex = thePlan->currentIndex() + 1;
        if (newIndex >= thePlan->numLegs()) {
            thePlan->finish();
            return;
        }
        
        thePlan->setCurrentIndex(newIndex);
    }
    
    void currentWaypointChanged() override
    {
    }
    
};

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void HoldControllerTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("hold-ctl");
    FGTestApi::setUp::initNavDataCache();
    
    setupRouteManager();
}

// Clean up after each test.
void HoldControllerTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void HoldControllerTests::setPositionAndStabilise(const SGGeod& g)
{
    FGTestApi::setPosition(g);
    for (int i=0; i<60; ++i) {
        m_gps->update(0.015);
    }
}

GPS* HoldControllerTests::setupStandardGPS(SGPropertyNode_ptr config,
                                const std::string name, const int index)
{
    SGPropertyNode_ptr configNode(config.valid() ? config
                                                 : SGPropertyNode_ptr{new SGPropertyNode});
    configNode->setStringValue("name", name);
    configNode->setIntValue("number", index);
    
    GPS* gps(new GPS(configNode));
    m_gps = gps;

    m_gpsNode = globals->get_props()->getNode("instrumentation", true)->getChild(name, index, true);
    m_gpsNode->setBoolValue("serviceable", true);
    globals->get_props()->setDoubleValue("systems/electrical/outputs/gps", 6.0);
    
    gps->bind();
    gps->init();
    
    globals->add_subsystem("gps", gps, SGSubsystemMgr::POST_FDM);
    return gps;
}

void HoldControllerTests::setupRouteManager()
{
    auto rm = globals->add_new_subsystem<FGRouteMgr>();
    rm->bind();
    rm->init();
    rm->postinit();
}

/////////////////////////////////////////////////////////////////////////////

void HoldControllerTests::testHoldEntryDirect()
{
  //  FGTestApi::setUp::logPositionToKML("hold_direct_entry");

    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                 "NIK COA DVR TAWNY WOD");
   // FGTestApi::writeFlightPlanToKML(fp);

    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);

    setupStandardGPS();
    m_gpsNode->setStringValue("command", "leg");
    setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));

    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});

    auto leg = fp->legAtIndex(3); // DVR vor
    leg->setHoldCount(3);
    CPPUNIT_ASSERT_EQUAL(std::string{"hold"}, leg->waypoint()->type());

    // sequence to that waypoint
    SGGeod nearDover = fp->pointAlongRoute(3, -6.0);
    setPositionAndStabilise(nearDover);
    
    // check course deviation / cross-track error - we should still be at zero
    auto doverVORPos = fp->legAtIndex(3)->waypoint()->position();
    const double crsToDover = SGGeodesy::courseDeg(globals->get_aircraft_position(), doverVORPos);
    const double legCrs = SGGeodesy::courseDeg(nearDover, doverVORPos);
    
    // verify hold entry course
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(nearDover);
    pilot->setCourseTrue(crsToDover); // initial course is correct
    pilot->setSpeedKts(250);
    pilot->flyGPSCourse(m_gps);
    
    // ensure GPS speed is plausible
    FGTestApi::runForTime(1.0);
    fp->setCurrentIndex(3);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"COA"}, std::string{m_gpsNode->getStringValue("wp/wp[0]/ID")});
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});

    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDover, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(legCrs, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);

    auto statusNode = m_gpsNode->getNode("rnav-controller-status");
    CPPUNIT_ASSERT_EQUAL(std::string{"leg-to-hold"}, std::string{statusNode->getStringValue()});

    bool ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-outbound") return true;
        return false;
    });
   
    CPPUNIT_ASSERT(ok);
    
    // first outbound leg
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    // check where we are!
    
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-outbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    FGTestApi::runForTime(45.0);
    
    // reqeust hold exit
    m_gpsNode->setStringValue("command", "exit-hold");
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);

    // and now we should exit the hold and sequence to TAWNY
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        if (fp->currentIndex() == 4) return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});

    // check we get back enroute okay
    FGTestApi::runForTime(45.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
}


void HoldControllerTests::testHoldEntryTeardrop()
{
  //  FGTestApi::setUp::logPositionToKML("hold_teardrop_entry");
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                             "NIK COA DVR TAWNY WOD");
    FGTestApi::writeFlightPlanToKML(fp);
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    
    setupStandardGPS();
    m_gpsNode->setStringValue("command", "leg");
    setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));
    
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    auto leg = fp->legAtIndex(3); // DVR vor
    leg->setHoldCount(3);
    CPPUNIT_ASSERT_EQUAL(std::string{"hold"}, leg->waypoint()->type());

    flightgear::Hold* holdWpt = static_cast<flightgear::Hold*>(leg->waypoint());
    holdWpt->setHoldRadial(120.0);
    
    // sequence to that waypoint
    SGGeod nearDover = fp->pointAlongRoute(3, -8.0);
    setPositionAndStabilise(nearDover);
    
    // check course deviation / cross-track error - we should still be at zero
    auto doverVORPos = fp->legAtIndex(3)->waypoint()->position();
    const double crsToDover = SGGeodesy::courseDeg(globals->get_aircraft_position(), doverVORPos);
    const double legCrs = SGGeodesy::courseDeg(nearDover, doverVORPos);
    
    // verify hold entry course
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(nearDover);
    pilot->setCourseTrue(crsToDover); // initial course is correct
    pilot->setSpeedKts(200);
    pilot->flyGPSCourse(m_gps);
    
    // ensure GPS speed is plausible
    FGTestApi::runForTime(5.0);
    
    // trigger entry to the hold
    fp->setCurrentIndex(3);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDover, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(legCrs, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);

    
    auto statusNode = m_gpsNode->getNode("rnav-controller-status");
    CPPUNIT_ASSERT_EQUAL(std::string{"leg-to-hold"}, std::string{statusNode->getStringValue()});
    
    bool ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "entry-teardrop") return true;
        return false;
    });
    CPPUNIT_ASSERT(ok);

    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    // check where we are
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-outbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    
    // outbound leg
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    FGTestApi::runForTime(80.0); // run inbound a bit
    m_gpsNode->setStringValue("command", "exit-hold");

    // check we exit ok
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        if (fp->currentIndex() == 4) return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    
    // check we get back enroute okay - will take a long time since we're pointing totally
    // the wrong way
    FGTestApi::runForTime(240.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
}


void HoldControllerTests::testHoldEntryParallel()
{
    //FGTestApi::setUp::logPositionToKML("hold_parallel_entry");
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                             "NIK COA DVR TAWNY WOD");
 //   FGTestApi::writeFlightPlanToKML(fp);
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    
    setupStandardGPS();
    m_gpsNode->setStringValue("command", "leg");
    setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));
    
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    auto leg = fp->legAtIndex(3); // DVR vor
    leg->setHoldCount(3);
    CPPUNIT_ASSERT_EQUAL(std::string{"hold"}, leg->waypoint()->type());
    
    flightgear::Hold* holdWpt = static_cast<flightgear::Hold*>(leg->waypoint());
    holdWpt->setHoldRadial(45.0);
    
    // sequence to that waypoint
    SGGeod nearDover = fp->pointAlongRoute(3, -5.0);
    setPositionAndStabilise(nearDover);
    
    // trigger entry to the hold
    fp->setCurrentIndex(3);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    // check course deviation / cross-track error - we should still be at zero
    auto doverVORPos = fp->legAtIndex(3)->waypoint()->position();
    const double crsToDover = SGGeodesy::courseDeg(globals->get_aircraft_position(), doverVORPos);
    const double legCrs = SGGeodesy::courseDeg(nearDover, doverVORPos);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDover, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(legCrs, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // verify hold entry course
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(nearDover);
    pilot->setCourseTrue(crsToDover); // initial course is correct
    pilot->setSpeedKts(250);
    pilot->flyGPSCourse(m_gps);
    
    auto statusNode = m_gpsNode->getNode("rnav-controller-status");
    CPPUNIT_ASSERT_EQUAL(std::string{"leg-to-hold"}, std::string{statusNode->getStringValue()});
    
    bool ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "entry-parallel") return true;
        return false;
    });
    CPPUNIT_ASSERT(ok);

    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-outbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    // check where we are
    
    // outbound leg
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    FGTestApi::runForTime(80.0); // run inbound a bit
    m_gpsNode->setStringValue("command", "exit-hold");
    
    // check we exit ok
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        if (fp->currentIndex() == 4) return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    
    // check we get back enroute okay - will take a long time since we're pointing totally
    // the wrong way
    FGTestApi::runForTime(240.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
}

/////////////////////////////////////////////////////////////////////////////

void HoldControllerTests::testLeftHoldEntryDirect()
{
 //   FGTestApi::setUp::logPositionToKML("hold_left_direct_entry");
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                             "NIK COA DVR TAWNY WOD");
  //  FGTestApi::writeFlightPlanToKML(fp);
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    
    setupStandardGPS();
    m_gpsNode->setStringValue("command", "leg");
    setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));
    
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    auto leg = fp->legAtIndex(3); // DVR vor
    leg->setHoldCount(3);
    CPPUNIT_ASSERT_EQUAL(std::string{"hold"}, leg->waypoint()->type());
    flightgear::Hold* holdWpt = static_cast<flightgear::Hold*>(leg->waypoint());
    
    // puts us near the 110-degree limit for a direct entry on a left-handed-hold
    holdWpt->setHoldRadial(15.0);
    holdWpt->setLeftHanded();
    
    // sequence to that waypoint
    SGGeod nearDover = fp->pointAlongRoute(3, -4.0);
    setPositionAndStabilise(nearDover);
    fp->setCurrentIndex(3);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"COA"}, std::string{m_gpsNode->getStringValue("wp/wp[0]/ID")});
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    // check course deviation / cross-track error - we should still be at zero
    auto doverVORPos = fp->legAtIndex(3)->waypoint()->position();
    const double crsToDover = SGGeodesy::courseDeg(globals->get_aircraft_position(), doverVORPos);
    const double legCrs = SGGeodesy::courseDeg(nearDover, doverVORPos);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDover, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(legCrs, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // verify hold entry course
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(nearDover);
    pilot->setCourseTrue(crsToDover); // initial course is correct
    pilot->setSpeedKts(300);
    pilot->flyGPSCourse(m_gps);
    
    auto statusNode = m_gpsNode->getNode("rnav-controller-status");
    CPPUNIT_ASSERT_EQUAL(std::string{"leg-to-hold"}, std::string{statusNode->getStringValue()});
    
    bool ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-outbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    
    // first outbound leg
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    // check where we are!
    
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-outbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    FGTestApi::runForTime(45.0);
    
    // reqeust hold exit
    m_gpsNode->setStringValue("command", "exit-hold");
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    
    // and now we should exit the hold and sequence to TAWNY
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        if (fp->currentIndex() == 4) return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    
    // check we get back enroute okay
    FGTestApi::runForTime(120.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
}

void HoldControllerTests::testLeftHoldEntryTeardrop()
{
    //FGTestApi::setUp::logPositionToKML("hold_left_teardrop_entry");
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                             "NIK COA DVR TAWNY WOD");
   // FGTestApi::writeFlightPlanToKML(fp);
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    
    setupStandardGPS();
    m_gpsNode->setStringValue("command", "leg");
    setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));
    
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    auto leg = fp->legAtIndex(3); // DVR vor
    leg->setHoldCount(3);
    CPPUNIT_ASSERT_EQUAL(std::string{"hold"}, leg->waypoint()->type());
    
    flightgear::Hold* holdWpt = static_cast<flightgear::Hold*>(leg->waypoint());
    holdWpt->setHoldRadial(25.0);
    holdWpt->setLeftHanded();

    // sequence to that waypoint
    SGGeod nearDover = fp->pointAlongRoute(3, -5.0);
    setPositionAndStabilise(nearDover);
    
    // trigger entry to the hold
    fp->setCurrentIndex(3);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    // check course deviation / cross-track error - we should still be at zero
    auto doverVORPos = fp->legAtIndex(3)->waypoint()->position();
    const double crsToDover = SGGeodesy::courseDeg(globals->get_aircraft_position(), doverVORPos);
    const double legCrs = SGGeodesy::courseDeg(nearDover, doverVORPos);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDover, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(legCrs, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // verify hold entry course
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(nearDover);
    pilot->setCourseTrue(crsToDover); // initial course is correct
    pilot->setSpeedKts(250);
    pilot->flyGPSCourse(m_gps);
    
    auto statusNode = m_gpsNode->getNode("rnav-controller-status");
    CPPUNIT_ASSERT_EQUAL(std::string{"leg-to-hold"}, std::string{statusNode->getStringValue()});
    
    bool ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "entry-teardrop") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    // check where we are
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-outbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    
    // outbound leg
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    FGTestApi::runForTime(80.0); // run inbound a bit
    m_gpsNode->setStringValue("command", "exit-hold");
    
    // check we exit ok
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        if (fp->currentIndex() == 4) return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    
    // check we get back enroute okay - will take a long time since we're pointing totally
    // the wrong way
    FGTestApi::runForTime(240.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
}

void HoldControllerTests::testLeftHoldEntryParallel()
{
  //  FGTestApi::setUp::logPositionToKML("hold_left_parallel_entry");
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                             "NIK COA DVR TAWNY WOD");
  //  FGTestApi::writeFlightPlanToKML(fp);
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    
    setupStandardGPS();
    m_gpsNode->setStringValue("command", "leg");
    setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));
    
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    auto leg = fp->legAtIndex(3); // DVR vor
    leg->setHoldCount(3);
    CPPUNIT_ASSERT_EQUAL(std::string{"hold"}, leg->waypoint()->type());
    
    flightgear::Hold* holdWpt = static_cast<flightgear::Hold*>(leg->waypoint());
    holdWpt->setHoldRadial(160);
    holdWpt->setLeftHanded();
    
    // sequence to that waypoint
    SGGeod nearDover = fp->pointAlongRoute(3, -5.0);
    setPositionAndStabilise(nearDover);
    
    // trigger entry to the hold
    fp->setCurrentIndex(3);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    // check course deviation / cross-track error - we should still be at zero
    auto doverVORPos = fp->legAtIndex(3)->waypoint()->position();
    const double crsToDover = SGGeodesy::courseDeg(globals->get_aircraft_position(), doverVORPos);
    const double legCrs = SGGeodesy::courseDeg(nearDover, doverVORPos);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDover, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(legCrs, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // verify hold entry course
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(nearDover);
    pilot->setCourseTrue(crsToDover); // initial course is correct
    pilot->setSpeedKts(190);
    pilot->flyGPSCourse(m_gps);
    
    auto statusNode = m_gpsNode->getNode("rnav-controller-status");
    CPPUNIT_ASSERT_EQUAL(std::string{"leg-to-hold"}, std::string{statusNode->getStringValue()});
    
    bool ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "entry-parallel") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-outbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    // check where we are
    
    // outbound leg
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    FGTestApi::runForTime(80.0); // run inbound a bit
    m_gpsNode->setStringValue("command", "exit-hold");
    
    // check we exit ok
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        if (fp->currentIndex() == 4) return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    
    // check we get back enroute okay - will take a long time since we're pointing totally
    // the wrong way
    FGTestApi::runForTime(240.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
}

/////////////////////////////////////////////////////////////////////////////

void HoldControllerTests::testHoldNotEntered()
{
   // FGTestApi::setUp::logPositionToKML("hold_no_entry");
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                             "NIK COA DVR TAWNY WOD");
  //  FGTestApi::writeFlightPlanToKML(fp);
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    
    setupStandardGPS();
    m_gpsNode->setStringValue("command", "leg");
    setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));
    
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    auto leg = fp->legAtIndex(3); // DVR vor
    leg->setHoldCount(3);
    CPPUNIT_ASSERT_EQUAL(std::string{"hold"}, leg->waypoint()->type());
    flightgear::Hold* holdWpt = static_cast<flightgear::Hold*>(leg->waypoint());
    
    holdWpt->setHoldRadial(15.0);
    
    // sequence to that waypoint
    SGGeod nearDover = fp->pointAlongRoute(3, -4.0);
    setPositionAndStabilise(nearDover);
    fp->setCurrentIndex(3);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"COA"}, std::string{m_gpsNode->getStringValue("wp/wp[0]/ID")});
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    // check course deviation / cross-track error - we should still be at zero
    auto doverVORPos = fp->legAtIndex(3)->waypoint()->position();
    const double crsToDover = SGGeodesy::courseDeg(globals->get_aircraft_position(), doverVORPos);
    const double legCrs = SGGeodesy::courseDeg(nearDover, doverVORPos);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDover, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(legCrs, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // verify hold entry course
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(nearDover);
    pilot->setCourseTrue(crsToDover); // initial course is correct
    pilot->setSpeedKts(300);
    pilot->flyGPSCourse(m_gps);
    
    auto statusNode = m_gpsNode->getNode("rnav-controller-status");
    CPPUNIT_ASSERT_EQUAL(std::string{"leg-to-hold"}, std::string{statusNode->getStringValue()});
    
    // now we reset the hold count, so we don't enter
    m_gpsNode->setStringValue("command", "exit-hold");
    
    // this should not do anything for now
    CPPUNIT_ASSERT_EQUAL(std::string{"leg-to-hold"}, std::string{statusNode->getStringValue()});

    bool ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-exiting") return true;
        return false;
    });
    
    // and now we should exit the hold and sequence to TAWNY
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        if (fp->currentIndex() == 4) return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    
    // check we get back enroute okay
    FGTestApi::runForTime(120.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
}

/////////////////////////////////////////////////////////////////////////////
// test entering the hold from wildly off the leg's track

void HoldControllerTests::testHoldEntryOffCourse()
{
  //  FGTestApi::setUp::logPositionToKML("hold_entry_off-course");
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                             "NIK COA DVR TAWNY WOD");
  //  FGTestApi::writeFlightPlanToKML(fp);
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    
    setupStandardGPS();
    m_gpsNode->setStringValue("command", "leg");
    setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));
    
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    auto leg = fp->legAtIndex(3); // DVR vor
    leg->setHoldCount(3);
    CPPUNIT_ASSERT_EQUAL(std::string{"hold"}, leg->waypoint()->type());
    
    flightgear::Hold* holdWpt = static_cast<flightgear::Hold*>(leg->waypoint());
    // this should be a direct entry, but now we're going to go off course!
    holdWpt->setHoldRadial(330);
    
    // sequence to that waypoint
    auto doverVORPos = fp->legAtIndex(3)->waypoint()->position();
    SGGeod nearDover = fp->pointAlongRoute(3, -6.0);
    SGGeod offCoursePos = SGGeodesy::direct(doverVORPos, 20, 3.0 * SG_NM_TO_METER);
    setPositionAndStabilise(nearDover);
    
    // check course deviation / cross-track error - we should still be at zero
    const double legCrs = SGGeodesy::courseDeg(nearDover, doverVORPos);
    
// sequence to the COA -> DVR leg
    fp->setCurrentIndex(3);
    
// assume we got wildly off course somehow
    setPositionAndStabilise(offCoursePos);
    
    // verify hold entry course
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->setCourseTrue(legCrs); // initial course is correct
    pilot->setSpeedKts(250);
    pilot->flyGPSCourse(m_gps);
    pilot->resetAtPosition(offCoursePos);
    const double crsToDover = SGGeodesy::courseDeg(globals->get_aircraft_position(), doverVORPos);
    pilot->setCourseTrue(crsToDover); // initial course is correct

    CPPUNIT_ASSERT_EQUAL(std::string{"COA"}, std::string{m_gpsNode->getStringValue("wp/wp[0]/ID")});
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDover, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(legCrs, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
   // CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDover - legCrs, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    auto statusNode = m_gpsNode->getNode("rnav-controller-status");
    CPPUNIT_ASSERT_EQUAL(std::string{"leg-to-hold"}, std::string{statusNode->getStringValue()});
    
    // we should select parallel entry due to being off course
    bool ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "entry-parallel") return true;
        return false;
    });
    CPPUNIT_ASSERT(ok);

    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-outbound") return true;
        return false;
    });
    
    
    CPPUNIT_ASSERT(ok);
    // first outbound leg
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-outbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    FGTestApi::runForTime(45.0);
    
    // reqeust hold exit
    m_gpsNode->setStringValue("command", "exit-hold");
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [statusNode]() {
        std::string s = statusNode->getStringValue();
        if (s == "hold-inbound") return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    
    // and now we should exit the hold and sequence to TAWNY
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        if (fp->currentIndex() == 4) return true;
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{m_gpsNode->getStringValue("mode")});
    
    // check we get back enroute okay
    FGTestApi::runForTime(45.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
}
