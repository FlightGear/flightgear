// JSBsim.cxx -- interface to the JSBsim flight model
//
// Written by Curtis Olson, started February 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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
// (Log is kept at end of this file)


#include <Include/compiler.h>

#include STL_STRING

#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Math/fg_geodesy.hxx>

#include <FDM/JSBsim/FGFDMExec.h>
#include <FDM/JSBsim/FGAircraft.h>
#include <FDM/JSBsim/FGFCS.h>
#include <FDM/JSBsim/FGPosition.h>
#include <FDM/JSBsim/FGRotation.h>
#include <FDM/JSBsim/FGState.h>
#include <FDM/JSBsim/FGTranslation.h>

#include "JSBsim.hxx"


// The default aircraft
FGFDMExec FDMExec;


// Initialize the JSBsim flight model, dt is the time increment for
// each subsequent iteration through the EOM
int fgJSBsimInit(double dt) {
    FG_LOG( FG_FLIGHT, FG_INFO, "Starting initializing JSBsim" );

    FG_LOG( FG_FLIGHT, FG_INFO, "  created FDMExec" );

    string aircraft_path = current_options.get_fg_root() + "/Aircraft";
    string engine_path = current_options.get_fg_root() + "/Engine";

    FDMExec.GetAircraft()->LoadAircraft(aircraft_path, engine_path, "X15");
    FG_LOG( FG_FLIGHT, FG_INFO, "  loaded aircraft" );

    FDMExec.GetState()->Reset(aircraft_path, "Reset00");
    FG_LOG( FG_FLIGHT, FG_INFO, "  loaded initial conditions" );

    FDMExec.GetState()->Setdt(dt);
    FG_LOG( FG_FLIGHT, FG_INFO, "  set dt" );

    FG_LOG( FG_FLIGHT, FG_INFO, "Finished initializing JSBsim" );

    return 1;
}


// Run an iteration of the EOM (equations of motion)
int fgJSBsimUpdate(FGInterface& f, int multiloop) {
    double save_alt = 0.0;

    // lets try to avoid really screwing up the JSBsim model
    if ( f.get_Altitude() < -9000 ) {
	save_alt = f.get_Altitude();
	f.set_Altitude( 0.0 );
    }

    // copy control positions into the JSBsim structure
    FDMExec.GetFCS()->SetDa( controls.get_aileron() );
    FDMExec.GetFCS()->SetDe( controls.get_elevator() 
			     + controls.get_elevator_trim() );
    FDMExec.GetFCS()->SetDr( controls.get_rudder() );
    FDMExec.GetFCS()->SetDf( 0.0 );
    FDMExec.GetFCS()->SetDs( 0.0 );
    FDMExec.GetFCS()->SetThrottle( FGControls::ALL_ENGINES, 
				   controls.get_throttle( 0 ) );
    // FCS->SetBrake( controls.get_brake( 0 ) );

    // Inform JSBsim of the local terrain altitude
    // Runway_altitude =   f.get_Runway_altitude();

    // old -- FGInterface_2_JSBsim() not needed except for Init()
    // translate FG to JSBsim structure
    // FGInterface_2_JSBsim(f);
    // printf("FG_Altitude = %.2f\n", FG_Altitude * 0.3048);
    // printf("Altitude = %.2f\n", Altitude * 0.3048);
    // printf("Radius to Vehicle = %.2f\n", Radius_to_vehicle * 0.3048);

    /* FDMExec.GetState()->Setsim_time(State->Getsim_time() 
				    + State->Getdt() * multiloop); */

    for ( int i = 0; i < multiloop; i++ ) {
	FDMExec.Run();
    }

    // printf("%d FG_Altitude = %.2f\n", i, FG_Altitude * 0.3048);
    // printf("%d Altitude = %.2f\n", i, Altitude * 0.3048);
    
    // translate JSBsim back to FG structure so that the
    // autopilot (and the rest of the sim can use the updated
    // values

    fgJSBsim_2_FGInterface(f);

    // but lets restore our original bogus altitude when we are done
    if ( save_alt < -9000.0 ) {
	f.set_Altitude( save_alt );
    }

    return 1;
}


// Convert from the FGInterface struct to the JSBsim generic_ struct
int FGInterface_2_JSBsim (FGInterface& f) {

    return 1;
}


// Convert from the JSBsim generic_ struct to the FGInterface struct
int fgJSBsim_2_FGInterface (FGInterface& f) {

    // Velocities
    f.set_Velocities_Local( FDMExec.GetPosition()->GetVn(), 
			    FDMExec.GetPosition()->GetVe(), 
			    FDMExec.GetPosition()->GetVd() );
    // f.set_Velocities_Ground( V_north_rel_ground, V_east_rel_ground, 
    // 		     V_down_rel_ground );
    // f.set_Velocities_Local_Airmass( V_north_airmass, V_east_airmass,
    // 			    V_down_airmass );
    // f.set_Velocities_Local_Rel_Airmass( V_north_rel_airmass, 
    //                          V_east_rel_airmass, V_down_rel_airmass );
    // f.set_Velocities_Gust( U_gust, V_gust, W_gust );
    // f.set_Velocities_Wind_Body( U_body, V_body, W_body );

    // f.set_V_rel_wind( V_rel_wind );
    // f.set_V_true_kts( V_true_kts );
    // f.set_V_rel_ground( V_rel_ground );
    // f.set_V_inertial( V_inertial );
    // f.set_V_ground_speed( V_ground_speed );
    // f.set_V_equiv( V_equiv );

    /* ***FIXME*** */ f.set_V_equiv_kts( FDMExec.GetState()->GetVt() );
    // f.set_V_calibrated( V_calibrated );
    // f.set_V_calibrated_kts( V_calibrated_kts );

    f.set_Omega_Body( FDMExec.GetRotation()->GetP(), 
		      FDMExec.GetRotation()->GetQ(), 
		      FDMExec.GetRotation()->GetR() );
    // f.set_Omega_Local( P_local, Q_local, R_local );
    // f.set_Omega_Total( P_total, Q_total, R_total );
    
    // f.set_Euler_Rates( Phi_dot, Theta_dot, Psi_dot );
    // ***FIXME*** f.set_Geocentric_Rates( Latitude_dot, Longitude_dot, Radius_dot );

    // Positions
    double lat_geoc = FDMExec.GetState()->Getlatitude();
    double lon = FDMExec.GetState()->Getlongitude();
    double alt = FDMExec.GetState()->Geth();
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
	    
    f.set_Geocentric_Position( lat_geoc, lon, 
			       sl_radius2 * METER_TO_FEET + alt );
    f.set_Geodetic_Position( lat_geod, lon, alt );
    f.set_Euler_Angles( FDMExec.GetRotation()->Getphi(), 
			FDMExec.GetRotation()->Gettht(),
			FDMExec.GetRotation()->Getpsi() );

    // Miscellaneous quantities
    // f.set_T_Local_to_Body(T_local_to_body_m);
    // f.set_Gravity( Gravity );
    // f.set_Centrifugal_relief( Centrifugal_relief );

    f.set_Alpha( FDMExec.GetTranslation()->Getalpha() );
    f.set_Beta( FDMExec.GetTranslation()->Getbeta() );
    // f.set_Alpha_dot( Alpha_dot );
    // f.set_Beta_dot( Beta_dot );

    // f.set_Cos_alpha( Cos_alpha );
    // f.set_Sin_alpha( Sin_alpha );
    // f.set_Cos_beta( Cos_beta );
    // f.set_Sin_beta( Sin_beta );

    // f.set_Cos_phi( Cos_phi );
    // f.set_Sin_phi( Sin_phi );
    // f.set_Cos_theta( Cos_theta );
    // f.set_Sin_theta( Sin_theta );
    // f.set_Cos_psi( Cos_psi );
    // f.set_Sin_psi( Sin_psi );

    // ***ATTENDTOME*** f.set_Gamma_vert_rad( Gamma_vert_rad );
    // f.set_Gamma_horiz_rad( Gamma_horiz_rad );

    // f.set_Sigma( Sigma );
    // f.set_Density( Density );
    // f.set_V_sound( V_sound );
    // f.set_Mach_number( Mach_number );

    // f.set_Static_pressure( Static_pressure );
    // f.set_Total_pressure( Total_pressure );
    // f.set_Impact_pressure( Impact_pressure );
    // f.set_Dynamic_pressure( Dynamic_pressure );

    // f.set_Static_temperature( Static_temperature );
    // f.set_Total_temperature( Total_temperature );

    /* **FIXME*** */ f.set_Sea_level_radius( sl_radius2 * METER_TO_FEET );
    /* **FIXME*** */ f.set_Earth_position_angle( 0.0 );

    /* ***FIXME*** */ f.set_Runway_altitude( 0.0 );
    // f.set_Runway_latitude( Runway_latitude );
    // f.set_Runway_longitude( Runway_longitude );
    // f.set_Runway_heading( Runway_heading );
    // f.set_Radius_to_rwy( Radius_to_rwy );

    // f.set_CG_Rwy_Local( D_cg_north_of_rwy, D_cg_east_of_rwy, D_cg_above_rwy);
    // f.set_CG_Rwy_Rwy( X_cg_rwy, Y_cg_rwy, H_cg_rwy );
    // f.set_Pilot_Rwy_Local( D_pilot_north_of_rwy, D_pilot_east_of_rwy, 
    //                        D_pilot_above_rwy );
    // f.set_Pilot_Rwy_Rwy( X_pilot_rwy, Y_pilot_rwy, H_pilot_rwy );

    f.set_sin_lat_geocentric( lat_geoc );
    f.set_cos_lat_geocentric( lat_geoc );
    f.set_sin_cos_longitude( lon );
    f.set_sin_cos_latitude( lat_geod );

    return 0;
}


// $Log$
// Revision 1.1  1999/04/05 21:32:45  curt
// Initial revision
//
// Revision 1.4  1999/04/03 04:20:01  curt
// Optimizations (tm) by Norman Vine.
//
// Revision 1.3  1999/02/26 22:09:10  curt
// Added initial support for native SGI compilers.
// Integrated Jon's next version of JSBsim.
//
// Revision 1.2  1999/02/11 21:09:40  curt
// Interface with Jon's submitted JSBsim changes.
//
// Revision 1.1  1999/02/05 21:29:38  curt
// Incorporating Jon S. Berndt's flight model code.
//
