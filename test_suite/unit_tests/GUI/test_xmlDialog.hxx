/*
 * SPDX-FileComment: Unit tests for XML UI system
 * SPDX-FileCopyrightText: Copyright (C) 2025 James Turner <james@flightgear.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class XMLDialogTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(XMLDialogTests);
    CPPUNIT_TEST(testParseVersion1);
    CPPUNIT_TEST(testParseVersion2);
    CPPUNIT_TEST(testNasalAPI);

    CPPUNIT_TEST_SUITE_END();

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    void testParseVersion1();
    void testParseVersion2();
    void testNasalAPI();

private:
};
