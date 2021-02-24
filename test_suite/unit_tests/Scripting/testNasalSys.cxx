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
#include "test_suite/FGTestApi/NavDataCache.hxx"

#include <simgear/structure/commands.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/util.hxx>

#include <Scripting/NasalSys.hxx>

#include <Main/FGInterpolator.hxx>

// Set up function for each test.
void NasalSysTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("NasalSys");
    FGTestApi::setUp::initNavDataCache();

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

void NasalSysTests::testCommands()
{
    auto nasalSys = globals->get_subsystem<FGNasalSys>();
    nasalSys->getAndClearErrorList();

    fgSetInt("/foo/test", 7);
    bool ok = FGTestApi::executeNasal(R"(
     var f = func { 
         var i = getprop('/foo/test');
         setprop('foo/test', i + 4);
     };
                                      
      addcommand('do-foo', f);
      var ok = fgcommand('do-foo');
      unitTest.assert(ok);
    )");
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(11, fgGetInt("/foo/test"));

    SGPropertyNode_ptr args(new SGPropertyNode);
    ok = globals->get_commands()->execute("do-foo", args);
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(15, fgGetInt("/foo/test"));

    ok = FGTestApi::executeNasal(R"(
       var g = func { print('fail'); };
       addcommand('do-foo', g);
    )");
    CPPUNIT_ASSERT(ok);
    auto errors = nasalSys->getAndClearErrorList();
    CPPUNIT_ASSERT_EQUAL(1UL, errors.size());

    // old command shoudl still be registered and work
    ok = globals->get_commands()->execute("do-foo", args);
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_EQUAL(19, fgGetInt("/foo/test"));

    ok = FGTestApi::executeNasal(R"(
      removecommand('do-foo');
   )");
    CPPUNIT_ASSERT(ok);

    ok = FGTestApi::executeNasal(R"(
     var ok = fgcommand('do-foo');
     unitTest.assert(!ok);
  )");
    CPPUNIT_ASSERT(ok);
    errors = nasalSys->getAndClearErrorList();
    CPPUNIT_ASSERT_EQUAL(0UL, errors.size());

    // should fail, command is removed
    ok = globals->get_commands()->execute("do-foo", args);
    CPPUNIT_ASSERT(!ok);
    CPPUNIT_ASSERT_EQUAL(19, fgGetInt("/foo/test"));
}

void NasalSysTests::testAirportGhost()
{
    auto nasalSys = globals->get_subsystem<FGNasalSys>();
    nasalSys->getAndClearErrorList();

    bool ok = FGTestApi::executeNasal(R"(
        var apt = airportinfo('LFBD');
        var taxiways = apt.taxiways;    
        unitTest.assert_equal(size(taxiways), 0);
    )");
    CPPUNIT_ASSERT(ok);

}
