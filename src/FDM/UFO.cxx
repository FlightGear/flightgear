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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/sg_geodesy.hxx>

#include <Aircraft/controls.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "UFO.hxx"

double FGUFO::lowpass::_dt;


FGUFO::FGUFO( double dt ) :
    Throttle(new lowpass(fgGetDouble("/controls/damping/throttle", 0.1))),
    Aileron(new lowpass(fgGetDouble("/controls/damping/aileron", 0.65))),
    Elevator(new lowpass(fgGetDouble("/controls/damping/elevator", 0.65))),
    Rudder(new lowpass(fgGetDouble("/controls/damping/rudder", 0.05))),
    Aileron_Trim(new lowpass(fgGetDouble("/controls/damping/aileron-trim", 0.65))),
    Elevator_Trim(new lowpass(fgGetDouble("/controls/damping/elevator-trim", 0.65))),
    Rudder_Trim(new lowpass(fgGetDouble("/controls/damping/rudder-trim", 0.05))),
    Speed_Max(fgGetNode("/engines/engine/speed-max-mps", true))
{
}


FGUFO::~FGUFO() {
    delete Throttle;
    delete Aileron;
    delete Elevator;
    delete Rudder;
    delete Aileron_Trim;
    delete Elevator_Trim;
    delete Rudder_Trim;
}


// Initialize the UFO flight model, dt is the time increment
// for each subsequent iteration through the EOM
void FGUFO::init() {
    common_init();
    if (Speed_Max->getDoubleValue() < 0.01)
        Speed_Max->setDoubleValue(2000.0);
}


// Run an iteration of the EOM (equations of motion)
void FGUFO::update( double dt ) {

    if (is_suspended())
        return;

    lowpass::set_delta(dt);
    double time_step = dt;
    FGControls *ctrl = globals->get_controls();

    // read the throttle
    double throttle = ctrl->get_throttle( 0 );
    double brake_left = ctrl->get_brake_left();
    double brake_right = ctrl->get_brake_right();

    if (brake_left > 0.5 || brake_right > 0.5)
        throttle = -throttle;

    double velocity = Throttle->filter(throttle) * Speed_Max->getDoubleValue(); // meters/sec


    // read and lowpass-filter the state of the control surfaces
    double aileron = Aileron->filter(ctrl->get_aileron());
    double elevator = Elevator->filter(ctrl->get_elevator());
    double rudder = Rudder->filter(ctrl->get_rudder());

    aileron += Aileron_Trim->filter(ctrl->get_aileron_trim());
    elevator += Elevator_Trim->filter(ctrl->get_elevator_trim());
    rudder += Rudder_Trim->filter(ctrl->get_rudder_trim());

    double old_pitch = get_Theta();
    double pitch_rate = SGD_PI_4;  // assume I will be pitching up
    double target_pitch = -elevator * SGD_PI_2;

    if (old_pitch > target_pitch)  // pitching down
        pitch_rate *= -1;

    double pitch = old_pitch + (pitch_rate * time_step);

    if (pitch_rate > 0.0) {        // pitching up
        if (pitch > target_pitch)
            pitch = target_pitch;

    } else if (pitch_rate < 0.0) { // pitching down
        if (pitch < target_pitch)
            pitch = target_pitch;
    }

    double old_roll    = get_Phi();
    double roll_rate   = SGD_PI_4;
    double target_roll = aileron * SGD_PI_2;

    if (old_roll > target_roll)
        roll_rate *= -1;

    double roll = old_roll + (roll_rate * time_step);

    if (roll_rate > 0.0) {         // rolling CW
        if (roll > target_roll)
            roll = target_roll;

    } else if (roll_rate < 0.0) {  // rolling CCW
        if (roll < target_roll)
            roll = target_roll;
    }

    // the vertical speed of the aircraft
    double real_climb_rate = sin (pitch) * SG_METER_TO_FEET * velocity; // feet/sec
    _set_Climb_Rate( -elevator * 10.0 );
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
    double yaw = fabs(rudder) < .2 ? 0.0 : rudder / (25 + fabs(speed) * .1);

    // update (lon/lat) position
    double lat2 = 0.0, lon2 = 0.0, az2 = 0.0;
    if ( fabs(speed) > SG_EPSILON ) {
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

    // update euler angles
    double heading = fmod(get_Psi() + turn + yaw, SGD_2PI);
    _set_Euler_Angles(roll, pitch, heading);
    _set_Euler_Rates(0,0,0);

    _set_Geocentric_Position( lat_geoc, get_Longitude(),
                             sl_radius + get_Altitude() + climb );
    // cout << "sea level radius (ft) = " << sl_radius << endl;
    // cout << "(setto) sea level radius (ft) = " << get_Sea_level_radius() << endl;
    _update_ground_elev_at_pos();
    _set_Sea_level_radius( sl_radius * SG_METER_TO_FEET);
    _set_Altitude( get_Altitude() + climb );
    _set_Altitude_AGL( get_Altitude() - get_Runway_altitude() );

    set_V_north(cos(heading) * velocity * SG_METER_TO_FEET);
    set_V_east(sin(heading) * velocity * SG_METER_TO_FEET);
    set_V_down(-real_climb_rate);
}


