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


#ifndef _FG_POSINIT_UNIT_TESTS_HXX
#define _FG_POSINIT_UNIT_TESTS_HXX


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>
#include "Airports/airport.hxx"


// The unit tests of the FGNasalSys subsystem.
class PosInitTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(PosInitTests);

    // Airport-based tests
    CPPUNIT_TEST(testAirportAltitudeOffsetStartup);
    CPPUNIT_TEST(testAirportAndMetarStartup);
    CPPUNIT_TEST(testAirportAndRunwayStartup);
    CPPUNIT_TEST(testAirportAndAvailableParkingStartup);
    CPPUNIT_TEST(testAirportAndParkingStartup);
    CPPUNIT_TEST(testAirportOnlyStartup);
    CPPUNIT_TEST(testAirportRunwayOffsetAltitudeStartup);
    CPPUNIT_TEST(testAirportRunwayOffsetGlideslopeStartup);
    CPPUNIT_TEST(testDefaultStartup);
    CPPUNIT_TEST(testRepositionAtParking);
    CPPUNIT_TEST(testParkAtOccupied);
    CPPUNIT_TEST(testParkInvalid);
    CPPUNIT_TEST(testAirportRunwayRepositionAirport);


    // Navaid tests
    CPPUNIT_TEST(testVOROnlyStartup);
    CPPUNIT_TEST(testVOROffsetAltitudeHeadingStartup);
    CPPUNIT_TEST(testFixOnlyStartup);
    CPPUNIT_TEST(testFixOffsetAltitudeHeadingStartup);
    CPPUNIT_TEST(testNDBOnlyStartup);
    CPPUNIT_TEST(testNDBOffsetAltitudeHeadingStartup);

    CPPUNIT_TEST(testLatLonStartup);
    //CPPUNIT_TEST(testLatLonOffsetStartup); This is not yet supported.

    // Carrier tests
    // We are not able to test the carrier code thoroughly as it depends
    // heavily on finalizePosition(), which requires that the carrier model
    // itself be loaded into the scenegraph.
    CPPUNIT_TEST(testCarrierStartup);

    // Reposition tests
    CPPUNIT_TEST(testAirportRepositionAirport);
    CPPUNIT_TEST(testRepositionAtSameParking);
    CPPUNIT_TEST(testRepositionAtOccupied);
    CPPUNIT_TEST(testRepositionAtInvalid);
    
    CPPUNIT_TEST_SUITE_END();

    void simulateStartReposition();
    void simulateFinalizePosition();
public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void testAirportAltitudeOffsetStartup();
    void testAirportAndMetarStartup();
    void testAirportAndAvailableParkingStartup();
    void testAirportAndParkingStartup();
    void testAirportAndRunwayStartup();
    void testAirportOnlyStartup();
    void testAirportRunwayOffsetAltitudeStartup();
    void testAirportRunwayOffsetGlideslopeStartup();
    void testDefaultStartup();
    void testParkAtOccupied();
    void testParkInvalid();
    
    // Navaid tests
    void testVOROnlyStartup();
    void testVOROffsetAltitudeHeadingStartup();
    void testFixOnlyStartup();
    void testFixOffsetAltitudeHeadingStartup();
    void testNDBOnlyStartup();
    void testNDBOffsetAltitudeHeadingStartup();

    //Lat Lon tests
    void testLatLonStartup();
    void testLatLonOffsetStartup();

    //Carrier tests
    void testCarrierStartup();

    //Reposition tests
    void testAirportRepositionAirport();
    void testRepositionAtParking();
    void testRepositionAtSameParking();
    void testRepositionAtOccupied();
    void testRepositionAtInvalid();
    void testAirportRunwayRepositionAirport();

private:
    // Helper functions for tests.  Return void as they use CPPUNIT_ASSERT
    void checkAlt(float value);
    void checkHeading(float value);
    void checkPosition(SGGeod expectedPos, float delta=1000.0);
    void checkClosestAirport(std::string icao);
    void checkStringProp(std::string property, std::string expected);
    void checkRunway(std::string expected);
    void checkOnGround();
    void checkInAir();
};

#endif  // _FG_POSINIT_UNIT_TESTS_HXX
