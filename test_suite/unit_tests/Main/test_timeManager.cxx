// Written by James Turner, started 2021.
//
// Copyright (C) 2021  James Turner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "config.h"

#include "test_timeManager.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"

#include <simgear/props/props_io.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_dir.hxx>

#include "Main/globals.hxx"
#include "Main/fg_props.hxx"

using namespace flightgear;


// Set up function for each test.
void TimeManagerTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("timeManager");

}


// Clean up after each test.
void TimeManagerTests::tearDown()
{
   FGTestApi::tearDown::shutdownTestGlobals();
}


void TimeManagerTests::testBasic()
{
    // SGPath testUserDataPath = globals->get_fg_home() / "test_autosave_migrate";
    // if (!testUserDataPath.exists()) {
    //     SGPath p = testUserDataPath / "foo";
    //     p.create_dir(0755);
    // }

    // simgear::Dir homeDir(testUserDataPath);
    // for (auto path : homeDir.children(simgear::Dir::TYPE_FILE, ".xml")) {
    //     path.remove();
    // }

    // writeLegacyAutosave(testUserDataPath, 2016, 1);

    // const string_list versionParts = simgear::strutils::split(VERSION, ".");
    // CPPUNIT_ASSERT(versionParts.size() == 3);
    // const int currentMajor = simgear::strutils::to_int(versionParts[0]);
    // const int currentMinor = simgear::strutils::to_int(versionParts[1]);

    // // none of these should not be read
    // writeLegacyAutosave2(testUserDataPath, 2016, 0);
    // writeLegacyAutosave2(testUserDataPath, currentMajor, currentMinor + 1);
    // writeLegacyAutosave2(testUserDataPath, currentMajor+1, currentMinor + 1);

    // SGPath p = globals->autosaveFilePath(testUserDataPath);
    // if (p.exists()) {
    //     CPPUNIT_ASSERT(p.remove());
    // }

    // // write some blck-list rules to property tree
    // SGPropertyNode_ptr blacklist = fgGetNode("/sim/autosave-migration/blacklist", true);
    // blacklist->addChild("path")->setStringValue("/sim[0]/presets[0]/*");
    // blacklist->addChild("path")->setStringValue("/sim[0]/rendering[0]/texture-");
    // blacklist->addChild("path")->setStringValue("/views[0]/view[*]/old-prop");
    // blacklist->addChild("path")->setStringValue("/sim[0]/gui");

    // // execute method under test
    // globals->loadUserSettings(testUserDataPath);

    // CPPUNIT_ASSERT_EQUAL((int)globals->get_props()->getNode("sim")->getChildren("presets").size(), 2);
    // CPPUNIT_ASSERT_EQUAL((int)globals->get_props()->getNode("sim")->getChildren("gui").size(), 0);

    // CPPUNIT_ASSERT_EQUAL(globals->get_props()->getIntValue("sim/window-height"), 42);
    // CPPUNIT_ASSERT_EQUAL(globals->get_props()->getIntValue("sim/presets/foo"), 0);
    // CPPUNIT_ASSERT_EQUAL(globals->get_props()->getIntValue("sim/presets[1]/foo"), 13);

    // CPPUNIT_ASSERT_EQUAL(globals->get_props()->getIntValue("some-setting"), 888);

    // // if this is not zero, one of the bad autosaves was read
    // CPPUNIT_ASSERT_EQUAL(globals->get_props()->getIntValue("sim/bad"), 0);
}
