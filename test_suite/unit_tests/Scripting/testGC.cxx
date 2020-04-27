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


#include "testGC.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Scripting/NasalSys.hxx>

#include <Main/FGInterpolator.hxx>

extern bool global_nasalMinimalInit;

// Set up function for each test.
void NasalGCTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("NasalGC");

    fgInitAllowedPaths();
    auto nasalNode = globals->get_props()->getNode("nasal", true);

    globals->add_subsystem("prop-interpolator", new FGInterpolator, SGSubsystemMgr::INIT);

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    
    global_nasalMinimalInit = true;
    globals->add_new_subsystem<FGNasalSys>(SGSubsystemMgr::INIT);
    
    globals->get_subsystem_mgr()->postinit();
}


// Clean up after each test.
void NasalGCTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


// Test test
void NasalGCTests::testDummy()
{
    bool ok = FGTestApi::executeNasal(R"(
     var foo = {
      "name": "PFD-Test",
          "size": [512, 512],
          "view": [768, 1024],
          "mipmapping": 1
        };
                                      
      globals.foo1 = foo;
    )");
    CPPUNIT_ASSERT(ok);
}
