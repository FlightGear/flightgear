// MagicCarpet.cxx -- interface to the "Magic Carpet" flight model
//
// Written by Curtis Olson, started October 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/sg_geodesy.hxx>

#include <Aircraft/controls.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "MagicCarpet.hxx"


FGMagicCarpet::FGMagicCarpet( double dt ) {
//     set_delta_t( dt );
}


FGMagicCarpet::~FGMagicCarpet() {
}


// Initialize the Magic Carpet flight model, dt is the time increment
// for each subsequent iteration through the EOM
void FGMagicCarpet::init() {
    common_init();
}


// Run an iteration of the EOM (equations of motion)
void FGMagicCarpet::update( double dt ) {
    // cout << "FGLaRCsim::update()" << endl;

    if (is_suspended())
      return;

    // int multiloop = _calc_multiloop(dt);

    double time_step = dt;

    // speed and distance traveled
    double speed = globals->get_controls()->get_throttle( 0 ) * 2000; // meters/sec
    if ( globals->get_controls()->get_brake_left() > 0.0
         || globals->get_controls()->get_brake_right() > 0.0 )
    {
        speed = -speed;
    }

    double dist = speed * time_step;
    double kts = speed * SG_METER_TO_NM * 3600.0;
    _set_V_equiv_kts( kts );
    _set_V_calibrated_kts( kts );
    _set_V_ground_speed( kts );

    // angle of turn
    double turn_rate = globals->get_controls()->get_aileron() * SGD_PI_4; // radians/sec
    double turn = turn_rate * time_step;

    // update euler angles
    _set_Euler_Angles( get_Phi(), get_Theta(),
                       fmod(get_Psi() + turn, SGD_2PI) );
    _set_Euler_Rates(0,0,0);

    // update (lon/lat) position
    double lat2 = 0.0, lon2 = 0.0, az2 = 0.0;
    if ( fabs( speed ) > SG_EPSILON ) {
	geo_direct_wgs_84 ( get_Altitude(),
			    get_Latitude() * SGD_RADIANS_TO_DEGREES,
			    get_Longitude() * SGD_RADIANS_TO_DEGREES,
			    get_Psi() * SGD_RADIANS_TO_DEGREES,
			    dist, &lat2, &lon2, &az2 );

        _set_Geodetic_Position( lat2 * SGD_DEGREES_TO_RADIANS,  lon2 * SGD_DEGREES_TO_RADIANS );
    }

    // cout << "lon error = " << fabs(end.x()*SGD_RADIANS_TO_DEGREES - lon2)
    //      << "  lat error = " << fabs(end.y()*SGD_RADIANS_TO_DEGREES - lat2)
    //      << endl;

    double sl_radius, lat_geoc;
    sgGeodToGeoc( get_Latitude(), get_Altitude(), &sl_radius, &lat_geoc );

    // update altitude
    double real_climb_rate = -globals->get_controls()->get_elevator() * 5000; // feet/sec
    _set_Climb_Rate( real_climb_rate / 500.0 );
    double climb = real_climb_rate * time_step;

    _set_Geocentric_Position( lat_geoc, get_Longitude(), 
			     sl_radius + get_Altitude() + climb );
    // cout << "sea level radius (ft) = " << sl_radius << endl;
    // cout << "(setto) sea level radius (ft) = " << get_Sea_level_radius() << endl;
    _update_ground_elev_at_pos();
    _set_Sea_level_radius( sl_radius * SG_METER_TO_FEET);
    _set_Altitude( get_Altitude() + climb );
    _set_Altitude_AGL( get_Altitude() - get_Runway_altitude() );
}
