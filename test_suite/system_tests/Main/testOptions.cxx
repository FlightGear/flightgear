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

#include <simgear/package/Root.hxx>
#include <simgear/props/props_io.hxx>

#include "Main/fg_props.hxx"
#include "Main/globals.hxx"
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
        const string fgAircraftArg = "--fg-aircraft=" + customFGAircraftPath.utf8Str();
        const char* args[] = {"dummypath", fgAircraftArg.c_str(), nullptr};
        runProcessOptions(args);
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
        const string aircraftDirArg = "--aircraft-dir=" + adPath.utf8Str();
        const char* args[] = {"dummypath", "--aircraft=ufo", aircraftDirArg.c_str(), nullptr};
        runProcessOptions(args);
    }

    fgInitAircraftPaths(false);
    fgInitAircraft(false, false);

    CPPUNIT_ASSERT_EQUAL("ufo"s, string{fgGetString("/sim/aircraft")});
    CPPUNIT_ASSERT_EQUAL("ufo"s, string{fgGetString("/sim/aircraft-id")});
    CPPUNIT_ASSERT_EQUAL(adPath.realpath().utf8Str(), string{fgGetString("/sim/aircraft-dir")});
}

void OptionsTests::testOptionAircraftWithFGAircraft()
{
    const auto customFGAircraftPath = SGPath::fromUtf8(FG_TEST_SUITE_DATA).realpath() / "customAircraftDir";

    {
        const string fgAircraftArg = "--fg-aircraft=" + customFGAircraftPath.utf8Str();
        const char* args[] = {"dummypath", "--aircraft=ufo", fgAircraftArg.c_str(), nullptr};
        runProcessOptions(args);
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
        const char* args[] = {"dummypath", "--aircraft=bob", nullptr};
        runProcessOptions(args);
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
        const char* args[] = {"dummypath", "--aircraft=org.fg.test.catalog1.bob", nullptr};
        runProcessOptions(args);
    }

    fgInitAircraftPaths(false);
    fgInitAircraft(false, false);

    CPPUNIT_ASSERT_EQUAL("bob"s, string{fgGetString("/sim/aircraft")});
    CPPUNIT_ASSERT_EQUAL("org.fg.test.catalog1.bob"s, string{fgGetString("/sim/aircraft-id")});

    const auto correctDir = (packageAircraftDir / "org.fg.test.catalog1" / "Aircraft" / "bobCraft").utf8Str();
    CPPUNIT_ASSERT_EQUAL(correctDir, string{fgGetString("/sim/aircraft-dir")});
}

Options* OptionsTests::runProcessOptions(const char* argv[])
{
    int argc = getElementsNumber(argv);

    Options* opts = Options::sharedInstance();
    opts->setShouldLoadDefaultConfig(false);

    opts->init(argc, (char**) argv, SGPath());
    opts->processOptions();

    return opts;
}

int OptionsTests::getElementsNumber(const char* argv[])
{
    int counter = 0;

    while (argv[counter] != nullptr) {
        counter++;
    }

    return counter;
}

void OptionsTests::testDisableSound()
{
    const char* args[] = {"dummypath", "--disable-sound", nullptr};
    runProcessOptions( args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/sound/working"));
}

void OptionsTests::testDisableSoundByFalseWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--sound", "false", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/sound/working"));
}

void OptionsTests::testDisableSoundBy0WithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--sound", "0", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/sound/working"));
}

void OptionsTests::testDisableSoundByNoWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--sound", "no", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/sound/working"));
}

void OptionsTests::testDisableSoundByFalseWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--sound=false", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/sound/working"));
}

void OptionsTests::testDisableSoundBy0WithEqualSeparator()
{
    const char* args[] = {"dummypath", "--sound=0", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/sound/working"));
}

void OptionsTests::testDisableSoundByNoWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--sound=no", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSound()
{
    const char* args[] = {"dummypath", "--enable-sound", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSoundWithoutValue()
{
    const char* args[] = {"dummypath", "--sound", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSoundWithIncorrectValue()
{
    const char* args[] = {"dummypath", "--sound", "dummytext", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSoundByTrueWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--sound", "true", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSoundBy1WithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--sound", "1", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSoundByYesWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--sound", "yes", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSoundWithoutValueWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--sound=", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSoundWithIncorrectValueWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--sound=dummytext", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSoundByTrueWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--sound=true", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSoundBy1WithEqualSeparator()
{
    const char* args[] = {"dummypath", "--sound=1", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testEnableSoundByYesWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--sound=yes", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
}

void OptionsTests::testDisableFreeze()
{
    const char* args[] = {"dummypath", "--disable-freeze", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testDisableFreezeByFalseWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--freeze", "false", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testDisableFreezeBy0WithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--freeze", "0", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testDisableFreezeByNoWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--freeze", "no", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testDisableFreezeByFalseWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--freeze=false", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testDisableFreezeBy0WithEqualSeparator()
{
    const char* args[] = {"dummypath", "--freeze=0", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testDisableFreezeByNoWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--freeze=no", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(!fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreeze()
{
    const char* args[] = {"dummypath", "--enable-freeze", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreezeWithoutValue()
{
    const char* args[] = {"dummypath", "--freeze", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreezeWithIncorrectValue()
{
    const char* args[] = {"dummypath", "--freeze", "dummytext", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreezeByTrueWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--freeze", "true", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreezeBy1WithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--freeze", "1", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreezeByYesWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--freeze", "yes", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreezeWithoutValueWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--freeze=", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreezeWithIncorrectValueWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--freeze=dummytext", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreezeByTrueWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--freeze=true", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreezeBy1WithEqualSeparator()
{
    const char* args[] = {"dummypath", "--freeze=1", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testEnableFreezeByYesWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--freeze=yes", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
}

void OptionsTests::testPropWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--prop:string:/sim/multiplay/chat=Greetings pilots", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT_EQUAL("Greetings pilots"s, string{fgGetString("/sim/multiplay/chat")});
}

void OptionsTests::testPropWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--prop:string:/sim/multiplay/chat", "Morning pilots", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT_EQUAL("Morning pilots"s, string{fgGetString("/sim/multiplay/chat")});
}

void OptionsTests::testMetarWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--metar=XXXX 012345Z 00000KT 0800 FG NCD 08/08 A3030", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT_EQUAL("XXXX 012345Z 00000KT 0800 FG NCD 08/08 A3030"s, string{fgGetString("/environment/metar/data")});
}

void OptionsTests::testMetarWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--metar", "XXXX 012345Z 00000KT 0800 FG NCD 08/08 A3030", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT_EQUAL("XXXX 012345Z 00000KT 0800 FG NCD 08/08 A3030"s, string{fgGetString("/environment/metar/data")});
}

void OptionsTests::testXmlFileBetweenOptions()
{
    const char* args[] = {"dummypath", "--freeze", "--sound=true", "--ai-traffic", "file.xml", "--on-ground", "0", nullptr};
    runProcessOptions(args);

    CPPUNIT_ASSERT(fgGetBool("/sim/freeze/master"));
    CPPUNIT_ASSERT(fgGetBool("/sim/sound/working"));
    CPPUNIT_ASSERT(fgGetBool("/sim/traffic-manager/enabled"));
    CPPUNIT_ASSERT(!fgGetBool("/sim/presets/onground"));
}

void OptionsTests::testGetArgValueWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--language", "pl", nullptr};
    int argc = getElementsNumber(args);

    const std::string lang = Options::getArgValue(argc, (char**) args, "--language");

    CPPUNIT_ASSERT_EQUAL("pl"s, lang);
}

void OptionsTests::testGetArgValueWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--language=es", nullptr};
    int argc = getElementsNumber(args);

    const std::string lang = Options::getArgValue(argc, (char**) args, "--language");

    CPPUNIT_ASSERT_EQUAL("es"s, lang);
}

void OptionsTests::testCheckForArgEnable()
{
    const char* args[] = {"dummypath", "--launcher", nullptr};
    int argc = getElementsNumber(args);

    bool result = Options::checkForArgEnable(argc, (char**) args, "launcher");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testCheckForArgEnableByTrueWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--launcher", "true", nullptr};
    int argc = getElementsNumber(args);

    bool result = Options::checkForArgEnable(argc, (char**) args, "launcher");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testCheckForArgEnableByTrueWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--launcher=true", nullptr};
    int argc = getElementsNumber(args);

    bool result = Options::checkForArgEnable(argc, (char**) args, "launcher");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testCheckForArgEnableMissingOption()
{
    const char* args[] = {"dummypath", "--freeze", "1", nullptr};
    int argc = getElementsNumber(args);

    bool result = Options::checkForArgEnable(argc, (char**) args, "launcher");

    CPPUNIT_ASSERT(!result);
}

void OptionsTests::testCheckForArgDisableByFalseWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--launcher", "false", nullptr};
    int argc = getElementsNumber(args);

    bool result = Options::checkForArgDisable(argc, (char**) args, "launcher");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testCheckForArgDisableByFalseWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--freeze", "1", "--launcher=false", nullptr};
    int argc = getElementsNumber(args);

    bool result = Options::checkForArgDisable(argc, (char**) args, "launcher");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testCheckForArgDisableMissingOption()
{
    const char* args[] = {"dummypath", "--freeze", "1", nullptr};
    int argc = getElementsNumber(args);

    bool result = Options::checkForArgDisable(argc, (char**) args, "launcher");

    CPPUNIT_ASSERT(!result);
}

void OptionsTests::testIsBoolOptionEnable()
{
    const char* args[] = {"dummypath", "--enable-fullscreen", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableWithoutValue()
{
    const char* args[] = {"dummypath", "--fullscreen", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableWithIncorrectValue()
{
    const char* args[] = {"dummypath", "--fullscreen", "dummytext", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableByTrueWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen", "true", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableBy1WithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen", "1", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableByYesWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen", "yes", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableWithoutValueWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen=", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableWithIncorrectValueWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen=dummytext", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableByTrueWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen=true", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableBy1WithEqualSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen=1", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableByYesWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen=yes", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionEnableMissingOption()
{
    const char* args[] = {"dummypath", "--sound", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionEnable("fullscreen");

    CPPUNIT_ASSERT(!result);
}

void OptionsTests::testIsBoolOptionDisable()
{
    const char* args[] = {"dummypath", "--disable-fullscreen", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionDisable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionDisableByFalseWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen", "false", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionDisable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionDisableBy0WithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen", "0", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionDisable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionDisableByNoWithSpaceSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen", "no", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionDisable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionDisableByFalseWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen=false", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionDisable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionDisableBy0WithEqualSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen=0", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionDisable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionDisableByNoWithEqualSeparator()
{
    const char* args[] = {"dummypath", "--fullscreen=no", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionDisable("fullscreen");

    CPPUNIT_ASSERT(result);
}

void OptionsTests::testIsBoolOptionDisableMissingOption()
{
    const char* args[] = {"dummypath", "--sound", nullptr};
    Options* opts = runProcessOptions(args);
    bool result = opts->isBoolOptionDisable("fullscreen");

    CPPUNIT_ASSERT(!result);
}
