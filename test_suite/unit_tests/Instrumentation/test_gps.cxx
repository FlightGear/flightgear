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

#include "test_suite/FGTestApi/globals.hxx"
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
#if 0
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
#endif
    
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
    
    setupRouteManager();
}

// Clean up after each test.
void GPSTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void GPSTests::setPositionAndStabilise(GPS* gps, const SGGeod& g)
{
    FGTestApi::setPosition(g);
    for (int i=0; i<60; ++i) {
        gps->update(0.015);
    }
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
    
    globals->add_subsystem("gps", gps, SGSubsystemMgr::POST_FDM);
    return gps;
}

void GPSTests::setupRouteManager()
{
    auto rm = globals->add_new_subsystem<FGRouteMgr>();
    rm->bind();
    rm->init();
    rm->postinit();
}

/////////////////////////////////////////////////////////////////////////////

void GPSTests::testBasic()
{
    auto gps = setupStandardGPS();
    
    FGPositioned::TypeFilter f{FGPositioned::VOR};
    auto bodrumVOR = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("BDR", SGGeod::fromDeg(27.6, 37), &f));
    SGGeod p1 = SGGeodesy::direct(bodrumVOR->geod(), 45.0, 5.0 * SG_NM_TO_METER);
    
    setPositionAndStabilise(gps, p1);
    
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
    auto gps = setupStandardGPS();

    FGPositioned::TypeFilter f{FGPositioned::VOR};
    auto bodrumVOR = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("BDR", SGGeod::fromDeg(27.6, 37), &f));
    SGGeod p1 = SGGeodesy::direct(bodrumVOR->geod(), 45.0, 5.0 * SG_NM_TO_METER);
    
    setPositionAndStabilise(gps, p1);
    
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
    
    setPositionAndStabilise(gps, p1);

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
    setPositionAndStabilise(gps, p2);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"obs"}, std::string{gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.0, gpsNode->getDoubleValue("wp/wp[1]/distance-nm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(220.0, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(225.0, gpsNode->getDoubleValue("desired-course-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-5.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.1);
    
    // off axis, perpendicular
    SGGeod p3 = SGGeodesy::direct(p1, 135, 0.5 * SG_NM_TO_METER);
    setPositionAndStabilise(gps, p3);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(225.0, gpsNode->getDoubleValue("desired-course-deg"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
}

void GPSTests::testDirectTo()
{
    auto gps = setupStandardGPS();
    
   
    
    FGPositioned::TypeFilter f{FGPositioned::VOR};
    auto bodrumVOR = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("BDR", SGGeod::fromDeg(27.6, 37), &f));
    SGGeod p1 = SGGeodesy::direct(bodrumVOR->geod(), 30.0, 16.0 * SG_NM_TO_METER);
    
    setPositionAndStabilise(gps, p1);
    
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
    setPositionAndStabilise(gps, p2);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.1);
    
    SGGeod p3 = SGGeodesy::direct(p1, 210, 6.0 * SG_NM_TO_METER);
    SGGeod xtk = SGGeodesy::direct(p3, 120, 0.8 * SG_NM_TO_METER);
    setPositionAndStabilise(gps, xtk);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    
    SGGeod xtk2 = SGGeodesy::direct(p3, 120, -1.8 * SG_NM_TO_METER);
    setPositionAndStabilise(gps, xtk2);
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
    
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                   "NIK COA DVR TAWNY WOD");
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    
    CPPUNIT_ASSERT(rm->activate());
    
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    
    setPositionAndStabilise(gps, fp->departureRunway()->pointOnCenterline(0.0));

    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
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
#if 0
    CPPUNIT_ASSERT_EQUAL(1, testDelegate->sequenceCount);
#endif
    CPPUNIT_ASSERT_EQUAL(1, fp->currentIndex());

    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{gpsNode->getStringValue("wp/wp[0]/ID")});
    CPPUNIT_ASSERT_EQUAL(std::string{"NIK"}, std::string{gpsNode->getStringValue("wp/wp[1]/ID")});
    
    // reposition along the leg, closer to NIK
    // and fly to COA
    
    SGGeod nikPos = fp->currentLeg()->waypoint()->position();
    SGGeod p2 = SGGeodesy::direct(nikPos, 90, 5 * SG_NM_TO_METER); // due east of NIK
    setPositionAndStabilise(gps, p2);
    
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

#if 0
    CPPUNIT_ASSERT_EQUAL(2, testDelegate->sequenceCount);
#endif
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
    
    setPositionAndStabilise(gps, coaPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(course, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    
    SGGeod off1 = SGGeodesy::direct(doverPos, revCourse - 5.0, 16 * SG_NM_TO_METER);
    setPositionAndStabilise(gps, off1);
    
    double courseToDover = SGGeodesy::courseDeg(off1, doverPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseToDover, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-5.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);

    CPPUNIT_ASSERT_EQUAL(true, gpsNode->getBoolValue("wp/wp[1]/to-flag"));
    CPPUNIT_ASSERT_EQUAL(false, gpsNode->getBoolValue("wp/wp[1]/from-flag"));
    
    SGGeod off2 = SGGeodesy::direct(doverPos, revCourse + 8.0, 40 * SG_NM_TO_METER);
    setPositionAndStabilise(gps, off2);
    
    courseToDover = SGGeodesy::courseDeg(off2, doverPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseToDover, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    CPPUNIT_ASSERT_EQUAL(true, gpsNode->getBoolValue("wp/wp[1]/to-flag"));
    CPPUNIT_ASSERT_EQUAL(false, gpsNode->getBoolValue("wp/wp[1]/from-flag"));
    
    // check cross-track error calculation
    SGGeod alongTrack = SGGeodesy::direct(coaPos, course, 20 * SG_NM_TO_METER);
    SGGeod offTrack = SGGeodesy::direct(alongTrack, course + 90, SG_NM_TO_METER * 0.7);
    
    setPositionAndStabilise(gps, offTrack);
    courseToDover = SGGeodesy::courseDeg(offTrack, doverPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseToDover, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(-0.7, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    
    SGGeod offTrack2 = SGGeodesy::direct(alongTrack, courseToDover - 90, SG_NM_TO_METER * 3.4);
    setPositionAndStabilise(gps, offTrack2);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.4, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);

    // check cross track very close to COA
    SGGeod alongTrack11 = SGGeodesy::direct(coaPos, course, 0.3);
    SGGeod offTrack25 = SGGeodesy::direct(alongTrack11, course + 90, SG_NM_TO_METER * -3.2);
    setPositionAndStabilise(gps, offTrack25);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.2, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(course, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.1);

    // check cross track very close to DVR
    double distanceCOA_DVR = SGGeodesy::distanceM(coaPos, doverPos);
    SGGeod alongTrack2 = SGGeodesy::direct(coaPos, course, distanceCOA_DVR - 0.3);
    SGGeod offTrack3 = SGGeodesy::direct(alongTrack2, course + 90, SG_NM_TO_METER * 1.6);
    setPositionAndStabilise(gps, offTrack3);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.6, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(revCourse + 180.0, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.1);

    // check cross track in the middle
    SGGeod alongTrack3 = SGGeodesy::direct(coaPos, course, distanceCOA_DVR * 0.52);
    SGGeod offTrack4 = SGGeodesy::direct(alongTrack3, course + 90, SG_NM_TO_METER * 15.6);
    setPositionAndStabilise(gps, offTrack4);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-15.6, gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(261.55, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.1);
}

void GPSTests::testDirectToLegOnFlightplan()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();
    
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "EBBR", "07L", "EGGD", "27",
                                 "NIK COA DVR TAWNY WOD");
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    
    CPPUNIT_ASSERT(rm->activate());
    
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    
    
    setPositionAndStabilise(gps, fp->departureRunway()->pointOnCenterline(0.0));
    
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setStringValue("command", "leg");
    
    CPPUNIT_ASSERT_EQUAL(std::string{"leg"}, std::string{gpsNode->getStringValue("mode")});
    CPPUNIT_ASSERT_EQUAL(std::string{"EBBR-07L"}, std::string{gpsNode->getStringValue("wp/wp[1]/ID")});
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(65.0, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(65.0, gpsNode->getDoubleValue("desired-course-deg"), 0.5);
    
    // initiate a direct to
    SGGeod p2 = fp->departureRunway()->pointOnCenterline(5.0* SG_NM_TO_METER);
    setPositionAndStabilise(gps, p2);
    
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

void GPSTests::testLongLeg()
{
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "KLAX", "25R", "KJFK", "22R","VNY TEB");
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    
    CPPUNIT_ASSERT(rm->activate());
    
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setStringValue("command", "leg");
    
    // custom CDI deflection output
    gpsNode->setDoubleValue("config/cdi-max-deflection-nm", 20.0);
    
    auto vanNuysVOR = fp->legAtIndex(1)->waypoint()->source();
    CPPUNIT_ASSERT_EQUAL(std::string{"VAN NUYS VOR-DME"}, vanNuysVOR->name());

    auto teterboroVOR = fp->legAtIndex(2)->waypoint()->source();
    CPPUNIT_ASSERT_EQUAL(std::string{"TETERBORO VOR-DME"}, teterboroVOR->name());

    
    fp->setCurrentIndex(2);
// initial course at VNY
    setPositionAndStabilise(gps, vanNuysVOR->geod());
    
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
    setPositionAndStabilise(gps, onTheWay);

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
    setPositionAndStabilise(gps, offTheWay);
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
    setPositionAndStabilise(gps, off2);
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
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithoutNasal(fp, "ENBR", "35", "BIKF", "29","VOO GAKTU");
    
    // takes the place of the Nasal delegates
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    
    CPPUNIT_ASSERT(rm->activate());
    
    fp->addDelegate(testDelegate);
    auto gps = setupStandardGPS();
  
    auto gpsNode = globals->get_props()->getNode("instrumentation/gps");
    gpsNode->setStringValue("command", "leg");
    // custom CDI deflection output
    gpsNode->setDoubleValue("config/cdi-max-deflection-nm", 25.0);
    
    auto volloVOR = fp->legAtIndex(1)->waypoint()->source();
    CPPUNIT_ASSERT_EQUAL(std::string{"VOLLO VOR-DME"}, volloVOR->name());
    
    auto gaktu = fp->legAtIndex(2)->waypoint()->source();
    CPPUNIT_ASSERT_EQUAL(std::string{"GAKTU"}, gaktu->name());
    
    fp->setCurrentIndex(2);
    // initial course at VNY
    setPositionAndStabilise(gps, volloVOR->geod());
    
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
    setPositionAndStabilise(gps, onTheWay);
    
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
    setPositionAndStabilise(gps, offTheWay);
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

void GPSTests::testTurnAnticipation()
{
}

