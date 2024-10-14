
// SPDX-FileCopyrightText: (C) 2020 James Turner
// SPDX-License-Identifier: GPL-2.0-or-later

#include "testNasalLib.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Scripting/NasalSys.hxx>

#include <Main/FGInterpolator.hxx>

extern bool global_nasalMinimalInit;

// Set up function for each test.
void NasalLibTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("NasalGC");

    fgInitAllowedPaths();

    globals->get_subsystem_mgr()->add<FGInterpolator>();

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();

    global_nasalMinimalInit = true;
    globals->get_subsystem_mgr()->add<FGNasalSys>();

    globals->get_subsystem_mgr()->postinit();
}


// Clean up after each test.
void NasalLibTests::tearDown()
{
    global_nasalMinimalInit = false;
    FGTestApi::tearDown::shutdownTestGlobals();
}


// Test test
void NasalLibTests::testVector()
{
    bool ok = FGTestApi::executeNasal(R"(

        var v1 = ['apples', 'pears', 'lemons', 'strawberries'];
        append(v1, 'melons');
        unitTest.assert_equal(size(v1), 5);

        var item = removeat(v1, 2);
        unitTest.assert_equal(size(v1), 4);
        unitTest.assert_equal(item,'lemons');
        unitTest.assert_equal(v1[2], 'strawberries');

        remove(v1, 'carrots');
        unitTest.assert_equal(size(v1), 4); # nothing changes

        remove(v1, 'pears');
        unitTest.assert_equal(size(v1), 3); 
        unitTest.assert_equal(v1[2], 'melons');

        unitTest.assert(contains(v1, 'pears') == 0);

    )");
    CPPUNIT_ASSERT(ok);
}
