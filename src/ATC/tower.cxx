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
#include <Airports/runways.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>

#include "tower.hxx"
#include "ATCdisplay.hxx"
#include "ATCmgr.hxx"
#include "ATCutils.hxx"
#include "commlist.hxx"
#include "AILocalTraffic.hxx"

SG_USING_STD(cout);

// TowerPlaneRec

TowerPlaneRec::TowerPlaneRec() :
clearedToLand(false),
clearedToLineUp(false),
clearedToTakeOff(false),
holdShortReported(false),
downwindReported(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false),
instructedToGoAround(false),
onRwy(false),
nextOnRwy(false),
opType(TTT_UNKNOWN),
leg(LEG_UNKNOWN),
landingType(AIP_LT_UNKNOWN),
isUser(false)
{
	plane.callsign = "UNKNOWN";
}

TowerPlaneRec::TowerPlaneRec(PlaneRec p) :
clearedToLand(false),
clearedToLineUp(false),
clearedToTakeOff(false),
holdShortReported(false),
downwindReported(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false),
instructedToGoAround(false),
onRwy(false),
nextOnRwy(false),
opType(TTT_UNKNOWN),
leg(LEG_UNKNOWN),
landingType(AIP_LT_UNKNOWN),
isUser(false)
{
	plane = p;
}

TowerPlaneRec::TowerPlaneRec(Point3D pt) :
clearedToLand(false),
clearedToLineUp(false),
clearedToTakeOff(false),
holdShortReported(false),
downwindReported(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false),
instructedToGoAround(false),
onRwy(false),
nextOnRwy(false),
opType(TTT_UNKNOWN),
leg(LEG_UNKNOWN),
landingType(AIP_LT_UNKNOWN),
isUser(false)
{
	plane.callsign = "UNKNOWN";
	pos = pt;
}

TowerPlaneRec::TowerPlaneRec(PlaneRec p, Point3D pt) :
clearedToLand(false),
clearedToLineUp(false),
clearedToTakeOff(false),
holdShortReported(false),
downwindReported(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false),
instructedToGoAround(false),
onRwy(false),
nextOnRwy(false),
opType(TTT_UNKNOWN),
leg(LEG_UNKNOWN),
landingType(AIP_LT_UNKNOWN),
isUser(false)
{
	plane = p;
	pos = pt;
}


// FGTower

/*******************************************
               TODO List
			   
Currently user is assumed to have taken off again when leaving the runway - check speed/elev for taxiing-in.

AI plane lands even when user on rwy - make it go-around instead.

Tell AI plane to contact ground when taxiing in.

Use track instead of heading to determine what leg of the circuit the user is flying.

Use altitude as well as position to try to determine if the user has left the circuit.

Currently HoldShortReported code assumes there will be only one plane holding for the runway at once and 
will break when planes start queueing.

Implement ReportRunwayVacated
*******************************************/

FGTower::FGTower() {
	ATCmgr = globals->get_ATC_mgr();
	
	// Init the property nodes - TODO - need to make sure we're getting surface winds.
	wind_from_hdg = fgGetNode("/environment/wind-from-heading-deg", true);
	wind_speed_knots = fgGetNode("/environment/wind-speed-kt", true);
	
	update_count = 0;
	update_count_max = 15;
	
	holdListItr = holdList.begin();
	appListItr = appList.begin();
	depListItr = depList.begin();
	rwyListItr = rwyList.begin();
	circuitListItr = circuitList.begin();
	trafficListItr = trafficList.begin();
	
	freqClear = true;
	
	timeSinceLastDeparture = 9999;
	departed = false;
}

FGTower::~FGTower() {
	if(!separateGround) {
		delete ground;
	}
}

void FGTower::Init() {
    display = false;
	
	// Pointers to user's position
	user_lon_node = fgGetNode("/position/longitude-deg", true);
	user_lat_node = fgGetNode("/position/latitude-deg", true);
	user_elev_node = fgGetNode("/position/altitude-ft", true);
	user_hdg_node = fgGetNode("/orientation/heading-deg", true);
	
	// Need some way to initialise rwyOccupied flag correctly if the user is on the runway and to know its the user.
	// I'll punt the startup issue for now though!!!
	rwyOccupied = false;
	
	// Setup the ground control at this airport
	AirportATC a;
	//cout << "Tower ident = " << ident << '\n';
	if(ATCmgr->GetAirportATCDetails(ident, &a)) {
		if(a.ground_freq) {		// Ground control
			ground = (FGGround*)ATCmgr->GetATCPointer(ident, GROUND);
			separateGround = true;
			if(ground == NULL) {
				// Something has gone wrong :-(
				SG_LOG(SG_ATC, SG_WARN, "ERROR - ground has frequency but can't get ground pointer :-(");
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
		SG_LOG(SG_ATC, SG_ALERT, "Unable to find airport details for " << ident << " in FGTower::Init()");
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
	
	// Get the airport elevation
	aptElev = dclGetAirportElev(ident.c_str()) * SG_FEET_TO_METER;
	
	DoRwyDetails();
	
	// FIXME - this currently assumes use of the active rwy by the user.
	rwyOccupied = OnAnyRunway(Point3D(user_lon_node->getDoubleValue(), user_lat_node->getDoubleValue(), 0.0));
	if(rwyOccupied) {
		// Assume the user is started at the threshold ready to take-off
		TowerPlaneRec* t = new TowerPlaneRec;
		t->plane.callsign = "Charlie Foxtrot Sierra";	// C-FGFS !!! - fixme - this is a bit hardwired
		t->plane.type = GA_SINGLE;
		t->opType = TTT_UNKNOWN;	// We don't know if the user wants to do circuits or a departure...
		t->landingType = AIP_LT_UNKNOWN;
		t->leg = TAKEOFF_ROLL;
		t->isUser = true;
		t->planePtr = NULL;
		t->clearedToTakeOff = true;
		rwyList.push_back(t);
		departed = false;
	}
}

void FGTower::Update(double dt) {
	//cout << "T" << endl;
	// Each time step, what do we need to do?
	// We need to go through the list of outstanding requests and acknowedgements
	// and process at least one of them.
	// We need to go through the list of planes under our control and check if
	// any need to be addressed.
	// We need to check for planes not under our control coming within our 
	// control area and address if necessary.
	
	// TODO - a lot of the below probably doesn't need to be called every frame and should be staggered.
	
	// Sort the arriving planes
	
	/*
	if(ident == "KEMT") {
		cout << update_count << "\ttL: " << trafficList.size() << "  cL: " << circuitList.size() << "  hL: " << holdList.size() << "  aL: " << appList.size() << '\n';
	}
	*/
	
	if(departed != false) {
		timeSinceLastDeparture += dt;
		//if(ident == "KEMT") 
		//	cout << "  dt = " << dt << "  timeSinceLastDeparture = " << timeSinceLastDeparture << '\n';
	}
	
	//cout << ident << " respond = " << respond << " responseReqd = " << responseReqd << '\n'; 
	if(respond) {
		if(!responseReqd) SG_LOG(SG_ATC, SG_ALERT, "ERROR - respond is true and responseReqd is false in FGTower::Update(...)");
		Respond();
		respond = false;
		responseReqd = false;
	}
	
	// Calculate the eta of each plane to the threshold.
	// For ground traffic this is the fastest they can get there.
	// For air traffic this is the middle approximation.
	if(update_count == 1) {
		doThresholdETACalc();
	}
	
	// Order the list of traffic as per expected threshold use and flag any conflicts
	if(update_count == 2) {
		//bool conflicts = doThresholdUseOrder();
		doThresholdUseOrder();
	}
	
	// sortConficts() !!!
	
	if(update_count == 4) {
		CheckHoldList(dt);
	}
	
	// Uggh - HACK - why have we got rwyOccupied - wouldn't simply testing rwyList.size() do?
	if(rwyList.size()) {
		rwyOccupied = true;
	} else {
		rwyOccupied = false;
	}
	
	if(update_count == 5 && rwyOccupied) {
		CheckRunwayList(dt);
	}
		
	if(update_count == 6) {
		CheckCircuitList(dt);
	}
	
	if(update_count == 7) {
		CheckApproachList(dt);
	}
	
	// TODO - do one plane from the departure list and set departed = false when out of consideration
	
	//doCommunication();
	
	if(!separateGround) {
		// The display stuff might have to get more clever than this when not separate 
		// since the tower and ground might try communicating simultaneously even though
		// they're mean't to be the same contoller/frequency!!
		if(display) {
			ground->SetDisplay();
		} else {
			ground->SetNoDisplay();
		}
		ground->Update(dt);
	}
	
	++update_count;
	// How big should ii get - ie how long should the update cycle interval stretch?
	if(update_count >= update_count_max) {
		update_count = 0;
	}
	
	// Call the base class update for the response time handling.
	FGATC::Update(dt);

	if(ident == "KEMT") {	
		// For AI debugging convienience - may be removed
		Point3D user_pos;
		user_pos.setlon(user_lon_node->getDoubleValue());
		user_pos.setlat(user_lat_node->getDoubleValue());
		user_pos.setelev(user_elev_node->getDoubleValue());
		Point3D user_ortho_pos = ortho.ConvertToLocal(user_pos);
		fgSetDouble("/AI/user/ortho-x", user_ortho_pos.x());
		fgSetDouble("/AI/user/ortho-y", user_ortho_pos.y());
		fgSetDouble("/AI/user/elev", user_elev_node->getDoubleValue());
	}
	
	//cout << "Done T" << endl;
}

void FGTower::Respond() {
	//cout << "Entering Respond..." << endl;
	TowerPlaneRec* t = FindPlane(responseID);
	if(t) {
		// This will grow!!!
		if(t->downwindReported) {
			t->downwindReported = false;
			int i = 1;
			for(tower_plane_rec_list_iterator twrItr = circuitList.begin(); twrItr != circuitList.end(); twrItr++) {
				if((*twrItr)->plane.callsign == responseID) break;
				++i;
			}			
			string trns = "Number ";
			trns += ConvertNumToSpokenDigits(i);
			trns += " ";
			trns += t->plane.callsign;
			if(display) {
				globals->get_ATC_display()->RegisterSingleMessage(trns, 0);
			}
			if(t->isUser && t->opType == TTT_UNKNOWN) {
				t->opType = CIRCUIT;
			}
		} else if(t->holdShortReported) {
			if(t->nextOnRwy) {
				if(rwyOccupied) {	// TODO - ought to add a sanity check that it isn't this plane only on the runway (even though it shouldn't be!!)
					// Do nothing for now - consider acknowloging hold short eventually
				} else {
					ClearHoldingPlane(t);
					t->leg = TAKEOFF_ROLL;
					rwyList.push_back(t);
					rwyOccupied = true;
					// WARNING - WE ARE ASSUMING ONLY ONE PLANE REPORTING HOLD AT A TIME BELOW
					// FIXME TODO - FIX THIS!!!
					if(holdList.size()) {
						if(holdListItr == holdList.end()) {
							holdListItr = holdList.begin();
						}
						holdList.erase(holdListItr);
						holdListItr = holdList.begin();
					}
				}
			} else {
				// Tell him to hold and what position he is.
				// Not currently sure under which circumstances we do or don't bother transmitting this.
				string trns = t->plane.callsign;
				trns += " hold position";
				if(display) {
					globals->get_ATC_display()->RegisterSingleMessage(trns, 0);
				}
				// TODO - add some idea of what traffic is blocking him.
			}
			t->holdShortReported = false;
		} else if(t->finalReported && !(t->finalAcknowledged)) {
			bool disp = true;
			string trns = t->plane.callsign;
			if(t->nextOnRwy && !rwyOccupied) {
				if(t->landingType == FULL_STOP) {
					trns += " cleared to land ";
				} else {
					trns += " cleared for the option ";
				}
				// TODO - add winds
				t->clearedToLand = true;
			} else if(t->eta < 20) {
				// Do nothing - we'll be telling it to go around in less than 10 seconds if the
				// runway doesn't clear so no point in calling "continue approach".
				disp = false;
			} else {
				trns += " continue approach";
				t->clearedToLand = false;
			}
			if(display && disp) {
				globals->get_ATC_display()->RegisterSingleMessage(trns, 0);
			}
			t->finalAcknowledged = true;
		}
	}
	freqClear = true;	// FIXME - set this to come true after enough time to render the message
	//cout << "Done Respond" << endl;
}

// Currently this assumes we *are* next on the runway and doesn't check for planes about to land - 
// this should be done prior to calling this function.
void FGTower::ClearHoldingPlane(TowerPlaneRec* t) {
	//cout << "Entering ClearHoldingPlane..." << endl;
	// Lets Roll !!!!
	string trns = t->plane.callsign;
	//if(departed plane < some threshold in time away) {
	if(0) {		// FIXME
	//if(timeSinceLastDeparture <= 60.0 && departed == true) {
		trns += " line up";
		t->clearedToLineUp = true;
		t->planePtr->RegisterTransmission(3);	// cleared to line-up
	//} else if(arriving plane < some threshold away) {
	} else if(GetTrafficETA(2) < 150.0 && (timeSinceLastDeparture > 60.0 || departed == false)) {	// Hack - hardwired time
		trns += " cleared immediate take-off";
		if(trafficList.size()) {
			tower_plane_rec_list_iterator trfcItr = trafficList.begin();
			trfcItr++;	// At the moment the holding plane should be first in trafficList.
			// Note though that this will break if holding planes aren't put in trafficList in the future.
			TowerPlaneRec* trfc = *trfcItr;
			trns += "... traffic is";
			switch(trfc->plane.type) {
			case UNKNOWN:
				break;
			case GA_SINGLE:
				trns += " a Cessna";	// TODO - add ability to specify actual plane type somewhere
				break;
			case GA_HP_SINGLE:
				trns += " a Piper";
				break;
			case GA_TWIN:
				trns += " a King-air";
				break;
			case GA_JET:
				trns += " a Learjet";
				break;
			case MEDIUM:
				trns += " a Regional";
				break;
			case HEAVY:
				trns += " a Heavy";
				break;
			case MIL_JET:
				trns += " Military";
				break;
			}
			//if(trfc->opType == STRAIGHT_IN || trfc->opType == TTT_UNKNOWN) {
			if(trfc->opType == STRAIGHT_IN) {
				double miles_out = CalcDistOutMiles(trfc);
				if(miles_out < 2) {
					trns += " on final";
				} else {
					trns += " on ";
					trns += ConvertNumToSpokenDigits((int)miles_out);
					trns += " mile final";
				}
			} else if(trfc->opType == CIRCUIT) {
				//cout << "Getting leg of " << trfc->plane.callsign << '\n';
				switch(trfc->leg) {
				case FINAL:
					trns += " on final";
					break;
				case TURN4:
					trns += " turning final";
					break;
				case BASE:
					trns += " on base";
					break;
				case TURN3:
					trns += " turning base";
					break;
				case DOWNWIND:
					trns += " in circuit";	// At the moment the user plane is generally flagged as unknown opType when downwind incase its a downwind departure which means we won't get here.
					break;
				// And to eliminate compiler warnings...
				case TAKEOFF_ROLL: break;
				case CLIMBOUT:     break;
				case TURN1:        break;
				case CROSSWIND:    break;
				case TURN2:        break;
				case LANDING_ROLL: break;
				case LEG_UNKNOWN:  break;
				}
			}
		} else {
			// By definition there should be some arriving traffic if we're cleared for immediate takeoff
			SG_LOG(SG_ATC, SG_WARN, "Warning: Departing traffic cleared for *immediate* take-off despite no arriving traffic in FGTower");
		}
		t->clearedToTakeOff = true;
		t->planePtr->RegisterTransmission(4);	// cleared to take-off - TODO differentiate between immediate and normal take-off
		departed = false;
		timeSinceLastDeparture = 0.0;
	} else {
	//} else if(timeSinceLastDeparture > 60.0 || departed == false) {	// Hack - test for timeSinceLastDeparture should be in lineup block eventually
		trns += " cleared for take-off";
		// TODO - add traffic is... ?
		t->clearedToTakeOff = true;
		t->planePtr->RegisterTransmission(4);	// cleared to take-off
		departed = false;
		timeSinceLastDeparture = 0.0;
	}
	if(display) {
		globals->get_ATC_display()->RegisterSingleMessage(trns, 0);
	}
	//cout << "Done ClearHoldingPlane " << endl;
}

// Do one plane from the hold list
void FGTower::CheckHoldList(double dt) {
	//cout << "Entering CheckHoldList..." << endl;
	if(holdList.size()) {
		//cout << "*holdListItr = " << *holdListItr << endl;
		if(holdListItr == holdList.end()) {
			holdListItr = holdList.begin();
		}
		//cout << "*holdListItr = " << *holdListItr << endl;
		//Process(*holdListItr);
		TowerPlaneRec* t = *holdListItr;
		//cout << "t = " << t << endl;
		if(t->holdShortReported) {
			// NO-OP - leave it to the response handler.
		} else {	// not responding to report, but still need to clear if clear
			if(t->nextOnRwy) {
				//cout << "departed = " << departed << '\n';
				//cout << "timeSinceLastDeparture = " << timeSinceLastDeparture << '\n';
				if(rwyOccupied) {
					// Do nothing
				} else if(timeSinceLastDeparture <= 60.0 && departed == true) {
					// Do nothing - this is a bit of a hack - should maybe do line up be ready here
				} else {
					ClearHoldingPlane(t);
					t->leg = TAKEOFF_ROLL;
					rwyList.push_back(t);
					rwyOccupied = true;
					holdList.erase(holdListItr);
					holdListItr = holdList.begin();
				}
			}
			// TODO - rationalise the considerable code duplication above!
		}
		++holdListItr;
	}
	//cout << "Done CheckHoldList" << endl;
}

// do the ciruit list
void FGTower::CheckCircuitList(double dt) {
	//cout << "Entering CheckCircuitList..." << endl;
	// Clear the constraints - we recalculate here.
	base_leg_pos = 0.0;
	downwind_leg_pos = 0.0;
	crosswind_leg_pos = 0.0;
	
	if(circuitList.size()) {	// Do one plane from the circuit
		if(circuitListItr == circuitList.end()) {
			circuitListItr = circuitList.begin();
		}
		TowerPlaneRec* t = *circuitListItr;
		if(t->isUser) {
			t->pos.setlon(user_lon_node->getDoubleValue());
			t->pos.setlat(user_lat_node->getDoubleValue());
			t->pos.setelev(user_elev_node->getDoubleValue());
		} else {
			t->pos = t->planePtr->GetPos();		// We should probably only set the pos's on one walk through the traffic list in the update function, to save a few CPU should we end up duplicating this.
			t->landingType = t->planePtr->GetLandingOption();
			//cout << "AI plane landing option is " << t->landingType << '\n';
		}
		Point3D tortho = ortho.ConvertToLocal(t->pos);
		if(t->isUser) {
			// Need to figure out which leg he's on
			//cout << "rwy.hdg = " << rwy.hdg << " user hdg = " << user_hdg_node->getDoubleValue();
			double ho = GetAngleDiff_deg(user_hdg_node->getDoubleValue(), rwy.hdg);
			//cout << " ho = " << ho << " abs(ho = " << abs(ho) << '\n';
			// TODO FIXME - get the wind and convert this to track, or otherwise use track somehow!!!
			// If it's gusty might need to filter the value, although we are leaving 30 degrees each way leeway!
			if(abs(ho) < 30) {
				// could be either takeoff, climbout or landing - check orthopos.y
				//cout << "tortho.y = " << tortho.y() << '\n';
				if((tortho.y() < 0) || (t->leg == TURN4) || (t->leg == FINAL)) {
					t->leg = FINAL;
					//cout << "Final\n";
				} else {
					t->leg = CLIMBOUT;	// TODO - check elev wrt. apt elev to differentiate takeoff roll and climbout
					//cout << "Climbout\n";
					// If it's the user we may be unsure of his/her intentions.
					// (Hopefully the AI planes won't try confusing the sim!!!)
					if(t->opType == TTT_UNKNOWN) {
						if(tortho.y() > 5000) {
							// 5 km out from threshold - assume it's a departure
							t->opType = OUTBOUND;	// TODO - could check if the user has climbed significantly above circuit altitude as well.
							// Since we are unknown operation we should be in depList already.
							circuitList.erase(circuitListItr);
							RemoveFromTrafficList(t->plane.callsign);
							circuitListItr = circuitList.begin();
						}
					} else if(t->opType == CIRCUIT) {
						if(tortho.y() > 10000) {
							// 10 km out - assume the user has abandoned the circuit!!
							t->opType = OUTBOUND;
							depList.push_back(t);
							circuitList.erase(circuitListItr);
							circuitListItr = circuitList.begin();
						}
					}
				}
			} else if(abs(ho) < 60) {
				// turn1 or turn 4
				// TODO - either fix or doublecheck this hack by looking at heading and pattern direction
				if((t->leg == CLIMBOUT) || (t->leg == TURN1)) {
					t->leg = TURN1;
					//cout << "Turn1\n";
				} else {
					t->leg = TURN4;
					//cout << "Turn4\n";
				}
			} else if(abs(ho) < 120) {
				// crosswind or base
				// TODO - either fix or doublecheck this hack by looking at heading and pattern direction
				if((t->leg == TURN1) || (t->leg == CROSSWIND)) {
					t->leg = CROSSWIND;
					//cout << "Crosswind\n";
				} else {
					t->leg = BASE;
					//cout << "Base\n";
				}
			} else if(abs(ho) < 150) {
				// turn2 or turn 3
				// TODO - either fix or doublecheck this hack by looking at heading and pattern direction
				if((t->leg == CROSSWIND) || (t->leg == TURN2)) {
					t->leg = TURN2;
					//cout << "Turn2\n";
				} else {
					t->leg = TURN3;
					// Probably safe now to assume the user is flying a circuit
					t->opType = CIRCUIT;
					//cout << "Turn3\n";
				}
			} else {
				// downwind
				t->leg = DOWNWIND;
				//cout << "Downwind\n";
			}
			if(t->leg == FINAL) {
				if(OnActiveRunway(t->pos)) {
					t->leg = LANDING_ROLL;
				}
			}
		} else {
			t->leg = t->planePtr->GetLeg();
		}
		
		// Set the constraints IF this is the first plane in the circuit
		// TODO - at the moment we're constraining plane 2 based on plane 1 - this won't (or might not) work for 3 planes in the circuit!!
		if(circuitListItr == circuitList.begin()) {
			switch(t->leg) {
			case FINAL:
				// Base leg must be at least as far out as the plane is - actually possibly not necessary for separation, but we'll use that for now.
				base_leg_pos = tortho.y();
				//cout << "base_leg_pos = " << base_leg_pos << '\n';
				break;
			case TURN4:
				// Fall through to base
			case BASE:
				base_leg_pos = tortho.y();
				//cout << "base_leg_pos = " << base_leg_pos << '\n';
				break;
			case TURN3:
				// Fall through to downwind
			case DOWNWIND:
				// Only have the downwind leg pos as turn-to-base constraint if more negative than we already have.
				base_leg_pos = (tortho.y() < base_leg_pos ? tortho.y() : base_leg_pos);
				//cout << "base_leg_pos = " << base_leg_pos;
				downwind_leg_pos = tortho.x();		// Assume that a following plane can simply be constrained by the immediately in front downwind plane
				//cout << " downwind_leg_pos = " << downwind_leg_pos << '\n';
				break;
			case TURN2:
				// Fall through to crosswind
			case CROSSWIND:
				crosswind_leg_pos = tortho.y();
				//cout << "crosswind_leg_pos = " << crosswind_leg_pos << '\n';
				t->instructedToGoAround = false;
				break;
			case TURN1:
				// Fall through to climbout
			case CLIMBOUT:
				// Only use current by constraint as largest
				crosswind_leg_pos = (tortho.y() > crosswind_leg_pos ? tortho.y() : crosswind_leg_pos);
				//cout << "crosswind_leg_pos = " << crosswind_leg_pos << '\n';
				break;
			case TAKEOFF_ROLL:
				break;
			case LEG_UNKNOWN:
				break;
			case LANDING_ROLL:
				break;
			default:
				break;
			}
		}
		
		if(t->leg == FINAL) {
			if(t->landingType == FULL_STOP) t->opType = INBOUND;
			if(t->eta < 12 && rwyList.size() && !(t->instructedToGoAround)) {
				// TODO - need to make this more sophisticated 
				// eg. is the plane accelerating down the runway taking off [OK],
				// or stationary near the start [V. BAD!!].
				// For now this should stop the AI plane landing on top of the user.
				string trns = t->plane.callsign;
				trns += " GO AROUND TRAFFIC ON RUNWAY I REPEAT GO AROUND";
				if(display) {
					globals->get_ATC_display()->RegisterSingleMessage(trns, 0);
				}
				t->instructedToGoAround = true;
				if(t->planePtr) {
					cout << "Registering Go-around transmission with AI plane\n";
					t->planePtr->RegisterTransmission(13);
				}
			}
		} else if(t->leg == LANDING_ROLL) {
			rwyList.push_front(t);
			// TODO - if(!clearedToLand) shout something!!
			t->clearedToLand = false;
			RemoveFromTrafficList(t->plane.callsign);
			if(t->isUser) {
				t->opType = TTT_UNKNOWN;
			}	// TODO - allow the user to specify opType via ATC menu
			circuitListItr = circuitList.erase(circuitListItr);
			if(circuitListItr == circuitList.end() ) {
				circuitListItr = circuitList.begin();
			}
		}
		++circuitListItr;
	}
	//cout << "Done CheckCircuitList" << endl;
}

// Do the runway list - we'll do the whole runway list since it's important and there'll never be many planes on the rwy at once!!
// FIXME - at the moment it looks like we're only doing the first plane from the rwy list.
// (However, at the moment there should only be one airplane on the rwy at once, until we
// start allowing planes to line up whilst previous arrival clears the rwy.)
void FGTower::CheckRunwayList(double dt) {
	//cout << "Entering CheckRunwayList..." << endl;
	if(rwyOccupied) {
		if(!rwyList.size()) {
			rwyOccupied = false;
		} else {
			rwyListItr = rwyList.begin();
			TowerPlaneRec* t = *rwyListItr;
			if(t->isUser) {
				t->pos.setlon(user_lon_node->getDoubleValue());
				t->pos.setlat(user_lat_node->getDoubleValue());
				t->pos.setelev(user_elev_node->getDoubleValue());
			} else {
				t->pos = t->planePtr->GetPos();		// We should probably only set the pos's on one walk through the traffic list in the update function, to save a few CPU should we end up duplicating this.
			}
			bool on_rwy = OnActiveRunway(t->pos);
			if(!on_rwy) {
				if((t->opType == INBOUND) || (t->opType == STRAIGHT_IN)) {
					rwyList.pop_front();
					delete t;
					// TODO - tell it to taxi / contact ground / don't delete it etc!
				} else if(t->opType == OUTBOUND) {
					depList.push_back(t);
					rwyList.pop_front();
					departed = true;
					timeSinceLastDeparture = 0.0;
				} else if(t->opType == CIRCUIT) {
					circuitList.push_back(t);
					AddToTrafficList(t);
					rwyList.pop_front();
					departed = true;
					timeSinceLastDeparture = 0.0;
				} else if(t->opType == TTT_UNKNOWN) {
					depList.push_back(t);
					circuitList.push_back(t);
					AddToTrafficList(t);
					rwyList.pop_front();
					departed = true;
					timeSinceLastDeparture = 0.0;	// TODO - we need to take into account that the user might taxi-in when flagged opType UNKNOWN - check speed/altitude etc to make decision as to what user is up to.
				} else {
					// HELP - we shouldn't ever get here!!!
				}
			}
		}
	}
	//cout << "Done CheckRunwayList" << endl;
}

// Do one plane from the approach list
void FGTower::CheckApproachList(double dt) {
	if(appList.size()) {
		if(appListItr == appList.end()) {
			appListItr = appList.begin();
		}
		TowerPlaneRec* t = *appListItr;
		//cout << "t = " << t << endl;
		if(t->isUser) {
			t->pos.setlon(user_lon_node->getDoubleValue());
			t->pos.setlat(user_lat_node->getDoubleValue());
			t->pos.setelev(user_elev_node->getDoubleValue());
		} else {
			// TODO - set/update the position if it's an AI plane
		}				
		if(t->nextOnRwy && !(t->clearedToLand)) {
			// check distance away and whether runway occupied
			// and schedule transmission if necessary
		}				
		++appListItr;
	}
}

// Returns true if positions of crosswind/downwind/base leg turns should be constrained by previous traffic
// plus the constraint position as a rwy orientated orthopos (meters)
bool FGTower::GetCrosswindConstraint(double& cpos) {
	if(crosswind_leg_pos != 0.0) {
		cpos = crosswind_leg_pos;
		return(true);
	} else {
		cpos = 0.0;
		return(false);
	}
}
bool FGTower::GetDownwindConstraint(double& dpos) {
	if(downwind_leg_pos != 0.0) {
		dpos = downwind_leg_pos;
		return(true);
	} else {
		dpos = 0.0;
		return(false);
	}
}
bool FGTower::GetBaseConstraint(double& bpos) {
	if(base_leg_pos != 0.0) {
		bpos = base_leg_pos;
		return(true);
	} else {
		bpos = 0.0;
		return(false);
	}
}


// Figure out which runways are active.
// For now we'll just be simple and do one active runway - eventually this will get much more complex
// This is a private function - public interface to the results of this is through GetActiveRunway
void FGTower::DoRwyDetails() {
	//cout << "GetRwyDetails called" << endl;
	
	// Based on the airport-id and wind get the active runway
	
	//wind
	double hdg = wind_from_hdg->getDoubleValue();
	double speed = wind_speed_knots->getDoubleValue();
	hdg = (speed == 0.0 ? 270.0 : hdg);
	//cout << "Heading = " << hdg << '\n';
	
	FGRunway runway;
	bool rwyGood = globals->get_runways()->search(ident, int(hdg), &runway);
	if(rwyGood) {
		activeRwy = runway.rwy_no;
		rwy.rwyID = runway.rwy_no;
		SG_LOG(SG_ATC, SG_INFO, "Active runway for airport " << ident << " is " << activeRwy);
		
		// Get the threshold position
		double other_way = runway.heading - 180.0;
		while(other_way <= 0.0) {
			other_way += 360.0;
		}
    	// move to the +l end/center of the runway
		//cout << "Runway center is at " << runway.lon << ", " << runway.lat << '\n';
    	Point3D origin = Point3D(runway.lon, runway.lat, aptElev);
		Point3D ref = origin;
    	double tshlon, tshlat, tshr;
		double tolon, tolat, tor;
		rwy.length = runway.length * SG_FEET_TO_METER;
		rwy.width = runway.width * SG_FEET_TO_METER;
    	geo_direct_wgs_84 ( aptElev, ref.lat(), ref.lon(), other_way, 
        	                rwy.length / 2.0 - 25.0, &tshlat, &tshlon, &tshr );
    	geo_direct_wgs_84 ( aptElev, ref.lat(), ref.lon(), runway.heading, 
        	                rwy.length / 2.0 - 25.0, &tolat, &tolon, &tor );
		// Note - 25 meters in from the runway end is a bit of a hack to put the plane ahead of the user.
		// now copy what we need out of runway into rwy
    	rwy.threshold_pos = Point3D(tshlon, tshlat, aptElev);
		Point3D takeoff_end = Point3D(tolon, tolat, aptElev);
		//cout << "Threshold position = " << tshlon << ", " << tshlat << ", " << aptElev << '\n';
		//cout << "Takeoff position = " << tolon << ", " << tolat << ", " << aptElev << '\n';
		rwy.hdg = runway.heading;
		// Set the projection for the local area based on this active runway
		ortho.Init(rwy.threshold_pos, rwy.hdg);	
		rwy.end1ortho = ortho.ConvertToLocal(rwy.threshold_pos);	// should come out as zero
		rwy.end2ortho = ortho.ConvertToLocal(takeoff_end);
	} else {
		SG_LOG(SG_ATC, SG_ALERT, "Help  - can't get good runway in FGTower!!");
		activeRwy = "NN";
	}
}


// Figure out if a given position lies on the active runway
// Might have to change when we consider more than one active rwy.
bool FGTower::OnActiveRunway(Point3D pt) {
	// TODO - check that the centre calculation below isn't confused by displaced thesholds etc.
	Point3D xyc((rwy.end1ortho.x() + rwy.end2ortho.x())/2.0, (rwy.end1ortho.y() + rwy.end2ortho.y())/2.0, 0.0);
	Point3D xyp = ortho.ConvertToLocal(pt);
	
	//cout << "Length offset = " << fabs(xyp.y() - xyc.y()) << '\n';
	//cout << "Width offset = " << fabs(xyp.x() - xyc.x()) << '\n';

	double rlen = rwy.length/2.0 + 5.0;
	double rwidth = rwy.width/2.0;
	double ldiff = fabs(xyp.y() - xyc.y());
	double wdiff = fabs(xyp.x() - xyc.x());

	return((ldiff < rlen) && (wdiff < rwidth));
}	


// Figure out if a given position lies on any runway or not
// Only call this at startup - reading the runways database is expensive and needs to be fixed!
bool FGTower::OnAnyRunway(Point3D pt) {
	ATCData ad;
	double dist = current_commlist->FindClosest(lon, lat, elev, ad, TOWER, 10.0);
	if(dist < 0.0) {
		return(false);
	}
	// Based on the airport-id, go through all the runways and check for a point in them
	
	// TODO - do we actually need to search for the airport - surely we already know our ident and
	// can just search runways of our airport???
	//cout << "Airport ident is " << ad.ident << '\n';
	FGRunway runway;
	bool rwyGood = globals->get_runways()->search(ad.ident, &runway);
	if(!rwyGood) {
		SG_LOG(SG_ATC, SG_WARN, "Unable to find any runways for airport ID " << ad.ident << " in FGTower");
	}
	bool on = false;
	while(runway.id == ad.ident) {		
		on = OnRunway(pt, runway);
		//cout << "Runway " << runway.rwy_no << ": On = " << (on ? "true\n" : "false\n");
		if(on) return(true);
		globals->get_runways()->next(&runway);		
	}
	return(on);
}


// Returns true if successful
bool FGTower::RemoveFromTrafficList(string id) {
	tower_plane_rec_list_iterator twrItr;
	for(twrItr = trafficList.begin(); twrItr != trafficList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		if(tpr->plane.callsign == id) {
			trafficList.erase(twrItr);
			return(true);
		}
	}	
	SG_LOG(SG_ATC, SG_WARN, "Warning - unable to remove aircraft " << id << " from trafficList in FGTower");
	return(false);
}


// Add a tower plane rec with ETA to the traffic list in the correct position ETA-wise
// and set nextOnRwy if so.
// Returns true if this could cause a threshold ETA conflict with other traffic, false otherwise.
// For planes holding they are put in the first position with time to go, and the return value is
// true if in the first position (nextOnRwy) and false otherwise.
// See the comments in FGTower::doThresholdUseOrder for notes on the ordering
bool FGTower::AddToTrafficList(TowerPlaneRec* t, bool holding) {
	//cout << "ADD: " << trafficList.size();
	//cout << "AddToTrafficList called, currently size = " << trafficList.size() << ", holding = " << holding << endl;
	double separation_time = 90.0;	// seconds - this is currently a guess for light plane separation, and includes a few seconds for a holding plane to taxi onto the rwy.
	double departure_sep_time = 60.0;	// Separation time behind departing airplanes.  Comments above also apply.
	bool conflict = false;
	double lastETA = 0.0;
	bool firstTime = true;
	// FIXME - make this more robust for different plane types eg. light following heavy.
	tower_plane_rec_list_iterator twrItr;
	//twrItr = trafficList.begin();
	//while(1) {
	for(twrItr = trafficList.begin(); twrItr != trafficList.end(); twrItr++) {
		//if(twrItr == trafficList.end()) {
		//	cout << "  END  ";
		//	trafficList.push_back(t);
		//	return(holding ? firstTime : conflict);
		//} else {
			TowerPlaneRec* tpr = *twrItr;
			if(holding) {
				//cout << (tpr->isUser ? "USER!\n" : "NOT user\n");
				//cout << "tpr->eta - lastETA = " << tpr->eta - lastETA << '\n';
				double dep_allowance = (timeSinceLastDeparture < departure_sep_time ? departure_sep_time - timeSinceLastDeparture : 0.0); 
				double slot_time = (firstTime ? separation_time + dep_allowance : separation_time + departure_sep_time);
				// separation_time + departure_sep_time in the above accounts for the fact that the arrival could be touch and go,
				// and if not needs time to clear the rwy anyway.
				if(tpr->eta  - lastETA > slot_time) {
					t->nextOnRwy = firstTime;
					trafficList.insert(twrItr, t);
					//cout << "\tH\t" << trafficList.size() << '\n';
					return(firstTime);
				}
				firstTime = false;
			} else {
				if(t->eta < tpr->eta) {
					// Ugg - this one's tricky.
					// It depends on what the two planes are doing and whether there's a conflict what we do.
					if(tpr->eta - t->eta > separation_time) {	// No probs, plane 2 can squeeze in before plane 1 with no apparent conflict
						if(tpr->nextOnRwy) {
							tpr->nextOnRwy = false;
							t->nextOnRwy = true;
						}
						trafficList.insert(twrItr, t);
					} else {	// Ooops - this ones tricky - we have a potential conflict!
						conflict = true;
						// HACK - just add anyway for now and flag conflict - TODO - FIX THIS using CIRCUIT/STRAIGHT_IN and VFR/IFR precedence rules.
						if(tpr->nextOnRwy) {
							tpr->nextOnRwy = false;
							t->nextOnRwy = true;
						}
						trafficList.insert(twrItr, t);
					}
					//cout << "\tC\t" << trafficList.size() << '\n';
					return(conflict);
				}
			}
		//}
		//++twrItr;
	}
	// If we get here we must be at the end of the list, or maybe the list is empty.
	if(!trafficList.size()) {
		t->nextOnRwy = true;
		// conflict and firstTime should be false and true respectively in this case anyway.
	}
	trafficList.push_back(t);
	//cout << "\tE\t" << trafficList.size() << endl;
	return(holding ? firstTime : conflict);
}

// Add a tower plane rec with ETA to the circuit list in the correct position ETA-wise
// Returns true if this might cause a separation conflict (based on ETA) with other traffic, false otherwise.
bool FGTower::AddToCircuitList(TowerPlaneRec* t) {
	//cout << "ADD: " << circuitList.size();
	//cout << "AddToCircuitList called, currently size = " << circuitList.size() << endl;
	double separation_time = 60.0;	// seconds - this is currently a guess for light plane separation, and includes a few seconds for a holding plane to taxi onto the rwy.
	bool conflict = false;
	tower_plane_rec_list_iterator twrItr;
	for(twrItr = circuitList.begin(); twrItr != circuitList.end(); twrItr++) {
			TowerPlaneRec* tpr = *twrItr;
				
				if(t->eta < tpr->eta) {
					// Ugg - this one's tricky.
					// It depends on what the two planes are doing and whether there's a conflict what we do.
					if(tpr->eta - t->eta > separation_time) {	// No probs, plane 2 can squeeze in before plane 1 with no apparent conflict
						circuitList.insert(twrItr, t);
					} else {	// Ooops - this ones tricky - we have a potential conflict!
						conflict = true;
						// HACK - just add anyway for now and flag conflict.
						circuitList.insert(twrItr, t);
					}
					//cout << "\tC\t" << circuitList.size() << '\n';
					return(conflict);
				}
	}
	// If we get here we must be at the end of the list, or maybe the list is empty.
	circuitList.push_back(t);	// TODO - check the separation with the preceding plane for the conflict flag.
	//cout << "\tE\t" << circuitList.size() << endl;
	return(conflict);
}


// Calculate the eta of a plane to the threshold.
// For ground traffic this is the fastest they can get there.
// For air traffic this is the middle approximation.
void FGTower::CalcETA(TowerPlaneRec* tpr, bool printout) {
	// For now we'll be very crude and hardwire expected speeds to C172-like values
	// The speeds below are specified in knots IAS and then converted to m/s
	double app_ias = 100.0 * 0.514444;			// Speed during straight-in approach
	double circuit_ias = 80.0 * 0.514444;		// Speed around circuit
	double final_ias = 70.0 * 0.514444;		// Speed during final approach
	
	//if(printout) {
	//cout << "In CalcETA, airplane ident = " << tpr->plane.callsign << '\n';
	//cout << (tpr->isUser ? "USER\n" : "AI\n");
	//cout << flush;
	//}
	
	// Sign convention - dist_out is -ve for approaching planes and +ve for departing planes
	// dist_across is +ve in the pattern direction - ie a plane correctly on downwind will have a +ve dist_across
	
	Point3D op = ortho.ConvertToLocal(tpr->pos);
	//if(printout) {
	//	cout << "Orthopos is " << op.x() << ", " << op.y() << '\n';
	//	cout << "opType is " << tpr->opType << '\n';
	//}
	double dist_out_m = op.y();
	double dist_across_m = fabs(op.x());	// FIXME = the fabs is a hack to cope with the fact that we don't know the circuit direction yet
	//cout << "Doing ETA calc for " << tpr->plane.callsign << '\n';
	
	if(tpr->opType == STRAIGHT_IN) {
		double dist_to_go_m = sqrt((dist_out_m * dist_out_m) + (dist_across_m * dist_across_m));
		if(dist_to_go_m < 1000) {
			tpr->eta = dist_to_go_m / final_ias;
		} else {
			tpr->eta = (1000.0 / final_ias) + ((dist_to_go_m - 1000.0) / app_ias);
		}
	} else if(tpr->opType == CIRCUIT || tpr->opType == TTT_UNKNOWN) {	// Hack alert - UNKNOWN has sort of been added here as a temporary hack.
		// It's complicated - depends on if base leg is delayed or not
		//if(printout) {
		//	cout << "Leg = " << tpr->leg << '\n';
		//}
		if(tpr->leg == LANDING_ROLL) {
			tpr->eta = 0;
		} else if((tpr->leg == FINAL) || (tpr->leg == TURN4)) {
			tpr->eta = fabs(dist_out_m) / final_ias;
		} else if((tpr->leg == BASE) || (tpr->leg == TURN3)) {
			tpr->eta = (fabs(dist_out_m) / final_ias) + (dist_across_m / circuit_ias);
		} else {
			// Need to calculate where base leg is likely to be
			// FIXME - for now I'll hardwire it to 1000m which is what AILocalTraffic uses!!!
			// TODO - as a matter of design - AILocalTraffic should get the nominal no-traffic base turn distance from Tower, since in real life the published pattern might differ from airport to airport
			double nominal_base_dist_out_m = -1000;
			double current_base_dist_out_m;
			if(!GetBaseConstraint(current_base_dist_out_m)) {
				current_base_dist_out_m = nominal_base_dist_out_m;
			}
			double nominal_dist_across_m = 1000;	// Hardwired value from AILocalTraffic
			double current_dist_across_m;
			if(!GetDownwindConstraint(current_dist_across_m)) {
				current_dist_across_m = nominal_dist_across_m;
			}
			double nominal_cross_dist_out_m = 2000;	// Bit of a guess - AI plane turns to crosswind at 600ft agl.
			tpr->eta = fabs(current_base_dist_out_m) / final_ias;	// final
			//if(printout) cout << "a = " << tpr->eta << '\n';
			if((tpr->leg == DOWNWIND) || (tpr->leg == TURN2)) {
				tpr->eta += dist_across_m / circuit_ias;
				//if(printout) cout << "b = " << tpr->eta << '\n';
				tpr->eta += fabs(current_base_dist_out_m - dist_out_m) / circuit_ias;
				//if(printout) cout << "c = " << tpr->eta << '\n';
			} else if((tpr->leg == CROSSWIND) || (tpr->leg == TURN1)) {
				if(dist_across_m > nominal_dist_across_m) {
					tpr->eta += dist_across_m / circuit_ias;
				} else {
					tpr->eta += nominal_dist_across_m / circuit_ias;
				}
				// should we use the dist across of the previous plane if there is previous still on downwind?
				//if(printout) cout << "bb = " << tpr->eta << '\n';
				if(dist_out_m > nominal_cross_dist_out_m) {
					tpr->eta += fabs(current_base_dist_out_m - dist_out_m) / circuit_ias;
				} else {
					tpr->eta += fabs(current_base_dist_out_m - nominal_cross_dist_out_m) / circuit_ias;
				}
				//if(printout) cout << "cc = " << tpr->eta << '\n';
				if(nominal_dist_across_m > dist_across_m) {
					tpr->eta += (nominal_dist_across_m - dist_across_m) / circuit_ias;
				} else {
					// Nothing to add
				}
				//if(printout) cout << "dd = " << tpr->eta << '\n';
			} else {
				// We've only just started - why not use a generic estimate?
				tpr->eta = 240.0;
			}
		}
		//if(printout) {
		//	cout << "ETA = " << tpr->eta << '\n';
		//}
	} else {
		tpr->eta = 99999;
	}	
}


// Calculate the distance of a plane to the threshold in meters
// TODO - Modify to calculate flying distance of a plane in the circuit
double FGTower::CalcDistOutM(TowerPlaneRec* tpr) {
	return(dclGetHorizontalSeparation(rwy.threshold_pos, tpr->pos));
}


// Calculate the distance of a plane to the threshold in miles
// TODO - Modify to calculate flying distance of a plane in the circuit
double FGTower::CalcDistOutMiles(TowerPlaneRec* tpr) {
	return(CalcDistOutM(tpr) / 1600.0);		// FIXME - use a proper constant if possible.
}


// Iterate through all the lists and call CalcETA for all the planes.
void FGTower::doThresholdETACalc() {
	//cout << "Entering doThresholdETACalc..." << endl;
	tower_plane_rec_list_iterator twrItr;
	// Do the approach list first
	for(twrItr = appList.begin(); twrItr != appList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		CalcETA(tpr);
	}	
	// Then the circuit list
	for(twrItr = circuitList.begin(); twrItr != circuitList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		CalcETA(tpr);
	}
	//cout << "Done doThresholdETCCalc" << endl;
}
		

// Check that the planes in traffic list are correctly ordered,
// that the nearest (timewise) is flagged next on rwy, and return
// true if any threshold use conflicts are detected, false otherwise.
bool FGTower::doThresholdUseOrder() {
	//cout << "Entering doThresholdUseOrder..." << endl;
	bool conflict = false;
	
	// Wipe out traffic list, go through circuit, app and hold list, and reorder them in traffic list.
	// Here's the rather simplistic assumptions we're using:
	// Currently all planes are assumed to be GA light singles with corresponding speeds and separation times.
	// In order of priority for runway use:
	// STRAIGHT_IN > CIRCUIT > HOLDING_FOR_DEPARTURE
	// No modification of planes speeds occurs - conflicts are resolved by delaying turn for base,
	// and holding planes until a space.
	// When calculating if a holding plane can use the runway, time clearance from last departure
	// as well as time clearance to next arrival must be considered.
	
	trafficList.clear();
	
	tower_plane_rec_list_iterator twrItr;
	// Do the approach list first
	//cout << "A" << flush;
	for(twrItr = appList.begin(); twrItr != appList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		conflict = AddToTrafficList(tpr);
	}	
	// Then the circuit list
	//cout << "C" << flush;
	for(twrItr = circuitList.begin(); twrItr != circuitList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		conflict = AddToTrafficList(tpr);
	}
	// And finally the hold list
	//cout << "H" << endl;
	for(twrItr = holdList.begin(); twrItr != holdList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		AddToTrafficList(tpr, true);
	}
	
	if(0) {
	//if(ident == "KEMT") {
		for(twrItr = trafficList.begin(); twrItr != trafficList.end(); twrItr++) {
			TowerPlaneRec* tpr = *twrItr;
			cout << tpr->plane.callsign << '\t' << tpr->eta << '\t';
		}
		cout << endl;
	}
	
	//cout << "Done doThresholdUseOrder" << endl;
	return(conflict);
}


// Return the ETA of plane no. list_pos (1-based) in the traffic list.
// i.e. list_pos = 1 implies next to use runway.
double FGTower::GetTrafficETA(unsigned int list_pos, bool printout) {
	if(trafficList.size() < list_pos) {
		return(99999);
	}

	tower_plane_rec_list_iterator twrItr;
	twrItr = trafficList.begin();
	for(unsigned int i = 1; i < list_pos; i++, twrItr++);
	TowerPlaneRec* tpr = *twrItr;
	CalcETA(tpr, printout);
	//cout << "ETA returned = " << tpr->eta << '\n';
	return(tpr->eta);
}
	

void FGTower::ContactAtHoldShort(PlaneRec plane, FGAIPlane* requestee, tower_traffic_type operation) {
	// HACK - assume that anything contacting at hold short is new for now - FIXME LATER
	TowerPlaneRec* t = new TowerPlaneRec;
	t->plane = plane;
	t->planePtr = requestee;
	t->holdShortReported = true;
	t->clearedToLineUp = false;
	t->clearedToTakeOff = false;
	t->opType = operation;
	t->pos = requestee->GetPos();
	
	//cout << "Hold Short reported by " << plane.callsign << '\n';
	SG_LOG(SG_ATC, SG_BULK, "Hold Short reported by " << plane.callsign);

/*	
	bool next = AddToTrafficList(t, true);
	if(next) {
		double teta = GetTrafficETA(2);
		if(teta < 150.0) {
			t->clearanceCounter = 7.0;	// This reduces the delay before response to 3 secs if an immediate takeoff is reqd
			//cout << "Reducing response time to request due imminent traffic\n";
		}
	} else {
	}
*/
	// TODO - possibly add the reduced interval to clearance when immediate back in under the new scheme

	holdList.push_back(t);
	
	responseReqd = true;
}

// Register the presence of an AI plane at a point where contact would already have been made in real life
// CAUTION - currently it is assumed that this plane's callsign is unique - it is up to AIMgr to generate unique callsigns.
void FGTower::RegisterAIPlane(PlaneRec plane, FGAIPlane* ai, tower_traffic_type op, PatternLeg lg) {
	// At the moment this is only going to be tested with inserting an AI plane on downwind
	TowerPlaneRec* t = new TowerPlaneRec;
	t->plane = plane;
	t->planePtr = ai;
	t->opType = op;
	t->leg = lg;
	t->pos = ai->GetPos();
	
	CalcETA(t);
	
	if(op == CIRCUIT && lg != LEG_UNKNOWN) {
		AddToCircuitList(t);
	} else {
		// FLAG A WARNING
	}
	
	doThresholdUseOrder();
}

void FGTower::RequestLandingClearance(string ID) {
	//cout << "Request Landing Clearance called...\n";
	
	// Assume this comes from the user - have another function taking a pointer to the AIplane for the AI traffic.
	// For now we'll also assume that the user is a light plane and can get him/her to join the circuit if necessary.
	
	TowerPlaneRec* t = new TowerPlaneRec;
	t->isUser = true;
	t->clearedToLand = false;
	t->pos.setlon(user_lon_node->getDoubleValue());
	t->pos.setlat(user_lat_node->getDoubleValue());
	t->pos.setelev(user_elev_node->getDoubleValue());
	
	// TODO
	// Calculate where the user is in relation to the active runway and it's circuit
	// and set the op-type as appropriate.
	
	// HACK - to get up and running I'm going to assume that the user contacts tower on a staight-in final for now.
	t->opType = STRAIGHT_IN;
	
	t->plane.type = GA_SINGLE;	// FIXME - Another assumption!
	t->plane.callsign = ID;
	
	appList.push_back(t);	// Not necessarily permanent
	AddToTrafficList(t);
}

void FGTower::RequestDepartureClearance(string ID) {
	//cout << "Request Departure Clearance called...\n";
}
	
void FGTower::ReportFinal(string ID) {
	TowerPlaneRec* t = FindPlane(ID);
	if(t) {
		t->finalReported = true;
		t->finalAcknowledged = false;
		if(!(t->clearedToLand)) {
			responseReqd = true;
		} // possibly respond with wind even if already cleared to land?
	} else {
		SG_LOG(SG_ATC, SG_WARN, "WARNING: Unable to find plane " << ID << " in FGTower::ReportFinal(...)");
	}
}

//void FGTower::ReportLongFinal(string ID);
//void FGTower::ReportOuterMarker(string ID);
//void FGTower::ReportMiddleMarker(string ID);
//void FGTower::ReportInnerMarker(string ID);
//void FGTower::ReportGoingAround(string ID);

void FGTower::ReportRunwayVacated(string ID) {
	//cout << "Report Runway Vacated Called...\n";
}

TowerPlaneRec* FGTower::FindPlane(string ID) {
	tower_plane_rec_list_iterator twrItr;
	// Do the approach list first
	for(twrItr = appList.begin(); twrItr != appList.end(); twrItr++) {
		if((*twrItr)->plane.callsign == ID) return(*twrItr);
	}	
	// Then the circuit list
	for(twrItr = circuitList.begin(); twrItr != circuitList.end(); twrItr++) {
		if((*twrItr)->plane.callsign == ID) return(*twrItr);
	}
	// And finally the hold list
	for(twrItr = holdList.begin(); twrItr != holdList.end(); twrItr++) {
		if((*twrItr)->plane.callsign == ID) return(*twrItr);
	}
	SG_LOG(SG_ATC, SG_WARN, "Unable to find " << ID << " in FGTower::FindPlane(...)");
	return(NULL);
}

void FGTower::ReportDownwind(string ID) {
	//cout << "ReportDownwind(...) called\n";
	// Tell the plane reporting what number she is in the circuit
	TowerPlaneRec* t = FindPlane(ID);
	if(t) {
		t->downwindReported = true;
		responseReqd = true;
	} else {
		SG_LOG(SG_ATC, SG_WARN, "WARNING: Unable to find plane " << ID << " in FGTower::ReportDownwind(...)");
	}
}

ostream& operator << (ostream& os, tower_traffic_type ttt) {
	switch(ttt) {
	case(CIRCUIT):      return(os << "CIRCUIT");
	case(INBOUND):      return(os << "INBOUND");
	case(OUTBOUND):     return(os << "OUTBOUND");
	case(TTT_UNKNOWN):  return(os << "UNKNOWN");
	case(STRAIGHT_IN):  return(os << "STRAIGHT_IN");
	}
	return(os << "ERROR - Unknown switch in tower_traffic_type operator << ");
}

