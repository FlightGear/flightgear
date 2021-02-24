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

#include "test_runway.hxx"

#include <iostream>
#include <cstring>
#include <memory>
#include <unistd.h>

#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/SGGeod.hxx>
#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/performancedb.hxx>
#include <Airports/airport.hxx>
#include <Airports/airportdynamicsmanager.hxx>
#include <Airports/runways.hxx>
#include <Traffic/TrafficMgr.hxx>
#include <Time/TimeManager.hxx>

#include <ATC/atc_mgr.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void RunwayTests::setUp()
{
}

// Clean up after each test.
void RunwayTests::tearDown()
{
}

void RunwayTests::testRunway()
{
    PositionedID aAirport = 0;
    SGGeod aGeod = SGGeod::fromDeg(-33.92935800, 151.17160300);
    double heading = 155;
    int length = 3962;
    double width = 45.0;
    double displ_thresh = 79;
    double stopway = 0;
    FGRunway runway = FGRunway( FGPositioned::RUNWAY,
           aAirport, 
           "16R",
            aGeod,
            heading, 
            length,
            width,
            displ_thresh,
            stopway,
            1);
    int calculated = SGGeodesy::distanceM(runway.begin(), runway.end());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("Distance between the runway endpoints should be runway length", length, calculated, 1);
    calculated = SGGeodesy::distanceM(runway.begin(), runway.pointOnCenterline(-length));
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("Distance between the runway start and point on centerline should be runway length", length, calculated, 1);
}
