/*
 * Copyright (C) 2020 James Turner
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

class FGAIAircraft;

// The flight plan unit tests.
class TrafficTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(TrafficTests);
    CPPUNIT_TEST(testPushback);
    CPPUNIT_TEST(testPushback2);
    CPPUNIT_TEST(testChangeRunway);
    CPPUNIT_TEST(testPushforward);
    CPPUNIT_TEST(testPushforwardSpeedy);
    CPPUNIT_TEST(testPushforwardParkYBBN);
    CPPUNIT_TEST(testPushforwardParkYBBNRepeat);
    CPPUNIT_TEST_SUITE_END();
public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void testPushback();
    void testPushback2();
    void testChangeRunway();
    void testPushforward();
    void testPushforwardSpeedy();
    void testPushforwardParkYBBN();
    void testPushforwardParkYBBNRepeat();
private:
    void dump(FGAIAircraft* aiAircraft);
    std::string getTimeString(int timeOffset);
    FGAIAircraft * flyAI(FGAIAircraft * aiAircraft, std::string fName);
};
