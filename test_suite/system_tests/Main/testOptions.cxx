// Written by James Turner, started 2017.
//
// Copyright (C) 2017  James Turner
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

#include "testOptions.hxx"
#include "config.h"

#include <chrono>
#include <thread>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/scene_graph.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <simgear/package/Root.hxx>
#include <simgear/props/props_io.hxx>

#include "Main/fg_props.hxx"
#include "Main/globals.hxx"
#include "Main/options.hxx"
#include <Main/fg_init.hxx>

using namespace std::string_literals;
using namespace flightgear;
using namespace simgear::pkg;

void OptionsTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("options");
    FGTestApi::setUp::initNavDataCache();
    Options::reset();
    fgLoadProps("defaults.xml", globals->get_props());
}


void OptionsTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void OptionsTests::testLoadDefaultAircraft()
{
    const auto customFGAircraftPath = SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "customAircraftDir";

    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const string fgAircraftArg = "--fg-aircraft=" + customFGAircraftPath.utf8Str();
        const char* args[] = {"dummypath", fgAircraftArg.c_str()};
        opts->init(2, (char**)args, SGPath());
        opts->processOptions();
    }

    fgInitAircraftPaths(false);
    fgInitAircraft(false, false);

    CPPUNIT_ASSERT_EQUAL("c172p"s, string{fgGetString("/sim/aircraft")});
    CPPUNIT_ASSERT_EQUAL("c172p"s, string{fgGetString("/sim/aircraft-id")});
    //CPPUNIT_ASSERT_EQUAL(adPath.utf8Str(), string{fgGetString("/sim/aircraft-dir")});
}

void OptionsTests::testOptionAircraftWithAircraftDir()
{
    const auto adPath = SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "customAircraftDir" / "overrideUfo";

    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const string aircraftDirArg = "--aircraft-dir=" + adPath.utf8Str();
        const char* args[] = {"dummypath", "--aircraft=ufo", aircraftDirArg.c_str()};
        opts->init(3, (char**)args, SGPath());
        opts->processOptions();
    }

    fgInitAircraftPaths(false);
    fgInitAircraft(false, false);

    CPPUNIT_ASSERT_EQUAL("ufo"s, string{fgGetString("/sim/aircraft")});
    CPPUNIT_ASSERT_EQUAL("ufo"s, string{fgGetString("/sim/aircraft-id")});
    CPPUNIT_ASSERT_EQUAL(adPath.utf8Str(), string{fgGetString("/sim/aircraft-dir")});
}

void OptionsTests::testOptionAircraftWithFGAircraft()
{
    const auto customFGAircraftPath = SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "customAircraftDir";

    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const string fgAircraftArg = "--fg-aircraft=" + customFGAircraftPath.utf8Str();
        const char* args[] = {"dummypath", "--aircraft=ufo", fgAircraftArg.c_str()};
        opts->init(3, (char**)args, SGPath());
        opts->processOptions();
    }

    fgInitAircraftPaths(false);
    fgInitAircraft(false, false);

    CPPUNIT_ASSERT_EQUAL("ufo"s, string{fgGetString("/sim/aircraft")});
    CPPUNIT_ASSERT_EQUAL("ufo"s, string{fgGetString("/sim/aircraft-id")});

    const auto correctDir = (customFGAircraftPath / "overrideUfo").utf8Str();
    CPPUNIT_ASSERT_EQUAL(correctDir, string{fgGetString("/sim/aircraft-dir")});
}

void OptionsTests::testOptionAircraftUnqualified()
{
    const auto packageAircraftDir = SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "dummy_package_root";
    SGSharedPtr<Root> pkgRoot(new Root(packageAircraftDir, FLIGHTGEAR_VERSION));
    globals->setPackageRoot(pkgRoot);

    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--aircraft=bob"};
        opts->init(2, (char**)args, SGPath());
        opts->processOptions();
    }

    fgInitAircraftPaths(false);
    fgInitAircraft(false, false);

    CPPUNIT_ASSERT_EQUAL("bob"s, string{fgGetString("/sim/aircraft")});
    CPPUNIT_ASSERT_EQUAL("org.fg.test.catalog1.bob"s, string{fgGetString("/sim/aircraft-id")});

    const auto correctDir = (packageAircraftDir / "org.fg.test.catalog1" / "Aircraft" / "bobCraft").utf8Str();
    CPPUNIT_ASSERT_EQUAL(correctDir, string{fgGetString("/sim/aircraft-dir")});
}

void OptionsTests::testOptionAircraftFullyQualified()
{
    const auto packageAircraftDir = SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "dummy_package_root";
    SGSharedPtr<Root> pkgRoot(new Root(packageAircraftDir, FLIGHTGEAR_VERSION));
    globals->setPackageRoot(pkgRoot);

    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--aircraft=org.fg.test.catalog1.bob"};
        opts->init(2, (char**)args, SGPath());
        opts->processOptions();
    }

    fgInitAircraftPaths(false);
    fgInitAircraft(false, false);

    CPPUNIT_ASSERT_EQUAL("bob"s, string{fgGetString("/sim/aircraft")});
    CPPUNIT_ASSERT_EQUAL("org.fg.test.catalog1.bob"s, string{fgGetString("/sim/aircraft-id")});

    const auto correctDir = (packageAircraftDir / "org.fg.test.catalog1" / "Aircraft" / "bobCraft").utf8Str();
    CPPUNIT_ASSERT_EQUAL(correctDir, string{fgGetString("/sim/aircraft-dir")});
}
