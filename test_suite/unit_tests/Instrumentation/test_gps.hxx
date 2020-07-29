/*
 * Copyright (C) 2019 James Turner
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


#ifndef _FG_GPS_UNIT_TESTS_HXX
#define _FG_GPS_UNIT_TESTS_HXX


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include <memory>

#include <simgear/props/props.hxx>

class SGGeod;
class GPS;

// The flight plan unit tests.
class GPSTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(GPSTests);
    CPPUNIT_TEST(testBasic);
    CPPUNIT_TEST(testNavRadioSlave);
    CPPUNIT_TEST(testTurnAnticipation);
    CPPUNIT_TEST(testOBSMode);
    CPPUNIT_TEST(testDirectTo);
    CPPUNIT_TEST(testLegMode);
    CPPUNIT_TEST(testDirectToLegOnFlightplan);
    CPPUNIT_TEST(testLongLeg);
    CPPUNIT_TEST(testLongLegWestbound);
    CPPUNIT_TEST(testOffsetFlight);
    CPPUNIT_TEST(testOverflightSequencing);
    CPPUNIT_TEST(testOffcourseSequencing);
    CPPUNIT_TEST(testLegIntercept);
    CPPUNIT_TEST(testDirectToLegOnFlightplanAndResumeBuiltin);
    CPPUNIT_TEST(testBuiltinRevertToOBSAtEnd);
    CPPUNIT_TEST(testRadialIntercept);
    CPPUNIT_TEST(testDMEIntercept);
    CPPUNIT_TEST(testFinalLegCourse);
    CPPUNIT_TEST(testCourseLegIntermediateWaypoint);

    CPPUNIT_TEST_SUITE_END();

    void setPositionAndStabilise(GPS* gps, const SGGeod& g);

    GPS* setupStandardGPS(SGPropertyNode_ptr config = {},
                                          const std::string name = "gps", const int index = 0);
    void setupRouteManager();
    
public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void testBasic();
    void testNavRadioSlave();
    void testTurnAnticipation();
    void testOBSMode();
    void testDirectTo();
    void testLegMode();
    void testDirectToLegOnFlightplan();
    void testLongLeg();
    void testLongLegWestbound();
    void testOffsetFlight();
    void testOffcourseSequencing();
    void testOverflightSequencing();
    void testLegIntercept();
    void testDirectToLegOnFlightplanAndResumeBuiltin();
    void testBuiltinRevertToOBSAtEnd();
    void testRadialIntercept();
    void testDMEIntercept();
    void testFinalLegCourse();
    void testCourseLegIntermediateWaypoint();
};

#endif  // _FG_GPS_UNIT_TESTS_HXX
