/******************************************************************************
 * atc_mgr.cxx
 * Written by Durk Talsma, started August 1, 2010.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <iostream>

#include <simgear/math/SGMath.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/simple.hxx>

#include "atc_mgr.hxx"


FGATCManager::FGATCManager() {

}

FGATCManager::~FGATCManager() {

}

void FGATCManager::init() {
    SGSubsystem::init();
    currentATCDialog = new FGATCDialogNew;
    currentATCDialog->init();

    // find a reasonable controller for our user's aircraft..
    // Let's start by working out the following three scenarios: 
    // Starting on ground at a parking position
    // Starting on ground at the runway.
    // Starting in the Air
    bool onGround  = fgGetBool("/sim/presets/onground");
    string runway  = fgGetString("/sim/atc/runway");
    string airport = fgGetString("/sim/presets/airport-id");
    string parking = fgGetString("/sim/presets/parkpos");

    FGAirport *apt = FGAirport::findByIdent(airport); 
    cerr << "found information: " << runway << " " << airport << ": parking = " << parking << endl;
     if (onGround) {
        if (parking.empty()) {
            controller = apt->getDynamics()->getTowerController();
            int stationFreq = apt->getDynamics()->getTowerFrequency(2);
            cerr << "Setting radio frequency to in airfrequency: " << stationFreq << endl;
            fgSetDouble("/instrumentation/comm[0]/frequencies/selected-mhz", ((double) stationFreq / 100.0));

        } else {
            controller = apt->getDynamics()->getGroundNetwork();
            int stationFreq = apt->getDynamics()->getGroundFrequency(2);
            cerr << "Setting radio frequency to : " << stationFreq << endl;
            fgSetDouble("/instrumentation/comm[0]/frequencies/selected-mhz", ((double) stationFreq / 100.0));

        }
     } else {
        controller = 0;
     }
    //controller = 

    //dialog.init();
}

void FGATCManager::addController(FGATCController *controller) {
    activeStations.push_back(controller);
}

void FGATCManager::update ( double time ) {
    //cerr << "ATC update code is running at time: " << time << endl;
   currentATCDialog->update(time);
}
