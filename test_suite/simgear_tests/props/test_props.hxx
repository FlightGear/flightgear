/*
 * Copyright (C) 2016 Edward d'Auvergne
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


#ifndef _FG_SIMGEAR_PROPS_SIMGEAR_UNIT_TESTS_HXX
#define _FG_SIMGEAR_PROPS_SIMGEAR_UNIT_TESTS_HXX


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include <simgear/props/props.hxx>


// The unit tests of the simgear property tree.
class SimgearPropsTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(SimgearPropsTests);
    CPPUNIT_TEST(testAliasLeak);
    CPPUNIT_TEST(testPropsCopyIf);
    CPPUNIT_TEST_SUITE_END();

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void testAliasLeak();
    void testPropsCopyIf();
private:
    // A property tree.
    SGPropertyNode *tree;
};

#endif  // _FG_SIMGEAR_PROPS_SIMGEAR_UNIT_TESTS_HXX
