// ACMS.cxx -- interface to the ACMS FDM
//
// Written by Erik Hofman, started October 2004
//
// Copyright (C) 2004  Erik Hofman <erik@ehofman.com>
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

#include <simgear/math/sg_geodesy.hxx>
#include <Main/fg_props.hxx>

#include "ACMS.hxx"

FGACMS::FGACMS( double dt )
{
//     set_delta_t( dt );
}


FGACMS::~FGACMS() {
}


// Initialize the ACMSFDM flight model, dt is the time increment
// for each subsequent iteration through the EOM
void FGACMS::init() {
    common_init();
}


// Run an iteration of the EOM (equations of motion)
void FGACMS::update( double dt ) {

    double pitch = get_Theta();
    double roll = get_Phi();
    double heading = get_Psi() * SG_DEGREES_TO_RADIANS;
    double alt = get_Altitude();

    double sl_radius, lat_geoc;
    sgGeodToGeoc( get_Latitude(), get_Altitude(), &sl_radius, &lat_geoc );

    double lon_acc = get_V_north();
    double lat_acc = get_V_east();
    double vert_acc = get_V_down();

    double accel_heading = atan( lon_acc/lat_acc );
    double accel_pitch = atan( vert_acc/accel_heading );

    double accel = sqrt(sqrt(lon_acc*lon_acc + lat_acc*lat_acc)
                        + vert_acc*vert_acc);

    double velocity = get_V_true_kts() * accel / (SG_METER_TO_NM * 3600.0);
    double speed = cos (pitch) * velocity; // meters/sec
    double dist = speed * dt;
    double kts = velocity * 6076.11549 * 3600.0;

    double climb_rate = fgGetDouble("/velocities/climb-rate", 0);
    double climb = climb_rate * dt;

    _set_Alpha( pitch - accel_pitch);
    _set_Beta( heading - accel_heading);
    _set_Climb_Rate( climb_rate );
    _set_V_equiv_kts( kts );
    _set_V_calibrated_kts( kts );
    _set_V_ground_speed( kts );
    _set_Altitude( get_Altitude() + climb );

    // update (lon/lat) position
    double lat2, lon2, az2;
    geo_direct_wgs_84 ( get_Altitude(),
                        get_Latitude() * SGD_RADIANS_TO_DEGREES,
                        get_Longitude() * SGD_RADIANS_TO_DEGREES,
                        get_Psi() * SGD_RADIANS_TO_DEGREES,
                        dist, &lat2, &lon2, &az2 );
    _set_Geocentric_Position( lat_geoc, get_Longitude(),
                             sl_radius + get_Altitude() + climb );
    _set_Sea_level_radius( sl_radius * SG_METER_TO_FEET);

    _set_Longitude( lon2 * SGD_DEGREES_TO_RADIANS );
    _set_Latitude( lat2 * SGD_DEGREES_TO_RADIANS );

}
