// FGTower - a class to provide tower control at towered airports.
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <Main/globals.hxx>

#include "tower.hxx"
#include "ATCdisplay.hxx"
#include "ATCmgr.hxx"

// TowerPlaneRec

TowerPlaneRec::TowerPlaneRec() :
id("UNKNOWN"),
clearedToLand(false),
clearedToDepart(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false)
{
}

TowerPlaneRec::TowerPlaneRec(string ID) :
clearedToLand(false),
clearedToDepart(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false)
{
	id = ID;
}

TowerPlaneRec::TowerPlaneRec(Point3D pt) :
id("UNKNOWN"),
clearedToLand(false),
clearedToDepart(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false)
{
	pos = pt;
}

TowerPlaneRec::TowerPlaneRec(string ID, Point3D pt) :
clearedToLand(false),
clearedToDepart(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false)
{
	id = ID;
	pos = pt;
}


// FGTower

FGTower::FGTower() {
}

FGTower::~FGTower() {
}

void FGTower::Init() {
    display = false;
}

void FGTower::Update() {
    // Each time step, what do we need to do?
    // We need to go through the list of outstanding requests and acknowedgements
    // and process at least one of them.
    // We need to go through the list of planes under our control and check if
    // any need to be addressed.
    // We need to check for planes not under our control coming within our 
    // control area and address if necessary.

    // Hardwired for testing
    static int play = 0;
    if(play == 200) {
	//cout << "Registering message in tower.cxx ****************************\n";
	//globals->get_ATC_display()->RegisterSingleMessage((string)"Cessna eight-two-zero Cleared for takeoff", 2);
    }
    ++play;
}
