
// SPDX-FileCopyrightText: (C) 2024 James Turner
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class BenchmarkPropsAccess : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(BenchmarkPropsAccess);
    CPPUNIT_TEST(benchSetProp);
    CPPUNIT_TEST(benchPropsNodeSet);
    CPPUNIT_TEST_SUITE_END();

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void benchSetProp();
    void benchPropsNodeSet();
};
