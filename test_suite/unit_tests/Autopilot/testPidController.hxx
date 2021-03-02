/*
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

#include <simgear/props/props.hxx>


// The system tests.
struct PidControllerTests : public CppUnit::TestFixture
{
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void test0();
    void test1();
    
    private:
    
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(PidControllerTests);
    CPPUNIT_TEST(test0);
    CPPUNIT_TEST(test1);
    CPPUNIT_TEST_SUITE_END();

    SGPropertyNode_ptr configFromString(const std::string& s);
    void test(bool startup_zeros);
};
