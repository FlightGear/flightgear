// FGAILocalTraffic - AIEntity derived class with enough logic to
// fly and interact with the traffic pattern.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Airports/runways.hxx>
#include <Main/globals.hxx>
#include <Main/location.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <string>
#include <math.h>

SG_USING_STD(string);

#include "ATCmgr.hxx"
#include "AILocalTraffic.hxx"
#include "ATCutils.hxx"

FGAILocalTraffic::FGAILocalTraffic() {
	ATC = globals->get_ATC_mgr();
	
	// TODO - unhardwire this - possibly let the AI manager set the callsign
	plane.callsign = "Trainer-two-five-charlie";
	plane.type = GA_SINGLE;
	
	roll = 0.0;
	pitch = 0.0;
	hdg = 270.0;
	
	//Hardwire initialisation for now - a lot of this should be read in from config eventually
	Vr = 70.0;
	best_rate_of_climb_speed = 70.0;
	//best_rate_of_climb;
	//nominal_climb_speed;
	//nominal_climb_rate;
	//nominal_circuit_speed;
	//min_circuit_speed;
	//max_circuit_speed;
	nominal_descent_rate = 500.0;
	nominal_final_speed = 65.0;
	//nominal_approach_speed;
	//stall_speed_landing_config;
	nominalTaxiSpeed = 8.0;
	taxiTurnRadius = 8.0;
	wheelOffset = 1.45;	// Warning - hardwired to the C172 - we need to read this in from file.
	elevInitGood = false;
	// Init the property nodes
	wind_from_hdg = fgGetNode("/environment/wind-from-heading-deg", true);
	wind_speed_knots = fgGetNode("/environment/wind-speed-kts", true);
	circuitsToFly = 0;
	liningUp = false;
	taxiRequestPending = false;
	taxiRequestCleared = false;
	holdingShort = false;
	clearedToLineUp = false;
	clearedToTakeOff = false;
	reportReadyForDeparture = false;
	contactTower = false;
	contactGround = false;
}

FGAILocalTraffic::~FGAILocalTraffic() {
}


// Get details of the active runway
// It is assumed that by the time this is called the tower control and airport code will have been set up.
void FGAILocalTraffic::GetRwyDetails() {
	//cout << "GetRwyDetails called" << endl;
	
	rwy.rwyID = tower->GetActiveRunway();
	
	// Now we need to get the threshold position and rwy heading
	
	SGPath path( globals->get_fg_root() );
	path.append( "Airports" );
	path.append( "runways.mk4" );
	FGRunways runways( path.c_str() );

	FGRunway runway;
	bool rwyGood = runways.search(airportID, rwy.rwyID, &runway);
	if(rwyGood) {
		// Get the threshold position
    	hdg = runway.heading;	// TODO - check - is this our heading we are setting here, and if so should we be?
		//cout << "hdg reset to " << hdg << '\n';
		double other_way = hdg - 180.0;
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
    	geo_direct_wgs_84 ( aptElev, ref.lat(), ref.lon(), other_way, 
        	                rwy.length / 2.0 - 25.0, &tshlat, &tshlon, &tshr );
    	geo_direct_wgs_84 ( aptElev, ref.lat(), ref.lon(), hdg, 
        	                rwy.length / 2.0 - 25.0, &tolat, &tolon, &tor );
		// Note - 25 meters in from the runway end is a bit of a hack to put the plane ahead of the user.
		// now copy what we need out of runway into rwy
    	rwy.threshold_pos = Point3D(tshlon, tshlat, aptElev);
		Point3D takeoff_end = Point3D(tolon, tolat, aptElev);
		//cout << "Threshold position = " << tshlon << ", " << tshlat << ", " << aptElev << '\n';
		//cout << "Takeoff position = " << tolon << ", " << tolat << ", " << aptElev << '\n';
		rwy.hdg = hdg;
		// Set the projection for the local area
		ortho.Init(rwy.threshold_pos, rwy.hdg);	
		rwy.end1ortho = ortho.ConvertToLocal(rwy.threshold_pos);	// should come out as zero
		rwy.end2ortho = ortho.ConvertToLocal(takeoff_end);
	} else {
		SG_LOG(SG_ATC, SG_ALERT, "Help  - can't get good runway in FGAILocalTraffic!!\n");
	}
}

/* 
There are two possible scenarios during initialisation:
The first is that the user is flying towards the airport, and hence the traffic
could be initialised anywhere, as long as the AI planes are consistent with
each other.
The second is that the user has started the sim at or close to the airport, and
hence the traffic must be initialised with respect to the user as well as each other.
To a certain extent it's FGAIMgr that has to worry about this, but we need to provide
sufficient initialisation functionality within the plane classes to allow the manager
to initialy position them where and how required.
*/
bool FGAILocalTraffic::Init(string ICAO, OperatingState initialState, PatternLeg initialLeg) {
	//cout << "FGAILocalTraffic.Init(...) called" << endl;
	// Hack alert - Hardwired path!!
	string planepath = "Aircraft/c172/Models/c172-dpm.ac";
	SGPath path = globals->get_fg_root();
	path.append(planepath);
	aip.init(planepath.c_str());
	aip.setVisible(false);		// This will be set to true once a valid ground elevation has been determined
	globals->get_scenery()->get_scene_graph()->addKid(aip.getSceneGraph());
	
	// Find the tower frequency - this is dependent on the ATC system being initialised before the AI system
	airportID = ICAO;
	AirportATC a;
	if(ATC->GetAirportATCDetails(airportID, &a)) {
		if(a.tower_freq) {	// Has a tower
			tower = (FGTower*)ATC->GetATCPointer((string)airportID, TOWER);	// Maybe need some error checking here
			if(tower == NULL) {
				// Something has gone wrong - abort or carry on with un-towered operation?
				return(false);
			}
			freq = (double)tower->get_freq() / 100.0;
			ground = tower->GetGroundPtr();
			if(ground == NULL) {
				// Something has gone wrong :-(
				SG_LOG(SG_ATC, SG_ALERT, "ERROR - can't get a ground pointer from tower control in FGAILocalTraffic::Init() :-(");
				return(false);
			} else if((initialState == PARKED) || (initialState == TAXIING)) {
				freq = (double)ground->get_freq() / 100.0;
			}
			//cout << "AILocalTraffic freq is " << freq << '\n';
		} else {
			// Check CTAF, unicom etc
		}
	} else {
		//cout << "Unable to find airport details in FGAILocalTraffic::Init()\n";
	}

	// Get the airport elevation
	aptElev = dclGetAirportElev(airportID.c_str()) * SG_FEET_TO_METER;
	//cout << "Airport elev in AILocalTraffic = " << aptElev << '\n';
	// WARNING - we use this elev for the whole airport - some assumptions in the code 
	// might fall down with very slopey airports.

	//cout << "In Init(), initialState = " << initialState << endl;
	operatingState = initialState;
	switch(operatingState) {
	case PARKED:
		ourGate = ground->GetGateNode();
		if(ourGate == NULL) {
			// Implies no available gates - what shall we do?
			// For now just vanish the plane - possibly we can make this more elegant in the future
			SG_LOG(SG_ATC, SG_ALERT, "No gate found by FGAILocalTraffic whilst attempting Init at " << airportID << '\n');
			return(false);
		}
		pitch = 0.0;
		roll = 0.0;
		vel = 0.0;
		slope = 0.0;
		pos = ourGate->pos;
		pos.setelev(aptElev);
		hdg = ourGate->heading;
		
		// Now we've set the position we can do the ground elev
		elevInitGood = false;
		inAir = false;
		DoGroundElev();
		
		Transform();
		break;
	case TAXIING:
		// FIXME - implement this case properly
		return(false);	// remove this line when fixed!
		break;
	case IN_PATTERN:
		// For now we'll always start the in_pattern case on the threshold ready to take-off
		// since we've got the implementation for this case already.
		// TODO - implement proper generic in_pattern startup.
		
		// Get the active runway details (and copy them into rwy)
		GetRwyDetails();

		// Initial position on threshold for now
		pos.setlat(rwy.threshold_pos.lat());
		pos.setlon(rwy.threshold_pos.lon());
		pos.setelev(rwy.threshold_pos.elev());
		hdg = rwy.hdg;
		
		// Now we've set the position we can do the ground elev
		// This might not always be necessary if we implement in-air start
		elevInitGood = false;
		inAir = false;
		DoGroundElev();
		
		pitch = 0.0;
		roll = 0.0;
		leg = TAKEOFF_ROLL;
		vel = 0.0;
		slope = 0.0;
		
		circuitsToFly = 0;		// ie just fly this circuit and then stop
		touchAndGo = false;
		// FIXME TODO - pattern direction is still hardwired
		patternDirection = -1;		// Left
		// At the bare minimum we ought to make sure it goes the right way at dual parallel rwy airports!
		if(rwy.rwyID.size() == 3) {
			patternDirection = (rwy.rwyID.substr(2,1) == "R" ? 1 : -1);
		}
		
		operatingState = IN_PATTERN;
		
		Transform();
		break;
	default:
		SG_LOG(SG_ATC, SG_ALERT, "Attempt to set unknown operating state in FGAILocalTraffic.Init(...)\n");
		return(false);
	}
	
	
	return(true);
}

// Commands to do something from higher level logic
void FGAILocalTraffic::FlyCircuits(int numCircuits, bool tag) {
	//cout << "FlyCircuits called" << endl;
	
	switch(operatingState) {
	case IN_PATTERN:
		circuitsToFly += numCircuits;
		return;
		break;
	case TAXIING:
		// For now we'll punt this and do nothing
		break;
	case PARKED:
		circuitsToFly = numCircuits;	// Note that one too many circuits gets flown because we only test and decrement circuitsToFly after landing
										// thus flying one too many circuits.  TODO - Need to sort this out better!
		touchAndGo = tag;
#if 0	
		// Get the active runway details (and copy them into rwy)
		GetRwyDetails();
		
		// Get the takeoff node for the active runway, get a path to it and start taxiing
		path = ground->GetPath(ourGate, rwy.rwyID);
		if(path.size() < 2) {
			// something has gone wrong
			SG_LOG(SG_ATC, SG_ALERT, "Invalid path from gate to theshold in FGAILocalTraffic::FlyCircuits\n");
			return;
		}
		/*
		cout << "path returned was:" << endl;
		for(unsigned int i=0; i<path.size(); ++i) {
			switch(path[i]->struct_type) {
			case NODE:
				cout << "NODE " << ((node*)(path[i]))->nodeID << endl;
				break;
			case ARC:
				cout << "ARC\n";
				break;
			}
		}
		*/
		// pop the gate - we're here already!
		path.erase(path.begin());
		//path.erase(path.begin());
		/*
		cout << "path after popping front is:" << endl;
		for(unsigned int i=0; i<path.size(); ++i) {
			switch(path[i]->struct_type) {
			case NODE:
				cout << "NODE " << ((node*)(path[i]))->nodeID << endl;
				break;
			case ARC:
				cout << "ARC\n";
				break;
			}
		}
		*/
		
		taxiState = TD_OUTBOUND;
		StartTaxi();
		
		// Maybe the below should be set when we get to the threshold and prepare for TO?
		// FIXME TODO - pattern direction is still hardwired
		patternDirection = -1;		// Left
		// At the bare minimum we ought to make sure it goes the right way at dual parallel rwy airports!
		if(rwy.rwyID.size() == 3) {
			patternDirection = (rwy.rwyID.substr(2,1) == "R" ? 1 : -1);
		}

		Transform();
#endif
		break;
	}
}   

// Run the internal calculations
void FGAILocalTraffic::Update(double dt) {
	//cout << "A" << flush;
	double responseTime = 10.0;		// seconds - this should get more sophisticated at some point
	responseCounter += dt;
	if((contactTower) && (responseCounter >= 8.0)) {
		// Acknowledge request before changing frequency so it gets rendered if the user is on the same freq
		string trns = "Tower ";
		double f = globals->get_ATC_mgr()->GetFrequency(airportID, TOWER) / 100.0;	
		char buf[10];
		sprintf(buf, "%f", f);
		trns += buf;
		trns += " ";
		trns += plane.callsign;
		Transmit(trns);
		responseCounter = 0.0;
		contactTower = false;
		changeFreq = true;
		changeFreqType = TOWER;
	}
	
	if((changeFreq) && (responseCounter > 8.0)) {
		switch(changeFreqType) {
		case TOWER:
			freq = (double)tower->get_freq() / 100.0;
			//Transmit("DING!");
			// Contact the tower, even if only virtually
			changeFreq = false;
			tower->ContactAtHoldShort(plane, this, CIRCUIT);
			break;
		case GROUND:
			freq = (double)ground->get_freq() / 100.0;
			break;
		// And to avoid compiler warnings...
		case APPROACH:
			break;
		case ATIS:
			break;
		case ENROUTE:
			break;
		case DEPARTURE:
			break;
		case INVALID:
			break;
		}
	}
	
	//cout << "." << flush;
		
	switch(operatingState) {
	case IN_PATTERN:
		//cout << "In IN_PATTERN\n";
		if(!inAir) DoGroundElev();
		if(!elevInitGood) {
			if(aip.getFGLocation()->get_cur_elev_m() > -9990.0) {
				pos.setelev(aip.getFGLocation()->get_cur_elev_m() + wheelOffset);
				//cout << "TAKEOFF_ROLL, POS = " << pos.lon() << ", " << pos.lat() << ", " << pos.elev() << '\n';
				//Transform();
				aip.setVisible(true);
				//cout << "Making plane visible!\n";
				elevInitGood = true;
			}
		}
		FlyTrafficPattern(dt);
		Transform();
		break;
	case TAXIING:
		//cout << "In TAXIING\n";
		//cout << "*" << flush;
		if(!elevInitGood) {
			//DoGroundElev();
			if(aip.getFGLocation()->get_cur_elev_m() > -9990.0) {
				pos.setelev(aip.getFGLocation()->get_cur_elev_m() + wheelOffset);
				//Transform();
				aip.setVisible(true);
				//Transform();
				//cout << "Making plane visible!\n";
				elevInitGood = true;
			}
		}
		DoGroundElev();
		//cout << "," << flush;
		if(!((holdingShort) && (!clearedToLineUp))) {
			//cout << "|" << flush;
			Taxi(dt);
		}
		//cout << ";" << flush;
		if((clearedToTakeOff) && (responseCounter >= 8.0)) {
			// possible assumption that we're at the hold short here - may not always hold
			// TODO - sort out the case where we're cleared to line-up first and then cleared to take-off on the rwy.
			taxiState = TD_LINING_UP;
			path = ground->GetPath(holdShortNode, rwy.rwyID);
			/*
			cout << "path returned was:" << endl;
			for(unsigned int i=0; i<path.size(); ++i) {
				switch(path[i]->struct_type) {
					case NODE:
					cout << "NODE " << ((node*)(path[i]))->nodeID << endl;
					break;
					case ARC:
					cout << "ARC\n";
					break;
				}
			}
			*/
			clearedToTakeOff = false;	// We *are* still cleared - this simply stops the response recurring!!
			holdingShort = false;
			string trns = "Cleared for take-off ";
			trns += plane.callsign;
			Transmit(trns);
			StartTaxi();
		}
		//cout << "^" << flush;
		Transform();
		break;
		case PARKED:
		//cout << "In PARKED\n";
		if(!elevInitGood) {
			DoGroundElev();
			if(aip.getFGLocation()->get_cur_elev_m() > -9990.0) {
				pos.setelev(aip.getFGLocation()->get_cur_elev_m() + wheelOffset);
				//Transform();
				aip.setVisible(true);
				//Transform();
				//cout << "Making plane visible!\n";
				elevInitGood = true;
			}
		}
		
		if(circuitsToFly) {
			if((taxiRequestPending) && (taxiRequestCleared)) {
				//cout << "&" << flush;
				// Get the active runway details (and copy them into rwy)
				GetRwyDetails();
				
				// Get the takeoff node for the active runway, get a path to it and start taxiing
				path = ground->GetPathToHoldShort(ourGate, rwy.rwyID);
				if(path.size() < 2) {
					// something has gone wrong
					SG_LOG(SG_ATC, SG_ALERT, "Invalid path from gate to theshold in FGAILocalTraffic::FlyCircuits\n");
					return;
				}
				/*
				cout << "path returned was:\n";
				for(unsigned int i=0; i<path.size(); ++i) {
					switch(path[i]->struct_type) {
						case NODE:
						cout << "NODE " << ((node*)(path[i]))->nodeID << endl;
						break;
						case ARC:
						cout << "ARC\n";
						break;
					}
				}
				*/
				path.erase(path.begin());	// pop the gate - we're here already!
				taxiState = TD_OUTBOUND;
				taxiRequestPending = false;
				holdShortNode = (node*)(*(path.begin() + path.size()));
				StartTaxi();
			} else if(!taxiRequestPending) {
				//cout << "(" << flush;
				ground->RequestDeparture(plane, this);
				// Do some communication
				// airport name + tower + airplane callsign + location + request taxi for + operation type + ?
				string trns = "";
				trns += tower->get_name();
				trns += " tower ";
				trns += plane.callsign;
				trns += " on apron parking request taxi for traffic pattern";
				//cout << "trns = " << trns << endl;
				Transmit(trns);
				taxiRequestCleared = false;
				taxiRequestPending = true;
			}
		}
		
		//cout << "!" << flush;
				
		// Maybe the below should be set when we get to the threshold and prepare for TO?
		// FIXME TODO - pattern direction is still hardwired
		patternDirection = -1;		// Left
		// At the bare minimum we ought to make sure it goes the right way at dual parallel rwy airports!
		if(rwy.rwyID.size() == 3) {
			patternDirection = (rwy.rwyID.substr(2,1) == "R" ? 1 : -1);
		}		
		// Do nothing
		Transform();
		//cout << ")" << flush;
		break;
	default:
		break;
	}
	//cout << "I " << flush;
}

void FGAILocalTraffic::RegisterTransmission(int code) {
	switch(code) {
	case 1:	// taxi request cleared
		taxiRequestCleared = true;
		SG_LOG(SG_ATC, SG_INFO, "AI local traffic " << plane.callsign << " cleared to taxi...");
		break;
	case 2:	// contact tower
		responseCounter = 0;
		contactTower = true;
		SG_LOG(SG_ATC, SG_INFO, "AI local traffic " << plane.callsign << " told to contact tower...");
		break;
	case 3: // Cleared to line up
		responseCounter = 0;
		clearedToLineUp = true;
		SG_LOG(SG_ATC, SG_INFO, "AI local traffic " << plane.callsign << " cleared to line-up...");
		break;
	case 4: // cleared to take-off
		responseCounter = 0;
		clearedToTakeOff = true;
		SG_LOG(SG_ATC, SG_INFO, "AI local traffic " << plane.callsign << " cleared to take-off...");
		break;
	default:
		break;
	}
}

// Fly a traffic pattern
// FIXME - far too much of the mechanics of turning, rolling, accellerating, descending etc is in here.
// 	   Move it out to FGAIPlane and have FlyTrafficPattern just specify what to do, not the implementation.
void FGAILocalTraffic::FlyTrafficPattern(double dt) {
	// Need to differentiate between in-air (IAS governed) and on-ground (vel governed)
	// Take-off is an interesting case - we are on the ground but takeoff speed is IAS governed.
	
	static bool transmitted = false;	// FIXME - this is a hack

	// WIND
	// Wind has two effects - a mechanical one in that IAS translates to a different vel, and the hdg != track,
	// but also a piloting effect, in that the AI must be able to descend at a different rate in order to hit the threshold.
	
	//cout << "dt = " << dt << '\n';
	double dist = 0;
	// ack - I can't remember how long a rate 1 turn is meant to take.
	double turn_time = 60.0;	// seconds - TODO - check this guess
	double turn_circumference;
	double turn_radius;
	Point3D orthopos = ortho.ConvertToLocal(pos);	// ortho position of the plane
	//cout << "runway elev = " << rwy.threshold_pos.elev() << ' ' << rwy.threshold_pos.elev() * SG_METER_TO_FEET << '\n';
	//cout << "elev = " << pos.elev() << ' ' << pos.elev() * SG_METER_TO_FEET << '\n';

	// HACK FOR TESTING - REMOVE
	//cout << "Calling ExitRunway..." << endl;
	//ExitRunway(orthopos);
	//return;
	// END HACK
	
	//wind
	double wind_from = wind_from_hdg->getDoubleValue();
	double wind_speed = wind_speed_knots->getDoubleValue();

	switch(leg) {
	case TAKEOFF_ROLL:
		//inAir = false;
		track = rwy.hdg;
		if(vel < 80.0) {
			double dveldt = 5.0;
			vel += dveldt * dt;
		}
		if(aip.getFGLocation()->get_cur_elev_m() > -9990.0) {
			pos.setelev(aip.getFGLocation()->get_cur_elev_m() + wheelOffset);
		}
		IAS = vel + (cos((hdg - wind_from) * DCL_DEGREES_TO_RADIANS) * wind_speed);
		if(IAS >= 70) {
			leg = CLIMBOUT;
			pitch = 10.0;
			IAS = best_rate_of_climb_speed;
			slope = 7.0;
			inAir = true;
		}
		break;
	case CLIMBOUT:
		track = rwy.hdg;
		if((pos.elev() - rwy.threshold_pos.elev()) * SG_METER_TO_FEET > 600) {
			cout << "Turning to crosswind, distance from threshold = " << orthopos.y() << '\n'; 
			leg = TURN1;
		}
		break;
	case TURN1:
		track += (360.0 / turn_time) * dt * patternDirection;
		Bank(25.0 * patternDirection);
		if((track < (rwy.hdg - 89.0)) || (track > (rwy.hdg + 89.0))) {
			leg = CROSSWIND;
		}
		break;
	case CROSSWIND:
		LevelWings();
		track = rwy.hdg + (90.0 * patternDirection);
		if((pos.elev() - rwy.threshold_pos.elev()) * SG_METER_TO_FEET > 1000) {
			slope = 0.0;
			pitch = 0.0;
			IAS = 80.0;		// FIXME - use smooth transistion to new speed
		}
		// turn 1000m out for now
		if(fabs(orthopos.x()) > 980) {
			leg = TURN2;
		}
		break;
	case TURN2:
		track += (360.0 / turn_time) * dt * patternDirection;
		Bank(25.0 * patternDirection);
		// just in case we didn't make height on crosswind
		if((pos.elev() - rwy.threshold_pos.elev()) * SG_METER_TO_FEET > 1000) {
			slope = 0.0;
			pitch = 0.0;
			IAS = 80.0;		// FIXME - use smooth transistion to new speed
		}
		if((track < (rwy.hdg - 179.0)) || (track > (rwy.hdg + 179.0))) {
			leg = DOWNWIND;
			transmitted = false;
			//roll = 0.0;
		}
		break;
	case DOWNWIND:
		LevelWings();
		track = rwy.hdg - (180 * patternDirection);	//should tend to bring track back into the 0->360 range
		// just in case we didn't make height on crosswind
		if((pos.elev() - rwy.threshold_pos.elev()) * SG_METER_TO_FEET > 1000) {
			slope = 0.0;
			pitch = 0.0;
			IAS = 90.0;		// FIXME - use smooth transistion to new speed
		}
		if((orthopos.y() < 0) && (!transmitted)) {
			TransmitPatternPositionReport();
			transmitted = true;
		}
		if(orthopos.y() < -480) {
			slope = -4.0;	// FIXME - calculate to descent at 500fpm and hit the threshold (taking wind into account as well!!)
			pitch = -3.0;
			IAS = 85.0;
		}
		if(orthopos.y() < -980) {
			//roll = -20;
			leg = TURN3;
			transmitted = false;
			IAS = 80.0;
		}
		break;
	case TURN3:
		track += (360.0 / turn_time) * dt * patternDirection;
		Bank(25.0 * patternDirection);
		if(fabs(rwy.hdg - track) < 91.0) {
			leg = BASE;
		}
		break;
	case BASE:
		LevelWings();
		if(!transmitted) {
			TransmitPatternPositionReport();
			transmitted = true;
		}
		track = rwy.hdg - (90 * patternDirection);
		slope = -6.0;	// FIXME - calculate to descent at 500fpm and hit the threshold
		pitch = -4.0;
		IAS = 70.0;	// FIXME - slowdown gradually
		// Try and arrange to turn nicely onto base
		turn_circumference = IAS * 0.514444 * turn_time;	
		//Hmmm - this is an interesting one - ground vs airspeed in relation to turn radius
		//We'll leave it as a hack with IAS for now but it needs revisiting.
		
		turn_radius = turn_circumference / (2.0 * DCL_PI);
		if(fabs(orthopos.x()) < (turn_radius + 50)) {
			leg = TURN4;
			transmitted = false;
			//roll = -20;
		}
		break;
	case TURN4:
		track += (360.0 / turn_time) * dt * patternDirection;
		Bank(25.0 * patternDirection);
		if(fabs(track - rwy.hdg) < 0.6) {
			leg = FINAL;
			vel = nominal_final_speed;
		}
		break;
	case FINAL:
		LevelWings();
		if(!transmitted) {
			TransmitPatternPositionReport();
			transmitted = true;
		}
		// Try and track the extended centreline
		track = rwy.hdg - (0.2 * orthopos.x());
		//cout << "orthopos.x() = " << orthopos.x() << " hdg = " << hdg << '\n';
		if(pos.elev() < (rwy.threshold_pos.elev()+20.0+wheelOffset)) {
			DoGroundElev();	// Need to call it here expicitly on final since it's only called
			               	// for us in update(...) when the inAir flag is false.
		}
		if(pos.elev() < (rwy.threshold_pos.elev()+10.0+wheelOffset)) {
			if(aip.getFGLocation()->get_cur_elev_m() > -9990.0) {
				if((aip.getFGLocation()->get_cur_elev_m() + wheelOffset) > pos.elev()) {
					slope = 0.0;
					pitch = 0.0;
					leg = LANDING_ROLL;
					inAir = false;
				}
			}	// else need a fallback position based on arpt elev in case ground elev determination fails?
		}
		break;
	case LANDING_ROLL:
		//inAir = false;
		if(aip.getFGLocation()->get_cur_elev_m() > -9990.0) {
			pos.setelev(aip.getFGLocation()->get_cur_elev_m() + wheelOffset);
		}
		track = rwy.hdg;
		double dveldt = -5.0;
		vel += dveldt * dt;
		// FIXME - differentiate between touch and go and full stops
		if(vel <= 15.0) {
			//cout << "Vel <= 15.0, circuitsToFly = " << circuitsToFly << endl;
			if(circuitsToFly <= 0) {
				//cout << "Calling ExitRunway..." << endl;
				ExitRunway(orthopos);
				return;
			} else {
				//cout << "Taking off again..." << endl;
				leg = TAKEOFF_ROLL;
				--circuitsToFly;
			}
		}
		break;
    }

	if(inAir) {
		// FIXME - at the moment this is a bit screwy
		// The velocity correction is applied based on the relative headings.
		// Then the heading is changed based on the velocity.
		// Which comes first, the chicken or the egg?
		// Does it really matter?
		
		// Apply wind to ground-relative velocity if in the air
		vel = IAS - (cos((hdg - wind_from) * DCL_DEGREES_TO_RADIANS) * wind_speed);
		//crab = f(track, wind, vel);
		// The vector we need to fly is our desired vector minus the wind vector
		// TODO - we probably ought to use plib's built in vector types and operations for this
		// ie.  There's almost *certainly* a better way to do this!
		double gxx = vel * sin(track * DCL_DEGREES_TO_RADIANS);	// Plane desired velocity x component wrt ground
		double gyy = vel * cos(track * DCL_DEGREES_TO_RADIANS);	// Plane desired velocity y component wrt ground
		double wxx = wind_speed * sin((wind_from + 180.0) * DCL_DEGREES_TO_RADIANS);	// Wind velocity x component
		double wyy = wind_speed * cos((wind_from + 180.0) * DCL_DEGREES_TO_RADIANS);	// Wind velocity y component
		double axx = gxx - wxx;	// Plane in-air velocity x component
		double ayy = gyy - wyy; // Plane in-air velocity y component
		// Now we want the angle between gxx and axx (which is the crab)
		double maga = sqrt(axx*axx + ayy*ayy);
		double magg = sqrt(gxx*gxx + gyy*gyy);
		crab = acos((axx*gxx + ayy*gyy) / (maga * magg));
		// At this point this works except we're getting the modulus of the angle
		//cout << "crab = " << crab << '\n';
		
		// Make sure both headings are in the 0->360 circle in order to get sane differences
		dclBoundHeading(wind_from);
		dclBoundHeading(track);
		if(track > wind_from) {
			if((track - wind_from) <= 180) {
				crab *= -1.0;
			}
		} else {
			if((wind_from - track) >= 180) {
				crab *= -1.0;
			}
		}
	} else {	// on the ground - crab dosen't apply
		crab = 0.0;
	}
	
	hdg = track + crab;
	dist = vel * 0.514444 * dt;
	pos = dclUpdatePosition(pos, track, slope, dist);
}

void FGAILocalTraffic::TransmitPatternPositionReport(void) {
	// airport name + "traffic" + airplane callsign + pattern direction + pattern leg + rwy + ?
	string trns = "";
	
	trns += tower->get_name();
	trns += " Traffic ";
	trns += plane.callsign;
	if(patternDirection == 1) {
		trns += " right ";
	} else {
		trns += " left ";
	}
	
	// We could probably get rid of this whole switch statement and just pass a string containing the leg from the FlyPattern function.
	switch(leg) {	// We'll assume that transmissions in turns are intended for next leg - do pilots ever call out that they are in the turn?
	case TURN1:
		// Fall through to CROSSWIND
	case CROSSWIND:	// I don't think this case will be used here but it can't hurt to leave it in
		trns += "crosswind ";
		break;
	case TURN2:
		// Fall through to DOWNWIND
	case DOWNWIND:
		trns += "downwind ";
		break;
	case TURN3:
		// Fall through to BASE
	case BASE:
		trns += "base ";
		break;
	case TURN4:
		// Fall through to FINAL
	case FINAL:		// maybe this should include long/short final if appropriate?
		trns += "final ";
		break;
	default:		// Hopefully this won't be used
		trns += "pattern ";
		break;
	}
	// FIXME - I've hardwired the runway call as well!! (We could work this out from rwy heading and mag deviation)
	trns += ConvertRwyNumToSpokenString(1);
	
	// And add the airport name again
	trns += tower->get_name();
	
	Transmit(trns);
}

void FGAILocalTraffic::ExitRunway(Point3D orthopos) {
	//cout << "In ExitRunway" << endl;
	//cout << "Runway ID is " << rwy.ID << endl;
	node_array_type exitNodes = ground->GetExits(rwy.rwyID);	//I suppose we ought to have some fallback for rwy with no defined exits?
	/*
	cout << "Node ID's of exits are ";
	for(unsigned int i=0; i<exitNodes.size(); ++i) {
		cout << exitNodes[i]->nodeID << ' ';
	}
	cout << endl;
	*/
	if(exitNodes.size()) {
		//Find the next exit from orthopos.y
		double d;
		double dist = 100000;	//ie. longer than any runway in existance
		double backdist = 100000;
		node_array_iterator nItr = exitNodes.begin();
		node* rwyExit = *(exitNodes.begin());
		//int gateID;		//This might want to be more persistant at some point
		while(nItr != exitNodes.end()) {
			d = ortho.ConvertToLocal((*nItr)->pos).y() - ortho.ConvertToLocal(pos).y(); 	//FIXME - consider making orthopos a class variable
			if(d > 0.0) {
				if(d < dist) {
					dist = d;
					rwyExit = *nItr;
				}
			} else {
				if(fabs(d) < backdist) {
					backdist = d;
					//TODO - need some logic here that if we don't get a forward exit we turn round and store the backwards one
				}
			}
			++nItr;
		}
		ourGate = ground->GetGateNode();
		if(ourGate == NULL) {
			// Implies no available gates - what shall we do?
			// For now just vanish the plane - possibly we can make this more elegant in the future
			SG_LOG(SG_ATC, SG_ALERT, "No gate found by FGAILocalTraffic whilst landing at " << airportID << '\n');
			aip.setVisible(false);
			operatingState = PARKED;
			return;
		}
		path = ground->GetPath(rwyExit, ourGate);
		/*
		cout << "path returned was:" << endl;
		for(unsigned int i=0; i<path.size(); ++i) {
			switch(path[i]->struct_type) {
			case NODE:
				cout << "NODE " << ((node*)(path[i]))->nodeID << endl;
				break;
			case ARC:
				cout << "ARC\n";
				break;
			}
		}
		*/
		taxiState = TD_INBOUND;
		StartTaxi();
	} else {
		// Something must have gone wrong with the ground network file - or there is only a rwy here and no exits defined
		SG_LOG(SG_ATC, SG_ALERT, "No exits found by FGAILocalTraffic from runway " << rwy.rwyID << " at " << airportID << '\n');
		// What shall we do - just remove the plane from sight?
		aip.setVisible(false);
		operatingState = PARKED;
	}
}

// Set the class variable nextTaxiNode to the next node in the path
// and update taxiPathPos, the class variable path iterator position
// TODO - maybe should return error codes to the calling function if we fail here
void FGAILocalTraffic::GetNextTaxiNode() {
	//cout << "GetNextTaxiNode called " << endl;
	//cout << "taxiPathPos = " << taxiPathPos << endl;
	ground_network_path_iterator pathItr = path.begin() + taxiPathPos;
	if(pathItr == path.end()) {
		SG_LOG(SG_ATC, SG_ALERT, "ERROR IN AILocalTraffic::GetNextTaxiNode - no more nodes in path\n");
	} else {
		if((*pathItr)->struct_type == NODE) {
			//cout << "ITS A NODE" << endl;
			//*pathItr = new node;
			nextTaxiNode = (node*)*pathItr;
			++taxiPathPos;
			//delete pathItr;
		} else {
			//cout << "ITS NOT A NODE" << endl;
			//The first item in found must have been an arc
			//Assume for now that it was straight
			pathItr++;
			taxiPathPos++;
			if(pathItr == path.end()) {
				SG_LOG(SG_ATC, SG_ALERT, "ERROR IN AILocalTraffic::GetNextTaxiNode - path ended with an arc\n");
			} else if((*pathItr)->struct_type == NODE) {
				nextTaxiNode = (node*)*pathItr;
				++taxiPathPos;
			} else {
				//OOPS - two non-nodes in a row - that shouldn't happen ATM
				SG_LOG(SG_ATC, SG_ALERT, "ERROR IN AILocalTraffic::GetNextTaxiNode - two non-nodes in sequence\n");
			}
		}
	}
}	    

// StartTaxi - set up the taxiing state - call only at the start of taxiing
void FGAILocalTraffic::StartTaxi() {
	//cout << "StartTaxi called" << endl;
	operatingState = TAXIING;
	taxiPathPos = 0;
	
	//Set the desired heading
	//Assume we are aiming for first node on path
	//Eventually we may need to consider the fact that we might start on a curved arc and
	//not be able to head directly for the first node.
	GetNextTaxiNode();	// sets the class variable nextTaxiNode to the next taxi node!
	desiredTaxiHeading = GetHeadingFromTo(pos, nextTaxiNode->pos);
	//cout << "First taxi heading is " << desiredTaxiHeading << endl;
}

// speed in knots, headings in degrees, radius in meters.
static double TaxiTurnTowardsHeading(double current_hdg, double desired_hdg, double speed, double radius, double dt) {
	// wrap heading - this prevents a logic bug where the plane would just go round in circles!!
	while(current_hdg < 0.0) {
		current_hdg += 360.0;
	}
	while(current_hdg > 360.0) {
		current_hdg -= 360.0;
	}
	if(fabs(current_hdg - desired_hdg) > 0.1) {
		// Which is the quickest direction to turn onto heading?
		if(desired_hdg > current_hdg) {
			if((desired_hdg - current_hdg) <= 180) {
				// turn right
				current_hdg += ((speed * 0.514444 * dt) / (radius * DCL_PI)) * 180.0;
				// TODO - check that increments are less than the delta that we check for the right direction
				// Probably need to reduce convergence speed as convergence is reached
			} else {
				current_hdg -= ((speed * 0.514444 * dt) / (radius * DCL_PI)) * 180.0; 	
			}
		} else {
			if((current_hdg - desired_hdg) <= 180) {
				// turn left
				current_hdg -= ((speed * 0.514444 * dt) / (radius * DCL_PI)) * 180.0;
				// TODO - check that increments are less than the delta that we check for the right direction
				// Probably need to reduce convergence speed as convergence is reached
			} else {
				current_hdg += ((speed * 0.514444 * dt) / (radius * DCL_PI)) * 180.0; 	
			}
		}	        
	}
	return(current_hdg);
}

void FGAILocalTraffic::Taxi(double dt) {
	//cout << "Taxi called" << endl;
	// Logic - if we are further away from next point than turn radius then head for it
	// If we have reached turning point then get next point and turn onto that heading
	// Look out for the finish!!

	//Point3D orthopos = ortho.ConvertToLocal(pos);	// ortho position of the plane
	desiredTaxiHeading = GetHeadingFromTo(pos, nextTaxiNode->pos);
	
	bool lastNode = (taxiPathPos == path.size() ? true : false);
	if(lastNode) {
		//cout << "LAST NODE\n";
	}

	// HACK ALERT! - for now we will taxi at constant speed for straights and turns
	
	// Remember that hdg is always equal to track when taxiing so we don't have to consider them both
	double dist_to_go = dclGetHorizontalSeparation(pos, nextTaxiNode->pos);	// we may be able to do this more cheaply using orthopos
	//cout << "dist_to_go = " << dist_to_go << endl;
	if((nextTaxiNode->type == GATE) && (dist_to_go <= 0.1)) {
		// This might be more robust to outward paths starting with a gate if we check for either
		// last node or TD_INBOUND ?
		// park up
		operatingState = PARKED;
	} else if(((dist_to_go > taxiTurnRadius) || (nextTaxiNode->type == GATE)) && (!liningUp)){
		// if the turn radius is r, and speed is s, then in a time dt we turn through
		// ((s.dt)/(PI.r)) x 180 degrees
		// or alternatively (s.dt)/r radians
		//cout << "hdg = " << hdg << " desired taxi heading = " << desiredTaxiHeading << '\n';
		hdg = TaxiTurnTowardsHeading(hdg, desiredTaxiHeading, nominalTaxiSpeed, taxiTurnRadius, dt);
		double vel = nominalTaxiSpeed;
		//cout << "vel = " << vel << endl;
		double dist = vel * 0.514444 * dt;
		//cout << "dist = " << dist << endl;
		double track = hdg;
		//cout << "track = " << track << endl;
		double slope = 0.0;
		pos = dclUpdatePosition(pos, track, slope, dist);
		//cout << "Updated position...\n";
		if(aip.getFGLocation()->get_cur_elev_m() > -9990) {
			pos.setelev(aip.getFGLocation()->get_cur_elev_m() + wheelOffset);
		} // else don't change the elev until we get a valid ground elev again!
	} else if(lastNode) {
		if(taxiState == TD_LINING_UP) {
			if((!liningUp) && (dist_to_go <= taxiTurnRadius)) {
				liningUp = true;
			}
			if(liningUp) {
				hdg = TaxiTurnTowardsHeading(hdg, rwy.hdg, nominalTaxiSpeed, taxiTurnRadius, dt);
				double vel = nominalTaxiSpeed;
				//cout << "vel = " << vel << endl;
				double dist = vel * 0.514444 * dt;
				//cout << "dist = " << dist << endl;
				double track = hdg;
				//cout << "track = " << track << endl;
				double slope = 0.0;
				pos = dclUpdatePosition(pos, track, slope, dist);
				//cout << "Updated position...\n";
				if(aip.getFGLocation()->get_cur_elev_m() > -9990) {
					pos.setelev(aip.getFGLocation()->get_cur_elev_m() + wheelOffset);
				} // else don't change the elev until we get a valid ground elev again!
				if(fabs(hdg - rwy.hdg) <= 1.0) {
					operatingState = IN_PATTERN;
					leg = TAKEOFF_ROLL;
					inAir = false;
					liningUp = false;
				}
			}
		} else if(taxiState == TD_OUTBOUND) {
			// Pause awaiting further instructions
			// and for now assume we've reached the hold-short node
			holdingShort = true;
		} // else at the moment assume TD_INBOUND always ends in a gate in which case we can ignore it
	} else {
		// Time to turn (we've already checked it's not the end we're heading for).
		// set the target node to be the next node which will prompt automatically turning onto
		// the right heading in the stuff above, with the usual provisos applied.
		GetNextTaxiNode();
		// For now why not just recursively call this function?
		Taxi(dt);
	}
}


// Warning - ground elev determination is CPU intensive
// Either this function or the logic of how often it is called
// will almost certainly change.
void FGAILocalTraffic::DoGroundElev() {

	// It would be nice if we could set the correct tile center here in order to get a correct
	// answer with one call to the function, but what I tried in the two commented-out lines
	// below only intermittently worked, and I haven't quite groked why yet.
	//SGBucket buck(pos.lon(), pos.lat());
	//aip.getFGLocation()->set_tile_center(Point3D(buck.get_center_lon(), buck.get_center_lat(), 0.0));
	
	double visibility_meters = fgGetDouble("/environment/visibility-m");
	//globals->get_tile_mgr()->prep_ssg_nodes( acmodel_location,
	globals->get_tile_mgr()->prep_ssg_nodes( aip.getFGLocation(),	visibility_meters );
	globals->get_tile_mgr()->update( aip.getFGLocation(), visibility_meters, (aip.getFGLocation())->get_absolute_view_pos() );
	// save results of update in FGLocation for fdm...
	
	//if ( globals->get_scenery()->get_cur_elev() > -9990 ) {
	//	acmodel_location->
	//	set_cur_elev_m( globals->get_scenery()->get_cur_elev() );
	//}
	
	// The need for this here means that at least 2 consecutive passes are needed :-(
	aip.getFGLocation()->set_tile_center( globals->get_scenery()->get_next_center() );
	
	//cout << "Transform Elev is " << globals->get_scenery()->get_cur_elev() << '\n';
	aip.getFGLocation()->set_cur_elev_m(globals->get_scenery()->get_cur_elev());
	//return(globals->get_scenery()->get_cur_elev());
}

