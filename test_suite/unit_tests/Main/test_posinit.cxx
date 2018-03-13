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

#include "test_posinit.hxx"

#include "tests/unitTestHelpers.hxx"

#include <simgear/props/props_io.hxx>

#include "Main/positioninit.hxx"
#include "Main/options.hxx"
#include "Main/globals.hxx"
#include "Main/fg_props.hxx"

#include "Airports/airport.hxx"

using namespace flightgear;

void PosInitTests::testDefaultStartup()
{
    fgtest::initTestGlobals("posinit");
    Options::reset();

    fgLoadProps("defaults.xml", globals->get_props());

    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath"};
        opts->init(1, (char**) args, SGPath());
        opts->processOptions();
    }

    initPosition();

    // verify we got the location specified in location-preset.xml
    // this unfortunately means manually parsing that file, oh well

    {
        SGPath presets = fgtest::fgdataPath() / "location-presets.xml";
        CPPUNIT_ASSERT(presets.exists());
        SGPropertyNode_ptr props(new SGPropertyNode);
        readProperties(presets, props);

        std::string icao = props->getStringValue("/sim/presets/airport-id");
        CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/airport/closest-airport-id") == icao);

        SGGeod pos = globals->get_aircraft_position();

        FGAirportRef defaultAirport = FGAirport::getByIdent(icao);

        double dist = SGGeodesy::distanceM(pos, defaultAirport->geod());
        CPPUNIT_ASSERT(dist < 10000);
    }
    fgtest::shutdownTestGlobals();

}

void PosInitTests::testAirportOnlyStartup()
{
    fgtest::initTestGlobals("posinit");
    Options::reset();
    fgLoadProps("defaults.xml", globals->get_props());

    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF"};
        opts->init(2, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    initPosition();

    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/airport/closest-airport-id") == std::string("EDDF"));
    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       FGAirport::getByIdent("EDDF")->geod());
    CPPUNIT_ASSERT(dist < 10000);
    fgtest::shutdownTestGlobals();

}

void PosInitTests::testAirportAndMetarStartup()
{
    fgtest::initTestGlobals("posinit");

    Options::reset();
    fgLoadProps("defaults.xml", globals->get_props());

    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);
        const char* args[] = {"dummypath", "--airport=LOWI", "--metar=XXXX 271320Z 08007KT 030V130 CAVOK 17/02 Q1020 NOSIG"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }

    initPosition();

    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/airport/closest-airport-id") == std::string("LOWI"));
    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       FGAirport::getByIdent("LOWI")->geod());
    CPPUNIT_ASSERT(dist < 10000);
    ///sim/atc/runway
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("sim/atc/runway") == std::string("26"));

    fgtest::shutdownTestGlobals();
}
