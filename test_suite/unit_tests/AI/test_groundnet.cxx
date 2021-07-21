/*
 * Copyright (C) 2021 Keith Paterson
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

#include "config.h"

#include "test_groundnet.hxx"

#include <cstring>
#include <memory>
#include <iostream>


#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestDataLogger.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/performancedb.hxx>
#include <Airports/airport.hxx>
#include <Airports/airportdynamicsmanager.hxx>
#include <Airports/groundnetwork.hxx>
#include <Airports/parking.hxx>
#include <Traffic/TrafficMgr.hxx>

#include <ATC/atc_mgr.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void GroundnetTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("Traffic");
    FGTestApi::setUp::initNavDataCache();


    auto props = globals->get_props();
    props->setBoolValue("sim/ai/enabled", true);
    props->setBoolValue("sim/signals/fdm-initialized", false);


    // ensure EGPH has a valid ground net for parking testing
    FGAirport::clearAirportsCache();
    FGAirportRef egph = FGAirport::getByIdent("EGPH");
    egph->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "EGPH.groundnet.xml");

    FGAirportRef ybbn = FGAirport::getByIdent("YBBN");
    ybbn->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "YBBN.groundnet.xml");


    globals->add_new_subsystem<PerformanceDB>(SGSubsystemMgr::GENERAL);
    globals->add_new_subsystem<FGATCManager>(SGSubsystemMgr::GENERAL);
    globals->add_new_subsystem<FGAIManager>(SGSubsystemMgr::GENERAL);
    globals->add_new_subsystem<flightgear::AirportDynamicsManager>(SGSubsystemMgr::GENERAL);

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();
}

// Clean up after each test.
void GroundnetTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void GroundnetTests::testShortestRoute()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");

    FGGroundNetwork* network = egph->groundNetwork();    
    FGParkingRef startParking = network->findParkingByName("main-apron10");
    FGRunwayRef runway = egph->getRunwayByIndex(0);
    FGTaxiNodeRef end = network->findNearestNodeOnRunway(runway->threshold());
    FGTaxiRoute route = network->findShortestRoute(startParking, end); 
    CPPUNIT_ASSERT_EQUAL(true, network->exists());
    CPPUNIT_ASSERT_EQUAL(29, route.size());
}

/**
 * Tests various find methods.
 */

void GroundnetTests::testFind()
{
    FGAirportRef ybbn = FGAirport::getByIdent("YBBN");

    FGGroundNetwork* network = ybbn->groundNetwork();    
    FGParkingRef startParking = network->findParkingByName("GA1");
    CPPUNIT_ASSERT_EQUAL(1018, startParking->getIndex());
    FGTaxiSegment* segment1 = network->findSegment(startParking, NULL);
    CPPUNIT_ASSERT(segment1);
    FGTaxiSegment* segment2 = network->findSegment(startParking, segment1->getEnd());
    CPPUNIT_ASSERT(segment2);
    FGTaxiNodeVector segmentList = network->findSegmentsFrom(startParking);
    CPPUNIT_ASSERT_EQUAL(2, (int)segmentList.size());
    CPPUNIT_ASSERT_EQUAL(1024, segmentList.front()->getIndex());
    CPPUNIT_ASSERT_EQUAL(1025, segmentList.back()->getIndex());
    FGTaxiSegment* pushForwardSegment = network->findSegmentByHeading(startParking, startParking->getHeading()); 
    CPPUNIT_ASSERT(pushForwardSegment);
    CPPUNIT_ASSERT_EQUAL(1025, pushForwardSegment->getEnd()->getIndex());
}
