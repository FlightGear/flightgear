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

SG_USING_STD(cout);

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
	ATCmgr = globals->get_ATC_mgr();
}

FGTower::~FGTower() {
	if(!separateGround) {
		delete ground;
	}
}

void FGTower::Init() {
    display = false;
	
	// Need some way to initialise rwyOccupied flag correctly if the user is on the runway and to know its the user.
	// I'll punt the startup issue for now though!!!
	rwyOccupied = false;
	
	// Setup the ground control at this airport
	AirportATC a;
	if(ATCmgr->GetAirportATCDetails(ident, &a)) {
		if(a.ground_freq) {		// Ground control
			ground = (FGGround*)ATCmgr->GetATCPointer(ident, GROUND);
			separateGround = true;
			if(ground == NULL) {
				// Something has gone wrong :-(
				cout << "ERROR - ground has frequency but can't get ground pointer :-(\n";
				ground = new FGGround(ident);
				separateGround = false;
				ground->Init();
				if(display) {
					ground->SetDisplay();
				} else {
					ground->SetNoDisplay();
				}
			}
		} else {
			// Initialise ground anyway to do the shortest path stuff!
			// Note that we're now responsible for updating and deleting this - NOT the ATCMgr.
			ground = new FGGround(ident);
			separateGround = false;
			ground->Init();
			if(display) {
				ground->SetDisplay();
			} else {
				ground->SetNoDisplay();
			}
		}
	} else {
		cout << "Unable to find airport details for " << ident << " in FGTower::Init()\n";
		// Initialise ground anyway to avoid segfault later
		ground = new FGGround(ident);
		separateGround = false;
		ground->Init();
		if(display) {
			ground->SetDisplay();
		} else {
			ground->SetNoDisplay();
		}
	}
}

void FGTower::Update() {
    // Each time step, what do we need to do?
    // We need to go through the list of outstanding requests and acknowedgements
    // and process at least one of them.
    // We need to go through the list of planes under our control and check if
    // any need to be addressed.
    // We need to check for planes not under our control coming within our 
    // control area and address if necessary.

	// TODO - a lot of the below probably doesn't need to be called every frame and should be staggered.
	
	// Sort the arriving planes

	// Calculate the eta of each plane to the threshold.
	// For ground traffic this is the fastest they can get there.
	// For air traffic this is the middle approximation.
	doThresholdETACalc();
	
	// Order the list of traffic as per expected threshold use and flag any conflicts
	bool conflicts = doThresholdUseOrder();
	
	// sortConficts() !!!
	
	doCommunication();
	
	if(!separateGround) {
		// The display stuff might have to get more clever than this when not separate 
		// since the tower and ground might try communicating simultaneously even though
		// they're mean't to be the same contoller/frequency!!
		if(display) {
			ground->SetDisplay();
		} else {
			ground->SetNoDisplay();
		}
		ground->Update();
	}
}

// Calculate the eta of each plane to the threshold.
// For ground traffic this is the fastest they can get there.
// For air traffic this is the middle approximation.
void FGTower::doThresholdETACalc() {
	// For now we'll be very crude and hardwire expected speeds to C172-like values
	double app_ias = 100.0;			// Speed during straight-in approach
	double circuit_ias = 80.0;		// Speed around circuit
	double final_ias = 70.0;		// Speed during final approach

	tower_plane_rec_list_iterator twrItr;

	for(twrItr = trafficList.begin(); twrItr != trafficList.end(); twrItr++) {
	}
	
}

bool FGTower::doThresholdUseOrder() {
	return(true);
}

void FGTower::doCommunication() {
}
		

void FGTower::RequestLandingClearance(string ID) {
	cout << "Request Landing Clearance called...\n";
}
void FGTower::RequestDepartureClearance(string ID) {
	cout << "Request Departure Clearance called...\n";
}	
//void FGTower::ReportFinal(string ID);
//void FGTower::ReportLongFinal(string ID);
//void FGTower::ReportOuterMarker(string ID);
//void FGTower::ReportMiddleMarker(string ID);
//void FGTower::ReportInnerMarker(string ID);
//void FGTower::ReportGoingAround(string ID);
void FGTower::ReportRunwayVacated(string ID) {
	cout << "Report Runway Vacated Called...\n";
}
