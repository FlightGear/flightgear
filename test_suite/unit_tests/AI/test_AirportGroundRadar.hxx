/*
 * Copyright (C) 2024 Keith Paterson
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

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <memory>

#include <simgear/props/props.hxx>

class SGGeod;

// The AI flight plan unit tests.
class AirportGroundRadarTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(AirportGroundRadarTests);
    CPPUNIT_TEST(testFillingTree);
    CPPUNIT_TEST(testFillingTreeSplit);
    CPPUNIT_TEST(testFillingTreeRemove);
    CPPUNIT_TEST(testBlocked);
    CPPUNIT_TEST(testBlocked1);
    CPPUNIT_TEST(testBlockedBy1);
    CPPUNIT_TEST(testBlockedByQueue);
    CPPUNIT_TEST(testMove);
    CPPUNIT_TEST(testMoveLarge);
    CPPUNIT_TEST(testAirport);
    CPPUNIT_TEST_SUITE_END();


public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    /**Filling of the Quadtree without split*/
    void testFillingTree();
    /**Filling of the Quadtree with split*/
    void testFillingTreeSplit();
    /**Filling of the Quadtree with split and removing elements.*/
    void testFillingTreeRemove();
    /**Testing detection of blocked pointing away from each other*/
    void testBlocked();
    /**Testing detection of blocked one behind the other*/
    void testBlocked1();
    /**Testing detection of blocked one behind the other*/
    void testBlockedBy1();
    /**Testing detection of blocked one behind the other*/
    void testBlockedByQueue();
    /**Testing movement*/
    void testMove();
    void testMoveLarge();

    void testAirport();
};
