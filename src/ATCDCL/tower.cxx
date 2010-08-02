// FGTower - a class to provide tower control at towered airports.
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
// Copyright (C) 2008  Daniyar Atadjanov (ground clearance, gear check, weather, etc.)
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_STRINGS_H
#  include <strings.h>   // bcopy()
#else
#  include <string.h>    // MSVC doesn't have strings.h
#endif

#include <sstream>
#include <iomanip>
#include <iostream>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>
#include <Airports/runways.hxx>

#include "tower.hxx"
#include "ground.hxx"
#include "ATCmgr.hxx"
#include "ATCutils.hxx"
#include "ATCDialog.hxx"
#include "commlist.hxx"


using std::cout;

// TowerPlaneRec

TowerPlaneRec::TowerPlaneRec() :
	eta(0),
	dist_out(0),
	clearedToLand(false),
	clearedToLineUp(false),
	clearedToTakeOff(false),
	holdShortReported(false),
	lineUpReported(false),
	downwindReported(false),
	longFinalReported(false),
	longFinalAcknowledged(false),
	finalReported(false),
	finalAcknowledged(false),
	rwyVacatedReported(false),
	rwyVacatedAcknowledged(false),
	goAroundReported(false),
	instructedToGoAround(false),
	onRwy(false),
	nextOnRwy(false),
	gearWasUp(false),
	gearUpReported(false),
	vfrArrivalReported(false),
	vfrArrivalAcknowledged(false),
	opType(TTT_UNKNOWN),
	leg(LEG_UNKNOWN),
	landingType(AIP_LT_UNKNOWN),
	isUser(false)
{
	plane.callsign = "UNKNOWN";
}

TowerPlaneRec::TowerPlaneRec(const PlaneRec& p) :
	eta(0),
	dist_out(0),
	clearedToLand(false),
	clearedToLineUp(false),
	clearedToTakeOff(false),
	holdShortReported(false),
	lineUpReported(false),
	downwindReported(false),
	longFinalReported(false),
	longFinalAcknowledged(false),
	finalReported(false),
	finalAcknowledged(false),
	rwyVacatedReported(false),
	rwyVacatedAcknowledged(false),
	goAroundReported(false),
	instructedToGoAround(false),
	onRwy(false),
	nextOnRwy(false),
	gearWasUp(false),
	gearUpReported(false),
	vfrArrivalReported(false),
	vfrArrivalAcknowledged(false),
	opType(TTT_UNKNOWN),
	leg(LEG_UNKNOWN),
	landingType(AIP_LT_UNKNOWN),
	isUser(false)
{
	plane = p;
}

TowerPlaneRec::TowerPlaneRec(const SGGeod& pt) :
	eta(0),
	dist_out(0),
	clearedToLand(false),
	clearedToLineUp(false),
	clearedToTakeOff(false),
	holdShortReported(false),
	lineUpReported(false),
	downwindReported(false),
	longFinalReported(false),
	longFinalAcknowledged(false),
	finalReported(false),
	finalAcknowledged(false),
	rwyVacatedReported(false),
	rwyVacatedAcknowledged(false),
	goAroundReported(false),
	instructedToGoAround(false),
	onRwy(false),
	nextOnRwy(false),
	gearWasUp(false),
	gearUpReported(false),
	vfrArrivalReported(false),
	vfrArrivalAcknowledged(false),
	opType(TTT_UNKNOWN),
	leg(LEG_UNKNOWN),
	landingType(AIP_LT_UNKNOWN),
	isUser(false)
{
	plane.callsign = "UNKNOWN";
 	pos = pt;
}

TowerPlaneRec::TowerPlaneRec(const PlaneRec& p, const SGGeod& pt) :
	eta(0),
	dist_out(0),
	clearedToLand(false),
	clearedToLineUp(false),
	clearedToTakeOff(false),
	holdShortReported(false),
	lineUpReported(false),
	downwindReported(false),
	longFinalReported(false),
	longFinalAcknowledged(false),
	finalReported(false),
	finalAcknowledged(false),
	rwyVacatedReported(false),
	rwyVacatedAcknowledged(false),
	goAroundReported(false),
	instructedToGoAround(false),
	onRwy(false),
	nextOnRwy(false),
	gearWasUp(false),
	gearUpReported(false),
	vfrArrivalReported(false),
	vfrArrivalAcknowledged(false),
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
			   
Currently user is assumed to have taken off again when leaving the runway - check speed/elev for taxiing-in. (MAJOR)

Use track instead of heading to determine what leg of the circuit the user is flying. (MINOR)

Use altitude as well as position to try to determine if the user has left the circuit. (MEDIUM - other issues as well).

Currently HoldShortReported code assumes there will be only one plane holding for the runway at once and 
will break when planes start queueing. (CRITICAL)

Report-Runway-Vacated is left as only user ATC option following a go-around. (MAJOR)

Report-Downwind is not added as ATC option when user takes off to fly a circuit. (MAJOR)

eta of USER can be calculated very wrongly in circuit if flying straight out and turn4 etc are with +ve ortho y. 
This can then screw up circuit ordering for other planes (MEDIUM)

USER leaving circuit needs to be more robustly considered when intentions unknown
Currently only considered during climbout and breaks when user turns (MEDIUM).

GetPos() of the AI planes is called erratically - either too much or not enough. (MINOR)

GO-AROUND is instructed very late at < 12s to landing - possibly make more dependent on chance of rwy clearing before landing (FEATURE)

Need to make clear when TowerPlaneRecs do or don't get deleted in RemoveFromCircuitList etc. (MINOR until I misuse it - then CRITICAL!)

FGTower::RemoveAllUserDialogOptions() really ought to be replaced by an ATCDialog::clear_entries() function. (MINOR - efficiency).

At the moment planes in the lists are not guaranteed to always have a sensible ETA - it should be set as part of AddList functions, and lists should only be accessed this way. (FAIRLY MAJOR). 
*******************************************/

FGTower::FGTower() :
	separateGround(true),
	ground(0)
{
	ATCmgr = globals->get_ATC_mgr();
	
	_type = TOWER;
	
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
	vacatedListItr = vacatedList.begin();
	
	freqClear = true;
	
	timeSinceLastDeparture = 9999;
	departed = false;
	
	nominal_downwind_leg_pos = 1000.0;
	nominal_base_leg_pos = -1000.0;
	// TODO - set nominal crosswind leg pos based on minimum distance from takeoff end of rwy.
	
	_departureControlled = false;
}

FGTower::~FGTower() {
	if(!separateGround) {
		delete ground;
	}
}

void FGTower::Init() {
	//cout << "Initialising tower " << ident << '\n';
	
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
				if(_display) {
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
			if(_display) {
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
		if(_display) {
			ground->SetDisplay();
		} else {
			ground->SetNoDisplay();
		}
	}
	
	RemoveAllUserDialogOptions();
	
	// TODO - attempt to get a departure control pointer to see if we need to hand off departing traffic to departure.
	
	// Get the airport elevation
	aptElev = fgGetAirportElev(ident.c_str());
	
	// TODO - this function only assumes one active rwy.
	DoRwyDetails();
	
	// TODO - this currently assumes only one active runway.
	rwyOccupied = OnActiveRunway(SGGeod::fromDegM(user_lon_node->getDoubleValue(), user_lat_node->getDoubleValue(), 0.0));
	
	if(!OnAnyRunway(SGGeod::fromDegM(user_lon_node->getDoubleValue(), user_lat_node->getDoubleValue(), 0.0), false)) {
		//cout << ident << "  ADD 0\n";
		current_atcdialog->add_entry(ident, "@AP Tower, @CS @MI miles @CD of the airport for full stop@AT",
				"Contact tower for VFR arrival (full stop)", TOWER,
				(int)USER_REQUEST_VFR_ARRIVAL_FULL_STOP);
	} else {
		//cout << "User found on active runway\n";
		// Assume the user is started at the threshold ready to take-off
		TowerPlaneRec* t = new TowerPlaneRec;
		t->plane.callsign = fgGetString("/sim/user/callsign");
		t->plane.type = GA_SINGLE;	// FIXME - hardwired!!
		t->opType = TTT_UNKNOWN;	// We don't know if the user wants to do circuits or a departure...
		t->landingType = AIP_LT_UNKNOWN;
		t->leg = TAKEOFF_ROLL;
		t->isUser = true;
		t->clearedToTakeOff = false;
		rwyList.push_back(t);
		rwyListItr = rwyList.begin();
		departed = false;
		current_atcdialog->add_entry(ident, "@CS @TO", "Request departure / take-off clearance",
				TOWER, (int)USER_REQUEST_TAKE_OFF);
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
	//if(ident == "EGNX") cout << display << '\n';
	
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
	
	if(update_count == 8) {
		CheckDepartureList(dt);
	}
	
	// TODO - do one plane from the departure list and set departed = false when out of consideration
	
	//doCommunication();
	
	if(!separateGround) {
		// The display stuff might have to get more clever than this when not separate 
		// since the tower and ground might try communicating simultaneously even though
		// they're mean't to be the same contoller/frequency!!
		// We could also get rid of this by overloading FGATC's Set(No)Display() functions.
		if(_display) {
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
}

void FGTower::ReceiveUserCallback(int code) {
	if(code == (int)USER_REQUEST_VFR_DEPARTURE) {
		RequestDepartureClearance("USER");
	} else if(code == (int)USER_REQUEST_VFR_ARRIVAL) {
		VFRArrivalContact("USER");
	} else if(code == (int)USER_REQUEST_VFR_ARRIVAL_FULL_STOP) {
		VFRArrivalContact("USER", FULL_STOP);
	} else if(code == (int)USER_REQUEST_VFR_ARRIVAL_TOUCH_AND_GO) {
		VFRArrivalContact("USER", TOUCH_AND_GO);
	} else if(code == (int)USER_REPORT_DOWNWIND) {
		ReportDownwind("USER");
	} else if(code == (int)USER_REPORT_3_MILE_FINAL) {
		// For now we'll just call report final instead of report long final to avoid having to alter the response code
		ReportFinal("USER");
	} else if(code == (int)USER_REPORT_RWY_VACATED) {
		ReportRunwayVacated("USER");
	} else if(code == (int)USER_REPORT_GOING_AROUND) {
		ReportGoingAround("USER");
	} else if(code == (int)USER_REQUEST_TAKE_OFF) {
		RequestTakeOffClearance("USER");
	}
}

// **************** RESPONSE FUNCTIONS ****************

void FGTower::Respond() {
	//cout << "\nEntering Respond, responseID = " << responseID << endl;
	TowerPlaneRec* t = FindPlane(responseID);
	if(t) {
		// This will grow!!!
		if(t->vfrArrivalReported && !t->vfrArrivalAcknowledged) {
			//cout << "Tower " << ident << " is responding to VFR arrival reported...\n";
			// Testing - hardwire straight in for now
			string trns = t->plane.callsign;
			trns += " ";
			trns += name;
			trns += " Tower,";
			// Should we clear staight in or for downwind entry?
			// For now we'll clear straight in if greater than 1km from a line drawn through the threshold perpendicular to the rwy.
			// Later on we might check the actual heading and direct some of those to enter on downwind or base.
			SGVec3d op = ortho.ConvertToLocal(t->pos);
			float gp = fgGetFloat("/gear/gear/position-norm");
			if(gp < 1)
				t->gearWasUp = true; // This will be needed on final to tell "Gear down, ready to land."
			if(op.y() < -1000) {
				trns += " Report three mile straight-in runway ";
				t->opType = STRAIGHT_IN;
				if(t->isUser) {
					current_atcdialog->add_entry(ident, "@CS @MI mile final runway @RW@GR", "Report Final", TOWER, (int)USER_REPORT_3_MILE_FINAL);
				}
			} else {
				// For now we'll just request reporting downwind.
				// TODO - In real life something like 'report 2 miles southwest right downwind rwy 19R' might be used
				// but I'm not sure how to handle all permutations of which direction to tell to report from yet.
				trns += " Report ";
				//cout << "Responding, rwy.patterDirection is " << rwy.patternDirection << '\n';
				trns += ((rwy.patternDirection == 1) ? "right " : "left ");
				trns += "downwind runway ";
				t->opType = CIRCUIT;
				// leave it in the app list until it gets into pattern though.
				if(t->isUser) {
					current_atcdialog->add_entry(ident, "@AP Tower, @CS Downwind @RW", "Report Downwind", TOWER, (int)USER_REPORT_DOWNWIND);
				}
			}
			trns += ConvertRwyNumToSpokenString(activeRwy);
			if(_display) {
				pending_transmission = trns;
				Transmit();
			} else {
				//cout << "Not displaying, trns was " << trns << '\n';
			}
			t->vfrArrivalAcknowledged = true;
		} else if(t->downwindReported) {
			//cout << "Tower " << ident << " is responding to downwind reported...\n";
			ProcessDownwindReport(t);
			t->downwindReported = false;
		} else if(t->lineUpReported) {
			string trns = t->plane.callsign;
			if(rwyOccupied) {
				double f = globals->get_ATC_mgr()->GetFrequency(ident, ATIS) / 100.0;
				string wtr;
				if(!f) {
					wtr = ", " + GetWeather();
				}
				trns += " Cleared for take-off" + wtr;
				t->clearedToTakeOff = true;
			} else {
				if(!OnAnyRunway(SGGeod::fromDegM(user_lon_node->getDoubleValue(), user_lat_node->getDoubleValue(), 0.0), true)) {
					// TODO: Check if any AI Planes on final and tell something like: "After the landing CALLSIGN line up runway two eight right"
					trns += " Line up runway " + ConvertRwyNumToSpokenString(activeRwy);
					t->clearedToTakeOff = false;
					current_atcdialog->add_entry(ident, "@CS @TO", "Report ready for take-off", TOWER, (int)USER_REQUEST_TAKE_OFF);

				} else {
					sg_srandom_time();
					if((int(sg_random() * 10) + 1) != 3) {
						t->clearedToTakeOff = true;
						trns += " Cleared immediate take-off ";
					} else {
						t->clearedToTakeOff = false;
						trns += " Negative, departure runway " + ConvertRwyNumToSpokenString(activeRwy);
					}
				}
			}
			if(_display) {
				pending_transmission = trns;
				Transmit();
			} else {
				//cout << "Not displaying, trns was " << trns << '\n';
			}
			t->lineUpReported = false;
		} else if(t->holdShortReported) {
			//cout << "Tower " << ident << " is reponding to holdShortReported...\n";
			if(t->nextOnRwy) {
				if(rwyOccupied) {	// TODO - ought to add a sanity check that it isn't this plane only on the runway (even though it shouldn't be!!)
					// Do nothing for now - consider acknowloging hold short eventually
				} else {
					ClearHoldingPlane(t);
					t->leg = TAKEOFF_ROLL;
					rwyList.push_back(t);
					rwyListItr = rwyList.begin();
					rwyOccupied = true;
					// WARNING - WE ARE ASSUMING ONLY ONE PLANE REPORTING HOLD AT A TIME BELOW
					// FIXME TODO - FIX THIS!!!
					if(!holdList.empty()) {
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
				if(_display) {
					pending_transmission = trns;
					Transmit();
				}
				// TODO - add some idea of what traffic is blocking him.
			}
			t->holdShortReported = false;
		} else if(t->finalReported && !(t->finalAcknowledged)) {
			//cout << "Tower " << ident << " is responding to finalReported...\n";
			bool disp = true;
			string trns = t->plane.callsign;
			//cout << (t->nextOnRwy ? "Next on rwy " : "Not next!! ");
			//cout << (rwyOccupied ? "RWY OCCUPIED!!\n" : "Rwy not occupied\n");
			if(t->nextOnRwy && !rwyOccupied && !(t->instructedToGoAround)) {
				if(t->landingType == FULL_STOP) {
					trns += " cleared to land ";
				} else {
					double f = globals->get_ATC_mgr()->GetFrequency(ident, ATIS) / 100.0;
					string wtr;
					if(!f) {
						wtr = ", " + GetWeather();
					} else {
						wtr = ", runway " + ConvertRwyNumToSpokenString(activeRwy);
					}
					trns += " cleared to land" + wtr;
				}
				// TODO - add winds
				t->clearedToLand = true;
				// Maybe remove report downwind from menu here as well incase user didn't bother to?
				if(t->isUser) {
					//cout << "ADD VACATED B\n";
					// Put going around at the top (and hence default) since that'll be more desperate,
					// or put rwy vacated at the top since that'll be more common?
					current_atcdialog->add_entry(ident, "@CS Going Around", "Report going around", TOWER, USER_REPORT_GOING_AROUND);
					current_atcdialog->add_entry(ident, "@CS Clear of the runway", "Report runway vacated", TOWER, USER_REPORT_RWY_VACATED);
				}
			} else if(t->eta < 20) {
				// Do nothing - we'll be telling it to go around in less than 10 seconds if the
				// runway doesn't clear so no point in calling "continue approach".
				disp = false;
			} else {
				trns += " continue approach";
				trns += " and report ";
				trns += ((rwy.patternDirection == 1) ? "right " : "left ");
				trns += "downwind runway " + ConvertRwyNumToSpokenString(activeRwy);
				t->opType = CIRCUIT;
				if(t->isUser) {
					current_atcdialog->add_entry(ident, "@AP Tower, @CS Downwind @RW", "Report Downwind", TOWER, (int)USER_REPORT_DOWNWIND);
				}
				t->clearedToLand = false;
			}
			if(_display && disp) {
				pending_transmission = trns;
				Transmit();
			}
			t->finalAcknowledged = true;
		} else if(t->rwyVacatedReported && !(t->rwyVacatedAcknowledged)) {
			ProcessRunwayVacatedReport(t);
			t->rwyVacatedAcknowledged = true;
		}
	}
	//freqClear = true;	// FIXME - set this to come true after enough time to render the message
	_releaseCounter = 0.0;
	_releaseTime = 5.5;
	_runReleaseCounter = true;
	//cout << "Done Respond\n" << endl;
}

void FGTower::ProcessDownwindReport(TowerPlaneRec* t) {
	int i = 1;
	int a = 0;	// Count of preceding planes on approach
	bool cf = false;	// conflicting traffic on final
	bool cc = false;	// preceding traffic in circuit
	TowerPlaneRec* tc = NULL;
	for(tower_plane_rec_list_iterator twrItr = circuitList.begin(); twrItr != circuitList.end(); twrItr++) {
		if((*twrItr)->plane.callsign == responseID) break;
		tc = *twrItr;
		++i;
	}
	if(i > 1) { cc = true; }
	doThresholdETACalc();
	TowerPlaneRec* tf = NULL;
	for(tower_plane_rec_list_iterator twrItr = appList.begin(); twrItr != appList.end(); twrItr++) {
		if((*twrItr)->eta < (t->eta + 45) && strcmp((*twrItr)->plane.callsign.c_str(), t->plane.callsign.c_str()) != 0) { // don't let ATC ask you to follow yourself
			a++;
			tf = *twrItr;
			cf = true;
			// This should set the flagged plane to be the last conflicting one, and hence the one to follow.
			// It ignores the fact that we might have problems slotting into the approach traffic behind it - 
			// eventually we'll need some fancy algorithms for that!
		}
	}
	string trns = t->plane.callsign;
	trns += " Number ";
	trns += ConvertNumToSpokenDigits(i + a);
	// This assumes that the number spoken is landing position, not circuit position, since some of the traffic might be on straight-in final.
	trns += " ";
	TowerPlaneRec* tt = NULL;
	if((i == 1) && rwyList.empty() && (t->nextOnRwy) && (!cf)) {	// Unfortunately nextOnRwy currently doesn't handle circuit/straight-in ordering properly at present, hence the cf check below.
		trns += "Cleared to land";	// TODO - clear for the option if appropriate
		t->clearedToLand = true;
	} else if((i+a) > 1) {
		//First set tt to point to the correct preceding plane - final or circuit
		if(tc && tf) {
			tt = (tf->eta < tc->eta ? tf : tc);
		} else if(tc) {
			tt = tc;
		} else if(tf) {
			tt = tf;
		} else {
			// We should never get here!
			SG_LOG(SG_ATC, SG_ALERT, "ALERT - Logic error in FGTower::ProcessDownwindReport");
			return;
		}
		trns += "Follow the ";
		string s = tt->plane.callsign;
		int p = s.find('-');
		s = s.substr(0,p);
		trns += s;
		if((tt->opType) == CIRCUIT) {
			PatternLeg leg;
			if(tt->isUser) {
				leg = tt->leg;
			}
			if(leg == FINAL) {
				trns += " on final";
			} else if(leg == TURN4) {
				trns += " turning final";
			} else if(leg == BASE) {
				trns += " on base";
			} else if(leg == TURN3) {
				trns += " turning base";
			}
		} else {
			double miles_out = CalcDistOutMiles(tt);
			if(miles_out < 2) {
				trns += " on short final";
			} else {
				trns += " on ";
				trns += ConvertNumToSpokenDigits((int)miles_out);
				trns += " mile final";
			}
		}
	}
	if(_display) {
		pending_transmission = trns;
		Transmit();
	}
	if(t->isUser) {
		if(t->opType == TTT_UNKNOWN) t->opType = CIRCUIT;
		//cout << "ADD VACATED A\n";
		// Put going around at the top (and hence default) since that'll be more desperate,
		// or put rwy vacated at the top since that'll be more common?
		//cout << "ident = " << ident << ", adding go-around option\n";
		current_atcdialog->add_entry(ident, "@CS Going Around", "Report going around", TOWER, USER_REPORT_GOING_AROUND);
		current_atcdialog->add_entry(ident, "@CS Clear of the runway", "Report runway vacated", TOWER, USER_REPORT_RWY_VACATED);
	}
}

void FGTower::ProcessRunwayVacatedReport(TowerPlaneRec* t) {
	//cout << "Processing rwy vacated...\n";
	if(t->isUser) current_atcdialog->remove_entry(ident, USER_REPORT_GOING_AROUND, TOWER);
	string trns = t->plane.callsign;
	if(separateGround) {
		trns += " Contact ground on ";
		double f = globals->get_ATC_mgr()->GetFrequency(ident, GROUND) / 100.0;	
		char buf[10];
		sprintf(buf, "%.2f", f);
		trns += buf;
		trns += " Good Day";
	} else {
		// Cop-out!!
		trns += " cleared for taxi to general aviation parking";
	}
	//cout << "trns = " << trns << '\n';
	if(_display) {
		pending_transmission = trns;
		Transmit();
	}
	RemoveFromRwyList(t->plane.callsign);
	AddToVacatedList(t);
	// Maybe we should check that the plane really *has* vacated the runway!
}

// *********** END RESPONSE FUNCTIONS *****************

// Currently this assumes we *are* next on the runway and doesn't check for planes about to land - 
// this should be done prior to calling this function.
void FGTower::ClearHoldingPlane(TowerPlaneRec* t) {
	//cout << "Entering ClearHoldingPlane..." << endl;
	// Lets Roll !!!!
	string trns = t->plane.callsign;
	//if(departed plane < some threshold in time away) {
	if(0) {		// FIXME
	//if(timeSinceLastDeparture <= 60.0 && departed == true) {
		trns += " line up runway " + ConvertRwyNumToSpokenString(activeRwy);
		t->clearedToLineUp = true;
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
		departed = false;
		timeSinceLastDeparture = 0.0;
	} else {
	//} else if(timeSinceLastDeparture > 60.0 || departed == false) {	// Hack - test for timeSinceLastDeparture should be in lineup block eventually
		trns += " cleared for take-off";
		// TODO - add traffic is... ?
		t->clearedToTakeOff = true;
		departed = false;
		timeSinceLastDeparture = 0.0;
	}
	if(_display) {
		pending_transmission = trns;
		Transmit();
	}
	//cout << "Done ClearHoldingPlane " << endl;
}


// ***************************************************************************************
// ********** Functions to periodically check what the various traffic is doing **********

// Do one plane from the hold list
void FGTower::CheckHoldList(double dt) {
	//cout << "Entering CheckHoldList..." << endl;
	if(!holdList.empty()) {
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
					RemoveAllUserDialogOptions();
					current_atcdialog->add_entry(ident, "@CS Ready for take-off", "Request take-off clearance", TOWER, (int)USER_REQUEST_TAKE_OFF);
				} else if(timeSinceLastDeparture <= 60.0 && departed == true) {
					// Do nothing - this is a bit of a hack - should maybe do line up be ready here
				} else {
					ClearHoldingPlane(t);
					t->leg = TAKEOFF_ROLL;
					rwyList.push_back(t);
					rwyListItr = rwyList.begin();
					rwyOccupied = true;
					holdList.erase(holdListItr);
					holdListItr = holdList.begin();
					if (holdList.empty())
					  return;
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
	
	if(!circuitList.empty()) {	// Do one plane from the circuit
		if(circuitListItr == circuitList.end()) {
			circuitListItr = circuitList.begin();
		}
		TowerPlaneRec* t = *circuitListItr;
		//cout << ident <<  ' ' << circuitList.size() << ' ' << t->plane.callsign << " " << t->leg << " eta " << t->eta << '\n';
		if(t->isUser) {
			t->pos.setLongitudeDeg(user_lon_node->getDoubleValue());
			t->pos.setLatitudeDeg(user_lat_node->getDoubleValue());
			t->pos.setElevationM(user_elev_node->getDoubleValue());
			//cout << ident <<  ' ' << circuitList.size() << ' ' << t->plane.callsign << " " << t->leg << " eta " << t->eta << '\n';
		}
		SGVec3d tortho = ortho.ConvertToLocal(t->pos);
		if(t->isUser) {
			// Need to figure out which leg he's on
			//cout << "rwy.hdg = " << rwy.hdg << " user hdg = " << user_hdg_node->getDoubleValue();
			double ho = GetAngleDiff_deg(user_hdg_node->getDoubleValue(), rwy.hdg);
			//cout << " ho = " << ho << " abs(ho = " << abs(ho) << '\n';
			// TODO FIXME - get the wind and convert this to track, or otherwise use track somehow!!!
			// If it's gusty might need to filter the value, although we are leaving 30 degrees each way leeway!
			if(fabs(ho) < 30) {
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
					//cout << "tortho.y = " << tortho.y() << '\n';
					if(t->opType == TTT_UNKNOWN) {
						if(tortho.y() > 5000) {
							// 5 km out from threshold - assume it's a departure
							t->opType = OUTBOUND;	// TODO - could check if the user has climbed significantly above circuit altitude as well.
							// Since we are unknown operation we should be in depList already.
							//cout << ident << " Removing user from circuitList (TTT_UNKNOWN)\n";
							circuitListItr = circuitList.erase(circuitListItr);
							RemoveFromTrafficList(t->plane.callsign);
							if (circuitList.empty())
							    return;
						}
					} else if(t->opType == CIRCUIT) {
						if(tortho.y() > 10000) {
							// 10 km out - assume the user has abandoned the circuit!!
							t->opType = OUTBOUND;
							depList.push_back(t);
							depListItr = depList.begin();
							//cout << ident << " removing user from circuitList (CIRCUIT)\n";
							circuitListItr = circuitList.erase(circuitListItr);
							if (circuitList.empty())
							  return;
						}
					}
				}
			} else if(fabs(ho) < 60) {
				// turn1 or turn 4
				// TODO - either fix or doublecheck this hack by looking at heading and pattern direction
				if((t->leg == CLIMBOUT) || (t->leg == TURN1)) {
					t->leg = TURN1;
					//cout << "Turn1\n";
				} else {
					t->leg = TURN4;
					//cout << "Turn4\n";
				}
			} else if(fabs(ho) < 120) {
				// crosswind or base
				// TODO - either fix or doublecheck this hack by looking at heading and pattern direction
				if((t->leg == TURN1) || (t->leg == CROSSWIND)) {
					t->leg = CROSSWIND;
					//cout << "Crosswind\n";
				} else {
					t->leg = BASE;
					//cout << "Base\n";
				}
			} else if(fabs(ho) < 150) {
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
		
		if(t->leg == FINAL && !(t->instructedToGoAround)) {
			doThresholdETACalc();
			doThresholdUseOrder();
			/*
			if(t->isUser) {
				cout << "Checking USER on final... ";
				cout << "eta " << t->eta;
				if(t->clearedToLand) cout << " cleared to land\n";
			}
			*/
			//cout << "YES FINAL, t->eta = " << t->eta << ", rwyList.size() = " << rwyList.size() << '\n';
			if(t->landingType == FULL_STOP) {
				t->opType = INBOUND;
				//cout << "\n******** SWITCHING TO INBOUND AT POINT AAA *********\n\n";
			}
			if(t->eta < 12 && rwyList.size()) {
				// TODO - need to make this more sophisticated 
				// eg. is the plane accelerating down the runway taking off [OK],
				// or stationary near the start [V. BAD!!].
				// For now this should stop the AI plane landing on top of the user.
				tower_plane_rec_list_iterator twrItr;
				twrItr = rwyList.begin();
				TowerPlaneRec* tpr = *twrItr;
				if(strcmp(tpr->plane.callsign.c_str(), t->plane.callsign.c_str()) == 0
						&& rwyList.size() == 1) {
					// Fixing bug when ATC says that we must go around because of traffic on rwy
					// but that traffic is our plane! In future we can use this expression
					// for other ATC-messages like "On ground at 46, vacate left."

				} else {
					string trns = t->plane.callsign;
					trns += " GO AROUND TRAFFIC ON RUNWAY I REPEAT GO AROUND";
					pending_transmission = trns;
					ImmediateTransmit();
					t->instructedToGoAround = true;
					t->clearedToLand = false;
					// Assume it complies!!!
					t->opType = CIRCUIT;
					t->leg = CLIMBOUT;
				}
			} else if(!t->clearedToLand) {
				// The whip through the appList is a hack since currently t->nextOnRwy doesn't always work
				// TODO - fix this!
				bool cf = false;
				for(tower_plane_rec_list_iterator twrItr = appList.begin(); twrItr != appList.end(); twrItr++) {
					if((*twrItr)->eta < t->eta) {
						cf = true;
					}
				}
				if(t->nextOnRwy && !cf) {
					if(!rwyList.size()) {
						string trns = t->plane.callsign;
						trns += " Cleared to land";
						pending_transmission = trns;
						Transmit();
						//if(t->isUser) cout << "Transmitting cleared to Land!!!\n";
						t->clearedToLand = true;
					}
				} else {
					//if(t->isUser) cout << "Not next\n";
				}
			}
		} else if(t->leg == LANDING_ROLL) {
			//cout << t->plane.callsign << " has landed - adding to rwyList\n";
			rwyList.push_front(t);
			// TODO - if(!clearedToLand) shout something!!
			t->clearedToLand = false;
			RemoveFromTrafficList(t->plane.callsign);
			if(t->isUser) {
				t->opType = TTT_UNKNOWN;
			}	// TODO - allow the user to specify opType via ATC menu
			//cout << ident << " Removing " << t->plane.callsign << " from circuitList..." << endl;
			circuitListItr = circuitList.erase(circuitListItr);
			if(circuitListItr == circuitList.end() ) {
				circuitListItr = circuitList.begin();
				// avoid increment of circuitListItr (would increment to second element, or crash if no element left)
				return;
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
				t->pos.setLongitudeDeg(user_lon_node->getDoubleValue());
				t->pos.setLatitudeDeg(user_lat_node->getDoubleValue());
				t->pos.setElevationM(user_elev_node->getDoubleValue());
			}
			bool on_rwy = OnActiveRunway(t->pos);
			if(!on_rwy) {
				// TODO - for all of these we need to check what the user is *actually* doing!
				if((t->opType == INBOUND) || (t->opType == STRAIGHT_IN)) {
					//cout << "Tower " << ident << " is removing plane " << t->plane.callsign << " from rwy list (vacated)\n";
					//cout << "Size of rwylist was " << rwyList.size() << '\n';
					//cout << "Size of vacatedList was " << vacatedList.size() << '\n';
					RemoveFromRwyList(t->plane.callsign);
					AddToVacatedList(t);
					//cout << "Size of rwylist is " << rwyList.size() << '\n';
					//cout << "Size of vacatedList is " << vacatedList.size() << '\n';
					// At the moment we wait until Runway Vacated is reported by the plane before telling to contact ground etc.
					// It's possible we could be a bit more proactive about this.
				} else if(t->opType == OUTBOUND) {
					depList.push_back(t);
					depListItr = depList.begin();
					rwyList.pop_front();
					departed = true;
					timeSinceLastDeparture = 0.0;
				} else if(t->opType == CIRCUIT) {
					//cout << ident << " adding " << t->plane.callsign << " to circuitList" << endl;
					circuitList.push_back(t);
					circuitListItr = circuitList.begin();
					AddToTrafficList(t);
					rwyList.pop_front();
					departed = true;
					timeSinceLastDeparture = 0.0;
				} else if(t->opType == TTT_UNKNOWN) {
					depList.push_back(t);
					depListItr = depList.begin();
					//cout << ident << " adding " << t->plane.callsign << " to circuitList" << endl;
					circuitList.push_back(t);
					circuitListItr = circuitList.begin();
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
	//cout << "CheckApproachList called for " << ident << endl;
	//cout << "AppList.size is " << appList.size() << endl;
	if(!appList.empty()) {
		if(appListItr == appList.end()) {
			appListItr = appList.begin();
		}
		TowerPlaneRec* t = *appListItr;
		//cout << "t = " << t << endl;
		//cout << "Checking " << t->plane.callsign << endl;
		if(t->isUser) {
			t->pos.setLongitudeDeg(user_lon_node->getDoubleValue());
			t->pos.setLatitudeDeg(user_lat_node->getDoubleValue());
			t->pos.setElevationM(user_elev_node->getDoubleValue());
		}
		doThresholdETACalc();	// We need this here because planes in the lists are not guaranteed to *always* have the correct ETA
		//cout << "eta is " << t->eta << ", rwy is " << (rwyList.size() ? "occupied " : "clear ") << '\n';
		SGVec3d tortho = ortho.ConvertToLocal(t->pos);
		if(t->eta < 12 && rwyList.size() && !(t->instructedToGoAround)) {
			// TODO - need to make this more sophisticated 
			// eg. is the plane accelerating down the runway taking off [OK],
			// or stationary near the start [V. BAD!!].
			// For now this should stop the AI plane landing on top of the user.
			tower_plane_rec_list_iterator twrItr;
			twrItr = rwyList.begin();
			TowerPlaneRec* tpr = *twrItr;
			if(strcmp ( tpr->plane.callsign.c_str(), t->plane.callsign.c_str() ) == 0 && rwyList.size() == 1) {
					// Fixing bug when ATC says that we must go around because of traffic on rwy
					// but that traffic is we! In future we can use this expression
					// for other ATC-messages like "On ground at 46, vacate left."

			} else {
				string trns = t->plane.callsign;
				trns += " GO AROUND TRAFFIC ON RUNWAY I REPEAT GO AROUND";
				pending_transmission = trns;
				ImmediateTransmit();
				t->instructedToGoAround = true;
				t->clearedToLand = false;
				t->nextOnRwy = false;	// But note this is recalculated so don't rely on it
				// Assume it complies!!!
				t->opType = CIRCUIT;
				t->leg = CLIMBOUT;
				if(t->isUser) {
					// TODO - add Go-around ack to comm options,
					// remove report rwy vacated. (possibly).
				}
			}
		} else if(t->isUser && t->eta < 90 && tortho.y() > -2500 && t->clearedToLand && t->gearUpReported == false) {
			// Check if gear up or down
			double gp = fgGetFloat("/gear/gear/position-norm");
			if(gp < 1) {
				string trnsm = t->plane.callsign;
				sg_srandom_time();
				int rnd = int(sg_random() * 2) + 1;
				if(rnd == 2) {				// Random message for more realistic ATC ;)
					trnsm += ", LANDING GEAR APPEARS UP!";
				} else {
					trnsm += ", Check wheels down and locked.";
				}
				pending_transmission = trnsm;
				ImmediateTransmit();
				t->gearUpReported = true;
			}
		} else if(t->eta < 90 && !t->clearedToLand) {
			//doThresholdETACalc();
			doThresholdUseOrder();
			// The whip through the appList is a hack since currently t->nextOnRwy doesn't always work
			// TODO - fix this!
			bool cf = false;
			for(tower_plane_rec_list_iterator twrItr = appList.begin(); twrItr != appList.end(); twrItr++) {
				if((*twrItr)->eta < t->eta) {
					cf = true;
				}
			}
			if(t->nextOnRwy && !cf) {
				if(!rwyList.size()) {
					string trns = t->plane.callsign;
					trns += " Cleared to land";
					pending_transmission = trns;
					Transmit();
					//if(t->isUser) cout << "Transmitting cleared to Land!!!\n";
					t->clearedToLand = true;
				}
			} else {
				//if(t->isUser) cout << "Not next\n";
			}
		}
		
		// Check for landing...
		bool landed = false;
		if(t->isUser) {
			if(OnActiveRunway(t->pos)) {
				landed = true;
			}
		}
		
		if(landed) {
			// Duplicated in CheckCircuitList - must be able to rationalise this somehow!
			//cout << "A " << t->plane.callsign << " has landed, adding to rwyList...\n";
			rwyList.push_front(t);
			// TODO - if(!clearedToLand) shout something!!
			t->clearedToLand = false;
			RemoveFromTrafficList(t->plane.callsign);
			//if(t->isUser) {
				//	t->opType = TTT_UNKNOWN;
			//}	// TODO - allow the user to specify opType via ATC menu
			appListItr = appList.erase(appListItr);
			if(appListItr == appList.end() ) {
				appListItr = appList.begin();
			}
			if (appList.empty())
			  return;

		}
		
		++appListItr;
	}
	//cout << "Done" << endl;
}

// Do one plane from the departure list
void FGTower::CheckDepartureList(double dt) {
	if(!depList.empty()) {
		if(depListItr == depList.end()) {
			depListItr = depList.begin();
		}
		TowerPlaneRec* t = *depListItr;
		//cout << "Dep list, checking " << t->plane.callsign;
		
		double distout;	// meters
		if(t->isUser) {
			distout = dclGetHorizontalSeparation(_geod, 
				SGGeod::fromDegM(user_lon_node->getDoubleValue(), user_lat_node->getDoubleValue(), user_elev_node->getDoubleValue()));
		}
		//cout << " distout = " << distout << '\n';
		if(t->isUser && !(t->clearedToTakeOff)) {	// HACK - we use clearedToTakeOff to check if ATC already contacted with plane (and cleared take-off) or not
			if(!OnAnyRunway(SGGeod::fromDegM(user_lon_node->getDoubleValue(), user_lat_node->getDoubleValue(), 0.0), false)) {
				current_atcdialog->remove_entry(ident, USER_REQUEST_TAKE_OFF, TOWER);
				t->clearedToTakeOff = true;	// FIXME
			}
		}
		if(distout > 10000) {
			string trns = t->plane.callsign;
			trns += " You are now clear of my airspace, good day";
			pending_transmission = trns;
			Transmit();
			if(t->isUser) {
				// Change the communication options
				RemoveAllUserDialogOptions();
				//cout << "ADD A\n";
				current_atcdialog->add_entry(ident, "@AP Tower, @CS @MI miles @CD of the airport for full stop@AT", "Contact tower for VFR arrival (full stop)", TOWER, (int)USER_REQUEST_VFR_ARRIVAL_FULL_STOP);
			}
			RemovePlane(t->plane.callsign);
		} else {
			++depListItr;
		}
	}
}

// ********** End periodic check functions ***********************************************
// ***************************************************************************************


// Remove all dialog options for this tower.
void FGTower::RemoveAllUserDialogOptions() {
	current_atcdialog->remove_entry(ident, USER_REQUEST_VFR_DEPARTURE, TOWER);
	current_atcdialog->remove_entry(ident, USER_REQUEST_VFR_ARRIVAL, TOWER);
	current_atcdialog->remove_entry(ident, USER_REQUEST_VFR_ARRIVAL_FULL_STOP, TOWER);
	current_atcdialog->remove_entry(ident, USER_REQUEST_VFR_ARRIVAL_TOUCH_AND_GO, TOWER);
	current_atcdialog->remove_entry(ident, USER_REPORT_3_MILE_FINAL, TOWER);
	current_atcdialog->remove_entry(ident, USER_REPORT_DOWNWIND, TOWER);
	current_atcdialog->remove_entry(ident, USER_REPORT_RWY_VACATED, TOWER);
	current_atcdialog->remove_entry(ident, USER_REPORT_GOING_AROUND, TOWER);	
	current_atcdialog->remove_entry(ident, USER_REQUEST_TAKE_OFF, TOWER);
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
	if(fabs(downwind_leg_pos) > nominal_downwind_leg_pos) {
		dpos = downwind_leg_pos;
		return(true);
	} else {
		dpos = 0.0;
		return(false);
	}
}
bool FGTower::GetBaseConstraint(double& bpos) {
	if(base_leg_pos < nominal_base_leg_pos) {
		bpos = base_leg_pos;
		return(true);
	} else {
		bpos = nominal_base_leg_pos;
		return(false);
	}
}


// Figure out which runways are active.
// For now we'll just be simple and do one active runway - eventually this will get much more complex
// This is a private function - public interface to the results of this is through GetActiveRunway
void FGTower::DoRwyDetails() {
	//cout << "GetRwyDetails called" << endl;
	
	// Based on the airport-id and wind get the active runway
	
  const FGAirport* apt = fgFindAirportID(ident);
  if (!apt) {
    SG_LOG(SG_ATC, SG_WARN, "FGTower::DoRwyDetails: unknown ICAO:" << ident);
    return;
  }
  
	FGRunway* runway = apt->getActiveRunwayForUsage();

  activeRwy = runway->ident();
  rwy.rwyID = runway->ident();
  SG_LOG(SG_ATC, SG_INFO, "In FGGround, active runway for airport " << ident << " is " << activeRwy);
  
  // Get the threshold position
  double other_way = runway->headingDeg() - 180.0;
  while(other_way <= 0.0) {
    other_way += 360.0;
  }
    // move to the +l end/center of the runway
  //cout << "Runway center is at " << runway._lon << ", " << runway._lat << '\n';
  double tshlon = 0.0, tshlat = 0.0, tshr = 0.0;
  double tolon = 0.0, tolat = 0.0, tor = 0.0;
  rwy.length = runway->lengthM();
  geo_direct_wgs_84 ( aptElev, runway->latitude(), runway->longitude(), other_way, 
                      rwy.length / 2.0 - 25.0, &tshlat, &tshlon, &tshr );
  geo_direct_wgs_84 ( aptElev, runway->latitude(), runway->longitude(), runway->headingDeg(), 
                      rwy.length / 2.0 - 25.0, &tolat, &tolon, &tor );
  
  // Note - 25 meters in from the runway end is a bit of a hack to put the plane ahead of the user.
  // now copy what we need out of runway into rwy
  rwy.threshold_pos = SGGeod::fromDegM(tshlon, tshlat, aptElev);
  SGGeod takeoff_end = SGGeod::fromDegM(tolon, tolat, aptElev);
  //cout << "Threshold position = " << tshlon << ", " << tshlat << ", " << aptElev << '\n';
  //cout << "Takeoff position = " << tolon << ", " << tolat << ", " << aptElev << '\n';
  rwy.hdg = runway->headingDeg();
  // Set the projection for the local area based on this active runway
  ortho.Init(rwy.threshold_pos, rwy.hdg);	
  rwy.end1ortho = ortho.ConvertToLocal(rwy.threshold_pos);	// should come out as zero
  rwy.end2ortho = ortho.ConvertToLocal(takeoff_end);
  
  // Set the pattern direction
  // TODO - we'll check for a facilities file with this in eventually - for now assume left traffic except
  // for certain circumstances (RH parallel rwy).
  rwy.patternDirection = -1;		// Left
  if(rwy.rwyID.size() == 3) {
    rwy.patternDirection = (rwy.rwyID.substr(2,1) == "R" ? 1 : -1);
  }
  //cout << "Doing details, rwy.patterDirection is " << rwy.patternDirection << '\n';
}


// Figure out if a given position lies on the active runway
// Might have to change when we consider more than one active rwy.
bool FGTower::OnActiveRunway(const SGGeod& pt) {
	// TODO - check that the centre calculation below isn't confused by displaced thesholds etc.
	SGVec3d xyc((rwy.end1ortho.x() + rwy.end2ortho.x())/2.0, (rwy.end1ortho.y() + rwy.end2ortho.y())/2.0, 0.0);
	SGVec3d xyp = ortho.ConvertToLocal(pt);
	
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
bool FGTower::OnAnyRunway(const SGGeod& pt, bool onGround) {
	ATCData ad;
	double dist = current_commlist->FindClosest(_geod, ad, TOWER, 7.0);
	if(dist < 0.0) {
		return(false);
	}
	
	// Based on the airport-id, go through all the runways and check for a point in them

  const FGAirport* apt = fgFindAirportID(ad.ident);
  assert(apt);
  
  for (unsigned int i=0; i<apt->numRunways(); ++i) {
    if (OnRunway(pt, apt->getRunwayByIndex(i))) {
      return true;
    }
  }

  // if onGround is true, we only match real runways, so we're done
  if (onGround) return false;

  // try taxiways as well
  for (unsigned int i=0; i<apt->numTaxiways(); ++i) {
    if (OnRunway(pt, apt->getTaxiwayByIndex(i))) {
      return true;
    }
  }
  
	return false;
}


// Returns true if successful
bool FGTower::RemoveFromTrafficList(const string& id) {
	tower_plane_rec_list_iterator twrItr;
	for(twrItr = trafficList.begin(); twrItr != trafficList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		if(tpr->plane.callsign == id) {
			trafficList.erase(twrItr);
			trafficListItr = trafficList.begin();
			return(true);
		}
	}	
	SG_LOG(SG_ATC, SG_WARN, "Warning - unable to remove aircraft " << id << " from trafficList in FGTower");
	return(false);
}


// Returns true if successful
bool FGTower::RemoveFromAppList(const string& id) {
	tower_plane_rec_list_iterator twrItr;
	for(twrItr = appList.begin(); twrItr != appList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		if(tpr->plane.callsign == id) {
			appList.erase(twrItr);
			appListItr = appList.begin();
			return(true);
		}
	}	
	//SG_LOG(SG_ATC, SG_WARN, "Warning - unable to remove aircraft " << id << " from appList in FGTower");
	return(false);
}

// Returns true if successful
bool FGTower::RemoveFromRwyList(const string& id) {
	tower_plane_rec_list_iterator twrItr;
	for(twrItr = rwyList.begin(); twrItr != rwyList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		if(tpr->plane.callsign == id) {
			rwyList.erase(twrItr);
			rwyListItr = rwyList.begin();
			return(true);
		}
	}	
	//SG_LOG(SG_ATC, SG_WARN, "Warning - unable to remove aircraft " << id << " from rwyList in FGTower");
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
	} else {
		t->nextOnRwy = false;
	}
	trafficList.push_back(t);
	//cout << "\tE\t" << trafficList.size() << endl;
	return(holding ? firstTime : conflict);
}

// Add a tower plane rec with ETA to the circuit list in the correct position ETA-wise
// Returns true if this might cause a separation conflict (based on ETA) with other traffic, false otherwise.
// Safe to add a plane that is already in - planes with the same callsign are not added.
bool FGTower::AddToCircuitList(TowerPlaneRec* t) {
	if(!t) {
		//cout << "**********************************************\n";
		//cout << "AddToCircuitList called with NULL pointer!!!!!\n";
		//cout << "**********************************************\n";
		return false;
	}
	//cout << "ADD: " << circuitList.size();
	//cout << ident << " AddToCircuitList called for " << t->plane.callsign << ", currently size = " << circuitList.size() << endl;
	double separation_time = 60.0;	// seconds - this is currently a guess for light plane separation, and includes a few seconds for a holding plane to taxi onto the rwy.
	bool conflict = false;
	tower_plane_rec_list_iterator twrItr;
	// First check if the plane is already in the list
	//cout << "A" << endl;
	//cout << "Checking whether " << t->plane.callsign << " is already in circuit list..." << endl;
	//cout << "B" << endl;
	for(twrItr = circuitList.begin(); twrItr != circuitList.end(); twrItr++) {
		if((*twrItr)->plane.callsign == t->plane.callsign) {
			//cout << "In list - returning...\n";
			return false;
		}
	}
	//cout << "Not in list - adding..." << endl;
	
	for(twrItr = circuitList.begin(); twrItr != circuitList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		//cout << tpr->plane.callsign << " eta is " << tpr->eta << '\n';
		//cout << "New eta is " << t->eta << '\n';		
		if(t->eta < tpr->eta) {
			// Ugg - this one's tricky.
			// It depends on what the two planes are doing and whether there's a conflict what we do.
			if(tpr->eta - t->eta > separation_time) {	// No probs, plane 2 can squeeze in before plane 1 with no apparent conflict
				circuitList.insert(twrItr, t);
				circuitListItr = circuitList.begin();
			} else {	// Ooops - this ones tricky - we have a potential conflict!
				conflict = true;
				// HACK - just add anyway for now and flag conflict.
				circuitList.insert(twrItr, t);
				circuitListItr = circuitList.begin();
			}
			//cout << "\tC\t" << circuitList.size() << '\n';
			return(conflict);
		}
	}
	// If we get here we must be at the end of the list, or maybe the list is empty.
	//cout << ident << " adding " << t->plane.callsign << " to circuitList" << endl;
	circuitList.push_back(t);	// TODO - check the separation with the preceding plane for the conflict flag.
	circuitListItr = circuitList.begin();
	//cout << "\tE\t" << circuitList.size() << endl;
	return(conflict);
}

// Add to vacated list only if not already present
void FGTower::AddToVacatedList(TowerPlaneRec* t) {
	tower_plane_rec_list_iterator twrItr;
	bool found = false;
	for(twrItr = vacatedList.begin(); twrItr != vacatedList.end(); twrItr++) {
		if((*twrItr)->plane.callsign == t->plane.callsign) {
			found = true;
		}
	}
	if(found) return;
	vacatedList.push_back(t);
}

void FGTower::AddToHoldingList(TowerPlaneRec* t) {
	tower_plane_rec_list_iterator it, end = holdList.end();
	for (it = holdList.begin(); it != end; ++it) {
		if ((*it)->plane.callsign == t->plane.callsign)
			return;
	
		holdList.push_back(t);
	}
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
	
	SGVec3d op = ortho.ConvertToLocal(tpr->pos);
	//if(printout) {
	//if(!tpr->isUser) cout << "Orthopos is " << op.x() << ", " << op.y() << ' ';
	//cout << "opType is " << tpr->opType << '\n';
	//}
	double dist_out_m = op.y();
	double dist_across_m = fabs(op.x());	// The fabs is a hack to cope with the fact that we don't know the circuit direction yet
	//cout << "Doing ETA calc for " << tpr->plane.callsign << '\n';
	
	if(tpr->opType == STRAIGHT_IN || tpr->opType == INBOUND) {
		//cout << "CASE 1\n";
		double dist_to_go_m = sqrt((dist_out_m * dist_out_m) + (dist_across_m * dist_across_m));
		if(dist_to_go_m < 1000) {
			tpr->eta = dist_to_go_m / final_ias;
		} else {
			tpr->eta = (1000.0 / final_ias) + ((dist_to_go_m - 1000.0) / app_ias);
		}
	} else if(tpr->opType == CIRCUIT || tpr->opType == TTT_UNKNOWN) {	// Hack alert - UNKNOWN has sort of been added here as a temporary hack.
		//cout << "CASE 2\n";
		// It's complicated - depends on if base leg is delayed or not
		//if(printout) {
		//cout << "Leg = " << tpr->leg << '\n';
		//}
		if(tpr->leg == LANDING_ROLL) {
			tpr->eta = 0;
		} else if((tpr->leg == FINAL) || (tpr->leg == TURN4)) {
			//cout << "dist_out_m = " << dist_out_m << '\n';
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
			//cout << "current_base_dist_out_m = " << current_base_dist_out_m << '\n';
			double nominal_dist_across_m = 1000;	// Hardwired value from AILocalTraffic
			double current_dist_across_m;
			if(!GetDownwindConstraint(current_dist_across_m)) {
				current_dist_across_m = nominal_dist_across_m;
			}
			double nominal_cross_dist_out_m = 2000;	// Bit of a guess - AI plane turns to crosswind at 700ft agl.
			tpr->eta = fabs(current_base_dist_out_m) / final_ias;	// final
			//cout << "a = " << tpr->eta << '\n';
			if((tpr->leg == DOWNWIND) || (tpr->leg == TURN2)) {
				tpr->eta += dist_across_m / circuit_ias;
				//cout << "b = " << tpr->eta << '\n';
				tpr->eta += fabs(current_base_dist_out_m - dist_out_m) / circuit_ias;
				//cout << "c = " << tpr->eta << '\n';
			} else if((tpr->leg == CROSSWIND) || (tpr->leg == TURN1)) {
				//cout << "CROSSWIND calc: ";
				//cout << tpr->eta << ' ';
				if(dist_across_m > nominal_dist_across_m) {
					tpr->eta += dist_across_m / circuit_ias;
					//cout << "a ";
				} else {
					tpr->eta += nominal_dist_across_m / circuit_ias;
					//cout << "b ";
				}
				//cout << tpr->eta << ' ';
				// should we use the dist across of the previous plane if there is previous still on downwind?
				//if(printout) cout << "bb = " << tpr->eta << '\n';
				if(dist_out_m > nominal_cross_dist_out_m) {
					tpr->eta += fabs(current_base_dist_out_m - dist_out_m) / circuit_ias;
					//cout << "c ";
				} else {
					tpr->eta += fabs(current_base_dist_out_m - nominal_cross_dist_out_m) / circuit_ias;
					//cout << "d ";
				}
				//cout << tpr->eta << ' ';
				//if(printout) cout << "cc = " << tpr->eta << '\n';
				if(nominal_dist_across_m > dist_across_m) {
					tpr->eta += (nominal_dist_across_m - dist_across_m) / circuit_ias;
					//cout << "e ";
				} else {
					// Nothing to add
					//cout << "f ";
				}
				//cout << tpr->eta << '\n';
				//if(printout) cout << "dd = " << tpr->eta << '\n';
			} else {
				// We've only just started - why not use a generic estimate?
				tpr->eta = 240.0;
			}
		}
		//if(printout) {
		//	cout << "ETA = " << tpr->eta << '\n';
		//}
		//if(!tpr->isUser) cout << tpr->plane.callsign << '\t' << tpr->eta << '\n';
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


// Iterate through all the lists, update the position of, and call CalcETA for all the planes.
void FGTower::doThresholdETACalc() {
	//cout << "Entering doThresholdETACalc..." << endl;
	tower_plane_rec_list_iterator twrItr;
	// Do the approach list first
	for(twrItr = appList.begin(); twrItr != appList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		CalcETA(tpr);
	}	
	// Then the circuit list
	//cout << "Circuit list size is " << circuitList.size() << '\n';
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
	//if(ident == "KRHV") cout << "A" << flush;
	for(twrItr = appList.begin(); twrItr != appList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		//if(ident == "KRHV") cout << tpr->plane.callsign << '\n';
		conflict = AddToTrafficList(tpr);
	}	
	// Then the circuit list
	//if(ident == "KRHV") cout << "C" << flush;
	for(twrItr = circuitList.begin(); twrItr != circuitList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		//if(ident == "KRHV") cout << tpr->plane.callsign << '\n';
		conflict = AddToTrafficList(tpr);
	}
	// And finally the hold list
	//cout << "H" << endl;
	for(twrItr = holdList.begin(); twrItr != holdList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		AddToTrafficList(tpr, true);
	}
	
	
	if(0) {
	//if(ident == "KRHV") {
		cout << "T\n";
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

// Contact tower for VFR approach
// eg "Cessna Charlie Foxtrot Golf Foxtrot Sierra eight miles South of the airport for full stop with Bravo"
// This function probably only called via user interaction - AI planes will have an overloaded function taking a planerec.
// opt defaults to AIP_LT_UNKNOWN
void FGTower::VFRArrivalContact(const string& ID, const LandingType& opt) {
	//cout << "USER Request Landing Clearance called for ID " << ID << '\n';
	
	// For now we'll assume that the user is a light plane and can get him/her to join the circuit if necessary.

	TowerPlaneRec* t;	
	string usercall = fgGetString("/sim/user/callsign");
	if(ID == "USER" || ID == usercall) {
		t = FindPlane(usercall);
		if(!t) {
			//cout << "NOT t\n";
			t = new TowerPlaneRec;
			t->isUser = true;
			t->pos.setLongitudeDeg(user_lon_node->getDoubleValue());
			t->pos.setLatitudeDeg(user_lat_node->getDoubleValue());
			t->pos.setElevationM(user_elev_node->getDoubleValue());
		} else {
			//cout << "IS t\n";
			// Oops - the plane is already registered with this tower - maybe we took off and flew a giant circuit without
			// quite getting out of tower airspace - just ignore for now and treat as new arrival.
			// TODO - Maybe should remove from departure and circuit list if in there though!!
		}
	} else {
		// Oops - something has gone wrong - put out a warning
		cout << "WARNING - FGTower::VFRContact(string ID, LandingType lt) called with ID " << ID << " which does not appear to be the user.\n";
		return;
	}
		
	
	// TODO
	// Calculate where the plane is in relation to the active runway and it's circuit
	// and set the op-type as appropriate.
	
	// HACK - to get up and running I'm going to assume that the user contacts tower on a staight-in final for now.
	t->opType = STRAIGHT_IN;
	
	t->plane.type = GA_SINGLE;	// FIXME - Another assumption!
	t->plane.callsign = usercall;
	CalcETA(t);
	
	t->vfrArrivalReported = true;
	responseReqd = true;
	
	appList.push_back(t);	// Not necessarily permanent
	appListItr = appList.begin();
	AddToTrafficList(t);
	
	current_atcdialog->remove_entry(ident, USER_REQUEST_VFR_ARRIVAL, TOWER);
	current_atcdialog->remove_entry(ident, USER_REQUEST_VFR_ARRIVAL_FULL_STOP, TOWER);
	current_atcdialog->remove_entry(ident, USER_REQUEST_VFR_ARRIVAL_TOUCH_AND_GO, TOWER);
}

void FGTower::RequestDepartureClearance(const string& ID) {
	//cout << "Request Departure Clearance called...\n";
}

void FGTower::RequestTakeOffClearance(const string& ID) {
	string uid=ID;
	if(ID == "USER") {
		uid = fgGetString("/sim/user/callsign");
		current_atcdialog->remove_entry(ident, USER_REQUEST_TAKE_OFF, TOWER);
	}
	TowerPlaneRec* t = FindPlane(uid);
	if(t) {
		if(!(t->clearedToTakeOff)) {
			departed = false;
			t->lineUpReported=true;
			responseReqd = true;
		}
	}
	else {
		SG_LOG(SG_ATC, SG_WARN, "WARNING: Unable to find plane " << ID << " in FGTower::RequestTakeOffClearance(...)");
	}
}
	
void FGTower::ReportFinal(const string& ID) {
	//cout << "Report Final Called at tower " << ident << " by plane " << ID << '\n';
	string uid=ID;
	if(ID == "USER") {
		uid = fgGetString("/sim/user/callsign");
		current_atcdialog->remove_entry(ident, USER_REPORT_3_MILE_FINAL, TOWER);
	}
	TowerPlaneRec* t = FindPlane(uid);
	if(t) {
		t->finalReported = true;
		t->finalAcknowledged = false;
		if(!(t->clearedToLand)) {
			responseReqd = true;
		} else {
			// possibly respond with wind even if already cleared to land?
			t->finalReported = false;
			t->finalAcknowledged = true;
			// HACK!! - prevents next reporting being misinterpreted as this one.
		}
	} else {
		SG_LOG(SG_ATC, SG_WARN, "WARNING: Unable to find plane " << ID << " in FGTower::ReportFinal(...)");
	}
}

void FGTower::ReportLongFinal(const string& ID) {
	string uid=ID;
	if(ID == "USER") {
		uid = fgGetString("/sim/user/callsign");
		current_atcdialog->remove_entry(ident, USER_REPORT_3_MILE_FINAL, TOWER);
	}
	TowerPlaneRec* t = FindPlane(uid);
	if(t) {
		t->longFinalReported = true;
		t->longFinalAcknowledged = false;
		if(!(t->clearedToLand)) {
			responseReqd = true;
		} // possibly respond with wind even if already cleared to land?
	} else {
		SG_LOG(SG_ATC, SG_WARN, "WARNING: Unable to find plane " << ID << " in FGTower::ReportLongFinal(...)");
	}
}

//void FGTower::ReportOuterMarker(string ID);
//void FGTower::ReportMiddleMarker(string ID);
//void FGTower::ReportInnerMarker(string ID);

void FGTower::ReportRunwayVacated(const string& ID) {
	//cout << "Report Runway Vacated Called at tower " << ident << " by plane " << ID << '\n';
	string uid=ID;
	if(ID == "USER") {
		uid = fgGetString("/sim/user/callsign");
		current_atcdialog->remove_entry(ident, USER_REPORT_RWY_VACATED, TOWER);
	}
	TowerPlaneRec* t = FindPlane(uid);
	if(t) {
		//cout << "Found it...\n";
		t->rwyVacatedReported = true;
		responseReqd = true;
	} else {
		SG_LOG(SG_ATC, SG_WARN, "WARNING: Unable to find plane " << ID << " in FGTower::ReportRunwayVacated(...)");
		SG_LOG(SG_ATC, SG_ALERT, "A WARNING: Unable to find plane " << ID << " in FGTower::ReportRunwayVacated(...)");
		//cout << "WARNING: Unable to find plane " << ID << " in FGTower::ReportRunwayVacated(...)\n";
	}
}

TowerPlaneRec* FGTower::FindPlane(const string& ID) {
	//cout << "FindPlane called for " << ID << "...\n";
	tower_plane_rec_list_iterator twrItr;
	// Do the approach list first
	for(twrItr = appList.begin(); twrItr != appList.end(); twrItr++) {
		//cout << "appList callsign is " << (*twrItr)->plane.callsign << '\n';
		if((*twrItr)->plane.callsign == ID) return(*twrItr);
	}	
	// Then the circuit list
	for(twrItr = circuitList.begin(); twrItr != circuitList.end(); twrItr++) {
		//cout << "circuitList callsign is " << (*twrItr)->plane.callsign << '\n';
		if((*twrItr)->plane.callsign == ID) return(*twrItr);
	}
	// Then the runway list
	//cout << "rwyList.size() is " << rwyList.size() << '\n';
	for(twrItr = rwyList.begin(); twrItr != rwyList.end(); twrItr++) {
		//cout << "rwyList callsign is " << (*twrItr)->plane.callsign << '\n';
		if((*twrItr)->plane.callsign == ID) return(*twrItr);
	}
	// The hold list
	for(twrItr = holdList.begin(); twrItr != holdList.end(); twrItr++) {
		if((*twrItr)->plane.callsign == ID) return(*twrItr);
	}
	// And finally the vacated list
	for(twrItr = vacatedList.begin(); twrItr != vacatedList.end(); twrItr++) {
		//cout << "vacatedList callsign is " << (*twrItr)->plane.callsign << '\n';
		if((*twrItr)->plane.callsign == ID) return(*twrItr);
	}
	SG_LOG(SG_ATC, SG_WARN, "Unable to find " << ID << " in FGTower::FindPlane(...)");
	//exit(-1);
	return(NULL);
}

void FGTower::RemovePlane(const string& ID) {
	//cout << ident << " RemovePlane called for " << ID << '\n';
	// We have to be careful here - we want to erase the plane from all lists it is in,
	// but we can only delete it once, AT THE END.
	TowerPlaneRec* t = NULL;
	tower_plane_rec_list_iterator twrItr;
	for(twrItr = appList.begin(); twrItr != appList.end();) {
		if((*twrItr)->plane.callsign == ID) {
			t = *twrItr;
			twrItr = appList.erase(twrItr);
			appListItr = appList.begin();
			// HACK: aircraft are sometimes more than once in a list, so we need to
			// remove them all before we can delete the TowerPlaneRec class
			//break;
		} else
                        ++twrItr;
	}
	for(twrItr = depList.begin(); twrItr != depList.end();) {
		if((*twrItr)->plane.callsign == ID) {
			t = *twrItr;
			twrItr = depList.erase(twrItr);
			depListItr = depList.begin();
		} else
                        ++twrItr;
	}
	for(twrItr = circuitList.begin(); twrItr != circuitList.end();) {
		if((*twrItr)->plane.callsign == ID) {
			t = *twrItr;
			twrItr = circuitList.erase(twrItr);
			circuitListItr = circuitList.begin();
                } else
                        ++twrItr;
	}
	for(twrItr = holdList.begin(); twrItr != holdList.end();) {
		if((*twrItr)->plane.callsign == ID) {
			t = *twrItr;
			twrItr = holdList.erase(twrItr);
			holdListItr = holdList.begin();
                } else
                        ++twrItr;
	}
	for(twrItr = rwyList.begin(); twrItr != rwyList.end();) {
		if((*twrItr)->plane.callsign == ID) {
			t = *twrItr;
			twrItr = rwyList.erase(twrItr);
			rwyListItr = rwyList.begin();
                } else
                        ++twrItr;
	}
	for(twrItr = vacatedList.begin(); twrItr != vacatedList.end();) {
		if((*twrItr)->plane.callsign == ID) {
			t = *twrItr;
			twrItr = vacatedList.erase(twrItr);
			vacatedListItr = vacatedList.begin();
                } else
                        ++twrItr;
	}
	for(twrItr = trafficList.begin(); twrItr != trafficList.end();) {
		if((*twrItr)->plane.callsign == ID) {
			t = *twrItr;
			twrItr = trafficList.erase(twrItr);
			trafficListItr = trafficList.begin();
                } else
                        ++twrItr;
	}
	// And finally, delete the record.
	delete t;
}

void FGTower::ReportDownwind(const string& ID) {
	//cout << "ReportDownwind(...) called\n";
	string uid=ID;
	if(ID == "USER") {
		uid = fgGetString("/sim/user/callsign");
		current_atcdialog->remove_entry(ident, USER_REPORT_DOWNWIND, TOWER);
	}
	TowerPlaneRec* t = FindPlane(uid);
	if(t) {
		t->downwindReported = true;
		responseReqd = true;
		// If the plane is in the app list, remove it and put it in the circuit list instead.
		// Ideally we might want to do this at the 2 mile report prior to 45 deg entry, but at
		// the moment that would b&gg?r up the constraint position calculations.
		RemoveFromAppList(ID);
		t->leg = DOWNWIND;
		if(t->isUser) {
			t->pos.setLongitudeDeg(user_lon_node->getDoubleValue());
			t->pos.setLatitudeDeg(user_lat_node->getDoubleValue());
			t->pos.setElevationM(user_elev_node->getDoubleValue());
		}
		CalcETA(t);
		AddToCircuitList(t);
	} else {
		SG_LOG(SG_ATC, SG_WARN, "WARNING: Unable to find plane " << ID << " in FGTower::ReportDownwind(...)");
	}
}

void FGTower::ReportGoingAround(const string& ID) {
	string uid=ID;
	if(ID == "USER") {
		uid = fgGetString("/sim/user/callsign");
		RemoveAllUserDialogOptions();	// TODO - it would be much more efficient if ATCDialog simply had a clear() function!!!
		current_atcdialog->add_entry(ident, "@AP Tower, @CS Downwind @RW", "Report Downwind", TOWER, (int)USER_REPORT_DOWNWIND);
	}
	TowerPlaneRec* t = FindPlane(uid);
	if(t) {
		//t->goAroundReported = true;  // No need to set this until we start responding to it.
		responseReqd = false;	// might change in the future but for now we'll not distract them during the go-around.
		// If the plane is in the app list, remove it and put it in the circuit list instead.
		RemoveFromAppList(ID);
		t->leg = CLIMBOUT;
		if(t->isUser) {
			t->pos.setLongitudeDeg(user_lon_node->getDoubleValue());
			t->pos.setLatitudeDeg(user_lat_node->getDoubleValue());
			t->pos.setElevationM(user_elev_node->getDoubleValue());
		}
		CalcETA(t);
		AddToCircuitList(t);
	} else {
		SG_LOG(SG_ATC, SG_WARN, "WARNING: Unable to find plane " << ID << " in FGTower::ReportDownwind(...)");
	}
}

string FGTower::GenText(const string& m, int c) {
	const int cmax = 300;
	//string message;
	char tag[4];
	char crej = '@';
	char mes[cmax];
	char dum[cmax];
	//char buf[10];
	char *pos;
	int len;
	//FGTransmission t;
	string usercall = fgGetString("/sim/user/callsign");
	TowerPlaneRec* t = FindPlane(responseID);
	
	//transmission_list_type     tmissions = transmissionlist_station[station];
	//transmission_list_iterator current   = tmissions.begin();
	//transmission_list_iterator last      = tmissions.end();
	
	//for ( ; current != last ; ++current ) {
	//	if ( current->get_code().c1 == code.c1 &&  
	//		current->get_code().c2 == code.c2 &&
	//	    current->get_code().c3 == code.c3 ) {
			
			//if ( ttext ) message = current->get_transtext();
			//else message = current->get_menutext();
			strcpy( &mes[0], m.c_str() ); 
			
			// Replace all the '@' parameters with the actual text.
			int check = 0;	// If mes gets overflowed the while loop can go infinite
			double gp = fgGetFloat("/gear/gear/position-norm");
			while ( strchr(&mes[0], crej) != NULL  ) {	// ie. loop until no more occurances of crej ('@') found
				pos = strchr( &mes[0], crej );
				memmove(&tag[0], pos, 3);
				tag[3] = '\0';
				int i;
				len = 0;
				for ( i=0; i<cmax; i++ ) {
					if ( mes[i] == crej ) {
						len = i; 
						break;
					}
				}
				strncpy( &dum[0], &mes[0], len );
				dum[len] = '\0';
				
				if ( strcmp ( tag, "@ST" ) == 0 )
					//strcat( &dum[0], tpars.station.c_str() );
					strcat(&dum[0], ident.c_str());
				else if ( strcmp ( tag, "@AP" ) == 0 )
					//strcat( &dum[0], tpars.airport.c_str() );
					strcat(&dum[0], name.c_str());
				else if ( strcmp ( tag, "@CS" ) == 0 ) 
					//strcat( &dum[0], tpars.callsign.c_str() );
					strcat(&dum[0], usercall.c_str());
				else if ( strcmp ( tag, "@TD" ) == 0 ) {
					/*
					if ( tpars.tdir == 1 ) {
						char buf[] = "left";
						strcat( &dum[0], &buf[0] );
					}
					else {
						char buf[] = "right";
						strcat( &dum[0], &buf[0] );
					}
					*/
				}
				else if ( strcmp ( tag, "@HE" ) == 0 ) {
					/*
					char buf[10];
					sprintf( buf, "%i", (int)(tpars.heading) );
					strcat( &dum[0], &buf[0] );
					*/
				}
				else if ( strcmp ( tag, "@AT" ) == 0 ) {	// ATIS ID
					/*
					char buf[10];
					sprintf( buf, "%i", (int)(tpars.heading) );
					strcat( &dum[0], &buf[0] );
					*/
					double f = globals->get_ATC_mgr()->GetFrequency(ident, ATIS) / 100.0;
					if(f) {
						string atis_id;
						atis_id = ", information " + GetATISID();
						strcat( &dum[0], atis_id.c_str() );
					}
				}
				else if ( strcmp ( tag, "@VD" ) == 0 ) {
					/*
					if ( tpars.VDir == 1 ) {
						char buf[] = "Descend and maintain";
						strcat( &dum[0], &buf[0] );
					}
					else if ( tpars.VDir == 2 ) {
						char buf[] = "Maintain";
						strcat( &dum[0], &buf[0] );
					}
					else if ( tpars.VDir == 3 ) {
						char buf[] = "Climb and maintain";
						strcat( &dum[0], &buf[0] );
					} 
					*/
				}
				else if ( strcmp ( tag, "@AL" ) == 0 ) {
					/*
					char buf[10];
					sprintf( buf, "%i", (int)(tpars.alt) );
					strcat( &dum[0], &buf[0] );
					*/
				}
				else if ( strcmp ( tag, "@TO" ) == 0 ) {      // Requesting take-off or departure clearance
					string tmp;
					if (rwyOccupied) {
						tmp = "Ready for take-off";
					} else {
						if (OnAnyRunway(SGGeod::fromDegM(user_lon_node->getDoubleValue(),
								user_lat_node->getDoubleValue(), 0.0),true)) {
							tmp = "Request take-off clearance";
						} else {
							tmp = "Request departure clearance";
						}
					}
					strcat(&dum[0], tmp.c_str());
				}
				else if ( strcmp ( tag, "@MI" ) == 0 ) {
					char buf[10];
					//sprintf( buf, "%3.1f", tpars.miles );
					int dist_miles = (int)dclGetHorizontalSeparation(_geod, SGGeod::fromDegM(user_lon_node->getDoubleValue(), user_lat_node->getDoubleValue(), user_elev_node->getDoubleValue())) / 1600;
					sprintf(buf, "%i", dist_miles);
					strcat( &dum[0], &buf[0] );
				}
				else if ( strcmp ( tag, "@FR" ) == 0 ) {
					/*
					char buf[10];
					sprintf( buf, "%6.2f", tpars.freq );
					strcat( &dum[0], &buf[0] );
					*/
				}
				else if ( strcmp ( tag, "@RW" ) == 0 ) {
					strcat(&dum[0], ConvertRwyNumToSpokenString(activeRwy).c_str());
				}
				else if ( strcmp ( tag, "@GR" ) == 0 ) {	// Gear position (on final)
					if(t->gearWasUp && gp > 0.99) {
						strcat(&dum[0], ", gear down, ready to land.");
					}
				}
				else if(strcmp(tag, "@CD") == 0) {	// @CD = compass direction
					double h = GetHeadingFromTo(_geod, SGGeod::fromDegM(user_lon_node->getDoubleValue(), user_lat_node->getDoubleValue(), user_elev_node->getDoubleValue()));
					while(h < 0.0) h += 360.0;
					while(h > 360.0) h -= 360.0;
					if(h < 22.5 || h > 337.5) {
						strcat(&dum[0], "North");
					} else if(h < 67.5) {
						strcat(&dum[0], "North-East");
					} else if(h < 112.5) {
						strcat(&dum[0], "East");
					} else if(h < 157.5) {
						strcat(&dum[0], "South-East");
					} else if(h < 202.5) {
						strcat(&dum[0], "South");
					} else if(h < 247.5) {
						strcat(&dum[0], "South-West");
					} else if(h < 292.5) {
						strcat(&dum[0], "West");
					} else {
						strcat(&dum[0], "North-West");
					}
				} else {
					cout << "Tag " << tag << " not found" << endl;
					break;
				}
				strcat( &dum[0], &mes[len+3] );
				strcpy( &mes[0], &dum[0] );
				
				++check;
				if(check > 10) {
					SG_LOG(SG_ATC, SG_WARN, "WARNING: Possibly endless loop terminated in FGTransmissionlist::gen_text(...)"); 
					break;
				}
			}
			
			//cout << mes  << endl;  
			//break;
		//}
	//}
	return mes[0] ? mes : "No transmission found";
}

string FGTower::GetWeather() {
	std::ostringstream msg;

	// wind
	double hdg = wind_from_hdg->getDoubleValue();
	double speed = wind_speed_knots->getDoubleValue();
	if (speed < 1)
		msg << "wind calm";
	else
		msg << "wind " << int(hdg) << " degrees at " << int(speed) << " knots";

	// visibility
	double visibility = fgGetDouble("/environment/visibility-m");
	if (visibility < 10000)
		msg << ", visibility " << int(visibility / 1609) << " miles";

	// pressure / altimeter
	double pressure = fgGetDouble("/environment/pressure-sea-level-inhg");
	msg << ", QFE " << fixed << setprecision(2) << pressure << ".";

	return msg.str();
}

string FGTower::GetATISID() {
        double tstamp = atof(fgGetString("sim/time/elapsed-sec"));
        const int minute(60);                   // in SI units
        int interval = ATIS ? 60*minute : 2*minute;	// AWOS updated frequently
        int sequence = current_commlist->GetAtisSequence(ident, 
                              tstamp, interval);

	return GetPhoneticLetter(sequence);  // the sequence letter
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

ostream& operator << (ostream& os, PatternLeg pl) {
	switch(pl) {
	case(TAKEOFF_ROLL):   return(os << "TAKEOFF ROLL");
	case(CLIMBOUT):       return(os << "CLIMBOUT");
	case(TURN1):          return(os << "TURN1");
	case(CROSSWIND):      return(os << "CROSSWIND");
	case(TURN2):          return(os << "TURN2");
	case(DOWNWIND):       return(os << "DOWNWIND");
	case(TURN3):          return(os << "TURN3");
	case(BASE):           return(os << "BASE");
	case(TURN4):          return(os << "TURN4");
	case(FINAL):          return(os << "FINAL");
	case(LANDING_ROLL):   return(os << "LANDING ROLL");
	case(LEG_UNKNOWN):    return(os << "UNKNOWN");
	}
	return(os << "ERROR - Unknown switch in PatternLeg operator << ");
}


ostream& operator << (ostream& os, LandingType lt) {
	switch(lt) {
	case(FULL_STOP):      return(os << "FULL STOP");
	case(STOP_AND_GO):    return(os << "STOP AND GO");
	case(TOUCH_AND_GO):   return(os << "TOUCH AND GO");
	case(AIP_LT_UNKNOWN): return(os << "UNKNOWN");
	}
	return(os << "ERROR - Unknown switch in LandingType operator << ");
}

