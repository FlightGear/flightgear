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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

/*==========================================================

TODO list.

Should get pattern direction from tower.

Need to continually monitor and adjust deviation from glideslope
during descent to avoid occasionally landing short or long.

============================================================*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Airports/runways.hxx>
#include <Main/globals.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sg_path.hxx>
#include <string>
#include <math.h>

using std::string;

#include "ATCmgr.hxx"
#include "AILocalTraffic.hxx"
#include "ATCutils.hxx"
#include "AIMgr.hxx"

FGAILocalTraffic::FGAILocalTraffic() {
	ATC = globals->get_ATC_mgr();
	
	// TODO - unhardwire this
	plane.type = GA_SINGLE;
	
	_roll = 0.0;
	_pitch = 0.0;
	_hdg = 270.0;
	
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
	nominalTaxiSpeed = 7.5;
	taxiTurnRadius = 8.0;
	wheelOffset = 1.45;	// Warning - hardwired to the C172 - we need to read this in from file.
	elevInitGood = false;
	// Init the property nodes
	wind_from_hdg = fgGetNode("/environment/wind-from-heading-deg", true);
	wind_speed_knots = fgGetNode("/environment/wind-speed-kt", true);
	circuitsToFly = 0;
	liningUp = false;
	taxiRequestPending = false;
	taxiRequestCleared = false;
	holdingShort = false;
	clearedToLineUp = false;
	clearedToTakeOff = false;
	_clearedToLand = false;
	reportReadyForDeparture = false;
	contactTower = false;
	contactGround = false;
	_taxiToGA = false;
	_removeSelf = false;
	
	descending = false;
	targetDescentRate = 0.0;
	goAround = false;
	goAroundCalled = false;
	
	transmitted = false;
	
	freeTaxi = false;
	_savedSlope = 0.0;
	
	_controlled = false;
	
	_invisible = false;

        ground = NULL;
        tower = NULL;
        ourGate = NULL;
        nextTaxiNode = NULL;
        holdShortNode = NULL;
}

FGAILocalTraffic::~FGAILocalTraffic() {
}

void FGAILocalTraffic::GetAirportDetails(const string& id) {
	AirportATC a;
	if(ATC->GetAirportATCDetails(airportID, &a)) {
		if(a.tower_freq) {	// Has a tower - TODO - check the opening hours!!!
			tower = (FGTower*)ATC->GetATCPointer(airportID, TOWER);
			if(tower == NULL) {
				// Something has gone wrong - abort or carry on with un-towered operation?
				SG_LOG(SG_ATC, SG_ALERT, "ERROR - can't get a tower pointer from tower control for " << airportID << " in FGAILocalTraffic::GetAirportDetails() :-(");
				_controlled = false;
			} else {
				_controlled = true;
			}
			if(tower) {
				ground = tower->GetGroundPtr();
				if(ground == NULL) {
					// Something has gone wrong :-(
					SG_LOG(SG_ATC, SG_ALERT, "ERROR - can't get a ground pointer from tower control in FGAILocalTraffic::GetAirportDetails() :-(");
				}
			}
		} else {
			_controlled = false;
			// TODO - Check CTAF, unicom etc
		}
	} else {
		SG_LOG(SG_ATC, SG_ALERT, "Unable to find airport details in for " << airportID << " in FGAILocalTraffic::GetAirportDetails() :-(");
		_controlled = false;
	}
	// Get the airport elevation
	aptElev = fgGetAirportElev(airportID.c_str());
	//cout << "Airport elev in AILocalTraffic = " << aptElev << '\n';
	// WARNING - we use this elev for the whole airport - some assumptions in the code 
	// might fall down with very slopey airports.
}

// Get details of the active runway
// It is assumed that by the time this is called the tower control and airport code will have been set up.
void FGAILocalTraffic::GetRwyDetails(const string& id) {
	//cout << "GetRwyDetails called" << endl;
	
  const FGAirport* apt = fgFindAirportID(id);
  assert(apt);
  FGRunway* runway(apt->getActiveRunwayForUsage());

  double hdg = runway->headingDeg();
  double other_way = hdg - 180.0;
  while(other_way <= 0.0) {
    other_way += 360.0;
  }
  
    	// move to the +l end/center of the runway
		//cout << "Runway center is at " << runway._lon << ", " << runway._lat << '\n';
    	double tshlon = 0.0, tshlat = 0.0, tshr;
		double tolon = 0.0, tolat = 0.0, tor;
		rwy.length = runway->lengthM();
		rwy.width = runway->widthM();
    	geo_direct_wgs_84 ( aptElev, runway->latitude(), runway->longitude(), other_way, 
        	                rwy.length / 2.0 - 25.0, &tshlat, &tshlon, &tshr );
    	geo_direct_wgs_84 ( aptElev, runway->latitude(), runway->longitude(), hdg, 
        	                rwy.length / 2.0 - 25.0, &tolat, &tolon, &tor );
		// Note - 25 meters in from the runway end is a bit of a hack to put the plane ahead of the user.
		// now copy what we need out of runway into rwy
    	rwy.threshold_pos = SGGeod::fromDegM(tshlon, tshlat, aptElev);
		SGGeod takeoff_end = SGGeod::fromDegM(tolon, tolat, aptElev);
		//cout << "Threshold position = " << tshlon << ", " << tshlat << ", " << aptElev << '\n';
		//cout << "Takeoff position = " << tolon << ", " << tolat << ", " << aptElev << '\n';
		rwy.hdg = hdg;
		// Set the projection for the local area
		//cout << "Initing ortho for airport " << id << '\n';
		ortho.Init(rwy.threshold_pos, rwy.hdg);	
		rwy.end1ortho = ortho.ConvertToLocal(rwy.threshold_pos);	// should come out as zero
		rwy.end2ortho = ortho.ConvertToLocal(takeoff_end);
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
to initially position them where and how required.
*/
bool FGAILocalTraffic::Init(const string& callsign, const string& ICAO, OperatingState initialState, PatternLeg initialLeg) {
	//cout << "FGAILocalTraffic.Init(...) called" << endl;
	airportID = ICAO;
	
	plane.callsign = callsign;
	
	if(initialState == EN_ROUTE) return(true);
	
	// Get the ATC pointers and airport elev
	GetAirportDetails(airportID);
	
	// Get the active runway details (and copy them into rwy)
	GetRwyDetails(airportID);
	//cout << "Runway is " << rwy.rwyID << '\n';
	
	// FIXME TODO - pattern direction is still hardwired
	patternDirection = -1;		// Left
	// At the bare minimum we ought to make sure it goes the right way at dual parallel rwy airports!
	if(rwy.rwyID.size() == 3) {
		patternDirection = (rwy.rwyID.substr(2,1) == "R" ? 1 : -1);
	}
	
	if(_controlled) {
		if((initialState == PARKED) || (initialState == TAXIING)) {
			freq = (double)ground->get_freq() / 100.0;
		} else {
			freq = (double)tower->get_freq() / 100.0;
		}
	} else {
		freq = 122.8;
		// TODO - find the proper freq if CTAF or unicom or after-hours.
	}

	//cout << "In Init(), initialState = " << initialState << endl;
	operatingState = initialState;
	SGVec3d orthopos;
	switch(operatingState) {
	case PARKED:
		tuned_station = ground;
		ourGate = ground->GetGateNode();
		if(ourGate == NULL) {
			// Implies no available gates - what shall we do?
			// For now just vanish the plane - possibly we can make this more elegant in the future
			SG_LOG(SG_ATC, SG_ALERT, "No gate found by FGAILocalTraffic whilst attempting Init at " << airportID << '\n');
			return(false);
		}
		_pitch = 0.0;
		_roll = 0.0;
		vel = 0.0;
		slope = 0.0;
		_pos = ourGate->pos;
		_pos.setElevationM(aptElev);
		_hdg = ourGate->heading;
		Transform();
		
		// Now we've set the position we can do the ground elev
		elevInitGood = false;
		inAir = false;
		DoGroundElev();
		
		break;
	case TAXIING:
		//tuned_station = ground;
		// FIXME - implement this case properly
		// For now we'll assume that the plane should start at the hold short in this case
		// and that we're working without ground network elements.  Ie. an airport with no facility file.
		if(_controlled) {
			tuned_station = tower;
		} else {
			tuned_station = NULL;
		}
		freeTaxi = true;
		// Set a position and orientation in an approximate place for hold short.
		//cout << "rwy.width = " << rwy.width << '\n';
		orthopos = SGVec3d((rwy.width / 2.0 + 10.0) * -1.0, 0.0, 0.0);
		// TODO - set the x pos to be +ve if a RH parallel rwy.
		_pos = ortho.ConvertFromLocal(orthopos);
		_pos.setElevationM(aptElev);
		_hdg = rwy.hdg + 90.0;
		// TODO - reset the heading if RH rwy.
		_pitch = 0.0;
		_roll = 0.0;
		vel = 0.0;
		slope = 0.0;
		elevInitGood = false;
		inAir = false;
		Transform();
		DoGroundElev();
		//Transform();
		
		responseCounter = 0.0;
		contactTower = false;
		changeFreq = true;
		holdingShort = true;
		clearedToLineUp = false;
		changeFreqType = TOWER;
		
		break;
	case IN_PATTERN:
		// For now we'll always start the in_pattern case on the threshold ready to take-off
		// since we've got the implementation for this case already.
		// TODO - implement proper generic in_pattern startup.
		
		// 18/10/03 - adding the ability to start on downwind (mainly to speed testing of the go-around code!!)
		
		//cout << "Starting in pattern...\n";
		
		if(_controlled) {
			tuned_station = tower;
		} else {
			tuned_station = NULL;
		}
		
		circuitsToFly = 0;		// ie just fly this circuit and then stop
		touchAndGo = false;

		if(initialLeg == DOWNWIND) {
                        _pos = ortho.ConvertFromLocal(SGVec3d(1000*patternDirection, 800, 0.0));
			_pos.setElevationM(rwy.threshold_pos.getElevationM() + 1000 * SG_FEET_TO_METER);
			_hdg = rwy.hdg + 180.0;
			leg = DOWNWIND;
			elevInitGood = false;
			inAir = true;
			SetTrack(rwy.hdg - (180 * patternDirection));
			slope = 0.0;
			_pitch = 0.0;
			_roll = 0.0;
			IAS = 90.0;
			descending = false;
			_aip.setVisible(true);
			if(_controlled) {
				tower->RegisterAIPlane(plane, this, CIRCUIT, DOWNWIND);
			}
			Transform();
		} else {			
			// Default to initial position on threshold for now
                        _pos = rwy.threshold_pos;
			_hdg = rwy.hdg;
			
			// Now we've set the position we can do the ground elev
			// This might not always be necessary if we implement in-air start
			elevInitGood = false;
			inAir = false;
			
			_pitch = 0.0;
			_roll = 0.0;
			leg = TAKEOFF_ROLL;
			vel = 0.0;
			slope = 0.0;
			
			Transform();
			DoGroundElev();
		}
	
		operatingState = IN_PATTERN;
		
		break;
	case EN_ROUTE:
		// This implies we're being init'd by AIGAVFRTraffic - simple return now
		return(true);
	default:
		SG_LOG(SG_ATC, SG_ALERT, "Attempt to set unknown operating state in FGAILocalTraffic.Init(...)\n");
		return(false);
	}
	
	
	return(true);
}


// Set up downwind state - this is designed to be called from derived classes who are already tuned to tower
void FGAILocalTraffic::DownwindEntry() {
	circuitsToFly = 0;		// ie just fly this circuit and then stop
	touchAndGo = false;
	operatingState = IN_PATTERN;
	leg = DOWNWIND;
	elevInitGood = false;
	inAir = true;
	SetTrack(rwy.hdg - (180 * patternDirection));
	slope = 0.0;
	_pitch = 0.0;
	_roll = 0.0;
	IAS = 90.0;
	descending = false;
}

void FGAILocalTraffic::StraightInEntry(bool des) {
	//cout << "************ STRAIGHT-IN ********************\n";
	circuitsToFly = 0;		// ie just fly this circuit and then stop
	touchAndGo = false;
	operatingState = IN_PATTERN;
	leg = FINAL;
	elevInitGood = false;
	inAir = true;
	SetTrack(rwy.hdg);
	transmitted = true;	// TODO - fix this hack.
	// TODO - set up the next 5 properly for a descent!
	slope = -5.5;
	_pitch = 0.0;
	_roll = 0.0;
	IAS = 90.0;
	descending = des;
}


// Return what type of landing we're doing on this circuit
LandingType FGAILocalTraffic::GetLandingOption() {
	//cout << "circuitsToFly = " << circuitsToFly << '\n';
	if(circuitsToFly) {
		return(touchAndGo ? TOUCH_AND_GO : STOP_AND_GO);
	} else {
		return(FULL_STOP);
	}
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
		// HACK - assume that we're taxiing out for now
		circuitsToFly += numCircuits;
		touchAndGo = tag;
		break;
	case PARKED:
		circuitsToFly = numCircuits;	// Note that one too many circuits gets flown because we only test and decrement circuitsToFly after landing
										// thus flying one too many circuits.  TODO - Need to sort this out better!
		touchAndGo = tag;
		break;
	case EN_ROUTE:
		break;
	}
}   

// Run the internal calculations
void FGAILocalTraffic::Update(double dt) {
	//cout << "U" << flush;
	
	// we shouldn't really need this since there's a LOD of 10K on the whole plane anyway I think.
	// At the moment though I need to to avoid DList overflows - the whole plane LOD obviously isn't getting picked up.
	if(!_invisible) {
                if(dclGetHorizontalSeparation(_pos, SGGeod::fromDegM(fgGetDouble("/position/longitude-deg"), fgGetDouble("/position/latitude-deg"), 0.0)) > 8000) _aip.setVisible(false);
		else _aip.setVisible(true);
	} else {
		_aip.setVisible(false);
	}
	
	//double responseTime = 10.0;		// seconds - this should get more sophisticated at some point
	responseCounter += dt;
	if((contactTower) && (responseCounter >= 8.0)) {
		// Acknowledge request before changing frequency so it gets rendered if the user is on the same freq
		string trns = "Tower ";
		double f = globals->get_ATC_mgr()->GetFrequency(airportID, TOWER) / 100.0;	
		char buf[10];
		sprintf(buf, "%.2f", f);
		trns += buf;
		trns += " ";
		trns += plane.callsign;
		pending_transmission = trns;
		ConditionalTransmit(30.0);
		responseCounter = 0.0;
		contactTower = false;
		changeFreq = true;
		changeFreqType = TOWER;
	}
	
	if((contactGround) && (responseCounter >= 8.0)) {
		// Acknowledge request before changing frequency so it gets rendered if the user is on the same freq
		string trns = "Ground ";
		double f = globals->get_ATC_mgr()->GetFrequency(airportID, GROUND) / 100.0;	
		char buf[10];
		sprintf(buf, "%.2f", f);
		trns += buf;
		trns += " ";
		trns += "Good Day";
		pending_transmission = trns;
		ConditionalTransmit(5.0);
		responseCounter = 0.0;
		contactGround = false;
		changeFreq = true;
		changeFreqType = GROUND;
	}
	
	if((_taxiToGA) && (responseCounter >= 8.0)) {
		// Acknowledge request before changing frequency so it gets rendered if the user is on the same freq
		string trns = "GA Parking, Thank you and Good Day";
		//double f = globals->get_ATC_mgr()->GetFrequency(airportID, GROUND) / 100.0;	
		pending_transmission = trns;
		ConditionalTransmit(5.0, 99);
		_taxiToGA = false;
		if(_controlled) {
			tower->DeregisterAIPlane(plane.callsign);
		}
		// NOTE - we can't delete this instance yet since then the frequency won't get release when the message display finishes.
	}

	if((_removeSelf) && (responseCounter >= 8.0)) {
		_removeSelf = false;
		// MEGA HACK - check if we are at a simple airport or not first instead of simply hardwiring KEMT as the only non-simple airport.
		// TODO FIXME TODO FIXME !!!!!!!
		if(airportID != "KEMT") globals->get_AI_mgr()->ScheduleRemoval(plane.callsign);
	}
	
	if((changeFreq) && (responseCounter > 8.0)) {
		switch(changeFreqType) {
		case TOWER:
			if(!tower) {
				SG_LOG(SG_ATC, SG_ALERT, "ERROR: Trying to change frequency to tower in FGAILocalTraffic, but tower is NULL!!!");
				break;
			}
			tuned_station = tower;
			freq = (double)tower->get_freq() / 100.0;
			//Transmit("DING!");
			// Contact the tower, even if only virtually
			pending_transmission = plane.callsign;
			pending_transmission += " at hold short for runway ";
			pending_transmission += ConvertRwyNumToSpokenString(rwy.rwyID);
			pending_transmission += " traffic pattern ";
			if(circuitsToFly) {
				pending_transmission += ConvertNumToSpokenDigits(circuitsToFly + 1);
				pending_transmission += " circuits touch and go";
			} else {
				pending_transmission += " one circuit to full stop";
			}
			Transmit(2);
			break;
		case GROUND:
			if(!tower) {
				SG_LOG(SG_ATC, SG_ALERT, "ERROR: Trying to change frequency to ground in FGAILocalTraffic, but tower is NULL!!!");
				break;
			}
			if(!ground) {
				SG_LOG(SG_ATC, SG_ALERT, "ERROR: Trying to change frequency to ground in FGAILocalTraffic, but ground is NULL!!!");
				break;
			}
			tower->DeregisterAIPlane(plane.callsign);
			tuned_station = ground;
			freq = (double)ground->get_freq() / 100.0;
			break;
		// And to avoid compiler warnings...
		case APPROACH:  break;
		case ATIS:      break;
    case AWOS:      break;
		case ENROUTE:   break;
		case DEPARTURE: break;
		case INVALID:   break;
		}
		changeFreq = false;
	}
	
	//cout << "," << flush;
		
	switch(operatingState) {
	case IN_PATTERN:
		//cout << "In IN_PATTERN\n";
		if(!inAir) {
			DoGroundElev();
			if(!elevInitGood) {
				if(_ground_elevation_m > -9990.0) {
					_pos.setElevationM(_ground_elevation_m + wheelOffset);
					//cout << "TAKEOFF_ROLL, POS = " << pos.lon() << ", " << pos.lat() << ", " << pos.elev() << '\n';
					//Transform();
					_aip.setVisible(true);
					//cout << "Making plane visible!\n";
					elevInitGood = true;
				}
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
			if(_ground_elevation_m > -9990.0) {
				_pos.setElevationM(_ground_elevation_m + wheelOffset);
				//Transform();
				_aip.setVisible(true);
				//Transform();
				//cout << "Making plane visible!\n";
				elevInitGood = true;
			}
		}
		DoGroundElev();
		//cout << "~" << flush;
		if(!((holdingShort) && (!clearedToLineUp))) {
			//cout << "|" << flush;
			Taxi(dt);
		}
		//cout << ";" << flush;
		if((clearedToTakeOff) && (responseCounter >= 8.0)) {
			// possible assumption that we're at the hold short here - may not always hold
			// TODO - sort out the case where we're cleared to line-up first and then cleared to take-off on the rwy.
			taxiState = TD_LINING_UP;
			//cout << "A" << endl;
			path = ground->GetPath(holdShortNode, rwy.rwyID);
			//cout << "B" << endl;
			if(!path.size()) {	// Assume no facility file so we'll just taxi to a point on the runway near the threshold
				//cout << "C" << endl;
				node* np = new node;
				np->struct_type = NODE;
				np->pos = ortho.ConvertFromLocal(SGVec3d(0.0, 10.0, 0.0));
				path.push_back(np);
			} else {
				//cout << "D" << endl;
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
			clearedToTakeOff = false;	// We *are* still cleared - this simply stops the response recurring!!
			holdingShort = false;
			string trns = "Cleared for take-off ";
			trns += plane.callsign;
			pending_transmission = trns;
			Transmit();
			StartTaxi();
		}
		//cout << "^" << flush;
		Transform();
		break;
	case PARKED:
		//cout << "In PARKED\n";
		if(!elevInitGood) {
			DoGroundElev();
			if(_ground_elevation_m > -9990.0) {
				_pos.setElevationM(_ground_elevation_m + wheelOffset);
				//Transform();
				_aip.setVisible(true);
				//Transform();
				//cout << "Making plane visible!\n";
				elevInitGood = true;
			}
		}
		
		if(circuitsToFly) {
			if((taxiRequestPending) && (taxiRequestCleared)) {
				//cout << "&" << flush;
				// Get the active runway details (in case they've changed since init)
				GetRwyDetails(airportID);
				
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
				// Do some communication
				// airport name + tower + airplane callsign + location + request taxi for + operation type + ?
				string trns = "";
				if(_controlled) {
					trns += tower->get_name();
					trns += " tower ";
				} else {
					trns += "Traffic ";
					// TODO - get the airport name somehow if uncontrolled
				}
				trns += plane.callsign;
				trns += " on apron parking request taxi for traffic pattern";
				//cout << "trns = " << trns << endl;
				pending_transmission = trns;
				Transmit(1);
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
	
	//cout << "Update _pos = " << _pos << ", vis = " << _aip.getVisible() << '\n';
	
	// Convienience output for AI debugging using the property logger
	//fgSetDouble("/AI/Local1/ortho-x", (ortho.ConvertToLocal(_pos)).x());
	//fgSetDouble("/AI/Local1/ortho-y", (ortho.ConvertToLocal(_pos)).y());
	//fgSetDouble("/AI/Local1/elev", _pos.elev() * SG_METER_TO_FEET);
	
	// And finally, call parent.
	FGAIPlane::Update(dt);
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
	case 5: // contact ground
		responseCounter = 0;
		contactGround = true;
		SG_LOG(SG_ATC, SG_INFO, "AI local traffic " << plane.callsign << " told to contact ground...");
		break;
	// case 6 is a temporary mega-hack for controlled airports without separate ground control
	case 6: // taxi to the GA parking
		responseCounter = 0;
		_taxiToGA = true;
		SG_LOG(SG_ATC, SG_INFO, "AI local traffic " << plane.callsign << " told to taxi to the GA parking...");
		break;
	case 7:  // Cleared to land (also implies cleared for the option
		_clearedToLand = true;
		SG_LOG(SG_ATC, SG_INFO, "AI local traffic " << plane.callsign << " cleared to land...");
		break;
	case 13: // Go around!
		responseCounter = 0;
		goAround = true;
		_clearedToLand = false;
		SG_LOG(SG_ATC, SG_INFO, "AI local traffic " << plane.callsign << " told to go-around!!");
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
	
	// WIND
	// Wind has two effects - a mechanical one in that IAS translates to a different vel, and the hdg != track,
	// but also a piloting effect, in that the AI must be able to descend at a different rate in order to hit the threshold.
	
	//cout << "dt = " << dt << '\n';
	double dist = 0;
	// ack - I can't remember how long a rate 1 turn is meant to take.
	double turn_time = 60.0;	// seconds - TODO - check this guess
	double turn_circumference;
	double turn_radius;
	SGVec3d orthopos = ortho.ConvertToLocal(_pos);	// ortho position of the plane
	//cout << "runway elev = " << rwy.threshold_pos.getElevationM() << ' ' << rwy.threshold_pos.getElevationM() * SG_METER_TO_FEET << '\n';
	//cout << "elev = " << _pos.elev() << ' ' << _pos.elev() * SG_METER_TO_FEET << '\n';

	// HACK FOR TESTING - REMOVE
	//cout << "Calling ExitRunway..." << endl;
	//ExitRunway(orthopos);
	//return;
	// END HACK
	
	//wind
	double wind_from = wind_from_hdg->getDoubleValue();
	double wind_speed = wind_speed_knots->getDoubleValue();

	double dveldt;
	
	switch(leg) {
	case TAKEOFF_ROLL:
		//inAir = false;
		track = rwy.hdg;
		if(vel < 80.0) {
			double dveldt = 5.0;
			vel += dveldt * dt;
		}
		if(_ground_elevation_m > -9990.0) {
			_pos.setElevationM(_ground_elevation_m + wheelOffset);
		}
		IAS = vel + (cos((_hdg - wind_from) * DCL_DEGREES_TO_RADIANS) * wind_speed);
		if(IAS >= 70) {
			leg = CLIMBOUT;
			SetTrack(rwy.hdg);	// Hands over control of turning to AIPlane
			_pitch = 10.0;
			IAS = best_rate_of_climb_speed;
			//slope = 7.0;	
			slope = 6.0;	// Reduced it slightly since it's climbing a lot steeper than I can in the JSBSim C172.
			inAir = true;
		}
		break;
	case CLIMBOUT:
		// Turn to crosswind if above 700ft AND if other traffic allows
		// (decided in FGTower and accessed through GetCrosswindConstraint(...)).
		// According to AIM, traffic should climb to within 300ft of pattern altitude before commencing crosswind turn.
		// TODO - At hot 'n high airports this may be 500ft AGL though - need to make this a variable.
		if((_pos.getElevationM() - rwy.threshold_pos.getElevationM()) * SG_METER_TO_FEET > 700) {
			double cc = 0.0;
			if(_controlled && tower->GetCrosswindConstraint(cc)) {
				if(orthopos.y() > cc) {
					//cout << "Turning to crosswind, distance from threshold = " << orthopos.y() << '\n'; 
					leg = TURN1;
				}
			} else if(orthopos.y() > 1500.0) {   // Added this constraint as a hack to prevent turning too early when going around.
				// TODO - We should be doing it as a distance from takeoff end, not theshold end though.
				//cout << "Turning to crosswind, distance from threshold = " << orthopos.y() << '\n'; 
				leg = TURN1;
			}
		}
		// Need to check for levelling off in case we can't turn crosswind as soon
		// as we would like due to other traffic.
		if((_pos.getElevationM() - rwy.threshold_pos.getElevationM()) * SG_METER_TO_FEET > 1000) {
			slope = 0.0;
			_pitch = 0.0;
			IAS = 80.0;		// FIXME - use smooth transistion to new speed and attitude.
		}
		if(goAround && !goAroundCalled) {
			if(responseCounter > 5.5) {
				pending_transmission = plane.callsign;
				pending_transmission += " going around";
				Transmit();
				goAroundCalled = true;
			}
		}		
		break;
	case TURN1:
		SetTrack(rwy.hdg + (90.0 * patternDirection));
		if((track < (rwy.hdg - 89.0)) || (track > (rwy.hdg + 89.0))) {
			leg = CROSSWIND;
		}
		break;
	case CROSSWIND:
		goAround = false;
		if((_pos.getElevationM() - rwy.threshold_pos.getElevationM()) * SG_METER_TO_FEET > 1000) {
			slope = 0.0;
			_pitch = 0.0;
			IAS = 80.0;		// FIXME - use smooth transistion to new speed
		}
		// turn 1000m out for now, taking other traffic into accout
		if(fabs(orthopos.x()) > 900) {
			double dd = 0.0;
			if(_controlled && tower->GetDownwindConstraint(dd)) {
				if(fabs(orthopos.x()) > fabs(dd)) {
					//cout << "Turning to downwind, distance from centerline = " << fabs(orthopos.x()) << '\n'; 
					leg = TURN2;
				}
			} else {
				//cout << "Turning to downwind, distance from centerline = " << fabs(orthopos.x()) << '\n'; 
				leg = TURN2;
			}
		}
		break;
	case TURN2:
		SetTrack(rwy.hdg - (180 * patternDirection));
		// just in case we didn't make height on crosswind
		if((_pos.getElevationM() - rwy.threshold_pos.getElevationM()) * SG_METER_TO_FEET > 1000) {
			slope = 0.0;
			_pitch = 0.0;
			IAS = 80.0;		// FIXME - use smooth transistion to new speed
		}
		if((track < (rwy.hdg - 179.0)) || (track > (rwy.hdg + 179.0))) {
			leg = DOWNWIND;
			transmitted = false;
		}
		break;
	case DOWNWIND:
		// just in case we didn't make height on crosswind
		if(((_pos.getElevationM() - rwy.threshold_pos.getElevationM()) * SG_METER_TO_FEET > 995) && ((_pos.getElevationM() - rwy.threshold_pos.getElevationM()) * SG_METER_TO_FEET < 1015)) {
			slope = 0.0;
			_pitch = 0.0;
			IAS = 90.0;		// FIXME - use smooth transistion to new speed
		}
		if((_pos.getElevationM() - rwy.threshold_pos.getElevationM()) * SG_METER_TO_FEET >= 1015) {
			slope = -1.0;
			_pitch = -1.0;
			IAS = 90.0;		// FIXME - use smooth transistion to new speed
		}
		if((orthopos.y() < 0) && (!transmitted)) {
			TransmitPatternPositionReport();
			transmitted = true;
		}
		if((orthopos.y() < -100) && (!descending)) {
			//cout << "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDdddd\n";
			// Maybe we should think about when to start descending.
			// For now we're assuming that we aim to follow the same glidepath regardless of wind.
			double d1;
			double d2;
			CalculateSoD(((_controlled && tower->GetBaseConstraint(d1)) ? d1 : -1000.0), ((_controlled && tower->GetDownwindConstraint(d2)) ? d2 : 1000.0 * patternDirection), (patternDirection ? true : false));
			if(SoD.leg == DOWNWIND) {
				descending = (orthopos.y() < SoD.y ? true : false);
			}

		}
		if(descending) {
			slope = -5.5;	// FIXME - calculate to descent at 500fpm and hit the desired point on the runway (taking wind into account as well!!)
			_pitch = -3.0;
			IAS = 85.0;
		}
		
		// Try and arrange to turn nicely onto base
		turn_circumference = IAS * 0.514444 * turn_time;	
		//Hmmm - this is an interesting one - ground vs airspeed in relation to turn radius
		//We'll leave it as a hack with IAS for now but it needs revisiting.		
		turn_radius = turn_circumference / (2.0 * DCL_PI);
		if(orthopos.y() < -1000.0 + turn_radius) {
		//if(orthopos.y() < -980) {
			double bb = 0.0;
			if(_controlled && tower->GetBaseConstraint(bb)) {
				if(fabs(orthopos.y()) > fabs(bb)) {
					//cout << "Turning to base, distance from threshold = " << fabs(orthopos.y()) << '\n'; 
					leg = TURN3;
					transmitted = false;
					IAS = 80.0;
				}
			} else {
				//cout << "Turning to base, distance from threshold = " << fabs(orthopos.y()) << '\n'; 
				leg = TURN3;
				transmitted = false;
				IAS = 80.0;
			}
		}
		break;
	case TURN3:
		SetTrack(rwy.hdg - (90 * patternDirection));
		if(fabs(rwy.hdg - track) < 91.0) {
			leg = BASE;
		}
		break;
	case BASE:
		if(!transmitted) {
			// Base report should only be transmitted at uncontrolled airport - not towered.
			if(!_controlled) TransmitPatternPositionReport();
			transmitted = true;
		}
		
		if(!descending) {
			double d1;
			// Make downwind leg position artifically large to avoid any chance of SoD being returned as
			// on downwind when we are already on base.
			CalculateSoD(((_controlled && tower->GetBaseConstraint(d1)) ? d1 : -1000.0), (10000.0 * patternDirection), (patternDirection ? true : false));
			if(SoD.leg == BASE) {
				descending = (fabs(orthopos.y()) < fabs(SoD.y) ? true : false);
			}

		}
		if(descending) {
			slope = -5.5;	// FIXME - calculate to descent at 500fpm and hit the threshold (taking wind into account as well!!)
			_pitch = -4.0;
			IAS = 70.0;
		}
		
		// Try and arrange to turn nicely onto final
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
		SetTrack(rwy.hdg);
		if(fabs(track - rwy.hdg) < 0.6) {
			leg = FINAL;
			vel = nominal_final_speed;
		}
		break;
	case FINAL:
		if(goAround && responseCounter > 2.0) {
			leg = CLIMBOUT;
			_pitch = 8.0;
			IAS = best_rate_of_climb_speed;
			slope = 5.0;	// A bit less steep than the initial climbout.
			inAir = true;
			goAroundCalled = false;
			descending = false;
			break;
		}
		LevelWings();
		if(!transmitted) {
			if((!_controlled) || (!_clearedToLand)) TransmitPatternPositionReport();
			transmitted = true;
		}
		if(!descending) {
			// Make base leg position artifically large to avoid any chance of SoD being returned as
			// on base or downwind when we are already on final.
			CalculateSoD(-10000.0, (1000.0 * patternDirection), (patternDirection ? true : false));
			if(SoD.leg == FINAL) {
				descending = (fabs(orthopos.y()) < fabs(SoD.y) ? true : false);
			}

		}
		if(descending) {
			if(orthopos.y() < -50.0) {
				double thesh_offset = 30.0;
				slope = atan((_pos.getElevationM() - fgGetAirportElev(airportID)) / (orthopos.y() - thesh_offset)) * DCL_RADIANS_TO_DEGREES;
				//cout << "slope = " << slope << ", elev = " << _pos.elev() << ", apt_elev = " << fgGetAirportElev(airportID) << ", op.y = " << orthopos.y() << '\n';
				if(slope < -10.0) slope = -10.0;
				_savedSlope = slope;
				_pitch = -4.0;
				IAS = 70.0;
			} else {
				if(_pos.getElevationM() < (rwy.threshold_pos.getElevationM()+10.0+wheelOffset)) {
					if(_ground_elevation_m > -9990.0) {
						if(_pos.getElevationM() < (_ground_elevation_m + wheelOffset + 1.0)) {
							slope = -2.0;
							_pitch = 1.0;
							IAS = 55.0;
						} else if(_pos.getElevationM() < (_ground_elevation_m + wheelOffset + 5.0)) {
							slope = -4.0;
							_pitch = -2.0;
							IAS = 60.0;
						} else {
							slope = _savedSlope;
							_pitch = -3.0;
							IAS = 65.0;
						}
					} else {
						// Elev not determined
						slope = _savedSlope;
						_pitch = -3.0;
						IAS = 65.0;
					}
				} else {
					slope = _savedSlope;
					_pitch = -3.0;
					IAS = 65.0;
				}
			}
		}
		// Try and track the extended centreline
		SetTrack(rwy.hdg - (0.2 * orthopos.x()));
		//cout << "orthopos.x() = " << orthopos.x() << " hdg = " << hdg << '\n';
		if(_pos.getElevationM() < (rwy.threshold_pos.getElevationM()+20.0+wheelOffset)) {
			DoGroundElev();	// Need to call it here expicitly on final since it's only called
			               	// for us in update(...) when the inAir flag is false.
		}
		if(_pos.getElevationM() < (rwy.threshold_pos.getElevationM()+10.0+wheelOffset)) {
			//slope = -1.0;
			//_pitch = 1.0;
			if(_ground_elevation_m > -9990.0) {
				if((_ground_elevation_m + wheelOffset) > _pos.getElevationM()) {
					slope = 0.0;
					_pitch = 0.0;
					leg = LANDING_ROLL;
					inAir = false;
					LevelWings();
					ClearTrack();	// Take over explicit track handling since AIPlane currently always banks when changing course 
				}
			}	// else need a fallback position based on arpt elev in case ground elev determination fails?
		} else {
			// TODO
		}			
		break;
	case LANDING_ROLL:
		//inAir = false;
		descending = false;
		if(_ground_elevation_m > -9990.0) {
			_pos.setElevationM(_ground_elevation_m + wheelOffset);
		}
		track = rwy.hdg;
		dveldt = -5.0;
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
	case LEG_UNKNOWN:
		break;
    }

	if(inAir) {
		// calculate ground speed and crab from the wind triangle
		double wind_angle = GetAngleDiff_deg(wind_from + 180, track);
		double wind_side = (wind_angle < 0) ? -1.0 : 1.0;

		double sine_of_crab = wind_speed / IAS * sin(fabs(wind_angle) * DCL_DEGREES_TO_RADIANS);
		if (sine_of_crab >= 1.0) {
			// The crosswind component is greater than the IAS,
			// we can't keep the aircraft on track.
			// Assume increased IAS such that it cancels lateral speed.
			// This is unrealistic, but not sure how the rest of the sim
			// would react to the aircraft going off course.
			// Should be a rare case anyway.
			crab = wind_side * 90.0;
		} else {
			crab = asin(sine_of_crab) * DCL_RADIANS_TO_DEGREES * wind_side;
		}
		vel = cos(wind_angle * DCL_DEGREES_TO_RADIANS) * wind_speed
			+ cos(crab * DCL_DEGREES_TO_RADIANS) * IAS;
	} else {	// on the ground - crab dosen't apply
		crab = 0.0;
	}
	
	//cout << "X " << orthopos.x() << "  Y " << orthopos.y() << "  SLOPE " << slope << "  elev " << _pos.elev() * SG_METER_TO_FEET << '\n';
	
	_hdg = track + crab;
	dclBoundHeading(_hdg);
	dist = vel * 0.514444 * dt;
	_pos = dclUpdatePosition(_pos, track, slope, dist);
}

// Pattern direction is true for right, false for left
void FGAILocalTraffic::CalculateSoD(double base_leg_pos, double downwind_leg_pos, bool pattern_direction) {
	// For now we'll ignore wind and hardwire the glide angle.
	double ga = 5.5;	//degrees
	double pa = 1000.0 * SG_FEET_TO_METER;	// pattern altitude in meters
	// FIXME - get glideslope angle and pattern altitude agl from airport details if available
	
	// For convienience, we'll have +ve versions of the input distances
	double blp = fabs(base_leg_pos);
	double dlp = fabs(downwind_leg_pos);
	
	//double turn_allowance = 150.0;	// Approximate distance in meters that a 90deg corner is shortened by turned in a light plane.
	
	double stod = pa / tan(ga * DCL_DEGREES_TO_RADIANS);	// distance in meters from touchdown point to start descent
	//cout << "Descent to start = " << stod << " meters out\n";
	if(stod < blp) {	// Start descending on final
		SoD.leg = FINAL;
		SoD.y = stod * -1.0;
		SoD.x = 0.0;
	} else if(stod < (blp + dlp)) {	// Start descending on base leg
		SoD.leg = BASE;
		SoD.y = blp * -1.0;
		SoD.x = (pattern_direction ? (stod - dlp) : (stod - dlp) * -1.0);
	} else {	// Start descending on downwind leg
		SoD.leg = DOWNWIND;
		SoD.x = (pattern_direction ? dlp : dlp * -1.0);
		SoD.y = (blp - (stod - (blp + dlp))) * -1.0;
	}
}

void FGAILocalTraffic::TransmitPatternPositionReport(void) {
	// airport name + "traffic" + airplane callsign + pattern direction + pattern leg + rwy + ?
	string trns;
	int code = 0;
	const string& apt_name = _controlled ? tower->get_name() : airportID;

	trns += apt_name;
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
		code = 11;
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
		code = 13;
		break;
	default:		// Hopefully this won't be used
		trns += "pattern ";
		break;
	}
	trns += ConvertRwyNumToSpokenString(rwy.rwyID);
	
	trns += ' ';
	
	// And add the airport name again
	trns += apt_name;
	
	pending_transmission = trns;
	ConditionalTransmit(60.0, code);		// Assume a report of this leg will be invalid if we can't transmit within a minute.
}

// Callback handler
// TODO - Really should enumerate these coded values.
void FGAILocalTraffic::ProcessCallback(int code) {
	// 1 - Request Departure from ground
	// 2 - Report at hold short
	// 3 - Report runway vacated
	// 10 - report crosswind
	// 11 - report downwind
	// 12 - report base
	// 13 - report final
	if(code == 1) {
		ground->RequestDeparture(plane, this);
	} else if(code == 2) {
		if (_controlled) tower->ContactAtHoldShort(plane, this, CIRCUIT);
	} else if(code == 3) {
                if (_controlled) tower->ReportRunwayVacated(plane.callsign);
	} else if(code == 11) {
                if (_controlled) tower->ReportDownwind(plane.callsign);
	} else if(code == 13) {
                if (_controlled) tower->ReportFinal(plane.callsign);
	} else if(code == 99) { // Flag this instance for deletion
		responseCounter = 0;
		_removeSelf = true;
		SG_LOG(SG_ATC, SG_INFO, "AI local traffic " << plane.callsign << " delete instance callback called.");
	}
}

void FGAILocalTraffic::ExitRunway(const SGVec3d& orthopos) {
	//cout << "In ExitRunway" << endl;
	//cout << "Runway ID is " << rwy.ID << endl;
	
	_clearedToLand = false;
	
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
                        d = ortho.ConvertToLocal((*nItr)->pos).y() - ortho.ConvertToLocal(_pos).y(); 	//FIXME - consider making orthopos a class variable
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
			//_aip.setVisible(false);
			//cout << "Setting visible false\n";
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
		SG_LOG(SG_ATC, SG_INFO, "No exits found by FGAILocalTraffic from runway " << rwy.rwyID << " at " << airportID << '\n');
		//if(airportID == "KRHV") cout << "No exits found by " << plane.callsign << " from runway " << rwy.rwyID << " at " << airportID << '\n';
		// What shall we do - just remove the plane from sight?
		_aip.setVisible(false);
		_invisible = true;
		//cout << "Setting visible false\n";
		//tower->ReportRunwayVacated(plane.callsign);
		string trns = "Clear of the runway ";
		trns += plane.callsign;
		pending_transmission = trns;
		Transmit(3);
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
	desiredTaxiHeading = GetHeadingFromTo(_pos, nextTaxiNode->pos);
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

        desiredTaxiHeading = GetHeadingFromTo(_pos, nextTaxiNode->pos);
	
	bool lastNode = (taxiPathPos == path.size() ? true : false);
	if(lastNode) {
		//cout << "LAST NODE\n";
	}

	// HACK ALERT! - for now we will taxi at constant speed for straights and turns
	
	// Remember that hdg is always equal to track when taxiing so we don't have to consider them both
	double dist_to_go = dclGetHorizontalSeparation(_pos, nextTaxiNode->pos);	// we may be able to do this more cheaply using orthopos
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
		_hdg = TaxiTurnTowardsHeading(_hdg, desiredTaxiHeading, nominalTaxiSpeed, taxiTurnRadius, dt);
		double vel = nominalTaxiSpeed;
		//cout << "vel = " << vel << endl;
		double dist = vel * 0.514444 * dt;
		//cout << "dist = " << dist << endl;
		double track = _hdg;
		//cout << "track = " << track << endl;
		double slope = 0.0;
		_pos = dclUpdatePosition(_pos, track, slope, dist);
		//cout << "Updated position...\n";
		if(_ground_elevation_m > -9990) {
			_pos.setElevationM(_ground_elevation_m + wheelOffset);
		} // else don't change the elev until we get a valid ground elev again!
	} else if(lastNode) {
		if(taxiState == TD_LINING_UP) {
			if((!liningUp) && (dist_to_go <= taxiTurnRadius)) {
				liningUp = true;
			}
			if(liningUp) {
				_hdg = TaxiTurnTowardsHeading(_hdg, rwy.hdg, nominalTaxiSpeed, taxiTurnRadius, dt);
				double vel = nominalTaxiSpeed;
				//cout << "vel = " << vel << endl;
				double dist = vel * 0.514444 * dt;
				//cout << "dist = " << dist << endl;
				double track = _hdg;
				//cout << "track = " << track << endl;
				double slope = 0.0;
				_pos = dclUpdatePosition(_pos, track, slope, dist);
				//cout << "Updated position...\n";
				if(_ground_elevation_m > -9990) {
					_pos.setElevationM(_ground_elevation_m + wheelOffset);
				} // else don't change the elev until we get a valid ground elev again!
				if(fabs(_hdg - rwy.hdg) <= 1.0) {
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
	// Only do the proper hitlist stuff if we are within visible range of the viewer.
	double visibility_meters = fgGetDouble("/environment/visibility-m");
	FGViewer* vw = globals->get_current_view();
	if(dclGetHorizontalSeparation(_pos, SGGeod::fromGeodM(vw->getPosition(), 0.0)) > visibility_meters) {
		_ground_elevation_m = aptElev;
		return;
	}

  // FIXME: make shure the pos.lat/pos.lon values are in degrees ...
  double range = 500.0;
  if (!globals->get_tile_mgr()->scenery_available(_aip.getPosition(), range)) {
    // Try to shedule tiles for that position.
    globals->get_tile_mgr()->update( _aip.getPosition(), range );
  }

  // FIXME: make shure the pos.lat/pos.lon values are in degrees ...
  double alt;
  if (globals->get_scenery()->get_elevation_m(SGGeod::fromGeodM(_aip.getPosition(), 20000), alt, 0, _aip.getSceneGraph()))
    _ground_elevation_m = alt;
}

