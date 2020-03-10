/*
 * Copyright (C) 2018 Edward d'Auvergne
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

#include "test_flightplan.hxx"
#include "test_navaids2.hxx"
#include "test_aircraftPerformance.hxx"
#include "test_routeManager.hxx"
#include "test_fpNasal.hxx"

// Set up the unit tests.
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(FlightplanTests, "Unit tests");
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(FPNasalTests, "Unit tests");
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(NavaidsTests, "Unit tests");
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(AircraftPerformanceTests, "Unit tests");
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(RouteManagerTests, "Unit tests");
