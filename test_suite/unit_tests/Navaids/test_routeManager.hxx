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


#ifndef _FG_ROUTE_MANAGER_UNIT_TESTS_HXX
#define _FG_ROUTE_MANAGER_UNIT_TESTS_HXX


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

class SGGeod;
class GPS;

// The flight plan unit tests.
class RouteManagerTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(RouteManagerTests);
    CPPUNIT_TEST(testBasic);
    CPPUNIT_TEST(testDefaultSID);
    CPPUNIT_TEST(testDefaultApproach);
    CPPUNIT_TEST(testDirectToLegOnFlightplanAndResume);
    CPPUNIT_TEST(testHoldFromNasal);
    CPPUNIT_TEST(testSequenceDiscontinuityAndResume);
    CPPUNIT_TEST(testHiddenWaypoints);
    CPPUNIT_TEST(loadGPX);
    CPPUNIT_TEST(loadFGFP);
    CPPUNIT_TEST(testRouteWithProcedures);
    CPPUNIT_TEST(testRouteWithApproachProcedures);
    CPPUNIT_TEST(testsSelectNavaid);
    CPPUNIT_TEST(testCommandAPI);
    CPPUNIT_TEST(testRMBug2616);
    CPPUNIT_TEST(testsSelectWaypoint);
    CPPUNIT_TEST(testsSelectWaypoint2);
    CPPUNIT_TEST(testAppendWaypoint);

    CPPUNIT_TEST_SUITE_END();

   // void setPositionAndStabilise(FGNavRadio* r, const SGGeod& g);

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    void setPositionAndStabilise(const SGGeod& g);
    
    // The tests.
    void testBasic();
    void testDefaultSID();
    void testDefaultApproach();
    void testDirectToLegOnFlightplanAndResume();
    void testHoldFromNasal();
    void testSequenceDiscontinuityAndResume();
    void testHiddenWaypoints();
    void loadGPX();
    void loadFGFP();
    void testRouteWithProcedures();
    void testRouteWithApproachProcedures();
    void testsSelectNavaid();
    void testCommandAPI();
    void testsSelectWaypoint();
    void testRMBug2616();
    void testsSelectWaypoint2();
    void testAppendWaypoint();
private:
    GPS* m_gps = nullptr;
};

#endif  // _FG_ROUTE_MANAGER_UNIT_TESTS_HXX
