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

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <string>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Aircraft/controls.hxx>

#include "Balloon.h"


/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/


FGBalloonSim::FGBalloonSim( double dt ) {
    //set the dt of the model
    current_balloon.set_dt(dt);
}


FGBalloonSim::~FGBalloonSim() {
}


// Initialize the BalloonSim flight model, dt is the time increment for
// each subsequent iteration through the EOM
void FGBalloonSim::init() {
		
    //do init common to all the FDM's		
    common_init();
    
    //now do init specific to the Balloon

    SG_LOG( SG_FLIGHT, SG_INFO, "Starting initializing BalloonSim" );

    SG_LOG( SG_FLIGHT, SG_INFO, "  created a balloon" );

    //set position
    SGVec3f temp(get_Latitude(),
                 get_Longitude(),
                 get_Altitude() * SG_FEET_TO_METER);
    current_balloon.setPosition( temp );

    //set Euler angles (?)
    temp = SGVec3f(get_Phi(), 
                   get_Theta(), 
                   get_Psi() );
    current_balloon.setHPR( temp );

    //set velocities
    temp = SGVec3f(fgGetDouble("/sim/presets/uBody-fps"),
                   fgGetDouble("/sim/presets/vBody-fps"),
                   fgGetDouble("/sim/presets/wBody-fps") );
    current_balloon.setVelocity( temp );

    SG_LOG( SG_FLIGHT, SG_INFO, "Finished initializing BalloonSim" );
}


// Run an iteration of the EOM (equations of motion)
void FGBalloonSim::update( double dt ) {
    double save_alt = 0.0;

    if (is_suspended())
      return;

    int multiloop = _calc_multiloop(dt);

    // lets try to avoid really screwing up the BalloonSim model
    if ( get_Altitude() < -9000 ) {
	save_alt = get_Altitude();
	set_Altitude( 0.0 );
    }

    // set control positions
    current_balloon.set_burner_strength ( globals->get_controls()->get_throttle(0) );
    //not more implemented yet

    // Inform BalloonSim of the local terrain altitude
    current_balloon.setGroundLevel ( get_Runway_altitude() * SG_FEET_TO_METER);

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
    SG_LOG( SG_FLIGHT, SG_INFO, "T: " << current_balloon.getTemperature() <<
				" alt: " << temp[2] <<
				" gr_alt: " << get_Runway_altitude() << 
				" burner: " << controls.get_elevator() <<
				" v[2]: " << temp2[2]);	*/			

    // but lets restore our original bogus altitude when we are done
    if ( save_alt < -9000.0 ) {
	set_Altitude( save_alt );
    }
}


// Convert from the FGInterface struct to the BalloonSim
bool FGBalloonSim::copy_to_BalloonSim() {
    return true;
}


// Convert from the BalloonSim to the FGInterface struct
bool FGBalloonSim::copy_from_BalloonSim() {

    SGVec3f temp;

    // Velocities
    current_balloon.getVelocity( temp );
    _set_Velocities_Local( temp[0], temp[1], temp[2] );

    /* ***FIXME*** */ _set_V_equiv_kts( length( temp ) );

    _set_Omega_Body( 0.0, 0.0, 0.0 );

    // Positions
    current_balloon.getPosition( temp );
    //temp[0]: geocentric latitude
    //temp[1]: longitude
    //temp[2]: altitude (meters)

    _updateGeocentricPosition( temp[0], temp[1], temp[2] * SG_METER_TO_FEET );
    
    current_balloon.getHPR( temp );
    set_Euler_Angles( temp[0], temp[1], temp[2] );
    
    return true;
}


