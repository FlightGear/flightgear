// LaRCsim.cxx -- interface to the LaRCsim flight model
//
// Written by Curtis Olson, started October 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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


#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>

#include <Scenery/scenery.hxx>

#include <Main/fg_props.hxx>
#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <FDM/flight.hxx>
#include <FDM/LaRCsim/ls_cockpit.h>
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_interface.h>
#include <FDM/LaRCsimIC.hxx>
#include <FDM/UIUCModel/uiuc_aircraft.h>

#include "IO360.hxx"
#include "LaRCsim.hxx"

FGLaRCsim::FGLaRCsim( double dt ) {
    set_delta_t( dt );

    speed_up = fgGetNode("/sim/speed-up", true);
    aero = fgGetNode("/sim/aero", true);

    ls_toplevel_init( 0.0, (char *)(aero->getStringValue().c_str()) );

    lsic=new LaRCsimIC; //this needs to be brought up after LaRCsim is
    if ( aero->getStringValue() == "c172" ) {
        copy_to_LaRCsim(); // initialize all of LaRCsim's vars

        //this should go away someday -- formerly done in fg_init.cxx
        Mass = 8.547270E+01;
        I_xx = 1.048000E+03;
        I_yy = 3.000000E+03;
        I_zz = 3.530000E+03;
        I_xz = 0.000000E+00;
    }

    ls_set_model_dt( get_delta_t() );

            // Initialize our little engine that hopefully might
    eng.init( get_delta_t() );
    // dcl - in passing dt to init rather than update I am assuming
    // that the LaRCsim dt is fixed at one value (yes it is 120hz CLO)
}

FGLaRCsim::~FGLaRCsim(void) {
    if ( lsic != NULL ) {
        delete lsic;
        lsic = NULL;
    }
}

// Initialize the LaRCsim flight model, dt is the time increment for
// each subsequent iteration through the EOM
void FGLaRCsim::init() {
   //do init common to all FDM's
   common_init();

   //now do any specific to LaRCsim
}


// Run an iteration of the EOM (equations of motion)
void FGLaRCsim::update( int multiloop ) {

    if ( aero->getStringValue() == "c172" ) {
	// set control inputs
	// cout << "V_calibrated_kts = " << V_calibrated_kts << '\n';
	eng.set_IAS( V_calibrated_kts );
	eng.set_Throttle_Lever_Pos( globals->get_controls()->get_throttle( 0 )
				    * 100.0 );
	eng.set_Propeller_Lever_Pos( 100 );
        eng.set_Mixture_Lever_Pos( globals->get_controls()->get_mixture( 0 )
				   * 100.0 );
	eng.set_Magneto_Switch_Pos( globals->get_controls()->get_magnetos(0) );
    	eng.setStarterFlag( globals->get_controls()->get_starter(0) );
	eng.set_p_amb( Static_pressure );
	eng.set_T_amb( Static_temperature );

	// update engine model
	eng.update();

	// copy engine state values onto "bus"
	fgSetDouble("/engines/engine/rpm", eng.get_RPM());
	fgSetDouble("/engines/engine/mp-osi", eng.get_Manifold_Pressure());
	fgSetDouble("/engines/engine/max-hp", eng.get_MaxHP());
	fgSetDouble("/engines/engine/power-pct", eng.get_Percentage_Power());
	fgSetDouble("/engines/engine/egt-degf", eng.get_EGT());
	fgSetDouble("/engines/engine/cht-degf", eng.get_CHT());
	fgSetDouble("/engines/engine/prop-thrust", eng.get_prop_thrust_SI());
	fgSetDouble("/engines/engine/fuel-flow-gph",
		    eng.get_fuel_flow_gals_hr());
	fgSetDouble("/engines/engine/oil-temperature-degf",
		    eng.get_oil_temp());
	fgSetDouble("/engines/engine/running", eng.getRunningFlag());
	fgSetDouble("/engines/engine/cranking", eng.getCrankingFlag());

	static const SGPropertyNode *fuel_freeze
	    = fgGetNode("/sim/freeze/fuel");

	if ( ! fuel_freeze->getBoolValue() ) {
	    //Assume we are using both tanks equally for now
	    fgSetDouble("/consumables/fuel/tank[0]/level-gal_us",
			fgGetDouble("/consumables/fuel/tank[0]")
			- (eng.get_fuel_flow_gals_hr() / (2 * 3600))
			* get_delta_t());
	    fgSetDouble("/consumables/fuel/tank[1]/level-gal_us",
			fgGetDouble("/consumables/fuel/tank[1]")
			- (eng.get_fuel_flow_gals_hr() / (2 * 3600))
			* get_delta_t());
	}

        F_X_engine = eng.get_prop_thrust_lbs();
	// cout << "F_X_engine = " << F_X_engine << '\n';
    }

    double save_alt = 0.0;

    // lets try to avoid really screwing up the LaRCsim model
    if ( get_Altitude() < -9000.0 ) {
	save_alt = get_Altitude();
	set_Altitude( 0.0 );
    }

    // copy control positions into the LaRCsim structure
    Lat_control = globals->get_controls()->get_aileron() /
    speed_up->getIntValue();
    Long_control = globals->get_controls()->get_elevator();
    Long_trim = globals->get_controls()->get_elevator_trim();
    Rudder_pedal = globals->get_controls()->get_rudder() /
        speed_up->getIntValue();
    Flap_handle = 30.0 * globals->get_controls()->get_flaps();

    if ( aero->getStringValue() == "c172" ) {
        Use_External_Engine = 1;
    } else {
	Use_External_Engine = 0;
    }

    Throttle_pct = globals->get_controls()->get_throttle( 0 ) * 1.0;

    Brake_pct[0] = globals->get_controls()->get_brake( 1 );
    Brake_pct[1] = globals->get_controls()->get_brake( 0 );

    // Inform LaRCsim of the local terrain altitude
    // Runway_altitude = get_Runway_altitude();
    Runway_altitude = scenery.get_cur_elev() * SG_METER_TO_FEET;

    // Weather
    /* V_north_airmass = get_V_north_airmass();
       V_east_airmass =  get_V_east_airmass();
       V_down_airmass =  get_V_down_airmass(); */


    // old -- FGInterface_2_LaRCsim() not needed except for Init()
    // translate FG to LaRCsim structure
    // FGInterface_2_LaRCsim(f);
    // printf("FG_Altitude = %.2f\n", FG_Altitude * 0.3048);
    // printf("Altitude = %.2f\n", Altitude * 0.3048);
    // printf("Radius to Vehicle = %.2f\n", Radius_to_vehicle * 0.3048);

    ls_update(multiloop);

    // printf("%d FG_Altitude = %.2f\n", i, FG_Altitude * 0.3048);
    // printf("%d Altitude = %.2f\n", i, Altitude * 0.3048);

    // translate LaRCsim back to FG structure so that the
    // autopilot (and the rest of the sim can use the updated
    // values
    copy_from_LaRCsim();

    // but lets restore our original bogus altitude when we are done
    if ( save_alt < -9000.0 ) {
	set_Altitude( save_alt );
    }
}


// Convert from the FGInterface struct to the LaRCsim generic_ struct
bool FGLaRCsim::copy_to_LaRCsim () {
    Mass =      get_Mass();
    I_xx =      get_I_xx();
    I_yy =      get_I_yy();
    I_zz =      get_I_zz();
    I_xz =      get_I_xz();
    // Dx_pilot =  get_Dx_pilot();
    // Dy_pilot =  get_Dy_pilot();
    // Dz_pilot =  get_Dz_pilot();
    Dx_cg =     get_Dx_cg();
    Dy_cg =     get_Dy_cg();
    Dz_cg =     get_Dz_cg();
    // F_X =       get_F_X();
    // F_Y =       get_F_Y();
    // F_Z =       get_F_Z();
    // F_north =   get_F_north();
    // F_east =    get_F_east();
    // F_down =    get_F_down();
    // F_X_aero =  get_F_X_aero();
    // F_Y_aero =  get_F_Y_aero();
    // F_Z_aero =  get_F_Z_aero();
    // F_X_engine =        get_F_X_engine();
    // F_Y_engine =        get_F_Y_engine();
    // F_Z_engine =        get_F_Z_engine();
    // F_X_gear =  get_F_X_gear();
    // F_Y_gear =  get_F_Y_gear();
    // F_Z_gear =  get_F_Z_gear();
    // M_l_rp =    get_M_l_rp();
    // M_m_rp =    get_M_m_rp();
    // M_n_rp =    get_M_n_rp();
    // M_l_cg =    get_M_l_cg();
    // M_m_cg =    get_M_m_cg();
    // M_n_cg =    get_M_n_cg();
    // M_l_aero =  get_M_l_aero();
    // M_m_aero =  get_M_m_aero();
    // M_n_aero =  get_M_n_aero();
    // M_l_engine =        get_M_l_engine();
    // M_m_engine =        get_M_m_engine();
    // M_n_engine =        get_M_n_engine();
    // M_l_gear =  get_M_l_gear();
    // M_m_gear =  get_M_m_gear();
    // M_n_gear =  get_M_n_gear();
    // V_dot_north =       get_V_dot_north();
    // V_dot_east =        get_V_dot_east();
    // V_dot_down =        get_V_dot_down();
    // U_dot_body =        get_U_dot_body();
    // V_dot_body =        get_V_dot_body();
    // W_dot_body =        get_W_dot_body();
    // A_X_cg =    get_A_X_cg();
    // A_Y_cg =    get_A_Y_cg();
    // A_Z_cg =    get_A_Z_cg();
    // A_X_pilot = get_A_X_pilot();
    // A_Y_pilot = get_A_Y_pilot();
    // A_Z_pilot = get_A_Z_pilot();
    // N_X_cg =    get_N_X_cg();
    // N_Y_cg =    get_N_Y_cg();
    // N_Z_cg =    get_N_Z_cg();
    // N_X_pilot = get_N_X_pilot();
    // N_Y_pilot = get_N_Y_pilot();
    // N_Z_pilot = get_N_Z_pilot();
    // P_dot_body =        get_P_dot_body();
    // Q_dot_body =        get_Q_dot_body();
    // R_dot_body =        get_R_dot_body();
    // V_north =   get_V_north();
    // V_east =    get_V_east();
    // V_down =    get_V_down();
    // V_north_rel_ground =        get_V_north_rel_ground();
    // V_east_rel_ground = get_V_east_rel_ground();
    // V_down_rel_ground = get_V_down_rel_ground();
    // V_north_airmass =   get_V_north_airmass();
    // V_east_airmass =    get_V_east_airmass();
    // V_down_airmass =    get_V_down_airmass();
    // V_north_rel_airmass =       get_V_north_rel_airmass();
    // V_east_rel_airmass =        get_V_east_rel_airmass();
    // V_down_rel_airmass =        get_V_down_rel_airmass();
    // U_gust =    get_U_gust();
    // V_gust =    get_V_gust();
    // W_gust =    get_W_gust();
    // U_body =    get_U_body();
    // V_body =    get_V_body();
    // W_body =    get_W_body();
    // V_rel_wind =        get_V_rel_wind();
    // V_true_kts =        get_V_true_kts();
    // V_rel_ground =      get_V_rel_ground();
    // V_inertial =        get_V_inertial();
    // V_ground_speed =    get_V_ground_speed();
    // V_equiv =   get_V_equiv();
    // V_equiv_kts =       get_V_equiv_kts();
    // V_calibrated =      get_V_calibrated();
    // V_calibrated_kts =  get_V_calibrated_kts();
    // P_body =    get_P_body();
    // Q_body =    get_Q_body();
    // R_body =    get_R_body();
    // P_local =   get_P_local();
    // Q_local =   get_Q_local();
    // R_local =   get_R_local();
    // P_total =   get_P_total();
    // Q_total =   get_Q_total();
    // R_total =   get_R_total();
    // Phi_dot =   get_Phi_dot();
    // Theta_dot = get_Theta_dot();
    // Psi_dot =   get_Psi_dot();
    // Latitude_dot =      get_Latitude_dot();
    // Longitude_dot =     get_Longitude_dot();
    // Radius_dot =        get_Radius_dot();
    // Lat_geocentric =    get_Lat_geocentric();
    // Lon_geocentric =    get_Lon_geocentric();
    // Radius_to_vehicle = get_Radius_to_vehicle();
    // Latitude =  get_Latitude();
    // Longitude = get_Longitude();
    // Altitude =  get_Altitude();
    // Phi =       get_Phi();
    // Theta =     get_Theta();
    // Psi =       get_Psi();
    // T_local_to_body_11 =        get_T_local_to_body_11();
    // T_local_to_body_12 =        get_T_local_to_body_12();
    // T_local_to_body_13 =        get_T_local_to_body_13();
    // T_local_to_body_21 =        get_T_local_to_body_21();
    // T_local_to_body_22 =        get_T_local_to_body_22();
    // T_local_to_body_23 =        get_T_local_to_body_23();
    // T_local_to_body_31 =        get_T_local_to_body_31();
    // T_local_to_body_32 =        get_T_local_to_body_32();
    // T_local_to_body_33 =        get_T_local_to_body_33();
    // Gravity =   get_Gravity();
    // Centrifugal_relief =        get_Centrifugal_relief();
    // Alpha =     get_Alpha();
    // Beta =      get_Beta();
    // Alpha_dot = get_Alpha_dot();
    // Beta_dot =  get_Beta_dot();
    // Cos_alpha = get_Cos_alpha();
    // Sin_alpha = get_Sin_alpha();
    // Cos_beta =  get_Cos_beta();
    // Sin_beta =  get_Sin_beta();
    // Cos_phi =   get_Cos_phi();
    // Sin_phi =   get_Sin_phi();
    // Cos_theta = get_Cos_theta();
    // Sin_theta = get_Sin_theta();
    // Cos_psi =   get_Cos_psi();
    // Sin_psi =   get_Sin_psi();
    // Gamma_vert_rad =    get_Gamma_vert_rad();
    // Gamma_horiz_rad =   get_Gamma_horiz_rad();
    // Sigma =     get_Sigma();
    // Density =   get_Density();
    // V_sound =   get_V_sound();
    // Mach_number =       get_Mach_number();
    // Static_pressure =   get_Static_pressure();
    // Total_pressure =    get_Total_pressure();
    // Impact_pressure =   get_Impact_pressure();
    // Dynamic_pressure =  get_Dynamic_pressure();
    // Static_temperature =        get_Static_temperature();
    // Total_temperature = get_Total_temperature();
    // Sea_level_radius =  get_Sea_level_radius();
    // Earth_position_angle =      get_Earth_position_angle();
    // Runway_altitude =   get_Runway_altitude();
    // Runway_latitude =   get_Runway_latitude();
    // Runway_longitude =  get_Runway_longitude();
    // Runway_heading =    get_Runway_heading();
    // Radius_to_rwy =     get_Radius_to_rwy();
    // D_cg_north_of_rwy = get_D_cg_north_of_rwy();
    // D_cg_east_of_rwy =  get_D_cg_east_of_rwy();
    // D_cg_above_rwy =    get_D_cg_above_rwy();
    // X_cg_rwy =  get_X_cg_rwy();
    // Y_cg_rwy =  get_Y_cg_rwy();
    // H_cg_rwy =  get_H_cg_rwy();
    // D_pilot_north_of_rwy =      get_D_pilot_north_of_rwy();
    // D_pilot_east_of_rwy =       get_D_pilot_east_of_rwy();
    // D_pilot_above_rwy = get_D_pilot_above_rwy();
    // X_pilot_rwy =       get_X_pilot_rwy();
    // Y_pilot_rwy =       get_Y_pilot_rwy();
    // H_pilot_rwy =       get_H_pilot_rwy();

    return true;
}


// Convert from the LaRCsim generic_ struct to the FGInterface struct
bool FGLaRCsim::copy_from_LaRCsim() {

    // Mass properties and geometry values
    _set_Inertias( Mass, I_xx, I_yy, I_zz, I_xz );
    // set_Pilot_Location( Dx_pilot, Dy_pilot, Dz_pilot );
    _set_CG_Position( Dx_cg, Dy_cg, Dz_cg );

    // Forces
    // set_Forces_Body_Total( F_X, F_Y, F_Z );
    // set_Forces_Local_Total( F_north, F_east, F_down );
    // set_Forces_Aero( F_X_aero, F_Y_aero, F_Z_aero );
    // set_Forces_Engine( F_X_engine, F_Y_engine, F_Z_engine );
    // set_Forces_Gear( F_X_gear, F_Y_gear, F_Z_gear );

    // Moments
    // set_Moments_Total_RP( M_l_rp, M_m_rp, M_n_rp );
    // set_Moments_Total_CG( M_l_cg, M_m_cg, M_n_cg );
    // set_Moments_Aero( M_l_aero, M_m_aero, M_n_aero );
    // set_Moments_Engine( M_l_engine, M_m_engine, M_n_engine );
    // set_Moments_Gear( M_l_gear, M_m_gear, M_n_gear );

    // Accelerations
    _set_Accels_Local( V_dot_north, V_dot_east, V_dot_down );
    _set_Accels_Body( U_dot_body, V_dot_body, W_dot_body );
    _set_Accels_CG_Body( A_X_cg, A_Y_cg, A_Z_cg );
    _set_Accels_Pilot_Body( A_X_pilot, A_Y_pilot, A_Z_pilot );
    // set_Accels_CG_Body_N( N_X_cg, N_Y_cg, N_Z_cg );
    // set_Accels_Pilot_Body_N( N_X_pilot, N_Y_pilot, N_Z_pilot );
    // set_Accels_Omega( P_dot_body, Q_dot_body, R_dot_body );

    // Velocities
    _set_Velocities_Local( V_north, V_east, V_down );
    // set_Velocities_Ground( V_north_rel_ground, V_east_rel_ground, 
    // 		     V_down_rel_ground );
    _set_Velocities_Local_Airmass( V_north_airmass, V_east_airmass,
				   V_down_airmass );
    // set_Velocities_Local_Rel_Airmass( V_north_rel_airmass, 
    //                          V_east_rel_airmass, V_down_rel_airmass );
    // set_Velocities_Gust( U_gust, V_gust, W_gust );
    _set_Velocities_Wind_Body( U_body, V_body, W_body );

    _set_V_rel_wind( V_rel_wind );
    // set_V_true_kts( V_true_kts );
    // set_V_rel_ground( V_rel_ground );
    // set_V_inertial( V_inertial );
    _set_V_ground_speed( V_ground_speed );
    // set_V_equiv( V_equiv );
    _set_V_equiv_kts( V_equiv_kts );
    // set_V_calibrated( V_calibrated );
    _set_V_calibrated_kts( V_calibrated_kts );

    _set_Omega_Body( P_body, Q_body, R_body );
    // set_Omega_Local( P_local, Q_local, R_local );
    // set_Omega_Total( P_total, Q_total, R_total );

    _set_Euler_Rates( Phi_dot, Theta_dot, Psi_dot );
    _set_Geocentric_Rates( Latitude_dot, Longitude_dot, Radius_dot );

    _set_Mach_number( Mach_number );

    SG_LOG( SG_FLIGHT, SG_DEBUG, "lon = " << Longitude 
	    << " lat_geoc = " << Lat_geocentric << " lat_geod = " << Latitude 
	    << " alt = " << Altitude << " sl_radius = " << Sea_level_radius 
	    << " radius_to_vehicle = " << Radius_to_vehicle );

    double tmp_lon_geoc = Lon_geocentric;
    while ( tmp_lon_geoc < -SGD_PI ) { tmp_lon_geoc += SGD_2PI; }
    while ( tmp_lon_geoc > SGD_PI ) { tmp_lon_geoc -= SGD_2PI; }

    double tmp_lon = Longitude;
    while ( tmp_lon < -SGD_PI ) { tmp_lon += SGD_2PI; }
    while ( tmp_lon > SGD_PI ) { tmp_lon -= SGD_2PI; }

    // Positions
    _set_Geocentric_Position( Lat_geocentric, tmp_lon_geoc, 
			      Radius_to_vehicle );
    _set_Geodetic_Position( Latitude, tmp_lon, Altitude );
    _set_Euler_Angles( Phi, Theta, Psi );

    _set_Altitude_AGL( Altitude - Runway_altitude );

    // Miscellaneous quantities
    _set_T_Local_to_Body(T_local_to_body_m);
    // set_Gravity( Gravity );
    // set_Centrifugal_relief( Centrifugal_relief );

    _set_Alpha( Alpha );
    _set_Beta( Beta );
    // set_Alpha_dot( Alpha_dot );
    // set_Beta_dot( Beta_dot );

    // set_Cos_alpha( Cos_alpha );
    // set_Sin_alpha( Sin_alpha );
    // set_Cos_beta( Cos_beta );
    // set_Sin_beta( Sin_beta );

    _set_Cos_phi( Cos_phi );
    // set_Sin_phi( Sin_phi );
    _set_Cos_theta( Cos_theta );
    // set_Sin_theta( Sin_theta );
    // set_Cos_psi( Cos_psi );
    // set_Sin_psi( Sin_psi );

    _set_Gamma_vert_rad( Gamma_vert_rad );
    // set_Gamma_horiz_rad( Gamma_horiz_rad );

    // set_Sigma( Sigma );
    _set_Density( Density );
    // set_V_sound( V_sound );
    // set_Mach_number( Mach_number );

    _set_Static_pressure( Static_pressure );
    // set_Total_pressure( Total_pressure );
    // set_Impact_pressure( Impact_pressure );
    // set_Dynamic_pressure( Dynamic_pressure );

    _set_Static_temperature( Static_temperature );
    // set_Total_temperature( Total_temperature );

    _set_Sea_level_radius( Sea_level_radius );
    _set_Earth_position_angle( Earth_position_angle );

    _set_Runway_altitude( Runway_altitude );
    // set_Runway_latitude( Runway_latitude );
    // set_Runway_longitude( Runway_longitude );
    // set_Runway_heading( Runway_heading );
    // set_Radius_to_rwy( Radius_to_rwy );

    // set_CG_Rwy_Local( D_cg_north_of_rwy, D_cg_east_of_rwy, D_cg_above_rwy);
    // set_CG_Rwy_Rwy( X_cg_rwy, Y_cg_rwy, H_cg_rwy );
    // set_Pilot_Rwy_Local( D_pilot_north_of_rwy, D_pilot_east_of_rwy, 
    //                        D_pilot_above_rwy );
    // set_Pilot_Rwy_Rwy( X_pilot_rwy, Y_pilot_rwy, H_pilot_rwy );

    _set_sin_lat_geocentric(Lat_geocentric);
    _set_cos_lat_geocentric(Lat_geocentric);
    _set_sin_cos_longitude(Longitude);
    _set_sin_cos_latitude(Latitude);

    // printf("sin_lat_geo %f  cos_lat_geo %f\n", sin_Lat_geoc, cos_Lat_geoc);
    // printf("sin_lat     %f  cos_lat     %f\n", 
    //        get_sin_latitude(), get_cos_latitude());
    // printf("sin_lon     %f  cos_lon     %f\n",
    //        get_sin_longitude(), get_cos_longitude());

    _set_Climb_Rate( -1 * V_down );
    // cout << "climb rate = " << -V_down * 60 << endl;

    if ( aero->getStringValue() == "uiuc" ) {
	if (pilot_elev_no) {
	    globals->get_controls()->set_elevator(Long_control);
	    globals->get_controls()->set_elevator_trim(Long_trim);
	    //	    controls.set_elevator(Long_control);
	    //	    controls.set_elevator_trim(Long_trim);
        }
	if (pilot_ail_no) {
            globals->get_controls()->set_aileron(Lat_control);
            //	  controls.set_aileron(Lat_control);
        }
	if (pilot_rud_no) {
            globals->get_controls()->set_rudder(Rudder_pedal);
            //	  controls.set_rudder(Rudder_pedal);
        }
	if (Throttle_pct_input) {
            globals->get_controls()->set_throttle(0,Throttle_pct);
            //	  controls.set_throttle(0,Throttle_pct);
        }
    }

    return true;
}


void FGLaRCsim::set_ls(void) {
    Phi=lsic->GetRollAngleRadIC();
    Theta=lsic->GetPitchAngleRadIC();
    Psi=lsic->GetHeadingRadIC();
    V_north=lsic->GetVnorthFpsIC();
    V_east=lsic->GetVeastFpsIC();
    V_down=lsic->GetVdownFpsIC();
    Altitude=lsic->GetAltitudeFtIC();
    Latitude=lsic->GetLatitudeGDRadIC();
    Longitude=lsic->GetLongitudeRadIC();
    Runway_altitude=lsic->GetRunwayAltitudeFtIC();
    V_north_airmass = lsic->GetVnorthAirmassFpsIC();
    V_east_airmass = lsic->GetVeastAirmassFpsIC();
    V_down_airmass = lsic->GetVdownAirmassFpsIC();
    ls_loop(0.0,-1);
    copy_from_LaRCsim();
    SG_LOG( SG_FLIGHT, SG_INFO, "  FGLaRCsim::set_ls(): "  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     Phi: " <<  Phi  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     Theta: " <<  Theta  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     Psi: " <<  Psi  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     V_north: " <<  V_north  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     V_east: " <<  V_east  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     V_down: " <<  V_down  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     Altitude: " <<  Altitude  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     Latitude: " <<  Latitude  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     Longitude: " <<  Longitude  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     Runway_altitude: " <<  Runway_altitude  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     V_north_airmass: " <<  V_north_airmass  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     V_east_airmass: " <<  V_east_airmass  );
    SG_LOG( SG_FLIGHT, SG_INFO, "     V_down_airmass: " <<  V_down_airmass  );
}

void FGLaRCsim::snap_shot(void) {
    lsic->SetLatitudeGDRadIC( get_Latitude() );
    lsic->SetLongitudeRadIC( get_Longitude() );
    lsic->SetAltitudeFtIC( get_Altitude() );
    lsic->SetRunwayAltitudeFtIC( get_Runway_altitude() );
    lsic->SetVtrueFpsIC( get_V_rel_wind() );
    lsic->SetPitchAngleRadIC( get_Theta() );
    lsic->SetRollAngleRadIC( get_Phi() );
    lsic->SetHeadingRadIC( get_Psi() );
    lsic->SetClimbRateFpsIC( get_Climb_Rate() );
    lsic->SetVNEDAirmassFpsIC( get_V_north_airmass(),
			       get_V_east_airmass(),
			       get_V_down_airmass() );
}				

//Positions
void FGLaRCsim::set_Latitude(double lat) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Latitude: " << lat  );
    snap_shot();
    lsic->SetLatitudeGDRadIC(lat);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

void FGLaRCsim::set_Longitude(double lon) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Longitude: " << lon  );
    snap_shot();
    lsic->SetLongitudeRadIC(lon);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

void FGLaRCsim::set_Altitude(double alt) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Altitude: " << alt  );
    snap_shot();
    lsic->SetAltitudeFtIC(alt);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

void FGLaRCsim::set_V_calibrated_kts(double vc) {
    SG_LOG( SG_FLIGHT, SG_INFO, 
	    "FGLaRCsim::set_V_calibrated_kts: " << vc  );
    snap_shot();
    lsic->SetVcalibratedKtsIC(vc);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

void FGLaRCsim::set_Mach_number(double mach) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Mach_number: " << mach  );
    snap_shot();
    lsic->SetMachIC(mach);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

void FGLaRCsim::set_Velocities_Local( double north, double east, double down ){
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Velocities_local: " 
	    << north << "  " << east << "  " << down   );
    snap_shot();
    lsic->SetVNEDFpsIC(north, east, down);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

void FGLaRCsim::set_Velocities_Wind_Body( double u, double v, double w){
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Velocities_Wind_Body: " 
	    << u << "  " << v << "  " << w   );
    snap_shot();
    lsic->SetUVWFpsIC(u,v,w);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

//Euler angles 
void FGLaRCsim::set_Euler_Angles( double phi, double theta, double psi ) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Euler_angles: " 
	    << phi << "  " << theta << "  " << psi   );

    snap_shot();
    lsic->SetPitchAngleRadIC(theta);
    lsic->SetRollAngleRadIC(phi);
    lsic->SetHeadingRadIC(psi);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

//Flight Path
void FGLaRCsim::set_Climb_Rate( double roc) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Climb_rate: " << roc  );
    snap_shot();
    lsic->SetClimbRateFpsIC(roc);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

void FGLaRCsim::set_Gamma_vert_rad( double gamma) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Gamma_vert_rad: " << gamma  );
    snap_shot();
    lsic->SetFlightPathAngleRadIC(gamma);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

void FGLaRCsim::set_Runway_altitude(double ralt) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Runway_altitude: " << ralt  );
    snap_shot();
    lsic->SetRunwayAltitudeFtIC(ralt);
    set_ls();
    copy_from_LaRCsim(); //update the bus
}

void FGLaRCsim::set_AltitudeAGL(double altagl) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_AltitudeAGL: " << altagl  );
    snap_shot();
    lsic->SetAltitudeAGLFtIC(altagl);
    set_ls();
    copy_from_LaRCsim();
}

void FGLaRCsim::set_Velocities_Local_Airmass (double wnorth, 
					      double weast, 
					      double wdown ) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Velocities_Local_Airmass: " 
	    << wnorth << "  " << weast << "  " << wdown );
    snap_shot();
    lsic->SetVNEDAirmassFpsIC( wnorth, weast, wdown );
    set_ls();
    copy_from_LaRCsim();
}

void FGLaRCsim::set_Static_pressure(double p) { 
    SG_LOG( SG_FLIGHT, SG_INFO, 
	    "FGLaRCsim::set_Static_pressure: " << p  );
    SG_LOG( SG_FLIGHT, SG_INFO, 
	    "LaRCsim does not support externally supplied atmospheric data" );
}

void FGLaRCsim::set_Static_temperature(double T) { 
    SG_LOG( SG_FLIGHT, SG_INFO, 
	    "FGLaRCsim::set_Static_temperature: " << T  );
    SG_LOG( SG_FLIGHT, SG_INFO, 
	    "LaRCsim does not support externally supplied atmospheric data" );

}

void FGLaRCsim::set_Density(double rho) {
    SG_LOG( SG_FLIGHT, SG_INFO, "FGLaRCsim::set_Density: " << rho  );
    SG_LOG( SG_FLIGHT, SG_INFO, 
	    "LaRCsim does not support externally supplied atmospheric data" );

}

