/*
 * Copyright (C) 2024 Keith Paterson
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

#include "test_ATCController.hxx"

#include "config.h"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include "ATC/atc_mgr.hxx"
#include "ATC/ATCController.hxx"
#include "ATC/GroundController.hxx"
#include <AIModel/AIManager.hxx>
#include <AIModel/performancedb.hxx>
#include <Airports/airport.hxx>
#include <Airports/airportdynamicsmanager.hxx>
#include <Airports/dynamics.hxx>

#include <Main/globals.hxx>

#include <string>
#include <memory>

using std::string;

/////////////////////////////////////////////////////////////////////////////

class TestATCController: public FGATCController {
    public:
        string getTransponderCode(const string& fltRules) {
           return genTransponderCode(fltRules);
        };

  int getFrequency() {return 1;};
  void announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute,
                                  double lat, double lon,
                                  double hdg, double spd, double alt, double radius, int leg,
                                  FGAIAircraft *aircraft) {};
  void             updateAircraftInformation(int id, SGGeod geod,
            double heading, double speed, double alt, double dt) {};
  void render(bool) {};
  std::string getName() { return "test";};
  void update(double) {};
};

// Set up function for each test.
void ATCControllerTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("ATCControllerTests");
    FGAirport::clearAirportsCache();

    globals->get_subsystem_mgr()->add<PerformanceDB>();
    globals->get_subsystem_mgr()->add<FGATCManager>();
    globals->get_subsystem_mgr()->add<FGAIManager>();
    globals->get_subsystem_mgr()->add<flightgear::AirportDynamicsManager>();

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();

}

// Clean up after each test.
void ATCControllerTests::tearDown()
{
}

void ATCControllerTests::testTransponder()
{
    /*
    FGAirportRef egph = FGAirport::getByIdent("EGPH");

    egph->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "EGPH.groundnet.xml");
    FGGroundController testObject(egph->getDynamics());
    */
    TestATCController testController;
    const string val1 = testController.getTransponderCode("FLT");
    const string val2 = testController.getTransponderCode("FLT");
    CPPUNIT_ASSERT(val1.compare(val2));
}

