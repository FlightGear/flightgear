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

SG_USING_STD(cout);

// TowerPlaneRec

TowerPlaneRec::TowerPlaneRec() :
clearedToLand(false),
clearedToLineUp(false),
clearedToTakeOff(false),
holdShortReported(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false),
leg(TWR_UNKNOWN),
isUser(false)
{
	plane.callsign = "UNKNOWN";
}

TowerPlaneRec::TowerPlaneRec(PlaneRec p) :
clearedToLand(false),
clearedToLineUp(false),
clearedToTakeOff(false),
holdShortReported(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false),
leg(TWR_UNKNOWN),
isUser(false)
{
	plane = p;
}

TowerPlaneRec::TowerPlaneRec(Point3D pt) :
clearedToLand(false),
clearedToLineUp(false),
clearedToTakeOff(false),
holdShortReported(false),
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false),
leg(TWR_UNKNOWN),
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
longFinalReported(false),
longFinalAcknowledged(false),
finalReported(false),
finalAcknowledged(false),
leg(TWR_UNKNOWN),
isUser(false)
{
	plane = p;
	pos = pt;
}


// FGTower

FGTower::FGTower() {
	ATCmgr = globals->get_ATC_mgr();
	
	// Init the property nodes - TODO - need to make sure we're getting surface winds.
	wind_from_hdg = fgGetNode("/environment/wind-from-heading-deg", true);
	wind_speed_knots = fgGetNode("/environment/wind-speed-kts", true);
	
	holdListItr = holdList.begin();
	appListItr = appList.begin();
	depListItr = depList.begin();
	rwyListItr = rwyList.begin();
	circuitListItr = circuitList.begin();
	trafficListItr = trafficList.begin();
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
		t->opType = OUTBOUND;	// How do we determine if the user actually wants to do circuits?
		t->isUser = true;
		t->planePtr = NULL;
		t->clearedToTakeOff = true;
		rwyList.push_back(t);
	}
}

void FGTower::Update(double dt) {
	//cout << "T" << flush;
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
	
	// Do one plane from the hold list
	if(holdList.size()) {
		//cout << "A" << endl;
		//cout << "*holdListItr = " << *holdListItr << endl;
		if(holdListItr == holdList.end()) {
			holdListItr = holdList.begin();
		}
		//cout << "*holdListItr = " << *holdListItr << endl;
		//Process(*holdListItr);
		TowerPlaneRec* t = *holdListItr;
		//cout << "t = " << t << endl;
		if(t->holdShortReported) {
			//cout << "B" << endl;
			double responseTime = 10.0;		// seconds - this should get more sophisticated at some point
			if(t->clearanceCounter > responseTime) {
				//cout << "C" << endl;
				if(t->nextOnRwy) {
					//cout << "D" << endl;
					if(rwyOccupied) {
						//cout << "E" << endl;
						// Do nothing for now - consider acknowloging hold short eventually
					} else {
						// Lets Roll !!!!
						string trns = t->plane.callsign;
						//if(departed plane < some threshold in time away) {
						if(0) {		// FIXME
							trns += " line up";
							t->clearedToLineUp = true;
							t->planePtr->RegisterTransmission(3);	// cleared to line-up
						//} else if(arriving plane < some threshold away) {
						} else if(0) {	// FIXME
							trns += " cleared immediate take-off";
							// TODO - add traffic is... ?
							t->clearedToTakeOff = true;
							t->planePtr->RegisterTransmission(4);	// cleared to take-off - TODO differentiate between immediate and normal take-off
						} else {
							trns += " cleared for take-off";
							// TODO - add traffic is... ?
							t->clearedToTakeOff = true;
							t->planePtr->RegisterTransmission(4);	// cleared to take-off
						}
						if(display) {
							globals->get_ATC_display()->RegisterSingleMessage(trns, 0);
						}
						t->holdShortReported = false;
						t->clearanceCounter = 0;
						rwyList.push_back(t);
						rwyOccupied = true;
						holdList.erase(holdListItr);
						holdListItr = holdList.begin();
					}
				} else {
					// possibly tell him to hold and what position he is?
				}
			} else {
				t->clearanceCounter += (dt * holdList.size());
			}
		}				
		++holdListItr;
	}
	
	// Do the runway list - we'll do the whole runway list since it's important and there'll never be many planes on the rwy at once!!
	if(rwyOccupied) {
		if(!rwyList.size()) {
			rwyOccupied = false;
		} else {
			rwyListItr = rwyList.begin();
			TowerPlaneRec* t = *rwyListItr;
			if(t->isUser) {
				bool on_rwy = OnActiveRunway(Point3D(user_lon_node->getDoubleValue(), user_lat_node->getDoubleValue(), 0.0));
				// TODO - how do we find the position when it's not the user?
				if(!on_rwy) {
					rwyList.pop_front();
					delete t;
				}
			} // else TODO figure out what to do when it's not the user
		}
	}
	
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
		ground->Update(dt);
	}
	//cout << "R " << flush;
}


// Figure out which runways are active.
// For now we'll just be simple and do one active runway - eventually this will get much more complex
// This is a private function - public interface to the results of this is through GetActiveRunway
void FGTower::DoRwyDetails() {
	//cout << "GetRwyDetails called" << endl;
	
	// Based on the airport-id and wind get the active runway
	SGPath path( globals->get_fg_root() );
	path.append( "Airports" );
	path.append( "runways.mk4" );
	FGRunways runways( path.c_str() );
	
	//wind
	double hdg = wind_from_hdg->getDoubleValue();
	double speed = wind_speed_knots->getDoubleValue();
	hdg = (speed == 0.0 ? 270.0 : hdg);
	//cout << "Heading = " << hdg << '\n';
	
	FGRunway runway;
	bool rwyGood = runways.search(ident, int(hdg), &runway);
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
	SGPath spath( globals->get_fg_root() );
	spath.append( "Airports" );
	spath.append( "runways.mk4" );
	FGRunways runways( spath.c_str() );
	
	// TODO - do we actually need to search for the airport - surely we already know our ident and
	// can just search runways of our airport???
	//cout << "Airport ident is " << ad.ident << '\n';
	FGRunway runway;
	bool rwyGood = runways.search(ad.ident, &runway);
	if(!rwyGood) {
		SG_LOG(SG_ATC, SG_WARN, "Unable to find any runways for airport ID " << ad.ident << " in FGTower");
	}
	bool on = false;
	while(runway.id == ad.ident) {		
		on = OnRunway(pt, runway);
		//cout << "Runway " << runway.rwy_no << ": On = " << (on ? "true\n" : "false\n");
		if(on) return(true);
		runways.next(&runway);		
	}
	return(on);
}


// Calculate the eta of each plane to the threshold.
// For ground traffic this is the fastest they can get there.
// For air traffic this is the middle approximation.
void FGTower::doThresholdETACalc() {
	// For now we'll be very crude and hardwire expected speeds to C172-like values
	// The speeds below are specified in knots IAS and then converted to m/s
	double app_ias = 100.0 * 0.514444;			// Speed during straight-in approach
	double circuit_ias = 80.0 * 0.514444;		// Speed around circuit
	double final_ias = 70.0 * 0.514444;		// Speed during final approach

	tower_plane_rec_list_iterator twrItr;

	// Sign convention - dist_out is -ve for approaching planes and +ve for departing planes
	// dist_across is +ve in the pattern direction - ie a plane correctly on downwind will have a +ve dist_across
	for(twrItr = trafficList.begin(); twrItr != trafficList.end(); twrItr++) {
		TowerPlaneRec* tpr = *twrItr;
		Point3D op = ortho.ConvertToLocal(tpr->pos);
		double dist_out_m = op.y();
		double dist_across_m = fabs(op.x());	// FIXME = the fabs is a hack to cope with the fact that we don't know the circuit direction yet
		//cout << "Doing ETA calc for " << tpr->plane.callsign << '\n';
		if(tpr->opType == CIRCUIT) {
			// It's complicated - depends on if base leg is delayed or not
			if(tpr->leg == TWR_LANDING_ROLL) {
				tpr->eta = 0;
			} else if(tpr->leg == TWR_FINAL) {
				tpr->eta = fabs(dist_out_m) / final_ias;
			} else if(tpr->leg == TWR_BASE) {
				tpr->eta = (fabs(dist_out_m) / final_ias) + (dist_across_m / circuit_ias);
			} else {
				// Need to calculate where base leg is likely to be
				// FIXME - for now I'll hardwire it to 1000m which is what AILocalTraffic uses!!!
				// TODO - as a matter of design - AILocalTraffic should get the nominal no-traffic base turn distance from Tower, since in real life the published pattern might differ from airport to airport
				double nominal_base_dist_out_m = -1000;
				double current_base_dist_out_m = nominal_base_dist_out_m;
				double nominal_dist_across_m = 1000;	// Hardwired value from AILocalTraffic
				double nominal_cross_dist_out_m = 1000;	// Bit of a guess - AI plane turns to crosswind at 600ft agl.
				tpr->eta = fabs(current_base_dist_out_m) / final_ias;	// final
				if(tpr->leg == TWR_DOWNWIND) {
					tpr->eta += dist_across_m / circuit_ias;
					tpr->eta += fabs(current_base_dist_out_m - dist_out_m) / circuit_ias;
				} else if(tpr->leg == TWR_CROSSWIND) {
					tpr->eta += nominal_dist_across_m / circuit_ias;	// should we use the dist across of the previous plane if there is previous still on downwind?
					tpr->eta += fabs(current_base_dist_out_m - nominal_cross_dist_out_m) / circuit_ias;
					tpr->eta += (nominal_dist_across_m - dist_across_m) / circuit_ias;
				} else {
					// We've only just started - why not use a generic estimate?
				}
			}
		} else if((tpr->opType == INBOUND) || (tpr->opType == STRAIGHT_IN)) {
			// It's simpler!
		} else {
			// Must be outbound - ignore it!
		}
		//cout << "ETA = " << tpr->eta << '\n';
	}
}
		

bool FGTower::doThresholdUseOrder() {
	return(true);
}

void FGTower::doCommunication() {
}

void FGTower::ContactAtHoldShort(PlaneRec plane, FGAIEntity* requestee, tower_traffic_type operation) {
	// HACK - assume that anything contacting at hold short is new for now - FIXME LATER
	TowerPlaneRec* t = new TowerPlaneRec;
	t->plane = plane;
	t->planePtr = requestee;
	t->holdShortReported = true;
	t->clearanceCounter = 0;
	t->clearedToLineUp = false;
	t->clearedToTakeOff = false;
	t->opType = operation;
	
	// HACK ALERT - THIS IS HARDWIRED FOR TESTING - FIXME TODO ETC
	t->nextOnRwy = true;
	
	//cout << "t = " << t << '\n';
	
	holdList.push_back(t);
}

void FGTower::RequestLandingClearance(string ID) {
	//cout << "Request Landing Clearance called...\n";
}
void FGTower::RequestDepartureClearance(string ID) {
	//cout << "Request Departure Clearance called...\n";
}	
//void FGTower::ReportFinal(string ID);
//void FGTower::ReportLongFinal(string ID);
//void FGTower::ReportOuterMarker(string ID);
//void FGTower::ReportMiddleMarker(string ID);
//void FGTower::ReportInnerMarker(string ID);
//void FGTower::ReportGoingAround(string ID);
void FGTower::ReportRunwayVacated(string ID) {
	//cout << "Report Runway Vacated Called...\n";
}
