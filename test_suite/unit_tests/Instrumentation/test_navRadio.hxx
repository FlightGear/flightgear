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


#ifndef _FG_NAVRADIO_UNIT_TESTS_HXX
#define _FG_NAVRADIO_UNIT_TESTS_HXX


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

class FGNavRadio;
class SGGeod;

// The flight plan unit tests.
class NavRadioTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(NavRadioTests);
    CPPUNIT_TEST(testBasic);
    CPPUNIT_TEST(testCDIDeflection);
    
    CPPUNIT_TEST(testILSBasic);
    CPPUNIT_TEST(testGS);
    CPPUNIT_TEST(testILSFalseCourse);
    CPPUNIT_TEST(testILSPaired);
    CPPUNIT_TEST(testILSAdjacentPaired);
    CPPUNIT_TEST(testGlideslopeLongDistance);

    CPPUNIT_TEST_SUITE_END();

    void setPositionAndStabilise(FGNavRadio* r, const SGGeod& g);

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void testBasic();
    void testCDIDeflection();
    
    void testILSBasic();
    void testGS();
    void testILSFalseCourse();
    void testILSPaired();
    void testILSAdjacentPaired();
    void testGlideslopeLongDistance();
};

#endif  // _FG_NAVRADIO_UNIT_TESTS_HXX
