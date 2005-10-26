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

//#include <simgear/scene/model/location.hxx>

#include <Airports/runways.hxx>
#include <Main/globals.hxx>
//#include <Scenery/scenery.hxx>
//#include <Scenery/tilemgr.hxx>
#include <simgear/math/point3d.hxx>
//#include <simgear/math/sg_geodesy.hxx>
//#include <simgear/misc/sg_path.hxx>
#include <string>
#include <math.h>

SG_USING_STD(string);

#include "ATCmgr.hxx"
#include "AILocalTraffic.hxx"
#include "AIGAVFRTraffic.hxx"
#include "ATCutils.hxx"

FGAIGAVFRTraffic::FGAIGAVFRTraffic() {
	ATC = globals->get_ATC_mgr();
	_towerContactedIncoming = false;
	_clearedStraightIn = false;
	_clearedDownwindEntry = false;
	_incoming = false;
	_straightIn = false;
	_downwindEntry = false;
	_climbout = false;
	_local = false;
	_established = false;
	_e45 = false;
	_entering = false;
	_turning = false;
	_cruise_climb_ias = 90.0;
	_cruise_ias = 110.0;
	patternDirection = -1.0;
	
	// TESTING - REMOVE OR COMMENT OUT BEFORE COMMIT!!!
	//_towerContactPrinted = false;
}

FGAIGAVFRTraffic::~FGAIGAVFRTraffic() {
}

// We should never need to Init FGAIGAVFRTraffic in the pattern since that implies arrivel
// and we can just use an FGAILocalTraffic instance for that instead.

// Init en-route to destID at point pt.
// TODO - no idea what to do if pt is above planes ceiling due mountains!!
bool FGAIGAVFRTraffic::Init(const Point3D& pt, const string& destID, const string& callsign) {
	FGAILocalTraffic::Init(callsign, destID, EN_ROUTE);
	// TODO FIXME - to get up and running we're going to ignore elev and get FGAIMgr to 
	// pass in known good values for the test location.  Need to fix this!!! (or at least canonically decide who has responsibility for setting elev).
	_enroute = true;
	_destID = destID;
	_pos = pt;
	_destPos = dclGetAirportPos(destID);	// TODO - check if we are within the tower catchment area already.
	_cruise_alt = (_destPos.elev() + 2500.0) * SG_FEET_TO_METER;	// TODO look at terrain elevation as well
	_pos.setelev(_cruise_alt);
	// initially set waypoint as airport location
	_wp = _destPos;
	// Set the initial track
	track = GetHeadingFromTo(_pos, _wp);
	// And set the plane to keep following it.
	SetTrack(GetHeadingFromTo(_pos, _wp));
	_roll = 0.0;
	_pitch = 0.0;
	slope = 0.0;
	// TODO - set climbout if altitude is below normal cruising altitude?
	//Transform();
	// Assume it's OK to set the plane visible
	_aip.setVisible(true);
	//cout << "Setting visible true\n";
	Transform();
	return(true);
}

// Init at srcID to fly to destID
bool FGAIGAVFRTraffic::Init(const string& srcID, const string& destID, const string& callsign, OperatingState state) {
	_enroute = false;
	FGAILocalTraffic::Init(callsign, srcID, PARKED);
	return(true);
}

void FGAIGAVFRTraffic::Update(double dt) {
	if(_enroute) {
		//cout << "_enroute\n";
		//cout << "e" << flush;
		FlyPlane(dt);
		//cout << "f" << flush;
		Transform();
		//cout << "g" << flush;
		FGAIPlane::Update(dt);
		//cout << "h" << flush;
		responseCounter += dt;
		
		// we shouldn't really need this since there's a LOD of 10K on the whole plane anyway I think.
		// There are two _aip.setVisible statements set when _local = true that can be removed if the below is removed.
		if(dclGetHorizontalSeparation(_pos, Point3D(fgGetDouble("/position/longitude-deg"), fgGetDouble("/position/latitude-deg"), 0.0)) > 8000) _aip.setVisible(false);
		else _aip.setVisible(true);
		
	} else if(_local) {
		//cout << "L";
		//cout << "_local\n";
		FGAILocalTraffic::Update(dt);
	}
}

void FGAIGAVFRTraffic::FlyPlane(double dt) {
	if(_climbout) {
		// Check whether to level off
		if(_pos.elev() >= _cruise_alt) {
			slope = 0.0;
			_pitch = 0.0;
			IAS = _cruise_ias;		// FIXME - use smooth transistion to new speed and attitude.
			_climbout = false;
		} else {
			slope = 4.0;
			_pitch = 5.0;
			IAS = _cruise_climb_ias;
		}
	} else {
		// TESTING
		/*
		if(dclGetHorizontalSeparation(_destPos, _pos) / 1600.0 < 8.1) {
			if(!_towerContactPrinted) {
				if(airportID == "KSQL") {
					cout << "****************************************************************\n";
					cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
					cout << "****************************************************************\n";
				}
				_towerContactPrinted = true;
			}
		}
		*/
		
		// if distance to destination is less than 6 - 9 miles contact tower
		// and prepare to become _incoming after response.
		// Possibly check whether to start descent before this?
		//cout << "." << flush;
		//cout << "sep = " << dclGetHorizontalSeparation(_destPos, _pos) / 1600.0 << '\n';
		if(dclGetHorizontalSeparation(_destPos, _pos) / 1600.0 < 8.0) {
			//cout << "-" << flush;
			if(!_towerContactedIncoming) {
				//cout << "_" << flush;
				GetAirportDetails(airportID);
				//cout << "L" << flush;
				if(_controlled) {
					freq = (double)tower->get_freq() / 100.0;
					tuned_station = tower;
				} else {
					freq = 122.8;	// TODO - need to get the correct CTAF/Unicom frequency if no tower
					tuned_station = NULL;
				}
				//cout << "freq = " << freq << endl;
				GetRwyDetails(airportID);
				//"@AP Tower @CS @MI miles @CD of the airport for full stop with the ATIS"
				// At the bare minimum we ought to make sure it goes the right way at dual parallel rwy airports!
				if(rwy.rwyID.size() == 3) {
					patternDirection = (rwy.rwyID.substr(2,1) == "R" ? 1 : -1);
				}
				if(_controlled) {
					pending_transmission = tower->get_name();
					pending_transmission += " Tower ";
				} else {
					pending_transmission = "Traffic ";
					// TODO - find some way of getting uncontrolled airport name
				}
				pending_transmission += plane.callsign;
				//char buf[10];
				int dist_miles = (int)dclGetHorizontalSeparation(_pos, _destPos) / 1600;
				//sprintf(buf, " %i ", dist_miles);
				pending_transmission += " ";
				pending_transmission += ConvertNumToSpokenDigits(dist_miles);
				if(dist_miles > 1) pending_transmission += " miles ";
				else pending_transmission += " mile ";
				pending_transmission += GetCompassDirection(GetHeadingFromTo(_destPos, _pos));
				pending_transmission += " of the airport for full stop with the ATIS";
				//cout << pending_transmission << endl;
				Transmit(14);	// 14 is the callback code, NOT the timeout!
				responseCounter = 0;
				_towerContactedIncoming = true;
			} else {
				//cout << "?" << flush;
				if(_clearedStraightIn && responseCounter > 5.5) {
					//cout << "5 " << flush;
					_clearedStraightIn = false;
					_straightIn = true;
					_incoming = true;
					_wp = GetPatternApproachPos();
					//_hdg = GetHeadingFromTo(_pos, _wp);	// TODO - turn properly!
					SetTrack(GetHeadingFromTo(_pos, _wp));
					slope = atan((_wp.elev() - _pos.elev()) / dclGetHorizontalSeparation(_wp, _pos)) * DCL_RADIANS_TO_DEGREES;
					double thesh_offset = 0.0;
					Point3D opos = ortho.ConvertToLocal(_pos);
					double angToApt = atan((_pos.elev() - dclGetAirportElev(airportID)) / (opos.y() - thesh_offset)) * DCL_RADIANS_TO_DEGREES;
					//cout << "angToApt = " << angToApt << ' ';
					slope = (angToApt > -5.0 ? 0.0 : angToApt);
					//cout << "slope = " << slope << '\n';
					pending_transmission = "Straight-in ";
					pending_transmission += ConvertRwyNumToSpokenString(rwy.rwyID);
					pending_transmission += " ";
					pending_transmission += plane.callsign;
					//cout << pending_transmission << '\n';
					ConditionalTransmit(4);
				} else if(_clearedDownwindEntry && responseCounter > 5.5) {
					//cout << "6" << flush;
					_clearedDownwindEntry = false;
					_downwindEntry = true;
					_incoming = true;
					_wp = GetPatternApproachPos();
					SetTrack(GetHeadingFromTo(_pos, _wp));
					slope = atan((_wp.elev() - _pos.elev()) / dclGetHorizontalSeparation(_wp, _pos)) * DCL_RADIANS_TO_DEGREES;
					//cout << "slope = " << slope << '\n';
					pending_transmission = "Report ";
					pending_transmission += (patternDirection == 1 ? "right downwind " : "left downwind ");
					pending_transmission += ConvertRwyNumToSpokenString(rwy.rwyID);
					pending_transmission += " ";
					pending_transmission += plane.callsign;
					//cout << pending_transmission << '\n';
					ConditionalTransmit(4);
				}
			}
			if(_pos.elev() < (dclGetAirportElev(airportID) + (1000.0 * SG_FEET_TO_METER))) slope = 0.0;	
		}
	}
	if(_incoming) {
		//cout << "i" << '\n';
		Point3D orthopos = ortho.ConvertToLocal(_pos);
		// TODO - Check whether to start descent
		// become _local after the 3 mile report.
		if(_pos.elev() < (dclGetAirportElev(airportID) + (1000.0 * SG_FEET_TO_METER))) slope = 0.0;	
		// TODO - work out why I needed to add the above line to stop the plane going underground!!!
		// (Although it's worth leaving it in as a robustness check anyway).
		if(_straightIn) {
			//cout << "A " << flush;
			if(fabs(orthopos.x()) < 10.0 && !_established) {
				SetTrack(rwy.hdg);
				_established = true;
				//cout << "Established at " << orthopos << '\n';
			}
			double thesh_offset = 30.0;
			//cout << "orthopos.y = " << orthopos.y() << " alt = " << _pos.elev() - dclGetAirportElev(airportID) << '\n';
			if(_established && (orthopos.y() > -5400.0)) {
				slope = atan((_pos.elev() - dclGetAirportElev(airportID)) / (orthopos.y() - thesh_offset)) * DCL_RADIANS_TO_DEGREES;
				//cout << "slope0 = " << slope << '\n';
			}
			//cout << "slope1 = " << slope << '\n';
			if(slope > -5.5) slope = 0.0;	// ie we're too low.
			//cout << "slope2 = " << slope << '\n';
			slope += 0.001;		// To avoid yo-yoing with the above.
			//if(_established && (orthopos.y() > -5400.0)) slope = -5.5;
			if(_established && (orthopos.y() > -4800.0)) {
				pending_transmission = "3 mile final Runway ";
				pending_transmission += ConvertRwyNumToSpokenString(rwy.rwyID);
				pending_transmission += " ";
				pending_transmission += plane.callsign;
				//cout << pending_transmission << '\n';
				ConditionalTransmit(35);
				_local = true;
				_aip.setVisible(true);	// HACK
				_enroute = false;
				StraightInEntry(true);
			}
		} else if(_downwindEntry) {
			//cout << "B" << flush;
			if(_entering) {
				//cout << "C" << flush;
				if(_turning) {
					if(fabs(_hdg - (rwy.hdg + 180)) < 2.0) {	// TODO - use track instead of _hdg?
						//cout << "Going Local...\n";
						leg = DOWNWIND;
						_local = true;
						_aip.setVisible(true);	// HACK
						_enroute = false;
						_entering = false;
						_turning = false;
						DownwindEntry();
					}
				}
				if(fabs(orthopos.x() - (patternDirection == 1 ? 1000 : -1000)) < (_e45 ? 175 : 550)) {	// Caution - hardwired turn clearances.
					//cout << "_turning...\n";
					_turning = true;
					SetTrack(rwy.hdg + 180.0);
				}	// TODO - need to check for other traffic in the pattern and enter much more integilently than that!!!
			} else {
				//cout << "D" << flush;
				//cout << '\n' << dclGetHorizontalSeparation(_wp, _pos) << '\n';
				//cout << ortho.ConvertToLocal(_pos);
				//cout << ortho.ConvertToLocal(_wp);
				if(dclGetHorizontalSeparation(_wp, _pos) < 100.0) {
					pending_transmission = "2 miles out for ";
					pending_transmission += (patternDirection == 1 ? "right " : "left ");
					pending_transmission += "downwind Runway ";
					pending_transmission += ConvertRwyNumToSpokenString(rwy.rwyID);
					pending_transmission += " ";
					pending_transmission += plane.callsign;
					//cout << pending_transmission << '\n';
					// TODO - are we at pattern altitude??
					slope = 0.0;
					ConditionalTransmit(30);
					if(_e45) {
						SetTrack(patternDirection == 1 ? rwy.hdg - 135.0 : rwy.hdg + 135.0);
					} else {
						SetTrack(patternDirection == 1 ? rwy.hdg + 90.0 : rwy.hdg - 90.0);
					}
					//if(_hdg < 0.0) _hdg += 360.0;
					_entering = true;
				} else {
					SetTrack(GetHeadingFromTo(_pos, _wp));
				}
			}	
		}
	} else {
		// !_incoming
		slope = 0.0;
	}
	// FIXME - lots of hackery in the next six lines!!!!
	//double track = _hdg;
	double crab = 0.0;	// This is a placeholder for when we take wind into account.	
	_hdg = track + crab;
	double vel = _cruise_ias;
	double dist = vel * 0.514444 * dt;
	_pos = dclUpdatePosition(_pos, track, slope, dist);
}

void FGAIGAVFRTraffic::RegisterTransmission(int code) {
	switch(code) {
	case 1:	// taxi request cleared
		FGAILocalTraffic::RegisterTransmission(code);
		break;
	case 2:	// contact tower
		FGAILocalTraffic::RegisterTransmission(code);
		break;
	case 3: // Cleared to line up
		FGAILocalTraffic::RegisterTransmission(code);
		break;
	case 4: // cleared to take-off
		FGAILocalTraffic::RegisterTransmission(code);
		break;
	case 5: // contact ground
		FGAILocalTraffic::RegisterTransmission(code);
		break;
	case 6: // taxi to the GA parking
		FGAILocalTraffic::RegisterTransmission(code);
		break;
	case 7: // Cleared to land
		FGAILocalTraffic::RegisterTransmission(code);
		break;
	case 13: // Go around!
		FGAILocalTraffic::RegisterTransmission(code);
		break;
	case 14: // VFR approach for straight-in
		responseCounter = 0;
		_clearedStraightIn = true;
		break;
	case 15: // VFR approach for downwind entry
		responseCounter = 0;
		_clearedDownwindEntry = true;
		break;
	default:
		SG_LOG(SG_ATC, SG_WARN, "FGAIGAVFRTraffic::RegisterTransmission(...) called with unknown code " << code);
		FGAILocalTraffic::RegisterTransmission(code);
		break;
	}
}

// Callback handler
// TODO - Really should enumerate these coded values.
void FGAIGAVFRTraffic::ProcessCallback(int code) {
	// 1 - Request Departure from ground
	// 2 - Report at hold short
	// 10 - report crosswind
	// 11 - report downwind
	// 12 - report base
	// 13 - report final
	// 14 - Contact Tower for VFR arrival
	// 99 - Remove self
	if(code < 14) {
		FGAILocalTraffic::ProcessCallback(code);
	} else if(code == 14) {
		if(_controlled) {
			tower->VFRArrivalContact(plane, this, FULL_STOP);
		}
		// TODO else possibly announce arrival intentions at uncontrolled airport?
	} else if(code == 99) {
		// Might handle this different in future - hence separated from the other codes to pass to AILocalTraffic.
		FGAILocalTraffic::ProcessCallback(code);
	}
}

// Return an appropriate altitude to fly at based on the desired altitude and direction
// whilst respecting the quadrangle rule.
int FGAIGAVFRTraffic::GetQuadrangleAltitude(int dir, int des_alt) {
	return(8888);
	// TODO - implement me!
}

// Calculates the position needed to set up for either pattern entry or straight in approach.
// Currently returns one of three positions dependent on initial position wrt threshold of active rwy.
// 1/ A few miles out on extended centreline for straight-in.
// 2/ At an appropriate point on circuit side of rwy for a 45deg entry to downwind.
// 3/ At and appropriate point on non-circuit side of rwy at take-off end for perpendicular entry to circuit overflying end-of-rwy.
Point3D FGAIGAVFRTraffic::GetPatternApproachPos() {
	//cout << "\n\n";
	//cout << "Calculating pattern approach pos for " << plane.callsign << '\n';
	Point3D orthopos = ortho.ConvertToLocal(_pos);
	Point3D tmp;
	//cout << "patternDirection = " << patternDirection << '\n';
	if(orthopos.y() >= -1000.0) {	// Note that this has to be set the same as the calculation in tower.cxx - at the moment approach type is not transmitted properly between the two.
		//cout << "orthopos.x = " << orthopos.x() << '\n';
		if((orthopos.x() * patternDirection) > 0.0) {	// 45 deg entry
			tmp.setx(2000 * patternDirection);
			tmp.sety((rwy.end2ortho.y() / 2.0) + 2000);
			tmp.setelev(dclGetAirportElev(airportID) + (1000 * SG_FEET_TO_METER));
			_e45 = true;
			//cout << "45 deg entry... ";
		} else {
			tmp.setx(1000 * patternDirection * -1);
			tmp.sety(rwy.end2ortho.y());
			tmp.setelev(dclGetAirportElev(airportID) + (1000 * SG_FEET_TO_METER));
			_e45 = false;
			//cout << "90 deg entry... ";
		}
	} else {
		tmp.setx(0);
		tmp.sety(-5400);
		tmp.setelev((5400.0 / 6.0) + dclGetAirportElev(airportID) + 10.0);
		//cout << "Straight in... ";
	}
	//cout << "Waypoint is " << tmp << '\n';
	//cout << ortho.ConvertFromLocal(tmp) << '\n';
	//cout << '\n';
	//exit(-1);
	return ortho.ConvertFromLocal(tmp);
}

//FGAIGAVFRTraffic::

//FGAIGAVFRTraffic::

//FGAIGAVFRTraffic::
