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

#include <Main/globals.hxx>
#include <Main/location.hxx>
#include <Scenery/scenery.hxx>
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
    wind_from_hdg = 0.0;
    wind_speed_knots = 0.0; 
}

FGAILocalTraffic::~FGAILocalTraffic() {
}

void FGAILocalTraffic::Init() {
    // Hack alert - Hardwired path!!
    string planepath = "Aircraft/c172/Models/c172-dpm.ac";
    SGPath path = globals->get_fg_root();
    path.append(planepath);
    aip.init(planepath.c_str());
    aip.setVisible(true);
    globals->get_scenery()->get_scene_graph()->addKid(aip.getSceneGraph());

#define DCL_KEMT true
//#define DCL_KPAO true
#ifdef DCL_KEMT
    // Hardwire to KEMT for now
    // Hardwired points at each end of KEMT runway
    Point3D P010(-118.037483, 34.081358, 296 * SG_FEET_TO_METER);
    Point3D P190(-118.032308, 34.090456, 299.395263 * SG_FEET_TO_METER);
    Point3D takeoff_end;
    bool d010 = true;	// use this to change the hardwired runway direction
    if(d010) {
	rwy.threshold_pos = P010;
	takeoff_end = P190;
	rwy.hdg = 25.32;	//from default.apt
	patternDirection = -1;	// Left
	pos.setelev(rwy.threshold_pos.elev() + (-8.5 * SG_FEET_TO_METER));  // This is a complete hack - the rendered runway takes the underlying scenery elev rather than the published runway elev so I should use height above terrain or something.
    } else {
	rwy.threshold_pos = P190;
	takeoff_end = P010;
	rwy.hdg = 205.32;
	patternDirection = 1;	// Right
	pos.setelev(rwy.threshold_pos.elev() + (-0.0 * SG_FEET_TO_METER));  // This is a complete hack - the rendered runway takes the underlying scenery elev rather than the published runway elev so I should use height above terrain or something.
    }
#else	
    //KPAO - might be a better choice since its in the default scenery
    //Hardwire it to the default (no wind) direction
    Point3D threshold_end(-122.1124358, 37.45848783, 6.8 * SG_FEET_TO_METER);	// These positions are from airnav.com and don't quite seem to correspond with the sim scenery
    Point3D takeoff_end(-122.1176522, 37.463752, 6.7 * SG_FEET_TO_METER);
    rwy.threshold_pos = threshold_end;
    rwy.hdg = 315.0;
    patternDirection = 1;	// Right
    pos.setelev(rwy.threshold_pos.elev() + (-0.0 * SG_FEET_TO_METER));  // This is a complete hack - the rendered runway takes the underlying scenery elev rather than the published runway elev so I should use height above terrain or something.
#endif

    //rwy.threshold_pos.setlat(34.081358);
    //rwy.threshold_pos.setlon(-118.037483);
    //rwy.mag_hdg = 12.0;
    //rwy.mag_var = 14.0;
    //rwy.hdg = rwy.mag_hdg + rwy.mag_var;
    //rwy.threshold_pos.setelev(296 * SG_FEET_TO_METER);

    // Initial position on threshold for now
    // TODO - check wind / default runway
    pos.setlat(rwy.threshold_pos.lat());
    pos.setlon(rwy.threshold_pos.lon());
    hdg = rwy.hdg;
    
    pitch = 0.0;
    roll = 0.0;
    leg = TAKEOFF_ROLL;
    vel = 0.0;
    slope = 0.0;

    // Now set the position of the plane and then re-get the elevation!! (Didn't work - elev always returned as zero) :-(
    //aip.setPosition(pos.lon(), pos.lat(), pos.elev() * SG_METER_TO_FEET);
    //cout << "*********************** elev in FGAILocalTraffic = " << aip.getFGLocation()->get_cur_elev_m() << '\n';

    // Activate the tower - this is dependent on the ATC system being initialised before the AI system
    AirportATC a;
    if(globals->get_ATC_mgr()->GetAirportATCDetails((string)"KEMT", &a)) {
	if(a.tower_freq) {	// Has a tower
	    tower = (FGTower*)globals->get_ATC_mgr()->GetATCPointer((string)"KEMT", TOWER);	// Maybe need some error checking here
	    freq = (double)tower->get_freq() / 100.0;
	    //cout << "***********************************AILocalTraffic freq = " << freq << '\n';
	} else {
	    // Check CTAF, unicom etc
	}
    } else {
	cout << "Unable to find airport details in FGAILocalTraffic::Init()\n";
    }

    // Set the projection for the local area
    ortho.Init(rwy.threshold_pos, rwy.hdg);	
    rwy.end1ortho = ortho.ConvertToLocal(rwy.threshold_pos);	// should come out as zero
    // Hardwire to KEMT for now
    rwy.end2ortho = ortho.ConvertToLocal(takeoff_end);
    //cout << "*********************************************************************************\n";
    //cout << "*********************************************************************************\n";
    //cout << "*********************************************************************************\n";
    //cout << "end1ortho = " << rwy.end1ortho << '\n';
    //cout << "end2ortho = " << rwy.end2ortho << '\n';	// end2ortho.x() should be zero or thereabouts

    Transform();
}

// Run the internal calculations
void FGAILocalTraffic::Update(double dt) {
    // cout << "In FGAILocalTraffic::Update\n";
    // Hardwire flying traffic pattern for now - eventually also needs to be able to taxi to and from runway and GA parking area.
    FlyTrafficPattern(dt);
    Transform();
    //cout << "elev in FGAILocalTraffic = " << aip.getFGLocation()->get_cur_elev_m() << '\n';
    // This should become if(the plane has moved) then Transform()
}

// Fly a traffic pattern
// FIXME - far too much of the mechanics of turning, rolling, accellerating, descending etc is in here.
// 	   Move it out to FGAIPlane and have FlyTrafficPattern just specify what to do, not the implementation.
void FGAILocalTraffic::FlyTrafficPattern(double dt) {
    // Need to differentiate between in-air (IAS governed) and on-ground (vel governed)
    // Take-off is an interesting case - we are on the ground but takeoff speed is IAS governed.
    bool inAir = true;	// FIXME - possibly make into a class variable

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
    switch(leg) {
    case TAKEOFF_ROLL:
	inAir = false;
	track = rwy.hdg;
	if(vel < 80.0) {
	    double dveldt = 5.0;
	    vel += dveldt * dt;
	}
	IAS = vel + (cos((hdg - wind_from_hdg) * DCL_DEGREES_TO_RADIANS) * wind_speed_knots);
	if(IAS >= 70) {
	    leg = CLIMBOUT;
	    pitch = 10.0;
	    IAS = best_rate_of_climb_speed;
	    slope = 7.0;
	}
	break;
    case CLIMBOUT:
	track = rwy.hdg;
	if((pos.elev() - rwy.threshold_pos.elev()) * SG_METER_TO_FEET > 600) {
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
	if(pos.elev() <= rwy.threshold_pos.elev()) {
	    pos.setelev(rwy.threshold_pos.elev());// + (-8.5 * SG_FEET_TO_METER));  // This is a complete hack - the rendered runway takes the underlying scenery elev rather than the published runway elev so I should use height above terrain or something.
	    slope = 0.0;
	    pitch = 0.0;
	    leg = LANDING_ROLL;
	}
	break;
    case LANDING_ROLL:
	inAir = false;
	track = rwy.hdg;
	double dveldt = -5.0;
	vel += dveldt * dt;
	if(vel <= 15.0) {
	    leg = TAKEOFF_ROLL;
	}
	break;
    }

    yaw = 0.0;	//yaw = f(track, wind);
    hdg = track + yaw;
    // Apply wind to ground-relative velocity if in the air
    if(inAir) {
	vel = IAS - (cos((hdg - wind_from_hdg) * DCL_DEGREES_TO_RADIANS) * wind_speed_knots);
    }
    dist = vel * 0.514444 * dt;
    pos = dclUpdatePosition(pos, track, slope, dist);
}

void FGAILocalTraffic::TransmitPatternPositionReport(void) {
    // airport name + "traffic" + airplane callsign + pattern direction + pattern leg + rwy + ?
    string trns = "";

    trns += tower->get_name();
    trns += " Traffic ";
    // FIXME - add the callsign to the class variables
    trns += "Trainer-two-five-charlie ";
    if(patternDirection == 1) {
	trns += "right ";
    } else {
	trns += "left ";
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
    trns += convertNumToSpokenString(1);

    // And add the airport name again
    trns += tower->get_name();
    
    Transmit(trns);
}
