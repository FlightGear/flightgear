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

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/fg_geodesy.hxx>

#include <FDM/LaRCsim/ls_interface.h>
#include <Main/options.hxx>
#include <Time/timestamp.hxx>

#include "External.hxx"
#include "flight.hxx"
#include "JSBSim.hxx"
#include "LaRCsim.hxx"
#include "Balloon.h"


// base_fdm_state is the internal state that is updated in integer
// multiples of "dt".  This leads to "jitter" with respect to the real
// world time, so we introduce cur_fdm_state which is extrapolated by
// the difference between sim time and real world time

FGInterface *cur_fdm_state;
FGInterface base_fdm_state;


// Constructor
FGInterface::FGInterface(void) {
   mass=i_xx=i_yy=i_zz=i_xz=0;
   nlf=0;  
   v_rel_wind=v_true_kts=v_rel_ground=v_inertial=0;
   v_ground_speed=v_equiv=v_equiv_kts=0;
   v_calibrated=v_calibrated_kts=0;
   gravity=0;            
   centrifugal_relief=0; 
   alpha=beta=alpha_dot=beta_dot=0;   
   cos_alpha=sin_alpha=cos_beta=sin_beta=0;
   cos_phi=sin_phi=cos_theta=sin_theta=cos_psi=sin_psi=0;
   gamma_vert_rad=gamma_horiz_rad=0;    
   sigma=density=v_sound=mach_number=0;
   static_pressure=total_pressure=impact_pressure=0;
   dynamic_pressure=0;
   static_temperature=total_temperature=0;
   sea_level_radius=earth_position_angle=0;
   runway_altitude=runway_latitude=runway_longitude=0;
   runway_heading=0;
   radius_to_rwy=0;
   climb_rate=0;           
   sin_lat_geocentric=cos_lat_geocentric=0;
   sin_longitude=cos_longitude=0;
}  


// Destructor
FGInterface::~FGInterface() {
}


int FGInterface::init( double dt ) {
    cout << "dummy init() ... SHOULDN'T BE CALLED!" << endl;
    return 0;
}


int FGInterface::update( int multi_loop ) {
    cout << "dummy update() ... SHOULDN'T BE CALLED!" << endl;
    return 0;
}


// Extrapolate fdm based on time_offset (in usec)
void FGInterface::extrapolate( int time_offset ) {
    double dt = time_offset / 1000000.0;

    // -dw- metrowerks complains about ambiguous access, not critical
    // to keep this ;)
#ifndef __MWERKS__
    cout << "extrapolating FDM by dt = " << dt << endl;
#endif

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
    cur_fdm_state->set_Runway_altitude( ground_meters * METER_TO_FEET );
}


