// flight.c -- a general interface to the various flight models
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
//
// $Id$


#include <stdio.h>

#include <Debug/logstream.hxx>
#include <FDM/External/external.hxx>
#include <FDM/LaRCsim/ls_interface.h>
#include <Include/fg_constants.h>
#include <Math/fg_geodesy.hxx>
#include <Time/timestamp.hxx>

#include "flight.hxx"
#include "JSBsim.hxx"
#include "LaRCsim.hxx"


// base_fdm_state is the internal state that is updated in integer
// multiples of "dt".  This leads to "jitter" with respect to the real
// world time, so we introduce cur_fdm_state which is extrapolated by
// the difference between sim time and real world time

FGInterface cur_fdm_state;
FGInterface base_fdm_state;


// Extrapolate fdm based on time_offset (in usec)
void FGInterface::extrapolate( int time_offset ) {
    double dt = time_offset / 1000000.0;
    cout << "extrapolating FDM by dt = " << dt << endl;

    double lat = geodetic_position_v[0] + geocentric_rates_v[0] * dt;
    double lat_geoc = geocentric_position_v[0] + geocentric_rates_v[0] * dt;

    double lon = geodetic_position_v[1] + geocentric_rates_v[1] * dt;
    double lon_geoc = geocentric_position_v[1] + geocentric_rates_v[1] * dt;

    double alt = geodetic_position_v[2] + geocentric_rates_v[2] * dt;
    double radius = geocentric_position_v[2] + geocentric_rates_v[2] * dt;

    geodetic_position_v[0] = lat;
    geocentric_position_v[0] = lat_geoc;

    geodetic_position_v[1] = lon;
    geocentric_position_v[1] = lon_geoc;

    geodetic_position_v[2] = alt;
    geocentric_position_v[2] = radius;
}


// Initialize the flight model parameters
int fgFDMInit(int model, FGInterface& f, double dt) {
    double save_alt = 0.0;

    FG_LOG( FG_FLIGHT ,FG_INFO, "Initializing flight model" );

    base_fdm_state = f;

    if ( model == FGInterface::FG_SLEW ) {
	// fgSlewInit(dt);
    } else if ( model == FGInterface::FG_JSBSIM ) {
	fgJSBsimInit(dt);
	fgJSBsim_2_FGInterface(base_fdm_state);
    } else if ( model == FGInterface::FG_LARCSIM ) {
	// lets try to avoid really screwing up the LaRCsim model
	if ( base_fdm_state.get_Altitude() < -9000.0 ) {
	    save_alt = base_fdm_state.get_Altitude();
	    base_fdm_state.set_Altitude( 0.0 );
	}

	// translate FG to LaRCsim structure
	FGInterface_2_LaRCsim(base_fdm_state);

	// initialize LaRCsim
	fgLaRCsimInit(dt);

	FG_LOG( FG_FLIGHT, FG_INFO, "FG pos = " << 
		base_fdm_state.get_Latitude() );

	// translate LaRCsim back to FG structure
	fgLaRCsim_2_FGInterface(base_fdm_state);

	// but lets restore our original bogus altitude when we are done
	if ( save_alt < -9000.0 ) {
	    base_fdm_state.set_Altitude( save_alt );
	}
    } else if ( model == FGInterface::FG_EXTERNAL ) {
	fgExternalInit(base_fdm_state);
    } else {
	FG_LOG( FG_FLIGHT, FG_WARN,
		"Unimplemented flight model == " << model );
    }

    // set valid time for this record
    base_fdm_state.stamp_time();
	
    f = base_fdm_state;

    return 1;
}


// Run multiloop iterations of the flight model
int fgFDMUpdate(int model, FGInterface& f, int multiloop, int time_offset) {
    double time_step, start_elev, end_elev;

    // printf("Altitude = %.2f\n", FG_Altitude * 0.3048);

    // set valid time for this record
    base_fdm_state.stamp_time();

    time_step = (1.0 / DEFAULT_MODEL_HZ) * multiloop;
    start_elev = base_fdm_state.get_Altitude();

    if ( model == FGInterface::FG_SLEW ) {
	// fgSlewUpdate(f, multiloop);
    } else if ( model == FGInterface::FG_JSBSIM ) {
	fgJSBsimUpdate(base_fdm_state, multiloop);
	f = base_fdm_state;
    } else if ( model == FGInterface::FG_LARCSIM ) {
	fgLaRCsimUpdate(base_fdm_state, multiloop);
	// extrapolate position based on actual time
	// f = extrapolate_fdm( base_fdm_state, time_offset );
	f = base_fdm_state;
    } else if ( model == FGInterface::FG_EXTERNAL ) {
	// fgExternalUpdate(f, multiloop);
	FGTimeStamp current;
	current.stamp();
	f = base_fdm_state;
	f.extrapolate( current - base_fdm_state.get_time_stamp() );
    } else {
	FG_LOG( FG_FLIGHT, FG_WARN,
		"Unimplemented flight model == " <<  model );
    }

    end_elev = base_fdm_state.get_Altitude();

    if ( time_step > 0.0 ) {
	// feet per second
	base_fdm_state.set_Climb_Rate( (end_elev - start_elev) / time_step );
    }

    return 1;
}


// Set the altitude (force)
void fgFDMForceAltitude(int model, double alt_meters) {
    double sea_level_radius_meters;
    double lat_geoc;

    // Set the FG variables first
    fgGeodToGeoc( base_fdm_state.get_Latitude(), alt_meters, 
		  &sea_level_radius_meters, &lat_geoc);

    base_fdm_state.set_Altitude( alt_meters * METER_TO_FEET );
    base_fdm_state.set_Radius_to_vehicle( base_fdm_state.get_Altitude() + 
					  (sea_level_radius_meters * 
					   METER_TO_FEET) );

    // additional work needed for some flight models
    if ( model == FGInterface::FG_LARCSIM ) {
	ls_ForceAltitude( base_fdm_state.get_Altitude() );
    }
}


// Set the local ground elevation
void fgFDMSetGroundElevation(int model, double ground_meters) {
    base_fdm_state.set_Runway_altitude( ground_meters * METER_TO_FEET );
    cur_fdm_state.set_Runway_altitude( ground_meters * METER_TO_FEET );
}


