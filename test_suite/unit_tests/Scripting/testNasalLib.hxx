
// SPDX-FileCopyrightText: (C) 2020 James Turner
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class NasalLibTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(NasalLibTests);
    CPPUNIT_TEST(testVector);
    CPPUNIT_TEST_SUITE_END();

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void testVector();
};
