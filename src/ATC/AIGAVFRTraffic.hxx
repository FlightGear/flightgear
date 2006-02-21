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

#ifndef _FG_AIGAVFRTraffic_HXX
#define _FG_AIGAVFRTraffic_HXX

#include <simgear/math/point3d.hxx>
#include <Main/fg_props.hxx>

#include "tower.hxx"
#include "AIPlane.hxx"
#include "ATCProjection.hxx"
#include "ground.hxx"
#include "AILocalTraffic.hxx"

#include <string>
SG_USING_STD(string);

class FGAIGAVFRTraffic : public FGAILocalTraffic {
	
public:
	
	FGAIGAVFRTraffic();
	~FGAIGAVFRTraffic();
	
	// Init en-route to destID at point pt. (lat, lon, elev) (elev in meters, lat and lon in degrees).
	bool Init(const Point3D& pt, const string& destID, const string& callsign);
	// Init at srcID to fly to destID
	bool Init(const string& srcID, const string& destID, const string& callsign, OperatingState state = PARKED);
	
	// Run the internal calculations
	void Update(double dt);
	
	// Return what type of landing we're doing on this circuit
	//LandingType GetLandingOption();
	
	void RegisterTransmission(int code);
	
	// Process callbacks sent by base class
	// (These codes are not related to the codes above)
	void ProcessCallback(int code);
	
protected:
	
	// Do what is necessary to land and parkup at home airport
	void ReturnToBase(double dt);
	
	//void GetRwyDetails(string id);
	
	
private:
	FGATCMgr* ATC;	
	// This is purely for synactic convienience to avoid writing globals->get_ATC_mgr()-> all through the code!

	// High-level stuff
	OperatingState operatingState;
	bool touchAndGo;	//True if circuits should be flown touch and go, false for full stop
	
	// Performance characteristics of the plane in knots and ft/min - some of this might get moved out into FGAIPlane
	double best_rate_of_climb_speed;
	double best_rate_of_climb;
	double nominal_climb_speed;
	double nominal_climb_rate;
	double nominal_cruise_speed;
	double nominal_circuit_speed;
	double nominal_descent_rate;
	double nominal_approach_speed;
	double nominal_final_speed;
	double stall_speed_landing_config;
	
	// environment - some of this might get moved into FGAIPlane
	SGPropertyNode* wind_from_hdg;	//degrees
	SGPropertyNode* wind_speed_knots;		//knots
	
	atc_type changeFreqType;	// the service we need to change to

	void CalculateSoD(double base_leg_pos, double downwind_leg_pos, bool pattern_direction);
	
	// GA VFR specific
	bool _towerContactedIncoming;
	bool _straightIn;
	bool _clearedStraightIn;
	bool _downwindEntry;
	bool _clearedDownwindEntry;
	Point3D _wp;	// Next waypoint (ie. the one we're currently heading for)
	bool _enroute;
	string _destID;
	bool _climbout;
	double _cruise_alt;
	double _cruise_ias;
	double _cruise_climb_ias;
	Point3D _destPos;
	bool _local;
	bool _incoming;
	bool _established;
	bool _e45;
	bool _entering;
	bool _turning;
	
	//ssgBranch* _model;
	
	int GetQuadrangleAltitude(int dir, int des_alt);
	
	Point3D GetPatternApproachPos();
	
	void FlyPlane(double dt);
	
	// HACK for testing - remove or comment out before CVS commit!!!
	//bool _towerContactPrinted;
};

#endif  // _FG_AILocalTraffic_HXX
