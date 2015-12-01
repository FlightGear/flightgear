
#include "config.h"

#include <iostream>

#include <Navaids/NavDataCache.hxx>
#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/airportdynamicsmanager.hxx>
#include <Airports/groundnetwork.hxx>

#include <Main/globals.hxx>
#include <Main/teststubs.hxx>

using namespace flightgear;

int main(int argc, char* argv[])
{

    test::initHomeAndData(argc, argv);

    globals->add_new_subsystem<AirportDynamicsManager>();

    test::bindAndInitSubsystems();

  FGAirportRef apt = FGAirport::findByIdent("KSFO");
    if (!apt) {
        std::cerr << "failed to find airport KSFO" << std::endl;
        return -1;
    }
  FGAirportDynamics* dyn = apt->getDynamics();
    if (!dyn) {
        std::cerr << "failed to find dynamics for KSFO" << std::endl;
        return -1;
    }

  std::string fltType;
  std::string acOperator;
  std::string acType; // Currently not used by findAvailable parking, so safe to leave empty.
  ParkingAssignment park1 = dyn->getAvailableParking(10.0, fltType, acOperator, acType);

  if (park1.isValid()) {
    std::cout << park1.parking()->name() << std::endl;
  } else {
      std::cerr << "no valid parking found" << std::endl;
  }

  std::cout << "ground net test complete" << std::endl;

  return 0;
}
