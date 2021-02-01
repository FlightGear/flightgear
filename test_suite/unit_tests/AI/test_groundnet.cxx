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


    globals->add_new_subsystem<PerformanceDB>();
    globals->add_new_subsystem<FGATCManager>();
    globals->add_new_subsystem<FGAIManager>();
    globals->add_new_subsystem<flightgear::AirportDynamicsManager>();

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

    FGAISchedule* schedule = new FGAISchedule;

    FGAIAircraft* aiAircraft = new FGAIAircraft{schedule};

    // std::cout << "*** Start ***\n";

    FGGroundNetwork* network = egph->groundNetwork();    

    FGParkingRef startParking = network->findParkingByName("main-apron10");

    FGRunwayRef runway = egph->getRunwayByIndex(0);

    // std::cout << startParking->getName() << "\n";

    // std::cout << runway->name() << " " << runway->begin() << " " << runway->end() << "\n";

    FGTaxiNodeRef end = network->findNearestNodeOnRunway(runway->threshold());

    // std::cout << end->geod() << "\n";

    FGTaxiRoute route = network->findShortestRoute(startParking, end); 

    // std::cout << route.size() << "\n";

    CPPUNIT_ASSERT_EQUAL(true, network->exists());
    CPPUNIT_ASSERT_EQUAL(25, route.size());
}
