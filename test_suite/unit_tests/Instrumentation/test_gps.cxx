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

#include "test_gps.hxx"

#include <memory>
#include <cstring>

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestPilot.hxx"
#include "test_suite/FGTestApi/TestDataLogger.hxx"

#include <Navaids/FlightPlan.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/fixlist.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/navrecord.hxx>

#include <Instrumentation/gps.hxx>
#include <Instrumentation/navradio.hxx>

#include <Autopilot/route_mgr.hxx>

using namespace flightgear;
using namespace std::string_literals;

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
void GPSTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("gps");
    FGTestApi::setUp::initNavDataCache();

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();

    setupRouteManager();
}

// Clean up after each test.
void GPSTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

GPS* GPSTests::setupStandardGPS(SGPropertyNode_ptr config,
                                const std::string name, const int index)
{
    SGPropertyNode_ptr configNode(config.valid() ? config
                                                 : SGPropertyNode_ptr{new SGPropertyNode});
    configNode->setStringValue("name", name);
    configNode->setIntValue("number", index);
    
    GPS* gps(new GPS(configNode));
    
    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation", true)->getChild(name, index, true);
    node->setBoolValue("serviceable", true);
    globals->get_props()->setDoubleValue("systems/electrical/outputs/gps", 6.0);
    
    gps->bind();
    gps->init();
    
    globals->get_subsystem_mgr()->add("gps", gps);
    return gps;
}

void GPSTests::setupRouteManager()
{
    auto rm = globals->get_subsystem_mgr()->add<FGRouteMgr>();
    rm->bind();
    rm->init();
    rm->postinit();
}

/////////////////////////////////////////////////////////////////////////////

void GPSTests::testBasic()
{
    setupStandardGPS();
    
    FGPositioned::TypeFilter f{FGPositioned::VOR};
    auto bodrumVOR = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("BDR", SGGeod::fromDeg(27.6, 37), &f));
    SGGeod p1 = SGGeodesy::direct(bodrumVOR->geod(), 45.0, 5.0 * SG_NM_TO_METER);
    
    FGTestApi::setPositionAndStabilise(p1);
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(p1.getLongitudeDeg(), gpsNode->getDoubleValue("indicated-longitude-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(p1.getLatitudeDeg(), gpsNode->getDoubleValue("indicated-latitude-deg"), 0.01);
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->setSpeedKts(120);
    pilot->setCourseTrue(225.0);
    
    FGTestApi::runForTime(30.0);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(225, gpsNode->getDoubleValue("indicated-track-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(120, gpsNode->getDoubleValue("indicated-ground-speed-kt"), 1);
    
    // 120kts =
    double speedMSec = 120 * SG_KT_TO_MPS;
    double components = speedMSec * (1.0 / sqrt(2.0));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-components, gpsNode->getDoubleValue("ew-velocity-msec"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-components, gpsNode->getDoubleValue("ns-velocity-msec"), 0.1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(120 * (30.0 / 3600), gpsNode->getDoubleValue("odometer"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(120 * (30.0 / 3600), gpsNode->getDoubleValue("trip-odometer"), 0.1);
}

void GPSTests::testOBSMode()
{
    setupStandardGPS();

    FGPositioned::TypeFilter f{FGPositioned::VOR};
    auto bodrumVOR = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("BDR", SGGeod::fromDeg(27.6, 37), &f));
    SGGeod p1 = SGGeodesy::direct(bodrumVOR->geod(), 45.0, 5.0 * SG_NM_TO_METER);
    
    FGTestApi::setPositionAndStabilise(p1);

    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(p1.getLongitudeDeg(), gpsNode->getDoubleValue("indicated-longitude-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(p1.getLatitudeDeg(), gpsNode->getDoubleValue("indicated-latitude-deg"), 0.01);
    
    gpsNode->setDoubleValue("selected-course-deg", 225);
    
    // query BDR from the GPS database
    gpsNode->setStringValue("scratch/query", "BDR");
    gpsNode->setStringValue("scratch/type", "vor");
    gpsNode->setStringValue("command", "search");
    
    CPPUNIT_ASSERT_EQUAL(true, gpsNode->getBoolValue("scratch/valid"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(225.0, gpsNode->getDoubleValue("scratch/true-bearing-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, gpsNode->getDoubleValue("scratch/distance-nm"), 0.1);

    // select OBS mode one it
    gpsNode->setStringValue("command", "obs");
    
    FGTestApi::setPositionAndStabilise(p1);

    CPPUNIT_ASSERT_EQUAL(std::string{"obs"}, std::string{gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(225.0, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(bodrumVOR->get_lon(), gpsNode->getDoubleValue("wp/wp[1]/longitude-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(bodrumVOR->get_lat(), gpsNode->getDoubleValue("wp/wp[1]/latitude-deg"), 0.01);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(225.0, gpsNode->getDoubleValue("desired-course-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);

    // off axis, angular
    SGGeod p2 = SGGeodesy::direct(bodrumVOR->geod(), 40.0, 4.0 * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(p2);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"obs"}, std::string{gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(220.0, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(225.0, gpsNode->getDoubleValue("desired-course-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-5.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.1);
    
    // off axis, perpendicular
    SGGeod p3 = SGGeodesy::direct(p1, 135, 0.5 * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(p3);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(225.0, gpsNode->getDoubleValue("desired-course-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
}

void GPSTests::testDirectTo()
{
    setupStandardGPS();
    
    FGPositioned::TypeFilter f{FGPositioned::VOR};
    auto bodrumVOR = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("BDR", SGGeod::fromDeg(27.6, 37), &f));
    SGGeod p1 = SGGeodesy::direct(bodrumVOR->geod(), 30.0, 16.0 * SG_NM_TO_METER);
    
    FGTestApi::setPositionAndStabilise(p1);
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    
    // load into the scratch
    gpsNode->setStringValue("scratch/query", "BDR");
    gpsNode->setStringValue("scratch/type", "vor");
    gpsNode->setStringValue("command", "search");
    
    CPPUNIT_ASSERT_EQUAL(true, gpsNode->getBoolValue("scratch/valid"));
    
    gpsNode->setStringValue("command", "direct");
    
    CPPUNIT_ASSERT_EQUAL(std::string{"dto"}, std::string{gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(p1.getLongitudeDeg(), gpsNode->getDoubleValue("wp/wp[0]/longitude-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(p1.getLatitudeDeg(), gpsNode->getDoubleValue("wp/wp[0]/latitude-deg"), 0.01);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"BDR"}, std::string{gpsNode->getStringValue("wp/wp[1]/ID")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(16.0, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(210, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(bodrumVOR->get_lon(), gpsNode->getDoubleValue("wp/wp[1]/longitude-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(bodrumVOR->get_lat(), gpsNode->getDoubleValue("wp/wp[1]/latitude-deg"), 0.01);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    
    // move off the direct-to line, check everything works
    
    SGGeod p2 = SGGeodesy::direct(bodrumVOR->geod(), 35.0, 12.0 * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(p2);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.1);
    
    SGGeod p3 = SGGeodesy::direct(p1, 210, 6.0 * SG_NM_TO_METER);
    SGGeod xtk = SGGeodesy::direct(p3, 120, 0.8 * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(xtk);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    
    SGGeod xtk2 = SGGeodesy::direct(p3, 120, -1.8 * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(xtk2);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.8, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
}

void GPSTests::testNavRadioSlave()
{
    SGPropertyNode_ptr radioConfigNode(new SGPropertyNode);
    radioConfigNode->setStringValue("name", "navtest");
    radioConfigNode->setIntValue("number", 2);
    std::unique_ptr<FGNavRadio> r(new FGNavRadio(radioConfigNode));
}

void GPSTests::testLegMode()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();
    
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                   "NIK COA DVR TAWNY WOD");
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    
    CPPUNIT_ASSERT(rm->activate());
    
    fp->addDelegate(testDelegate);
    setupStandardGPS();
    
    FGTestApi::setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));

    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setStringValue("command", "leg");

    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{gpsNode->getStringValue("wp/wp[1]/ID")});

    CPPUNIT_ASSERT_DOUBLES_EQUAL(65.0, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(65.0, gpsNode->getDoubleValue("desired-course-deg"), 0.5);
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->setSpeedKts(200);
    pilot->setCourseTrue(65.0);
    pilot->setTargetAltitudeFtMSL(6000);
    FGTestApi::runForTime(60.0);

    // check we sequenced to NIK
    CPPUNIT_ASSERT_EQUAL(1, testDelegate->sequenceCount);
    CPPUNIT_ASSERT_EQUAL(1, fp->currentIndex());

    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{gpsNode->getStringValue("wp/wp[0]/ID")});
    CPPUNIT_ASSERT_EQUAL(std::string{"NIK"}, std::string{gpsNode->getStringValue("wp/wp[1]/ID")});
    
    // reposition along the leg, closer to NIK
    // and fly to COA
    
    SGGeod nikPos = fp->currentLeg()->waypoint()->position();
    SGGeod p2 = SGGeodesy::direct(nikPos, 90, 5 * SG_NM_TO_METER); // due east of NIK
    FGTestApi::setPositionAndStabilise(p2);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(270, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);

    auto depRwy = fp->departureRunway();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(SGGeodesy::distanceNm(nikPos, depRwy->end()), gpsNode->getDoubleValue("wp/leg-distance-nm"), 0.1);
    const double legCourse = SGGeodesy::courseDeg(depRwy->end(), nikPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(legCourse, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);

    const double crsToNIK = SGGeodesy::courseDeg(p2, nikPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToNIK - legCourse, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    pilot->setSpeedKts(200);
    pilot->setCourseTrue(270);
    FGTestApi::runForTime(120.0);

    CPPUNIT_ASSERT_EQUAL(2, testDelegate->sequenceCount);
    CPPUNIT_ASSERT_EQUAL(2, fp->currentIndex());
    CPPUNIT_ASSERT_EQUAL(std::string{"NIK"}, std::string{gpsNode->getStringValue("wp/wp[0]/ID")});
    CPPUNIT_ASSERT_EQUAL(std::string{"COA"}, std::string{gpsNode->getStringValue("wp/wp[1]/ID")});
    
    SGGeod coaPos = fp->currentLeg()->waypoint()->position();
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(SGGeodesy::distanceNm(nikPos, coaPos), gpsNode->getDoubleValue("wp/leg-distance-nm"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(SGGeodesy::courseDeg(nikPos, coaPos), gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    
    // check manual sequencing
    fp->setCurrentIndex(3);
    CPPUNIT_ASSERT_EQUAL(std::string{"COA"}, std::string{gpsNode->getStringValue("wp/wp[0]/ID")});
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, std::string{gpsNode->getStringValue("wp/wp[1]/ID")});
    
    // check course deviation / cross-track error
    SGGeod doverPos = fp->currentLeg()->waypoint()->position();
    double course = SGGeodesy::courseDeg(coaPos, doverPos);
    double revCourse = SGGeodesy::courseDeg(doverPos, coaPos);
    
    FGTestApi::setPositionAndStabilise(coaPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(course, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    
    SGGeod off1 = SGGeodesy::direct(doverPos, revCourse - 5.0, 16 * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(off1);
    
    double courseToDover = SGGeodesy::courseDeg(off1, doverPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseToDover, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-5.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);

    CPPUNIT_ASSERT_EQUAL(true, gpsNode->getBoolValue("wp/wp[1]/to-flag"));
    CPPUNIT_ASSERT_EQUAL(false, gpsNode->getBoolValue("wp/wp[1]/from-flag"));
    
    SGGeod off2 = SGGeodesy::direct(doverPos, revCourse + 8.0, 40 * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(off2);
    
    courseToDover = SGGeodesy::courseDeg(off2, doverPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseToDover, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    CPPUNIT_ASSERT_EQUAL(true, gpsNode->getBoolValue("wp/wp[1]/to-flag"));
    CPPUNIT_ASSERT_EQUAL(false, gpsNode->getBoolValue("wp/wp[1]/from-flag"));
    
    // check cross-track error calculation
    SGGeod alongTrack = SGGeodesy::direct(coaPos, course, 20 * SG_NM_TO_METER);
    SGGeod offTrack = SGGeodesy::direct(alongTrack, course + 90, SG_NM_TO_METER * 0.7);
    
    FGTestApi::setPositionAndStabilise(offTrack);
    courseToDover = SGGeodesy::courseDeg(offTrack, doverPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseToDover, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(-0.7, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    
    SGGeod offTrack2 = SGGeodesy::direct(alongTrack, courseToDover - 90, SG_NM_TO_METER * 3.4);
    FGTestApi::setPositionAndStabilise(offTrack2);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.4, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);

    // check cross track very close to COA
    SGGeod alongTrack11 = SGGeodesy::direct(coaPos, course, 0.3);
    SGGeod offTrack25 = SGGeodesy::direct(alongTrack11, course + 90, SG_NM_TO_METER * -3.2);
    FGTestApi::setPositionAndStabilise(offTrack25);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.2, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(course, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.1);

    // check cross track very close to DVR
    double distanceCOA_DVR = SGGeodesy::distanceM(coaPos, doverPos);
    SGGeod alongTrack2 = SGGeodesy::direct(coaPos, course, distanceCOA_DVR - 0.3);
    SGGeod offTrack3 = SGGeodesy::direct(alongTrack2, course + 90, SG_NM_TO_METER * 1.6);
    FGTestApi::setPositionAndStabilise(offTrack3);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.6, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(revCourse + 180.0, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.1);

    // check cross track in the middle
    SGGeod alongTrack3 = SGGeodesy::direct(coaPos, course, distanceCOA_DVR * 0.52);
    SGGeod offTrack4 = SGGeodesy::direct(alongTrack3, course + 90, SG_NM_TO_METER * 15.6);
    FGTestApi::setPositionAndStabilise(offTrack4);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-15.6, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(261.55, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.1);
}

void GPSTests::testBuiltinRevertToOBSAtEnd()
{
    //FGTestApi::setUp::logPositionToKML("gps_route_end_revert_to_obs");

    auto rm = globals->get_subsystem<FGRouteMgr>();
    
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                             "NIK COA DVR TAWNY BDN");
    
   // FGTestApi::writeFlightPlanToKML(fp);

    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    
    CPPUNIT_ASSERT(rm->activate());
    
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    
    FGTestApi::setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));

    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setStringValue("command", "leg");
    
    // move to point 18.0 nm after BDN
    auto posNearApproach = fp->pointAlongRoute(5, 18.0);
    FGTestApi::setPositionAndStabilise(posNearApproach);
    fp->setCurrentIndex(6);
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(posNearApproach);
    pilot->setSpeedKts(250);
    pilot->flyGPSCourse(gps);
    
    bool ok = FGTestApi::runForTimeWithCheck(600.0, [gpsNode] () {
        const std::string mode = gpsNode->getStringValue("mode");
        if (mode == "obs") {
            return true;
        }
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"obs"}, std::string{gpsNode->getStringValue("mode")});
    // should have deactivated
    CPPUNIT_ASSERT_EQUAL(false, fp->isActive());
}

void GPSTests::testDirectToLegOnFlightplan()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();
    
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                 "NIK COA DVR TAWNY WOD");
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    
    CPPUNIT_ASSERT(rm->activate());
    
    fp->addDelegate(testDelegate);
    setupStandardGPS();
    
    FGTestApi::setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setStringValue("command", "leg");

    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{gpsNode->getStringValue("wp/wp[1]/ID")});
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(65.0, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(65.0, gpsNode->getDoubleValue("desired-course-deg"), 0.5);
    
    // initiate a direct to
    SGGeod p2 = fp->departureRunway()->pointOnCenterline(5.0* SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(p2);
    
    auto doverVOR = fp->legAtIndex(3)->waypoint()->source();
    
    double distanceToDover = SGGeodesy::distanceNm(p2, doverVOR->geod());
    double bearingToDover = SGGeodesy::courseDeg(p2, doverVOR->geod());
    
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, doverVOR->ident());
    gpsNode->setStringValue("scratch/ident", "DVR");
    gpsNode->setDoubleValue("scratch/longitude-deg", doverVOR->geod().getLongitudeDeg());
    gpsNode->setDoubleValue("scratch/latitude-deg", doverVOR->geod().getLatitudeDeg());

    gpsNode->setStringValue("command", "direct");

    CPPUNIT_ASSERT_EQUAL(std::string{"dto"}, std::string{gpsNode->getStringValue("mode")});

    CPPUNIT_ASSERT_DOUBLES_EQUAL(p2.getLongitudeDeg(), gpsNode->getDoubleValue("wp/wp[0]/longitude-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(p2.getLatitudeDeg(), gpsNode->getDoubleValue("wp/wp[0]/latitude-deg"), 0.01);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, std::string{gpsNode->getStringValue("wp/wp[1]/ID")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distanceToDover, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(bearingToDover, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(doverVOR->geod().getLongitudeDeg(), gpsNode->getDoubleValue("wp/wp[1]/longitude-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(doverVOR->geod().getLatitudeDeg(), gpsNode->getDoubleValue("wp/wp[1]/latitude-deg"), 0.01);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
}

void GPSTests::testDirectToLegOnFlightplanAndResumeBuiltin()
{
    // this tests uses the legacy built-in sequencing behaviour of the GPS
    // which is now handled by the Nasal delegate in some cases.
    auto rm = globals->get_subsystem<FGRouteMgr>();
   // FGTestApi::setUp::logPositionToKML("gps_dto_resume_leg");

    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                             "NIK COA DVR TAWNY WOD");
    
    //FGTestApi::writeFlightPlanToKML(fp);
    
    CPPUNIT_ASSERT(rm->activate());
        auto gps = setupStandardGPS();
    
    FGTestApi::setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setStringValue("command", "leg");
    
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{gpsNode->getStringValue("wp/wp[1]/ID")});
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(65.0, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(65.0, gpsNode->getDoubleValue("desired-course-deg"), 0.5);
    
    // initiate a direct to
    SGGeod p2 = fp->departureRunway()->pointOnCenterline(5.0* SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(p2);
    
    auto doverVOR = fp->legAtIndex(3)->waypoint()->source();
    
    double distanceToDover = SGGeodesy::distanceNm(p2, doverVOR->geod());
    double bearingToDover = SGGeodesy::courseDeg(p2, doverVOR->geod());
    
    CPPUNIT_ASSERT_EQUAL(std::string{"DVR"}, doverVOR->ident());
    gpsNode->setStringValue("scratch/ident", "DVR");
    gpsNode->setDoubleValue("scratch/longitude-deg", doverVOR->geod().getLongitudeDeg());
    gpsNode->setDoubleValue("scratch/latitude-deg", doverVOR->geod().getLatitudeDeg());
    gpsNode->setStringValue("command", "direct");
    CPPUNIT_ASSERT_EQUAL(std::string{"dto"}, std::string{gpsNode->getStringValue("mode")});
    
    // check that upon reaching DOVER, we sequence to TAWNY and resume leg mode
    // note this behaviour is from the old C++ sequencing
    
    SGGeod posNearDover = SGGeodesy::direct(p2, bearingToDover, (distanceToDover - 8.0) * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(posNearDover);
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(posNearDover);
    pilot->setSpeedKts(250);
    pilot->flyGPSCourse(gps);
    
    bool ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 4) {
            return true;
        }
        return false;
    });
    
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{gpsNode->getStringValue("mode")});
}

void GPSTests::testLongLeg()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "KLAX", "25R", "KJFK", "22R","VNY TEB");
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    
    CPPUNIT_ASSERT(rm->activate());
    
    fp->addDelegate(testDelegate);
    setupStandardGPS();
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setStringValue("command", "leg");

    // custom CDI deflection output
    gpsNode->setDoubleValue("config/cdi-max-deflection-nm", 20.0);
    
    auto vanNuysVOR = fp->legAtIndex(1)->waypoint()->source();
    CPPUNIT_ASSERT_EQUAL(std::string{"VAN NUYS VOR-DME"}, vanNuysVOR->name());

    auto teterboroVOR = fp->legAtIndex(2)->waypoint()->source();
    CPPUNIT_ASSERT_EQUAL(std::string{"TETERBORO VOR-DME"}, teterboroVOR->name());

    
    fp->setCurrentIndex(2);
// initial course at VNY
    FGTestApi::setPositionAndStabilise(vanNuysVOR->geod());
    
    const double initialCourse = SGGeodesy::courseDeg(vanNuysVOR->geod(), teterboroVOR->geod());
    const double distanceM = SGGeodesy::distanceM(vanNuysVOR->geod(), teterboroVOR->geod());
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(initialCourse, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(initialCourse, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distanceM * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distanceM * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/leg-distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("cdi-deflection"), 0.01);

// part of the way, check enroute leg course
    const SGGeod onTheWay = SGGeodesy::direct(vanNuysVOR->geod(), initialCourse, distanceM * 0.7);
    FGTestApi::setPositionAndStabilise(onTheWay);

    const double courseNow = SGGeodesy::courseDeg(onTheWay, teterboroVOR->geod());
    const double distNow = SGGeodesy::distanceM(onTheWay, teterboroVOR->geod());
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseNow, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseNow, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distNow * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distanceM * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/leg-distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("cdi-deflection"), 0.01);

// check a seriously abeam point, we got far of course
    // to the right of the course, i.e desired track is to our left, so -ve
    const SGGeod offTheWay = SGGeodesy::direct(onTheWay, courseNow + 90, SG_NM_TO_METER * 13.5);
    FGTestApi::setPositionAndStabilise(offTheWay);
    const double courseFromOff1 = SGGeodesy::courseDeg(offTheWay, teterboroVOR->geod());
    const double dist2 = SGGeodesy::distanceM(offTheWay, teterboroVOR->geod());

    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseFromOff1, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseNow, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-13.5, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseFromOff1 - courseNow, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(dist2 * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    
    const double expectedDefl1 = -13.5 / 20.0;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(expectedDefl1 * 10.0, gpsNode->getDoubleValue("cdi-deflection"), 0.01);
    
// check off the other side, close to the destination
    const SGGeod onTheWay2 = SGGeodesy::direct(vanNuysVOR->geod(), initialCourse, distanceM * 0.92);
    const double courseOn2 = SGGeodesy::courseDeg(onTheWay2, teterboroVOR->geod());
    const SGGeod off2 = SGGeodesy::direct(onTheWay2, courseOn2 - 90, SG_NM_TO_METER * 31.2);
    FGTestApi::setPositionAndStabilise(off2);
    const double courseFromOff2 = SGGeodesy::courseDeg(off2, teterboroVOR->geod());
    const double dist3 = SGGeodesy::distanceM(off2, teterboroVOR->geod());
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseFromOff2, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseOn2, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(31.2, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseFromOff2 - courseOn2, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(dist3 * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);

    // should peg the CDI, +ve
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, gpsNode->getDoubleValue("cdi-deflection"), 0.01);
}

void GPSTests::testLongLegWestbound()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "ENBR", "35", "BIKF", "29","VOO GAKTU");
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    
    CPPUNIT_ASSERT(rm->activate());
    
    fp->addDelegate(testDelegate);
    setupStandardGPS();
  
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setStringValue("command", "leg");
    // custom CDI deflection output
    gpsNode->setDoubleValue("config/cdi-max-deflection-nm", 25.0);
    
    auto volloVOR = fp->legAtIndex(1)->waypoint()->source();
    CPPUNIT_ASSERT_EQUAL(std::string{"VOLLO VOR-DME"}, volloVOR->name());
    
    auto gaktu = fp->legAtIndex(2)->waypoint()->source();
    CPPUNIT_ASSERT_EQUAL(std::string{"GAKTU"}, gaktu->name());
    
    fp->setCurrentIndex(2);
    // initial course at VNY
    FGTestApi::setPositionAndStabilise(volloVOR->geod());
    
    const double initialCourse = SGGeodesy::courseDeg(volloVOR->geod(), gaktu->geod());
    const double distanceM = SGGeodesy::distanceM(volloVOR->geod(), gaktu->geod());
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(initialCourse, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(initialCourse, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distanceM * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distanceM * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/leg-distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("cdi-deflection"), 0.01);

    // part of the way, check enroute leg course
    const SGGeod onTheWay = SGGeodesy::direct(volloVOR->geod(), initialCourse, distanceM * 0.4);
    FGTestApi::setPositionAndStabilise(onTheWay);
    
    const double courseNow = SGGeodesy::courseDeg(onTheWay, gaktu->geod());
    const double distNow = SGGeodesy::distanceM(onTheWay, gaktu->geod());
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseNow, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseNow, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distNow * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distanceM * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/leg-distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("cdi-deflection"), 0.01);

    // check a seriously abeam point, we got far of course
    // to the right of the course, i.e desired track is to our left, so -ve
    const SGGeod offTheWay = SGGeodesy::direct(onTheWay, courseNow + 90, SG_NM_TO_METER * 18.6);
    FGTestApi::setPositionAndStabilise(offTheWay);
    const double courseFromOff1 = SGGeodesy::courseDeg(offTheWay, gaktu->geod());
    const double dist2 = SGGeodesy::distanceM(offTheWay, gaktu->geod());
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseFromOff1, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseNow, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-18.6, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseFromOff1 - courseNow, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(dist2 * SG_METER_TO_NM, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    
    const double expectedDefl1 = -18.6 / 25.0;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(expectedDefl1 * 10.0, gpsNode->getDoubleValue("cdi-deflection"), 0.01);
}

void GPSTests::testOverflightSequencing()
{
    // check that we sequence as we pass over the top
    
  //  FGTestApi::setUp::logPositionToKML("gps_sequence");
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    
    // let's use New Zealand for some southern hemisphere confusion :)
    
    // this route has some deliberately sharp turns to work the turn code
    FGTestApi::setUp::populateFPWithoutNasal(fp, "NZCH", "02", "NZAA", "05L",
                                             "ALADA NS WB WN MAMOD KAPTI OH");
    
  //  FGTestApi::writeFlightPlanToKML(fp);
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setStringValue("command", "leg");
    
    // very tight tolerance on the overflight. We need a little bit or the
    // test-piot gets confused right on top
    gpsNode->setDoubleValue("config/over-flight-distance-nm", 0.01);
    
    // enroute to NELSON VOR
    fp->setCurrentIndex(2);
    SGGeod pos = fp->pointAlongRoute(2, -5.0);
    FGTestApi::setPositionAndStabilise(pos);
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(pos);
    pilot->setSpeedKts(300); // decent speed to make things tougher
    pilot->flyGPSCourse(gps);
    
    bool ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 3) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    // check we're on top of NELSON
    auto nelsonVOR = fp->legAtIndex(2)->waypoint()->source();
    CPPUNIT_ASSERT_EQUAL(std::string{"NELSON VOR-DME"}, nelsonVOR->name());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, SGGeodesy::distanceNm(globals->get_aircraft_position(),  nelsonVOR->geod()), 0.1);
    
    FGTestApi::runForTime(120.0);
    // check we're on course to Woodburne
    
    auto woodbourneVOR = fp->legAtIndex(3)->waypoint()->source();
    const double crsToWB = SGGeodesy::courseDeg(globals->get_aircraft_position(),  woodbourneVOR->geod());
    const double crsNSWB = SGGeodesy::courseDeg(nelsonVOR->geod(), woodbourneVOR->geod());
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToWB, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsNSWB, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // point a bit before MAMOD
    SGGeod pos2 = fp->pointAlongRoute(5, -5.0);
    FGTestApi::setPositionAndStabilise(pos2);
    fp->setCurrentIndex(5);
    auto mamod = fp->legAtIndex(5)->waypoint()->source();

    ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 6) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    // check we're on top of MAMOD
    CPPUNIT_ASSERT_EQUAL(std::string{"MAMOD"}, mamod->name());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, SGGeodesy::distanceNm(globals->get_aircraft_position(),  mamod->geod()), 0.1);
    
    FGTestApi::runForTime(180.0);
    // check we're on course to KAPTI
    
    auto kapti = fp->legAtIndex(6)->waypoint()->source();
    const double crsKapti = SGGeodesy::courseDeg(globals->get_aircraft_position(),  kapti->geod());
    const double crsMamodKapti = SGGeodesy::courseDeg(mamod->geod(), kapti->geod());
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsKapti, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsMamodKapti, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
}

void GPSTests::testOffcourseSequencing()
{
    // ensure that if we pass through the overflight arm cone, but not
    // over the waypoint direclty, we still sequence at the mid-point
    
 //   FGTestApi::setUp::logPositionToKML("gps_sequence_off");
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    
    
    FGTestApi::setUp::populateFPWithoutNasal(fp, "NZCH", "02", "NZAA", "05L",
                                             "ALADA NS WB WN MAMOD KAPTI OH");

   // FGTestApi::writeFlightPlanToKML(fp);
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setStringValue("command", "leg");
    gpsNode->setDoubleValue("config/over-flight-distance-nm", 0.01);
    
    // enroute to NELSON VOR
    fp->setCurrentIndex(2);
    SGGeod pos = fp->pointAlongRoute(2, -5.0);
    
    pos = SGGeodesy::direct(pos, 180, 1.5 * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(pos);
    
    
    auto nelsonVOR = fp->legAtIndex(2)->waypoint()->source();
    CPPUNIT_ASSERT_EQUAL(std::string{"NELSON VOR-DME"}, nelsonVOR->name());
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(pos);
    pilot->setSpeedKts(300); // decent speed to make things tougher
    
    // since we're south of the route, but will fly parallel to it, we won't
    // pass over NELSON directly.
    
    const double legCourse =  gpsNode->getDoubleValue("wp/leg-true-course-deg");
    pilot->turnToCourse(legCourse);
    
    bool ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 3) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    // check we're close NELSON
    const double distToNELSON = SGGeodesy::distanceNm(globals->get_aircraft_position(),  nelsonVOR->geod());
    CPPUNIT_ASSERT(distToNELSON < 1.0);
    
    const double bearingToNELSON = SGGeodesy::courseDeg(globals->get_aircraft_position(),  nelsonVOR->geod());
    // find the inbound course from ALADA at NELSON
    double finalLegTrack = SGGeodesy::courseDeg(nelsonVOR->geod(), fp->legAtIndex(1)->waypoint()->position()) + 180.0;
    SG_NORMALIZE_RANGE(finalLegTrack, 0.0, 360.0);
    
    double dev = bearingToNELSON - finalLegTrack;
    SG_NORMALIZE_RANGE(dev, -180.0, 180.0);
    double absDeviation = fabs(dev);
    
    // 90 is the overflight arm angle, i.e we sequence as soon as the waypoint is abeam us
    CPPUNIT_ASSERT_DOUBLES_EQUAL(90.0, absDeviation, 1.0);
    
    pilot->flyGPSCourse(gps);
    FGTestApi::runForTime(120.0);
    // check we're on course to Woodburne
    
    auto woodbourneVOR = fp->legAtIndex(3)->waypoint()->source();
    const double crsToWB = SGGeodesy::courseDeg(globals->get_aircraft_position(),  woodbourneVOR->geod());
    const double crsNSWB = SGGeodesy::courseDeg(nelsonVOR->geod(), woodbourneVOR->geod());
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToWB, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsNSWB, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
   
    // let's go wide at MAMOD
    SGGeod pos2 = fp->pointAlongRoute(5, -5.0);
    pos2 = SGGeodesy::direct(pos2, 180, 0.8 * SG_NM_TO_METER);

    fp->setCurrentIndex(5);
    FGTestApi::setPositionAndStabilise(pos2);
    // start facing the right way
    pilot->setCourseTrue(gpsNode->getDoubleValue("wp/leg-true-course-deg"));
    pilot->flyHeading(gpsNode->getDoubleValue("wp/leg-true-course-deg"));
    auto mamod = fp->legAtIndex(5)->waypoint()->source();
    
    ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 6) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    // check we're on top of MAMOD
    CPPUNIT_ASSERT_EQUAL(std::string{"MAMOD"}, mamod->name());
    const double distToMAMOD = SGGeodesy::distanceNm(globals->get_aircraft_position(),  mamod->geod());
    CPPUNIT_ASSERT(distToMAMOD < 1.0);
    
    // check the angle we sequenced at
    const double bearingToMAMOD = SGGeodesy::courseDeg(globals->get_aircraft_position(),  mamod->geod());
    // find the inbound course from WELLINGTON at MAMOD
    finalLegTrack = SGGeodesy::courseDeg(mamod->geod(), fp->legAtIndex(4)->waypoint()->position()) + 180.0;
    
    SG_NORMALIZE_RANGE(finalLegTrack, 0.0, 360.0);
    dev = bearingToMAMOD - finalLegTrack;
    SG_NORMALIZE_RANGE(dev, -180.0, 180.0);
    absDeviation = fabs(dev);
    
    // 90 is the overflight arm angle, i.e we sequence as soon as the waypoint is abeam us
    CPPUNIT_ASSERT_DOUBLES_EQUAL(90.0, absDeviation, 1.0);
    
 
    // check we don't sequence, if we're too far off course
    
    SGGeod pos3 = fp->pointAlongRoute(6, -5.0);
    pos3 = SGGeodesy::direct(pos3, 90, 1.8 * SG_NM_TO_METER);
    
    fp->setCurrentIndex(6);
    FGTestApi::setPositionAndStabilise(pos3);
    // start facing the right way
    pilot->setCourseTrue(gpsNode->getDoubleValue("wp/leg-true-course-deg"));
    pilot->flyHeading(gpsNode->getDoubleValue("wp/leg-true-course-deg"));
    
    ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() != 6) {
            CPPUNIT_ASSERT(false);
            return true;
        }
        return false;
    });
    
    
}

void GPSTests::testOffsetFlight()
{
    // verify the XTK value during waypoint transitions, especially that we
    // don't get any weird discontinuities
    
   // FGTestApi::setUp::logPositionToKML("gps_offset_flight");
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    
    
    FGTestApi::setUp::populateFPWithoutNasal(fp, "UHMA", "01", "PAMR", "25",
                                             "SOMEG BE KIVAK BET");
    
   // FGTestApi::writeFlightPlanToKML(fp);
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setStringValue("command", "leg");
    gpsNode->setDoubleValue("config/over-flight-distance-nm", 0.01);
    
    // enroute to BERINGOVSKY NDB
    fp->setCurrentIndex(2);
 
    SGGeod pos = fp->pointAlongRoute(2, -10.0);
    pos = SGGeodesy::direct(pos, 180, 2.0 * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(pos);
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(pos);
    pilot->setSpeedKts(300); // decent speed to make things tougher
    
    // start facing the right way
    pilot->setCourseTrue(gpsNode->getDoubleValue("wp/leg-true-course-deg"));
    pilot->flyGPSCourseOffset(gps, 0.8);
    
    FGTestApi::runForTime(90.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);

    bool ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 3) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);

    FGTestApi::runForTime(120.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);

    // fly until we cross the awkward line :)
    ok = FGTestApi::runForTimeWithCheck(6000.0, [] () {
        const double lon = globals->get_aircraft_position().getLongitudeDeg();
        if ((lon > -178.0) && (lon < -170.0)) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    
    // and try an inside corrner
    fp->setCurrentIndex(3);
    pos = fp->pointAlongRoute(3, -18.0);
    pos = SGGeodesy::direct(pos, 180, 2.0 * SG_NM_TO_METER);
    FGTestApi::setPositionAndStabilise(pos);
    pilot->resetAtPosition(pos);
    // start facing the right way
    pilot->setCourseTrue(gpsNode->getDoubleValue("wp/leg-true-course-deg"));
    
    FGTestApi::runForTime(120.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    
    ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 4) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    FGTestApi::runForTime(120.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);

}

void GPSTests::testLegIntercept()
{
  //  FGTestApi::setUp::logPositionToKML("gps_intercept");
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    
        FGTestApi::setUp::populateFPWithoutNasal(fp, "NZCH", "02", "NZAA", "05L",
                                             "ALADA NS WB WN MAMOD KAPTI OH");
    
//    auto dl =  FGTestApi::DataLogger::instance();
//   dl->initTest("gps_leg_intercept");
//   dl->recordProperty("gps-error-nm", globals->get_props()->getNode("instrumentation/gps/wp/wp[1]/course-error-nm", true));
//   dl->recordProperty("gps-dev-deg", globals->get_props()->getNode("instrumentation/gps/wp/wp[1]/course-deviation-deg", true));
//   dl->recordProperty("hdg", globals->get_props()-> getNode("orientation/heading-deg", true));
//   dl->recordProperty("leg-track", globals->get_props()->getNode("instrumentation/gps/wp/leg-true-course-deg", true));
       
    FGTestApi::writeFlightPlanToKML(fp);
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setBoolValue("config/enable-fly-by", true);
    gpsNode->setStringValue("command", "leg");
    
    
    // enroute to NELSON VOR
    fp->setCurrentIndex(2);
    
    
    // point that should intercept
    SGGeod pos = fp->pointAlongRoute(1, 5.0);
    pos = SGGeodesy::direct(pos, 0, 12.0 * SG_NM_TO_METER);

    FGTestApi::setPositionAndStabilise(pos);
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(pos);
    pilot->setCourseTrue(180.0); // start facing an annoying direction
    pilot->setSpeedKts(300); // decent speed to make things tougher
    pilot->flyGPSCourse(gps);
    
    auto courseErrorNode = gpsNode->getNode("wp/wp[1]/course-error-nm");
    bool ok = FGTestApi::runForTimeWithCheck(600.0, [courseErrorNode]() {
        return fabs(courseErrorNode->getDoubleValue()) < 0.1;
    });
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(2, fp->currentIndex()); // shouldn;t have sequenced
    
// repeat this test from right of the leg, and much further out
    pos = fp->pointAlongRoute(1, -5.0);
    pos = SGGeodesy::direct(pos, 240, 10.0 * SG_NM_TO_METER);
    
    // cycle the GPS mode to re-do the intercept logic
    FGTestApi::setPositionAndStabilise(pos);
    gpsNode->setStringValue("command", "obs");
    gpsNode->setStringValue("command", "leg");

    pilot->resetAtPosition(pos);
    pilot->setCourseTrue(360.0);
    pilot->setSpeedKts(300);
    pilot->flyGPSCourse(gps);
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [courseErrorNode]() {
        return fabs(courseErrorNode->getDoubleValue()) < 0.1;
    });
    CPPUNIT_ASSERT(ok);
    
// repeat much closer to NELSON
    pos = fp->pointAlongRoute(2, -10.0);
    pos = SGGeodesy::direct(pos, 180, 5.0 * SG_NM_TO_METER);
    
    // cycle the GPS mode to re-do the intercept logic
    FGTestApi::setPositionAndStabilise(pos);
    gpsNode->setStringValue("command", "obs");
    gpsNode->setStringValue("command", "leg");
    
    pilot->resetAtPosition(pos);
    pilot->setCourseTrue(300.0); // awkward course
    pilot->setSpeedKts(300);
    pilot->flyGPSCourse(gps);
    
    ok = FGTestApi::runForTimeWithCheck(600.0, [courseErrorNode]() {
        return fabs(courseErrorNode->getDoubleValue()) < 0.1;
    });
    CPPUNIT_ASSERT(ok);
    
// now try a start location well outside the 45-degree envelope
    pos = fp->pointAlongRoute(2, -5.0);
    pos = SGGeodesy::direct(pos, 160, 18.0 * SG_NM_TO_METER);
    
    // cycle the GPS mode to re-do the intercept logic
    FGTestApi::setPositionAndStabilise(pos);
    gpsNode->setStringValue("command", "obs");
    gpsNode->setStringValue("command", "leg");
    
    pilot->resetAtPosition(pos);
    pilot->setCourseTrue(45.0); // awkward course
    pilot->setSpeedKts(300);
    pilot->flyGPSCourse(gps);
    
    auto nelsonVORPos = fp->legAtIndex(2)->waypoint()->position();
    const double crsToNS = SGGeodesy::courseDeg(globals->get_aircraft_position(), nelsonVORPos);
    const double crsFromStart = SGGeodesy::courseDeg(pos, nelsonVORPos);
    
    // we should be established on a direct to, from our start pos, let's check
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToNS, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsFromStart, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // run until we sequence, which should happen as normal
    ok = FGTestApi::runForTimeWithCheck(6000.0, [fp] () {
        if (fp->currentIndex() == 3) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    // check we manage the punishing turn back onto track
    FGTestApi::runForTime(180.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    
// similar, but onthe other side, so closer aligned to the exit course at nelson.
// this should do a fly-by
    
   
    pos = fp->pointAlongRoute(2, -4.0);
    pos = SGGeodesy::direct(pos, 1, 10.0 * SG_NM_TO_METER);
    fp->setCurrentIndex(2);
    
    // cycle the GPS mode to re-do the intercept logic
    FGTestApi::setPositionAndStabilise(pos);
    gpsNode->setStringValue("command", "obs");
    gpsNode->setStringValue("command", "leg");
    
    pilot->resetAtPosition(pos);
    pilot->setCourseTrue(320.0); // awkward course
    pilot->setSpeedKts(300);
    pilot->flyGPSCourse(gps);
    
    const double crsToNS2 = SGGeodesy::courseDeg(globals->get_aircraft_position(), nelsonVORPos);
    const double crsFromStart2 = SGGeodesy::courseDeg(pos, nelsonVORPos);
    
    // we should be established on a direct to, from our start pos, let's check
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToNS2, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsFromStart2, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // run until we sequence, which should happen as normal
    ok = FGTestApi::runForTimeWithCheck(6000.0, [fp] () {
        if (fp->currentIndex() == 3) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    // check we are on course the whole way through
    FGTestApi::runForTime(10);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
}

void GPSTests::testTurnAnticipation()
{
    //FGTestApi::setUp::logPositionToKML("gps_flyby_sequence");
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
       
   // this route has some deliberately sharp turns to work the turn code
   FGTestApi::setUp::populateFPWithoutNasal(fp, "LEMG", "30", "EIDW", "28",
                                            "BLN VTB TLD DELOG IDRIK ARE KOFAL STU VATRY");
    
//    auto dl =  FGTestApi::DataLogger::instance();
//    dl->initTest("gps_flyby_sequence");
//    dl->recordProperty("gps-error-nm", globals->get_props()->getNode("instrumentation/gps/wp/wp[1]/course-error-nm", true));
//    dl->recordProperty("gps-dev-deg", globals->get_props()->getNode("instrumentation/gps/wp/wp[1]/course-deviation-deg", true));
//    dl->recordProperty("hdg", globals->get_props()-> getNode("orientation/heading-deg", true));
//    dl->recordProperty("leg-track", globals->get_props()->getNode("instrumentation/gps/wp/leg-true-course-deg", true));

                       
    FGTestApi::writeFlightPlanToKML(fp);
       
       // takes the place of the Nasal delegates
       auto testDelegate = new TestFPDelegate;
       testDelegate->thePlan = fp;
       CPPUNIT_ASSERT(rm->activate());
       fp->addDelegate(testDelegate);
       auto gps = setupStandardGPS();
       
       auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
       gpsNode->setBoolValue("config/delegate-sequencing", true);
        gpsNode->setBoolValue("config/enable-fly-by", true);
       gpsNode->setStringValue("command", "leg");
       
       
       // enroute to VILLATOBAS VOR
       fp->setCurrentIndex(2);
       SGGeod pos = fp->pointAlongRoute(2, -5.0);
       FGTestApi::setPositionAndStabilise(pos);
       
       auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
       pilot->resetAtPosition(pos);
       pilot->setSpeedKts(300); // decent speed to make things tougher
       pilot->flyGPSCourse(gps);
       
       bool ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
           if (fp->currentIndex() == 3) {
               return true;
           }
           return false;
       });
       CPPUNIT_ASSERT(ok);
       
       // check we sequenced at the half way point
       auto vtbVOR = fp->legAtIndex(2)->waypoint()->source();
       CPPUNIT_ASSERT_EQUAL(std::string{"VILLATOBAS VOR-DME"}, vtbVOR->name());
       
    const double courseToVTB = SGGeodesy::courseDeg(globals->get_aircraft_position(), vtbVOR->geod());
    // course should be half-way between the inbound and outbound leg courses, and then offset
    // let's see
    double ht = (fp->legAtIndex(3)->courseDeg() - fp->legAtIndex(2)->courseDeg());
    SG_NORMALIZE_RANGE(ht, -180.0, 180.0);
    ht *= 0.5;
    ht += fp->legAtIndex(2)->courseDeg();
    ht += 90;
    SG_NORMALIZE_RANGE(ht, 0.0, 360.0);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ht, courseToVTB, 5.0);
    
    FGTestApi::runForTime(30.0);
       // check we're on course to TOLEDO
       
       auto toledoVOR = fp->legAtIndex(3)->waypoint()->source();
      // const double crsToWB = SGGeodesy::courseDeg(globals->get_aircraft_position(),  woodbourneVOR->geod());
      // const double crsNSWB = SGGeodesy::courseDeg(nelsonVOR->geod(), woodbourneVOR->geod());
       
   //   CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToWB, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
     //  CPPUNIT_ASSERT_DOUBLES_EQUAL(crsNSWB, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
       CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
       CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
// check a right-hand turn
    pos = fp->pointAlongRoute(3, -5.0);
    FGTestApi::setPositionAndStabilise(pos);
     pilot->resetAtPosition(pos);
    
     ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
         if (fp->currentIndex() == 4) {
             return true;
         }
         return false;
     });
     CPPUNIT_ASSERT(ok);
    
    
    const double courseToTLD = SGGeodesy::courseDeg(globals->get_aircraft_position(), toledoVOR->geod());
    // course should be half-way between the inbound and outbound leg courses, and then offset
    // let's see
    ht = (fp->legAtIndex(4)->courseDeg() - fp->legAtIndex(3)->courseDeg());
    SG_NORMALIZE_RANGE(ht, -180.0, 180.0);
    ht *= 0.5;
    ht += fp->legAtIndex(3)->courseDeg();
    ht -= 90; // right hand turn
    SG_NORMALIZE_RANGE(ht, 0.0, 360.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseToTLD, ht, 5.0);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    FGTestApi::runForTime(30.0);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);

}

void GPSTests::testExceedFlyByMaxAngleTurn()
{
    //  FGTestApi::setUp::logPositionToKML("gps_fly_by_exceed_max_angle");
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);

    FGTestApi::setUp::populateFPWithoutNasal(fp, "LFBD", "23", "EHAM", "18L", "SOMOS ROYAN TIRAV BOBRI ADABI");
    FGTestApi::writeFlightPlanToKML(fp);

    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();

    fp->setCurrentIndex(4); // BOBRI

    // somehwat before BOBRI
    SGGeod initPos = fp->pointAlongRouteNorm(4, -0.1);
    FGTestApi::setPositionAndStabilise(initPos);
    FGTestApi::writePointToKML("init point", initPos);

    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setDoubleValue("config/max-fly-by-turn-angle-deg", 75.0);
    gpsNode->setStringValue("command", "leg");

    FGPositioned::TypeFilter fixFilter(FGPositioned::FIX);
    auto tiravFix = FGPositioned::findFirstWithIdent("TIRAV", &fixFilter);
    auto bobriFix = FGPositioned::findFirstWithIdent("BOBRI", &fixFilter);
    auto adabiFix = FGPositioned::findFirstWithIdent("ADABI", &fixFilter);

    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(initPos);
    pilot->setCourseTrue(fp->legAtIndex(4)->courseDeg());
    pilot->setSpeedKts(280);
    pilot->flyGPSCourse(gps);

    bool ok = FGTestApi::runForTimeWithCheck(1200.0, [&fp]() {
        return fp->currentIndex() == 5;
    });
    CPPUNIT_ASSERT(ok);
    // ensure we did a fly-over
    CPPUNIT_ASSERT(!gps->previousLegData().value().didFlyBy);
    FGTestApi::writePointToKML("seq point", globals->get_aircraft_position());

    // ensure leg course is the one we want
    const auto crs = SGGeodesy::courseDeg(bobriFix->geod(), adabiFix->geod());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crs, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 1.0);

    // ensure we fly the comepnsation turns, so that end up back on the next leg's course
    // exactly
    FGTestApi::runForTime(60.0);


    CPPUNIT_ASSERT_DOUBLES_EQUAL(crs, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 1.0);
}

void GPSTests::testRadialIntercept()
{
   // FGTestApi::setUp::logPositionToKML("gps_radial_intercept");

    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
       
    FGTestApi::setUp::populateFPWithoutNasal(fp, "LFKC", "36", "LIRF", "25", "BUNAX BEBEV AJO");
    
    // set BUNAX as overflight
    fp->legAtIndex(1)->waypoint()->setFlag(WPT_OVERFLIGHT);
    
    // insert KC502 manually
    fp->insertWayptAtIndex(new BasicWaypt(SGGeod::fromDeg(8.78333, 42.566), "KC502", fp), 1);
    
    
    SGGeod pos = SGGeod::fromDeg(8.445556,42.216944);
    auto intc = new RadialIntercept(fp, "INTC", pos, 230, 5);
    fp->insertWayptAtIndex(intc, 3);
    
    FGTestApi::writeFlightPlanToKML(fp);
    
    // position slightly before BUNAX
    SGGeod initPos = fp->pointAlongRoute(2, -3.0);
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    
    FGTestApi::setPositionAndStabilise(initPos);
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setBoolValue("config/enable-fly-by", false);
    gpsNode->setStringValue("command", "leg");
    
    fp->setCurrentIndex(2);

    CPPUNIT_ASSERT_EQUAL("BUNAX"s, gpsNode->getStringValue("wp/wp[1]/ID"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(312, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 1.0);

  
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(initPos);
    pilot->setCourseTrue(fp->legAtIndex(2)->courseDeg());
    pilot->setSpeedKts(300); // decent speed to make things tougher
    pilot->flyGPSCourse(gps);
    
    bool ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        return fp->currentIndex() == 4;
    });
    CPPUNIT_ASSERT(ok);
    
    // flying to BEBEV now
    CPPUNIT_ASSERT_DOUBLES_EQUAL(185, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 1.0);
    FGTestApi::runForTime(30.0);
    
    // repeat but with fly-by enabled
    gpsNode->setBoolValue("config/enable-fly-by", true);
    gpsNode->setStringValue("command", "obs");
  
    FGTestApi::setPositionAndStabilise(initPos);
    pilot->resetAtPosition(initPos);
    fp->setCurrentIndex(2);
    gpsNode->setStringValue("command", "leg");

    CPPUNIT_ASSERT_EQUAL("BUNAX"s, gpsNode->getStringValue("wp/wp[1]/ID"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(312, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 1.0);


    pilot->setCourseTrue(fp->legAtIndex(2)->courseDeg());
    pilot->flyGPSCourse(gps);
  
    ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        return fp->currentIndex() == 4;
    });
    CPPUNIT_ASSERT(ok);
  
    // flying to BEBEV now
    CPPUNIT_ASSERT_DOUBLES_EQUAL(185, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 1.0);
    FGTestApi::runForTime(30.0);
}

void GPSTests::testSWIFT8()
{
    FGTestApi::setUp::logPositionToKML("gps_swift8");

    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
       
    FGTestApi::setUp::populateFPWithoutNasal(fp, "YBCS", "15", "YBTL", "25", "SWIFT");
    
    auto hdgToAlt_400 = new HeadingToAltitude(fp, "(400)", 150);
    hdgToAlt_400->setAltitude(400, RESTRICT_AT);
    fp->insertWayptAtIndex(hdgToAlt_400, 1);
	
    SGGeod pos = SGGeod::fromDeg(145.743889,-16.850028);
    auto intc = new RadialIntercept(fp, "(INTC)", pos, 30, 80);
    fp->insertWayptAtIndex(intc, 2);
	
    auto hdgToAlt_4000 = new HeadingToAltitude(fp, "(4000)", 80);
    hdgToAlt_4000->setAltitude(4000, RESTRICT_AT);
    fp->insertWayptAtIndex(hdgToAlt_4000, 3);
	
    FGTestApi::writeFlightPlanToKML(fp);
	
    // position slightly before INTC
    SGGeod initPos = fp->pointAlongRoute(1, -1);
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    
    FGTestApi::setPositionAndStabilise(initPos);
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setBoolValue("config/enable-fly-by", false);
    gpsNode->setStringValue("command", "leg");

    fp->setCurrentIndex(1);
	
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(initPos);
    pilot->setCourseTrue(fp->legAtIndex(1)->courseDeg());
    pilot->setSpeedKts(200);
    pilot->setTargetAltitudeFtMSL(6000);
    pilot->flyGPSCourse(gps);
    
    bool ok = FGTestApi::runForTimeWithCheck(600.0, [fp]() {
        return fp->currentIndex() == 4;
    });
    CPPUNIT_ASSERT(ok);
	
    FGTestApi::runForTime(600.0);
	
    // Flying to SWIFT
    CPPUNIT_ASSERT_DOUBLES_EQUAL(149, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 1.0);
    FGTestApi::runForTime(30.0);
}

void GPSTests::testDMEIntercept()
{
   // FGTestApi::setUp::logPositionToKML("gps_dme_intercept");
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    
    // manually consturct something close to the publichsed approach transition for EGPH ILS 06
    
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EGLL", "27R", "EGPH", "06", "SHAPP TLA");
    
    //   DMEIntercept(RouteBase* aOwner, const std::string& aIdent, const SGGeod& aPos,
   //   double aCourseDeg, double aDistanceNm);
    auto dmeArc = new DMEIntercept(fp, "TLA-11", SGGeod::fromDeg(-3.352833, 55.499145), 323.0, 11.0);
    fp->insertWayptAtIndex(dmeArc, 3);
    
    // now a normal WP
    fp->insertWayptAtIndex(new BasicWaypt(SGGeod::fromDeg(-3.636708, 55.689981), "D3230", fp), 4);
    
    auto dmeArc2 = new DMEIntercept(fp, "TLA-20", SGGeod::fromDeg(-3.352833, 55.499145), 323.0, 20.0);
    fp->insertWayptAtIndex(dmeArc2, 5);
    
    // and another normal WP
    fp->insertWayptAtIndex(new BasicWaypt(SGGeod::fromDeg(-3.751056, 55.766122), "D323U", fp), 6);
    
    // and another one
    fp->insertWayptAtIndex(new BasicWaypt(SGGeod::fromDeg(-3.690845, 55.841378), "CI06", fp), 7);
    
    FGTestApi::writeFlightPlanToKML(fp);

    // position slightly before TLA
    SGGeod initPos = fp->pointAlongRoute(2, -2.0);
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    
    FGTestApi::setPositionAndStabilise(initPos);
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setBoolValue("config/enable-fly-by", true);
    gpsNode->setStringValue("command", "leg");
    
    fp->setCurrentIndex(2);

    CPPUNIT_ASSERT_EQUAL("TLA"s, gpsNode->getStringValue("wp/wp[1]/ID"));
    // CPPUNIT_ASSERT_DOUBLES_EQUAL(312, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 1.0);
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(initPos);
    pilot->setCourseTrue(fp->legAtIndex(2)->courseDeg());
    pilot->setSpeedKts(220); 
    pilot->flyGPSCourse(gps);
    
    bool ok = FGTestApi::runForTimeWithCheck(1200.0, [fp]() {
        return fp->currentIndex() == 8;
    });
    CPPUNIT_ASSERT(ok);
   
}

void GPSTests::testFinalLegCourse()
{
    // FGTestApi::setUp::logPositionToKML("gps_final_course");
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);
    
    // we can't use the standard function as it puts a waypoint on 
    // the extended centerline
    FGAirportRef depApt = FGAirport::getByIdent("EGAA");
    fp->setDeparture(depApt->getRunwayByIdent("07"));


    FGAirportRef destApt = FGAirport::getByIdent("EGPF");
    fp->setDestination(destApt->getRunwayByIdent("23"));

    // since we don't have the Nasal route-manager delegate, insert the
    // runway waypoints manually
    auto depRwy = new RunwayWaypt(fp->departureRunway(), fp);
    depRwy->setFlag(WPT_DEPARTURE);
    fp->insertWayptAtIndex(depRwy, -1);
    
    fp->insertWayptAtIndex(fp->waypointFromString("LISBO"), -1);
    
    auto destRwy = fp->destinationRunway();
    fp->insertWayptAtIndex(new RunwayWaypt(destRwy, fp), -1);
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    setupStandardGPS();
    
    fp->setCurrentIndex(2);
    
    // position halfway between EGAA and EGPF
    SGGeod initPos = fp->pointAlongRoute(1, -30.5);
    FGTestApi::setPositionAndStabilise(initPos);
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setStringValue("command", "leg");


    // FGTestApi::writeFlightPlanToKML(fp);
    // check that the final leg course doesn't fall back to 233 deg

    // EXPECTED FAIL: at the moment, we don't do the right thing for this, yet
    // CPPUNIT_ASSERT_DOUBLES_EQUAL(37, gpsNode->getDoubleValue("desired-course-deg"), 2.0);
}

// Test to check the situation where you have two legs forming a straight line
// with the current waypoint being the third waypoint
void GPSTests::testCourseLegIntermediateWaypoint()
{
    // FGTestApi::setUp::logPositionToKML("gps_leg_course_intermediate");
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);

    FGTestApi::setUp::populateFPWithoutNasal(fp, "EGAA", "25", "EGPH", "06", "LISBO BLACA");

    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();

    SGGeod decelPos = fp->pointAlongRoute(2, -15.0);
    fp->insertWayptAtIndex(new BasicWaypt(decelPos, "DECEL", fp), 2);
    fp->setCurrentIndex(2); // DECEL

    SGGeod initPos = fp->pointAlongRouteNorm(1, 0.1);
    FGTestApi::setPositionAndStabilise(initPos);

    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setStringValue("command", "leg");


    // FGTestApi::writeFlightPlanToKML(fp);
    // check that the leg course is correct
    CPPUNIT_ASSERT_DOUBLES_EQUAL(56.5, gpsNode->getDoubleValue("desired-course-deg"), 2.0);

    auto legToDecel = fp->legAtIndex(2);
    auto legToBlaca = fp->legAtIndex(3);

    auto wpNode = gpsNode->getChild("wp", 0, true);

    FGPositioned::TypeFilter fixFilter(FGPositioned::FIX);
    auto lisbo = FGPositioned::findClosestWithIdent("LISBO", fp->departureAirport()->geod(), &fixFilter);
    auto blaca = FGPositioned::findClosestWithIdent("BLACA", fp->departureAirport()->geod(), &fixFilter);

    const double crsToDecel = SGGeodesy::courseDeg(lisbo->geod(), decelPos);
    const double crsToBlaca = SGGeodesy::courseDeg(decelPos, blaca->geod());

    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDecel, legToDecel->courseDeg(), 1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToBlaca, legToBlaca->courseDeg(), 1.0);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDecel, gpsNode->getDoubleValue("desired-course-deg"), 2.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDecel, wpNode->getDoubleValue("leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToDecel, wpNode->getDoubleValue("leg-mag-course-deg"), 0.5);

    fp->setCurrentIndex(3); // BLACA

    FGTestApi::runForTime(0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToBlaca, gpsNode->getDoubleValue("desired-course-deg"), 2.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToBlaca, wpNode->getDoubleValue("leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(crsToBlaca, wpNode->getDoubleValue("leg-mag-course-deg"), 0.5);

    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(initPos);
    pilot->setCourseTrue(fp->legAtIndex(2)->courseDeg());
    pilot->setSpeedKts(280);
    pilot->flyGPSCourse(gps);

    bool ok = FGTestApi::runForTimeWithCheck(1200.0, [&fp]() {
        return fp->currentIndex() == 4;
    });
    CPPUNIT_ASSERT(ok);
}

// Test to check the situation where you have two legs forming a straight line
// with the current waypoint being the third waypoint
void GPSTests::testFlyOverMaxInterceptAngle()
{
    //FGTestApi::setUp::logPositionToKML("gps_fly_over_max_intercept_angle");
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = FlightPlan::create();
    rm->setFlightPlan(fp);

    FGTestApi::setUp::populateFPWithoutNasal(fp, "EGAA", "25", "EGPH", "06", "LISBO BLACA");

    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    CPPUNIT_ASSERT(rm->activate());
    fp->addDelegate(testDelegate);

    FGTestApi::writeFlightPlanToKML(fp);
    
    SGGeod initPos = fp->pointAlongRouteNorm(1, -0.15);
    FGTestApi::writePointToKML("Start pos", initPos);

    auto gps = setupStandardGPS();
    FGTestApi::setPositionAndStabilise(initPos);
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setBoolValue("config/delegate-sequencing", true);
    gpsNode->setStringValue("command", "leg");

    auto gpsErrorNmNode = gpsNode->getNode("wp/wp[1]/course-error-nm");

    fp->setCurrentIndex(1); // BLACA

    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(initPos);
    pilot->setCourseTrue(fp->legAtIndex(1)->courseDeg());
    pilot->setSpeedKts(280);
    pilot->flyGPSCourse(gps);

    bool ok = FGTestApi::runForTimeWithCheck(1200.0, [&fp]() {
        return fp->currentIndex() == 2;
    });
    CPPUNIT_ASSERT(ok);
    
    FGPositioned::TypeFilter fixFilter(FGPositioned::FIX);
    auto lisbo = FGPositioned::findClosestWithIdent("LISBO", fp->departureAirport()->geod(), &fixFilter);
    auto blaca = FGPositioned::findClosestWithIdent("BLACA", fp->departureAirport()->geod(), &fixFilter);

    const double crsToBlaca = SGGeodesy::courseDeg(lisbo->geod(), blaca->geod());
    
    ok = FGTestApi::runForTimeWithCheck(1200.0, [pilot, crsToBlaca, gpsErrorNmNode]()
                                        {
        const double heading = pilot->trueCourseDeg();
        if (heading < (crsToBlaca - 45.0)) {
            CPPUNIT_FAIL("Turned too far on intercept");
        }
        
        // check if we're near the leg
        if (fabs(gpsErrorNmNode->getDoubleValue()) > 0.05) {
            return false;
        }
        
        return pilot->isOnHeading(crsToBlaca);
    });
    CPPUNIT_ASSERT(ok);
}
