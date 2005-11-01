// UFO.cxx -- interface to the "UFO" flight model
//
// Written by Curtis Olson, started October 1999.
// Slightly modified from MagicCarpet.cxx by Jonathan Polley, April 2002
//
// Copyright (C) 1999-2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>

#include <Aircraft/controls.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "UFO.hxx"

const double throttle_damp = 0.2;
const double aileron_damp = 0.05;
const double elevator_damp = 0.05;
const double rudder_damp = 0.4;

FGUFO::FGUFO( double dt )
  : Throttle(0.0),
    Aileron(0.0),
    Elevator(0.0),
    Rudder(0.0)
{
//     set_delta_t( dt );
}


FGUFO::~FGUFO() {
}


// Initialize the UFO flight model, dt is the time increment
// for each subsequent iteration through the EOM
void FGUFO::init() {
    common_init();
}


// Run an iteration of the EOM (equations of motion)
void FGUFO::update( double dt ) {
    // cout << "FGLaRCsim::update()" << endl;

    if (is_suspended())
      return;

    double time_step = dt;

    // read the throttle
    double th = globals->get_controls()->get_throttle( 0 );
    if ( globals->get_controls()->get_brake_left() > 0.0 
         || globals->get_controls()->get_brake_right() > 0.0 )
    {
        th = -th;
    }
    Throttle = th * throttle_damp + Throttle * (1 - throttle_damp);

    // read the state of the control surfaces
    Aileron  = globals->get_controls()->get_aileron() * aileron_damp
               + Aileron * (1 - aileron_damp);
    Elevator = globals->get_controls()->get_elevator() * elevator_damp
               + Elevator * (1 - elevator_damp);
    Rudder = globals->get_controls()->get_rudder() * rudder_damp
               + Rudder * (1 - rudder_damp);

    // the velocity of the aircraft
    double velocity = Throttle * 2000; // meters/sec

    double old_pitch = get_Theta();
    double pitch_rate = SGD_PI_4; // assume I will be pitching up
    double target_pitch = -Elevator * SGD_PI_2;

    // if I am pitching down
    if (old_pitch > target_pitch)
        // set the pitch rate to negative (down)
        pitch_rate *= -1;

    double pitch = old_pitch + (pitch_rate * time_step);

    // if I am pitching up
    if (pitch_rate > 0.0)
    {
        // clip the pitch at the limit
        if ( pitch > target_pitch)
        {
            pitch = target_pitch;
        }
    }
    // if I am pitching down
    else if (pitch_rate < 0.0)
    {
        // clip the pitch at the limit
        if ( pitch < target_pitch)
        {
            pitch = target_pitch;
        }
    }

    double old_roll     = get_Phi();
    double roll_rate    = SGD_PI_4;
    double target_roll  = Aileron * SGD_PI_2;

    if (old_roll > target_roll)
        roll_rate *= -1;
    
    double roll = old_roll + (roll_rate * time_step);

    // if I am rolling CW
    if (roll_rate > 0.0)
    {
        // clip the roll at the limit
        if ( roll > target_roll)
        {
            roll = target_roll;
        }
    }
    // if I am rolling CCW
    else if (roll_rate < 0.0)
    {
        // clip the roll at the limit
        if ( roll < target_roll)
        {
            roll = target_roll;
        }
    }

    // the vertical speed of the aircraft
    double real_climb_rate = sin (pitch) * SG_METER_TO_FEET * velocity; // feet/sec
    _set_Climb_Rate( -Elevator * 10.0 );
    double climb = real_climb_rate * time_step;

    // the lateral speed of the aircraft
    double speed = cos (pitch) * velocity; // meters/sec
    double dist = speed * time_step;
    double kts = velocity * SG_METER_TO_NM * 3600.0;
    _set_V_equiv_kts( kts );
    _set_V_calibrated_kts( kts );
    _set_V_ground_speed( kts );

    // angle of turn
    double turn_rate = sin(roll) * SGD_PI_4; // radians/sec
    double turn = turn_rate * time_step;
    double yaw = fabs(Rudder) < .2 ? 0.0 : Rudder / (25 + fabs(speed) * .1);

    // update (lon/lat) position
    double lat2, lon2, az2;
    if ( fabs(speed) > SG_EPSILON ) {
	geo_direct_wgs_84 ( get_Altitude(),
			    get_Latitude() * SGD_RADIANS_TO_DEGREES,
			    get_Longitude() * SGD_RADIANS_TO_DEGREES,
			    get_Psi() * SGD_RADIANS_TO_DEGREES,
			    dist, &lat2, &lon2, &az2 );

	_set_Longitude( lon2 * SGD_DEGREES_TO_RADIANS );
	_set_Latitude( lat2 * SGD_DEGREES_TO_RADIANS );
    }

    // cout << "lon error = " << fabs(end.x()*SGD_RADIANS_TO_DEGREES - lon2)
    //      << "  lat error = " << fabs(end.y()*SGD_RADIANS_TO_DEGREES - lat2)
    //      << endl;

    double sl_radius, lat_geoc;
    sgGeodToGeoc( get_Latitude(), get_Altitude(), &sl_radius, &lat_geoc );

    // update euler angles
    _set_Euler_Angles( roll, pitch,
                       fmod(get_Psi() + turn + yaw, SGD_2PI) );
    _set_Euler_Rates(0,0,0);

    _set_Geocentric_Position( lat_geoc, get_Longitude(), 
			     sl_radius + get_Altitude() + climb );
    // cout << "sea level radius (ft) = " << sl_radius << endl;
    // cout << "(setto) sea level radius (ft) = " << get_Sea_level_radius() << endl;
    _update_ground_elev_at_pos();
    _set_Sea_level_radius( sl_radius * SG_METER_TO_FEET);
    _set_Altitude( get_Altitude() + climb );
}
