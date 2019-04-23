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
    SGGeod p1 = SGGeodesy::direct(bodrumVOR->geod(), 45.0, 5.0 * SG_NM_TO_METER);
    
    setPositionAndStabilise(gps, p1);
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
    setupRouteManager();
    auto rm = globals->get_subsystem<FGRouteMgr>();
    
    auto fp = new FlightPlan;
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFP(fp, "EBBR", "07L", "EGGD", "27",
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
    CPPUNIT_ASSERT_EQUAL(1, testDelegate->sequenceCount);
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
    
    //CPPUNIT_ASSERT_DOUBLES_EQUAL(legCourse - 270, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);

    
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

    CPPUNIT_ASSERT_DOUBLES_EQUAL(course, gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    
    SGGeod off1 = SGGeodesy::direct(coaPos, course + 5.0, 8 * SG_NM_TO_METER);
    setPositionAndStabilise(gps, off1);
    
    double courseToDover = SGGeodesy::courseDeg(off1, doverPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseToDover, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);

    // right of the desired course, negative sign, ho hum
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-5.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    CPPUNIT_ASSERT_EQUAL(true, gpsNode->getBoolValue("wp/wp[1]/to-flag"));
    CPPUNIT_ASSERT_EQUAL(false, gpsNode->getBoolValue("wp/wp[1]/from-flag"));
    
    SGGeod off2 = SGGeodesy::direct(doverPos, course - 5.0, -18 * SG_NM_TO_METER);
    setPositionAndStabilise(gps, off2);
    
    courseToDover = SGGeodesy::courseDeg(off2, doverPos);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(courseToDover, gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    // disabled becuase GPS course deviation is using from the FROM wp, when
    // it should be using the TO point (DVR in this case)
#if 0
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
#endif
    CPPUNIT_ASSERT_EQUAL(true, gpsNode->getBoolValue("wp/wp[1]/to-flag"));
    CPPUNIT_ASSERT_EQUAL(false, gpsNode->getBoolValue("wp/wp[1]/from-flag"));
}

void GPSTests::testTurnAnticipation()
{
}
    

#if 0
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "navtest");
    configNode->setIntValue("number", 2);
    std::unique_ptr<FGNavRadio> r(new FGNavRadio(configNode));

    r->bind();
    r->init();

    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/navtest[2]");
    node->setBoolValue("serviceable", true);
    // needed for the radio to power up
    globals->get_props()->setDoubleValue("systems/electrical/outputs/navtest", 6.0);
    node->setDoubleValue("frequencies/selected-mhz", 113.8);

    SGGeod pos = SGGeod::fromDegFt(-3.352780, 55.499199, 20000);
    setPositionAndStabilise(r.get(), pos);

    CPPUNIT_ASSERT(!strcmp("TLA", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("in-range"));
}

void NavRadioTests::testCDIDeflection()
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "navtest");
    configNode->setIntValue("number", 2);
    std::unique_ptr<FGNavRadio> r(new FGNavRadio(configNode));

    r->bind();
    r->init();

    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/navtest[2]");
    node->setBoolValue("serviceable", true);
    // needed for the radio to power up
    globals->get_props()->setDoubleValue("systems/electrical/outputs/navtest", 6.0);
    node->setDoubleValue("frequencies/selected-mhz", 113.55);

    node->setDoubleValue("radials/selected-deg", 25);

    FGPositioned::TypeFilter f{FGPositioned::VOR};
    FGNavRecordRef nav = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("MCT", SGGeod::fromDeg(-2.26, 53.3), &f));

    // twist of MCT is 5.0, so we use a bearing of 20 here, not 25
    SGGeod posOnRadial = SGGeodesy::direct(nav->geod(), 20.0, 10 * SG_NM_TO_METER);
    posOnRadial.setElevationFt(10000);
    setPositionAndStabilise(r.get(), posOnRadial);

    CPPUNIT_ASSERT(!strcmp("MCT", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("crosstrack-error-m"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(!node->getBoolValue("to-flag"));


    // move off course
    SGGeod posOffRadial = SGGeodesy::direct(nav->geod(), 15.0, 20 * SG_NM_TO_METER); // 5 degress off
    posOffRadial.setElevationFt(12000);
    setPositionAndStabilise(r.get(), posOffRadial);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);

    double xtkE = sin(5.0 * SG_DEGREES_TO_RADIANS) * 20 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 50.0);
    CPPUNIT_ASSERT(node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(!node->getBoolValue("to-flag"));

    posOffRadial = SGGeodesy::direct(nav->geod(), 28.0, 30 * SG_NM_TO_METER); // 8 degress off
    posOffRadial.setElevationFt(16000);
    setPositionAndStabilise(r.get(), posOffRadial);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-8.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-0.8, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);

    xtkE = sin(-8.0 * SG_DEGREES_TO_RADIANS) * 30 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 50.0);

    // move more than ten degrees off course
    posOffRadial = SGGeodesy::direct(nav->geod(), 33.0, 40 * SG_NM_TO_METER); // 13 degress off
    posOffRadial.setElevationFt(16000);
    setPositionAndStabilise(r.get(), posOffRadial);

    // pegged to full deflection
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-10.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.0, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);

    // cross track error is computed based on true deflection, not clamped
    xtkE = sin(-13.0 * SG_DEGREES_TO_RADIANS) * 40 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 50.0);
    CPPUNIT_ASSERT(node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(!node->getBoolValue("to-flag"));

// try on the TO side of the station
    // let's use Perth VOR, but the Australian one to check southern
    // hemisphere operation
    node->setDoubleValue("frequencies/selected-mhz", 113.7);
    node->setDoubleValue("radials/selected-deg", 42.0); // twist is -2.0
    CPPUNIT_ASSERT(!strcmp("113.70", node->getStringValue("frequencies/selected-mhz-fmt")));

    auto perthVOR = fgpositioned_cast<FGNavRecord>(
        FGPositioned::findClosestWithIdent("PH", SGGeod::fromDeg(115.95, -31.9), &f));

    SGGeod p = SGGeodesy::direct(perthVOR->geod(), 220.0, 20 * SG_NM_TO_METER); // on the reciprocal radial
    p.setElevationFt(12000);
    setPositionAndStabilise(r.get(), p);

    CPPUNIT_ASSERT(!strcmp("PH", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(40.0, node->getDoubleValue("heading-deg"), 0.5);

    // actual radial has twist subtracted
    CPPUNIT_ASSERT_DOUBLES_EQUAL(222.0, node->getDoubleValue("radials/actual-deg"), 0.01);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("crosstrack-error-m"), 50.0);
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));

// off course on the TO side
    p = SGGeodesy::direct(perthVOR->geod(), 227.0, 100 * SG_NM_TO_METER);
    p.setElevationFt(18000);
    setPositionAndStabilise(r.get(), p);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(47.0, node->getDoubleValue("heading-deg"), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(229.0, node->getDoubleValue("radials/actual-deg"), 0.01);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.7, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);

    xtkE = sin(7.0 * SG_DEGREES_TO_RADIANS) * 100 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 50.0);
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));
}

void NavRadioTests::testILSBasic()
{
    // radio setup
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "navtest");
    configNode->setIntValue("number", 2);
    std::unique_ptr<FGNavRadio> r(new FGNavRadio(configNode));
    r->bind();
    r->init();

    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/navtest[2]");
    node->setBoolValue("serviceable", true);
    globals->get_props()->setDoubleValue("systems/electrical/outputs/navtest", 6.0);

    // test basic ILS: KSFO 28L
    FGPositioned::TypeFilter f{{FGPositioned::VOR, FGPositioned::ILS, FGPositioned::LOC}};
    FGNavRecordRef ils = fgpositioned_cast<FGNavRecord>(
        FGPositioned::findClosestWithIdent("ISFO", SGGeod::fromDeg(-112, 37.6), &f));
    CPPUNIT_ASSERT(ils->type() == FGPositioned::ILS);

    node->setDoubleValue("frequencies/selected-mhz", 109.55);
   // node->setDoubleValue("radials/selected-deg", 42.0); // twist is -2.0
    CPPUNIT_ASSERT(!strcmp("109.55", node->getStringValue("frequencies/selected-mhz-fmt")));

    // note we need full precision here, due to ILS sensitivity
    SGGeod p = SGGeodesy::direct(ils->geod(), 117.932, 10 * SG_NM_TO_METER);
    p.setElevationFt(2500);
    setPositionAndStabilise(r.get(), p);

    CPPUNIT_ASSERT(!strcmp("ISFO", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(297.9, node->getDoubleValue("radials/target-radial-deg"), 0.1);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(297.9, node->getDoubleValue("heading-deg"), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(117.932, node->getDoubleValue("radials/actual-deg"), 0.1);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("crosstrack-error-m"), 10.0);
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));

    //  1 degree offset
    p = SGGeodesy::direct(ils->geod(), 116.932, 6 * SG_NM_TO_METER);
    p.setElevationFt(1500);
    setPositionAndStabilise(r.get(), p);

    const double locWidth = ils->localizerWidth();
    const double deflectionScale = 20.0 / locWidth; // 20 degrees is full VOR swing (-10 to +10 degrees)

    CPPUNIT_ASSERT(!strcmp("ISFO", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(297.9, node->getDoubleValue("radials/target-radial-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(296.9, node->getDoubleValue("heading-deg"), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(116.932, node->getDoubleValue("radials/actual-deg"), 0.1);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.0 * deflectionScale, node->getDoubleValue("heading-needle-deflection"), 0.1);

    double xtkE = sin(-1.0 * SG_DEGREES_TO_RADIANS) * 6.0 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 1.0);
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));


    // test pegged (4 degrees off course)
    p = SGGeodesy::direct(ils->geod(), 121.932, 3 * SG_NM_TO_METER);
    p.setElevationFt(600);
    setPositionAndStabilise(r.get(), p);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(301.9, node->getDoubleValue("heading-deg"), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(121.932, node->getDoubleValue("radials/actual-deg"), 0.1);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, node->getDoubleValue("heading-needle-deflection"), 0.1);

    xtkE = sin(4.0 * SG_DEGREES_TO_RADIANS) * 3.0 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 1.0);
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));


    // also check ILS back course
    // 1 degree offset on the BC
    p = SGGeodesy::direct(ils->geod(), 298.932, 4 * SG_NM_TO_METER);
    p.setElevationFt(1500);
    setPositionAndStabilise(r.get(), p);

    CPPUNIT_ASSERT(!strcmp("ISFO", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(297.9, node->getDoubleValue("radials/target-radial-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(118.9, node->getDoubleValue("heading-deg"), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(298.932, node->getDoubleValue("radials/actual-deg"), 0.1);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.0 * deflectionScale, node->getDoubleValue("heading-needle-deflection"), 0.1);

    xtkE = sin(-1.0 * SG_DEGREES_TO_RADIANS) * 4.0 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 1.0);

    // these don't change for an ILS
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));
}



void NavRadioTests::testGS()
{
    // radio setup
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "navtest");
    configNode->setIntValue("number", 2);
    std::unique_ptr<FGNavRadio> r(new FGNavRadio(configNode));
    r->bind();
    r->init();

    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/navtest[2]");
    node->setBoolValue("serviceable", true);
    globals->get_props()->setDoubleValue("systems/electrical/outputs/navtest", 6.0);

    // EDDT 28R
    FGPositioned::TypeFilter f{FGPositioned::GS};
    FGNavRecordRef gs = fgpositioned_cast<FGNavRecord>(
                                                        FGPositioned::findClosestWithIdent("ITLW", SGGeod::fromDeg(13, 52), &f));
    CPPUNIT_ASSERT(gs->type() == FGPositioned::GS);
    node->setDoubleValue("frequencies/selected-mhz", 110.10);
    CPPUNIT_ASSERT(!strcmp("110.10", node->getStringValue("frequencies/selected-mhz-fmt")));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(gs->glideSlopeAngleDeg(), 3.0, 0.001);
    double gsAngleRad = gs->glideSlopeAngleDeg() * SG_DEGREES_TO_RADIANS;

    /////////////
    // derive the GS geometry in cartesian vectors, to match what
    // navradio.cxx does
    SGGeod aboveGS = gs->geod();
    aboveGS.setElevationM(gs->geod().getElevationM() + 100.0);
    SGVec3d gsVerticalAxis = SGVec3d::fromGeod(aboveGS) - gs->cart();
    // intentionally different approach to what navradio uses

    gsVerticalAxis *= 0.01; // make it per meter, since we used 100m above

    // dervice the baseline
    SGQuatd baseLineRot = SGQuatd::fromLonLat(gs->geod()) * SGQuatd::fromHeadAttBankDeg(80.828, 0, 0);
    SGVec3d gsAltAxis = baseLineRot.backTransform(SGVec3d(1.0, 0.0, 0.0));

    const SGVec3d gsCart = gs->cart();

   //////////////////

    SGVec3d radioPos = gsCart;
    radioPos += (gsVerticalAxis * tan(gsAngleRad) * 8 * SG_NM_TO_METER);
    radioPos += (gsAltAxis * 8 * SG_NM_TO_METER);

    setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));

    CPPUNIT_ASSERT(!strcmp("ITLW", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, node->getDoubleValue("gs-direct-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("gs-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("gs-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("gs-in-range"));

// 0.5 degree offset above
    gsAngleRad = (gs->glideSlopeAngleDeg() + 0.5) * SG_DEGREES_TO_RADIANS;
    radioPos = gsCart;
    radioPos += (gsVerticalAxis * tan(gsAngleRad) * 4 * SG_NM_TO_METER);
    radioPos += (gsAltAxis * 4 * SG_NM_TO_METER);

    setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));

    CPPUNIT_ASSERT(!strcmp("ITLW", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.5, node->getDoubleValue("gs-direct-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-2.5, node->getDoubleValue("gs-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-0.714, node->getDoubleValue("gs-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("gs-in-range"));

// 1 degree below (danger!)
    gsAngleRad = (gs->glideSlopeAngleDeg() - 1.0) * SG_DEGREES_TO_RADIANS;
    radioPos = gsCart;
    radioPos += (gsVerticalAxis * tan(gsAngleRad) * 2 * SG_NM_TO_METER);
    radioPos += (gsAltAxis * 2 * SG_NM_TO_METER);

    setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, node->getDoubleValue("gs-direct-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.5, node->getDoubleValue("gs-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("gs-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("gs-in-range"));

// false course above, reversed
    gsAngleRad = (gs->glideSlopeAngleDeg() + 3.0) * SG_DEGREES_TO_RADIANS;
    radioPos = gsCart;
    radioPos += (gsVerticalAxis * tan(gsAngleRad) * 5 * SG_NM_TO_METER);
    radioPos += (gsAltAxis * 5 * SG_NM_TO_METER);

    setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(6.0, node->getDoubleValue("gs-direct-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("gs-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("gs-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("gs-in-range"));

    // false course above, reversed, 0.35 offset below
    gsAngleRad = (gs->glideSlopeAngleDeg() + 2.65) * SG_DEGREES_TO_RADIANS;
    radioPos = gsCart;
    radioPos += (gsVerticalAxis * tan(gsAngleRad) * 3 * SG_NM_TO_METER);
    radioPos += (gsAltAxis * 3 * SG_NM_TO_METER);

    setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.65, node->getDoubleValue("gs-direct-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.75, node->getDoubleValue("gs-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-0.5, node->getDoubleValue("gs-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("gs-in-range"));
}

void NavRadioTests::testILSFalseCourse()
{

    // also GS false lobes
}

void NavRadioTests::testILSPaired()
{
    // EGPH and countless more
}

void NavRadioTests::testILSAdjacentPaired()
{
    // eg KJFK
}

#endif

