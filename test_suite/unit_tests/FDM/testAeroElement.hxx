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


#ifndef _FG_AERO_ELEMENT_UNIT_TESTS_HXX
#define _FG_AERO_ELEMENT_UNIT_TESTS_HXX


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>


// The unit tests.
class AeroElementTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(AeroElementTests);
    CPPUNIT_TEST(testBoundVortex);
    CPPUNIT_TEST(testBoundVortexMidPoint);
    CPPUNIT_TEST(testCollocationPoint);
    CPPUNIT_TEST(testInducedVelocityAbove);
    CPPUNIT_TEST(testInducedVelocityAboveWithOffset);
    CPPUNIT_TEST(testInducedVelocityAtFarField);
    CPPUNIT_TEST(testInducedVelocityOnBoundVortex);
    //CPPUNIT_TEST(testInducedVelocityOnCollocationPoint);    // Not run in the original ctest.
    CPPUNIT_TEST(testInducedVelocityUpstream);
    CPPUNIT_TEST(testNormal);
    CPPUNIT_TEST_SUITE_END();

public:
    // Set up function for each test.
    void setUp() {}

    // Clean up after each test.
    void tearDown() {}

    // The tests.
    void testBoundVortex();
    void testBoundVortexMidPoint();
    void testCollocationPoint();
    void testInducedVelocityAbove();
    void testInducedVelocityAboveWithOffset();
    void testInducedVelocityAtFarField();
    void testInducedVelocityOnBoundVortex();
    void testInducedVelocityOnCollocationPoint();
    void testInducedVelocityUpstream();
    void testNormal();
};

#endif  // _FG_AERO_ELEMENT_UNIT_TESTS_HXX
