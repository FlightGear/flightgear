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


#pragma once


#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <simgear/structure/subsystem_mgr.hxx>

class FGNavRadio;
class SGGeod;

// The flight plan unit tests.
class CommRadioTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(CommRadioTests);

    CPPUNIT_TEST(testBasic);
    CPPUNIT_TEST(testEightPointThree);
    CPPUNIT_TEST(testEPLLTuning833);
    CPPUNIT_TEST(testEPLLTuning25);

    CPPUNIT_TEST_SUITE_END();

    SGSubsystemRef setupStandardRadio(const std::string& name, int index, bool enable833);

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    //  std::string formatFrequency(double f);

    // The tests.
    void testBasic();
    void testEightPointThree();
    void testEPLLTuning833();
    void testEPLLTuning25();
};
