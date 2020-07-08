/*
 * Copyright (C) 2016 Edward d'Auvergne
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


#include "testNasalSys.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Scripting/NasalSys.hxx>

#include <Main/FGInterpolator.hxx>

// Set up function for each test.
void NasalSysTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("NasalSys");

    fgInitAllowedPaths();
    globals->get_props()->getNode("nasal", true);

    globals->add_subsystem("prop-interpolator", new FGInterpolator, SGSubsystemMgr::INIT);

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();

    globals->add_new_subsystem<FGNasalSys>(SGSubsystemMgr::INIT);

    globals->get_subsystem_mgr()->postinit();
}


// Clean up after each test.
void NasalSysTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


// Test test
void NasalSysTests::testStructEquality()
{
    bool ok = FGTestApi::executeNasal(R"(
     var foo = {
      "name": "Bob",
          "size": [512, 512],
          "mipmapping": 1.9
        };
                                      
    var bar = {
      "name": "Bob",
          "size": [512, 512],
          "mipmapping": 1.9
        };       

    unitTest.assert_equal(foo, bar);
                                      
    append(bar.size, "Wowow");
    unitTest.assert(unitTest.equal(foo, bar) == 0);
                                      
    append(foo.size, "Wowow");
    unitTest.assert_equal(foo, bar);
                                      
    foo.wibble = 99.1;
    unitTest.assert(unitTest.equal(foo, bar) == 0);
                                      
    bar.wibble = 99;
    unitTest.assert(unitTest.equal(foo, bar) == 0);
    bar.wibble = 99.1;
    unitTest.assert_equal(foo, bar);

    )");
    CPPUNIT_ASSERT(ok);
}
