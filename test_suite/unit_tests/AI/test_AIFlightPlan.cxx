/*
 * Copyright (C) 2020 James Turner
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

#include "test_AIFlightPlan.hxx"

#include <cstring>
#include <memory>

#include "config.h"
#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestDataLogger.hxx"
#include "test_suite/FGTestApi/TestPilot.hxx"

#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>

#include <Airports/airport.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>

using std::string;
using namespace flightgear;

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void AIFlightPlanTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("AI");
    FGTestApi::setUp::initNavDataCache();

    globals->get_subsystem_mgr()->add<FGAIManager>();

    auto props = globals->get_props();
    props->setBoolValue("sim/ai/enabled", true);

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();
}

// Clean up after each test.
void AIFlightPlanTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void AIFlightPlanTests::testAIFlightPlan()
{
    std::unique_ptr<FGAIFlightPlan> aiFP(new FGAIFlightPlan);
    aiFP->setName("Bob");
    aiFP->setRunway("24");

    CPPUNIT_ASSERT_EQUAL(string{"Bob"}, aiFP->getName());
    CPPUNIT_ASSERT_EQUAL(string{"24"}, aiFP->getRunway());

    CPPUNIT_ASSERT_EQUAL(0, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getNextWaypoint());
    CPPUNIT_ASSERT_EQUAL(0, aiFP->getLeg());

    FGPositioned::TypeFilter ty(FGPositioned::VOR);
    auto cache = flightgear::NavDataCache::instance();
    auto shannonVOR = cache->findClosestWithIdent("SHA", SGGeod::fromDeg(-8, 52), &ty);
    CPPUNIT_ASSERT_EQUAL(string{"SHANNON VOR-DME"}, shannonVOR->name());

    auto wp1 = new FGAIWaypoint;
    wp1->setPos(shannonVOR->geod());
    wp1->setName("testWp_0");
    wp1->setOn_ground(true);
    wp1->setGear_down(true);
    wp1->setSpeed(100);

    auto wp2 = new FGAIWaypoint;
    const auto g1 = SGGeodesy::direct(shannonVOR->geod(), 10.0, SG_NM_TO_METER * 5.0);
    wp2->setPos(g1);
    wp2->setName("upInTheAir");
    wp2->setOn_ground(false);
    wp2->setGear_down(true);
    wp2->setSpeed(150);


    aiFP->addWaypoint(wp1);
    aiFP->addWaypoint(wp2);

    CPPUNIT_ASSERT_EQUAL(2, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp1, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp2, aiFP->getNextWaypoint());
    CPPUNIT_ASSERT_EQUAL(0, aiFP->getLeg());

    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, aiFP->getBearing(wp1, wp2), 0.1);

    time_t startTime = 1498;
    aiFP->setTime(startTime);
    CPPUNIT_ASSERT(!aiFP->isActive(1400));
    CPPUNIT_ASSERT(aiFP->isActive(1500));

    aiFP->IncrementWaypoint(false);
    CPPUNIT_ASSERT_EQUAL(2, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp1, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp2, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getNextWaypoint());
    CPPUNIT_ASSERT_EQUAL(0, aiFP->getLeg());

    auto wp3 = new FGAIWaypoint;
    auto diganWpt = cache->findClosestWithIdent("DIGAN", shannonVOR->geod(), nullptr);

    wp3->setPos(diganWpt->geod());
    wp3->setName("overDIGAN");
    wp3->setOn_ground(false);
    wp3->setGear_down(false);
    wp3->setSpeed(180);

    // check that adding a waypoint doesn't mess up the iterators or
    // current position
    aiFP->addWaypoint(wp3);
    CPPUNIT_ASSERT_EQUAL(3, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp1, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp2, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp3, aiFP->getNextWaypoint());
    CPPUNIT_ASSERT_EQUAL(0, aiFP->getLeg());

    auto p3 = SGGeodesy::direct(diganWpt->geod(), 45, SG_NM_TO_METER * 4);
    p3.setElevationFt(12000);
    auto wp4 = new FGAIWaypoint;
    wp4->setPos(p3);
    wp4->setName("passDIGAN");
    wp4->setSpeed(200);
    aiFP->addWaypoint(wp4);

    auto ingur = cache->findClosestWithIdent("INGUR", shannonVOR->geod(), nullptr);
    auto p4 = ingur->geod();
    p4.setElevationFt(16000);
    auto wp5 = new FGAIWaypoint;
    wp5->setPos(p4);
    wp5->setName("INGUR");
    wp5->setSpeed(250);
    aiFP->addWaypoint(wp5);

    aiFP->IncrementWaypoint(false);
    CPPUNIT_ASSERT_EQUAL(5, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp2, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp3, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp4, aiFP->getNextWaypoint());
    CPPUNIT_ASSERT_EQUAL(0, aiFP->getLeg());

    // let's increment to the end
    aiFP->IncrementWaypoint(false);
    aiFP->IncrementWaypoint(false);
    CPPUNIT_ASSERT_EQUAL(5, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp4, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp5, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getNextWaypoint());
    CPPUNIT_ASSERT_EQUAL(0, aiFP->getLeg());

    // one more increment 'off the end'
    aiFP->IncrementWaypoint(false);
    CPPUNIT_ASSERT_EQUAL(5, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp5, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getNextWaypoint());

    // should put us back on the last waypoint
    aiFP->DecrementWaypoint();
    CPPUNIT_ASSERT_EQUAL(5, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp4, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp5, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getNextWaypoint());
    CPPUNIT_ASSERT_EQUAL(0, aiFP->getLeg());

    aiFP->DecrementWaypoint(); // back to wp4
    aiFP->DecrementWaypoint(); // back to wp3
    aiFP->DecrementWaypoint(); // back to wp2

    CPPUNIT_ASSERT_EQUAL(5, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp1, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp2, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp3, aiFP->getNextWaypoint());
    CPPUNIT_ASSERT_EQUAL(0, aiFP->getLeg());

    // restart to the beginning
    aiFP->restart();
    CPPUNIT_ASSERT_EQUAL(5, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp1, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp2, aiFP->getNextWaypoint());
    CPPUNIT_ASSERT_EQUAL(0, aiFP->getLeg());

    // test increment with delete
    aiFP->IncrementWaypoint(true);
    CPPUNIT_ASSERT_EQUAL(5, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp1, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp2, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp3, aiFP->getNextWaypoint());

    aiFP->IncrementWaypoint(true);
    CPPUNIT_ASSERT_EQUAL(4, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp2, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp3, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp4, aiFP->getNextWaypoint());

    aiFP->IncrementWaypoint(true);
    CPPUNIT_ASSERT_EQUAL(3, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp3, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp4, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp5, aiFP->getNextWaypoint());

    // let's run up to the end and check nothing explodes
    aiFP->IncrementWaypoint(true);
    CPPUNIT_ASSERT_EQUAL(2, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp4, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(wp5, aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getNextWaypoint());

    aiFP->IncrementWaypoint(true);
    CPPUNIT_ASSERT_EQUAL(1, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(wp5, aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getNextWaypoint());
}

void AIFlightPlanTests::testAIFlightPlanLeftCircle()
{
    auto aiFP = new FGAIFlightPlan;
    aiFP->setName("Bob");
    aiFP->setRunway("24");

    CPPUNIT_ASSERT_EQUAL(string{"Bob"}, aiFP->getName());
    CPPUNIT_ASSERT_EQUAL(string{"24"}, aiFP->getRunway());

    CPPUNIT_ASSERT_EQUAL(0, aiFP->getNrOfWayPoints());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getPreviousWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getCurrentWaypoint());
    CPPUNIT_ASSERT_EQUAL(static_cast<FGAIWaypoint*>(nullptr), aiFP->getNextWaypoint());
    CPPUNIT_ASSERT_EQUAL(0, aiFP->getLeg());

    FGPositioned::TypeFilter ty(FGPositioned::VOR);
    auto cache = flightgear::NavDataCache::instance();
    auto shannonVOR = cache->findClosestWithIdent("SHA", SGGeod::fromDeg(-8, 52), &ty);
    CPPUNIT_ASSERT_EQUAL(string{"SHANNON VOR-DME"}, shannonVOR->name());
    auto wp1 = new FGAIWaypoint;
    wp1->setPos(shannonVOR->geod());
    wp1->setName("testWp_0");
    wp1->setOn_ground(true);
    wp1->setGear_down(true);
    wp1->setSpeed(10);
    aiFP->addWaypoint(wp1);

    auto lastWp = wp1;

    int course = 0;

    for(int i = 1; i <= 10; i++) {
        auto wp = new FGAIWaypoint;
        course += 10;
        const auto g1 = SGGeodesy::direct(lastWp->getPos(), course, SG_NM_TO_METER * 5.0);
        wp->setPos(g1);
        wp->setName("testWp_" + std::to_string(i));
        wp->setOn_ground(true);
        wp->setGear_down(true);
        wp->setSpeed(10);
        aiFP->addWaypoint(wp);
        lastWp = wp;
    }
    CPPUNIT_ASSERT_EQUAL(aiFP->getNrOfWayPoints(), 11);
}

void AIFlightPlanTests::testAIFlightPlanLoadXML()
{
    const auto xml = R"(<?xml version="1.0" encoding="UTF-8"?>
      <PropertyList>
        <flightplan>
            <wp>
                <name>onGroundWP</name>
                <lat>57</lat>
                <lon>3</lon>
                <ktas>10</ktas>
                <on-ground>1</on-ground>
            </wp>
            <wp>
                <name>someWP</name>
                    <lat>57</lat>
                    <lon>4</lon>
                <ktas>200</ktas>
                <alt>8000</alt>
            </wp>
            <wp>
                <name>END</name>
            </wp>
        </flightplan>
        </PropertyList>
    )";

    std::istringstream is(xml);

    std::unique_ptr<FGAIFlightPlan> aiFP(new FGAIFlightPlan);
    bool ok = aiFP->readFlightplan(is, sg_location("In-memory test_ai_fp.xml"));
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(false, aiFP->getCurrentWaypoint()->getInAir());
    CPPUNIT_ASSERT_EQUAL(true, aiFP->getCurrentWaypoint()->getGear_down());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, aiFP->getCurrentWaypoint()->getFlaps(), 0.1);

    auto wp2 = aiFP->getNextWaypoint();
    CPPUNIT_ASSERT_EQUAL(true, wp2->getInAir());
    CPPUNIT_ASSERT_EQUAL(false, wp2->getGear_down());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, wp2->getFlaps(), 0.1);
}

void AIFlightPlanTests::testLeftTurnFlightplanXML()
{
    std::unique_ptr<FGAIFlightPlan> aiFP(new FGAIFlightPlan);
    const auto fpath = SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "AI"/"Flightplan"/"left_onground.xml";

    std::fstream fs(fpath.c_str());

    bool ok = aiFP->readFlightplan(fs, sg_location("In-memory test_ai_fp.xml"));
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(false, aiFP->getCurrentWaypoint()->getInAir());

    auto wp2 = aiFP->getNextWaypoint();
    CPPUNIT_ASSERT_EQUAL(false, wp2->getInAir());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, wp2->getSpeed(), 0.1);
}

void AIFlightPlanTests::testRightTurnFlightplanXML()
{
    std::unique_ptr<FGAIFlightPlan> aiFP(new FGAIFlightPlan);
    const auto fpath = SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "AI"/"Flightplan"/"right_onground.xml";

    std::fstream fs(fpath.c_str());

    bool ok = aiFP->readFlightplan(fs, sg_location("In-memory test_ai_fp.xml"));
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(false, aiFP->getCurrentWaypoint()->getInAir());

    auto wp2 = aiFP->getNextWaypoint();
    CPPUNIT_ASSERT_EQUAL(false, wp2->getInAir());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, wp2->getSpeed(), 0.1);
}
