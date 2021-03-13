/*
 * Copyright (C) 2021 James Turner <james@flightgear.org>
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


#pragma once

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>


// The unit tests.
class TimeManagerTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(TimeManagerTests);
    CPPUNIT_TEST(testBasic);
    CPPUNIT_TEST(testTimeZones);
    CPPUNIT_TEST(testFreezeUnfreeze);
    CPPUNIT_TEST(testSpecifyTimeOffset);
    CPPUNIT_TEST(testETCTimeZones);
    CPPUNIT_TEST_SUITE_END();

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void testBasic();
    void testTimeZones();
    void testFreezeUnfreeze();
    void testSpecifyTimeOffset();
    void testETCTimeZones();
};
