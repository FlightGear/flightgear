/*****************************************************************************

 Module:       BalloonSimInterface.cxx
 Author:       Christian Mayer
 Date started: 07.10.99
 Called by:    

 -------- Copyright (C) 1999 Christian Mayer (fgfs@christianmayer.de) --------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
------------------------------------------------------------------------------
Interface to the hot air balloon simulator

HISTORY
------------------------------------------------------------------------------
01.09.1999 Christian Mayer	Created
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/

#include <Include/compiler.h>

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_STRING

#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Math/fg_geodesy.hxx>
#include <Misc/fgpath.hxx>

#include "Balloon.h"

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/


// Initialize the BalloonSim flight model, dt is the time increment for
// each subsequent iteration through the EOM
int FGBalloonSim::init( double dt ) {
    sgVec3 temp;

    FG_LOG( FG_FLIGHT, FG_INFO, "Starting initializing BalloonSim" );

    FG_LOG( FG_FLIGHT, FG_INFO, "  created a balloon" );

    //set the dt of the model
    current_balloon.set_dt(dt);

    //set position
    sgSetVec3( temp,
	get_Latitude(), 
	get_Longitude(), 
	get_Altitude() * FEET_TO_METER);
    current_balloon.setPosition( temp );

    //set Euler angles (?)
    sgSetVec3( temp,
	get_Phi(), 
	get_Theta(), 
	get_Psi() );
    current_balloon.setHPR( temp );

    //set velocities
    sgSetVec3( temp,
	current_options.get_uBody(), 
	current_options.get_vBody(), 
	current_options.get_wBody() );
    current_balloon.setVelocity( temp );

    FG_LOG( FG_FLIGHT, FG_INFO, "Finished initializing BalloonSim" );

    return 1;
}


// Run an iteration of the EOM (equations of motion)
int FGBalloonSim::update( int multiloop ) {
    double save_alt = 0.0;

    // lets try to avoid really screwing up the BalloonSim model
    if ( get_Altitude() < -9000 ) {
	save_alt = get_Altitude();
	set_Altitude( 0.0 );
    }

    // set control positions
    current_balloon.set_burner_strength ( controls.get_elevator() ); /*controls.get_throttle( 0 )*/
    //not more implemented yet

    // Inform BalloonSim of the local terrain altitude
    current_balloon.setGroundLevel ( get_Runway_altitude() * FEET_TO_METER);

    // old -- FGInterface_2_JSBsim() not needed except for Init()
    // translate FG to JSBsim structure
    // FGInterface_2_JSBsim(f);
    // printf("FG_Altitude = %.2f\n", FG_Altitude * 0.3048);
    // printf("Altitude = %.2f\n", Altitude * 0.3048);
    // printf("Radius to Vehicle = %.2f\n", Radius_to_vehicle * 0.3048);

    /* FDMExec.GetState()->Setsim_time(State->Getsim_time() 
				    + State->Getdt() * multiloop); */

    for ( int i = 0; i < multiloop; i++ ) {
	current_balloon.update(); 
    }

    // translate BalloonSim back to FG structure so that the
    // autopilot (and the rest of the sim can use the updated
    // values

    copy_from_BalloonSim();
    
    /*sgVec3 temp, temp2;
    current_balloon.getPosition( temp );
    current_balloon.getVelocity( temp2 );
    FG_LOG( FG_FLIGHT, FG_INFO, "T: " << current_balloon.getTemperature() <<
				" alt: " << temp[2] <<
				" gr_alt: " << get_Runway_altitude() << 
				" burner: " << controls.get_elevator() <<
				" v[2]: " << temp2[2]);	*/			

    // but lets restore our original bogus altitude when we are done
    if ( save_alt < -9000.0 ) {
	set_Altitude( save_alt );
    }

    return 1;
}


// Convert from the FGInterface struct to the BalloonSim
int FGBalloonSim::copy_to_BalloonSim() {
    return 1;
}


// Convert from the BalloonSim to the FGInterface struct
int FGBalloonSim::copy_from_BalloonSim() {

    sgVec3 temp;

    // Velocities
    current_balloon.getVelocity( temp );
    set_Velocities_Local( temp[0], temp[1], temp[2] );

    /* ***FIXME*** */ set_V_equiv_kts( sgLengthVec3 ( temp ) );

    set_Omega_Body( 0.0, 0.0, 0.0 );

    // Positions
    current_balloon.getPosition( temp );
    double lat_geoc = temp[0];
    double lon	    = temp[1];
    double alt	    = temp[2] * METER_TO_FEET;

    double lat_geod, tmp_alt, sl_radius1, sl_radius2, tmp_lat_geoc;
    fgGeocToGeod( lat_geoc, EQUATORIAL_RADIUS_M + alt * FEET_TO_METER,
		  &lat_geod, &tmp_alt, &sl_radius1 );
    fgGeodToGeoc( lat_geod, alt * FEET_TO_METER, &sl_radius2, &tmp_lat_geoc );

    FG_LOG( FG_FLIGHT, FG_DEBUG, "lon = " << lon << " lat_geod = " << lat_geod
	    << " lat_geoc = " << lat_geoc
	    << " alt = " << alt << " tmp_alt = " << tmp_alt * METER_TO_FEET
	    << " sl_radius1 = " << sl_radius1 * METER_TO_FEET
	    << " sl_radius2 = " << sl_radius2 * METER_TO_FEET
	    << " Equator = " << EQUATORIAL_RADIUS_FT );
	    
    set_Geocentric_Position( lat_geoc, lon, 
			       sl_radius2 * METER_TO_FEET + alt );
    set_Geodetic_Position( lat_geod, lon, alt );

    current_balloon.getHPR( temp );
    set_Euler_Angles( temp[0], temp[1], temp[2] );


    set_Alpha( 0.0/*FDMExec.GetTranslation()->Getalpha()*/ );
    set_Beta( 0.0/*FDMExec.GetTranslation()->Getbeta()*/ );

    /* **FIXME*** */ set_Sea_level_radius( sl_radius2 * METER_TO_FEET );
    /* **FIXME*** */ set_Earth_position_angle( 0.0 );

    /* ***FIXME*** */ set_Runway_altitude( 0.0 );

    set_sin_lat_geocentric( lat_geoc );
    set_cos_lat_geocentric( lat_geoc );
    set_sin_cos_longitude( lon );
    set_sin_cos_latitude( lat_geod );

    return 0;
}


