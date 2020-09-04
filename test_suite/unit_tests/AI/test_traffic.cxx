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

#include "test_traffic.hxx"

#include <cstring>
#include <memory>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestDataLogger.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void TrafficTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("Traffic");
    FGTestApi::setUp::initNavDataCache();
}

// Clean up after each test.
void TrafficTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void TrafficTests::testBasic()
{
}
