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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/SGMath.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <Main/fg_props.hxx>

#include "ACMS.hxx"

FGACMS::FGACMS( double dt )
 : _alt    (fgGetNode("/fdm/acms/position/altitude-ft", true)),
   _speed  (fgGetNode("/fdm/acms/velocities/airspeed-kt", true)),
   _climb_rate( fgGetNode("/fdm/acms/velocities/vertical-speed-fps", true)),
   _pitch  (fgGetNode("/fdm/acms/orientation/pitch-rad", true)),
   _roll   (fgGetNode("/fdm/acms/orientation/roll-rad", true)),
   _heading(fgGetNode("/fdm/acms/orientation/heading-rad", true)),
   _acc_lat(fgGetNode("/fdm/acms/accelerations/ned/east-accel-fps_sec", true)),
   _acc_lon(fgGetNode("/fdm/acms/accelerations/ned/north-accel-fps_sec", true)),
   _acc_down(fgGetNode("/fdm/acms/accelerations/ned/down-accel-fps_sec", true)),
   _temp    (fgGetNode("/fdm/acms/environment/temperature-degc", true)),
   _wow     (fgGetNode("/fdm/acms/gear/wow", true))
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

    if (is_suspended())
        return;

    double pitch = _pitch->getDoubleValue();
    double roll = _roll->getDoubleValue();
    double heading = _heading->getDoubleValue();
    double alt = _alt->getDoubleValue();

    set_Theta(pitch);
    set_Phi(roll);
    set_Psi(heading);
    set_Altitude(alt);
    _set_Climb_Rate( _climb_rate->getDoubleValue() );


    double acc_lat = _acc_lat->getDoubleValue();
    double acc_lon = _acc_lon->getDoubleValue();
    double acc_down = _acc_down->getDoubleValue();
    _set_Accels_Local( acc_lon, acc_lat, acc_down );

    double accel = norm(SGVec3d(acc_lon, acc_lat, acc_down)) * SG_FEET_TO_METER;

    double velocity = (_speed->getDoubleValue() * SG_KT_TO_MPS) + accel * dt;
    double dist = cos (pitch) * velocity * dt;
    double kts = velocity * SG_MPS_TO_KT;
    _set_V_equiv_kts( kts );
    _set_V_calibrated_kts( kts );
    _set_V_ground_speed( kts );

    SGGeod pos = getPosition();
    // update (lon/lat) position
    SGGeod pos2;
    double az2;
    geo_direct_wgs_84 ( pos, heading * SGD_RADIANS_TO_DEGREES,
                        dist, pos2, &az2 );

    _set_Geodetic_Position(  pos2.getLatitudeRad(), pos2.getLongitudeRad(), pos.getElevationFt() );

    double sl_radius, lat_geoc;
    sgGeodToGeoc( get_Latitude(), get_Altitude(), &sl_radius, &lat_geoc );

    _set_Euler_Angles( roll, pitch, heading );
    _set_Euler_Rates(0,0,0);

    _set_Geocentric_Position( lat_geoc, get_Longitude(), sl_radius);
    _update_ground_elev_at_pos();
    _set_Sea_level_radius( sl_radius * SG_METER_TO_FEET);

}
