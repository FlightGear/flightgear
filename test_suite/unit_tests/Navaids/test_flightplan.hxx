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


#ifndef FG_FLIGHTPLAN_UNIT_TESTS_HXX
#define FG_FLIGHTPLAN_UNIT_TESTS_HXX


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>


// The flight plan unit tests.
class FlightplanTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(FlightplanTests);
    CPPUNIT_TEST(testBasic);
    CPPUNIT_TEST(testRoutePathBasic);
    CPPUNIT_TEST(testRoutePathSkipped);
    CPPUNIT_TEST(testRoutePathTrivialFlightPlan);
    CPPUNIT_TEST(testBasicAirways);
    CPPUNIT_TEST(testAirwayNetworkRoute);
    CPPUNIT_TEST(testBug1814);
    CPPUNIT_TEST(testRoutPathWpt0Midflight);
    CPPUNIT_TEST(testRoutePathVec);
    CPPUNIT_TEST(testRoutePathFinalLegVQPR15);
    CPPUNIT_TEST(testLoadSaveMachRestriction);
    CPPUNIT_TEST(testOnlyDiscontinuityRoute);
    CPPUNIT_TEST(testBasicDiscontinuity);
    CPPUNIT_TEST(testLeadingWPDynamic);
    CPPUNIT_TEST(testRadialIntercept);
    CPPUNIT_TEST(loadFGFPWithoutDepartureArrival);
    CPPUNIT_TEST(loadFGFPWithEmbeddedProcedures);
    CPPUNIT_TEST(loadFGFPWithOldProcedures);
    CPPUNIT_TEST(loadFGFPWithProcedureIdents);
    CPPUNIT_TEST(testCloningBasic);
    CPPUNIT_TEST(testCloningFGFP);
    CPPUNIT_TEST(testCloningProcedures);
    
  //  CPPUNIT_TEST(testParseICAORoute);
   // CPPUNIT_TEST(testParseICANLowLevelRoute);
    CPPUNIT_TEST_SUITE_END();

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void testBasic();
    void testRoutePathBasic();
    void testRoutePathSkipped();
    void testRoutePathTrivialFlightPlan();
    void testBasicAirways();
    void testAirwayNetworkRoute();
    void testParseICAORoute();
    void testParseICANLowLevelRoute();
    void testBug1814();
    void testRoutPathWpt0Midflight();
    void testRoutePathVec();
    void testRoutePathFinalLegVQPR15();
    void testLoadSaveMachRestriction();
    void testBasicDiscontinuity();
    void testOnlyDiscontinuityRoute();
    void testLeadingWPDynamic();
    void testRadialIntercept();
    void loadFGFPWithoutDepartureArrival();
    void loadFGFPWithEmbeddedProcedures();
    void loadFGFPWithOldProcedures();
    void loadFGFPWithProcedureIdents();
    void testCloningBasic();
    void testCloningFGFP();
    void testCloningProcedures();
};

#endif  // FG_FLIGHTPLAN_UNIT_TESTS_HXX
