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

#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <FDM/flight.hxx>
#include <FDM/LaRCsim/ls_cockpit.h>
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_interface.h>

#include "10520d.hxx"
#include "LaRCsim.hxx"


FGEngine eng;


// Initialize the LaRCsim flight model, dt is the time increment for
// each subsequent iteration through the EOM
int FGLaRCsim::init( double dt ) {

#ifdef USE_NEW_ENGINE_CODE
    // Initialize our little engine that hopefully might
    eng.init();
#endif

    // cout << "FGLaRCsim::init()" << endl;

    double save_alt = 0.0;

    if ( get_Altitude() < -9000.0 ) {
	save_alt = get_Altitude();
	set_Altitude( 0.0 );
    }

    // translate FG to LaRCsim structure
    copy_to_LaRCsim();

    // actual LaRCsim top level init
    ls_toplevel_init( dt, (char *)current_options.get_aircraft().c_str() );

    FG_LOG( FG_FLIGHT, FG_INFO, "FG pos = " << 
	    get_Latitude() );

    // translate LaRCsim back to FG structure
    copy_from_LaRCsim();

    // but lets restore our original bogus altitude when we are done
    if ( save_alt < -9000.0 ) {
	set_Altitude( save_alt );
    }

    // set valid time for this record
    stamp_time();

    return 1;
}


// Run an iteration of the EOM (equations of motion)
int FGLaRCsim::update( int multiloop ) {
    // cout << "FGLaRCsim::update()" << endl;

#ifdef USE_NEW_ENGINE_CODE
    // update simple engine model
    eng.set_IAS( V_calibrated_kts );
    eng.set_Throttle_Lever_Pos( Throttle_pct * 100.0 );
    eng.set_Propeller_Lever_Pos( 95 );
    eng.set_Mixture_Lever_Pos( 100 );
    eng.update();
    cout << "  Thrust = " << eng.get_FGProp1_Thrust() << endl;
    F_X_engine = eng.get_FGProp1_Thrust() * 7;
#endif

    double save_alt = 0.0;
    double time_step = (1.0 / current_options.get_model_hz()) * multiloop;
    double start_elev = get_Altitude();

    // lets try to avoid really screwing up the LaRCsim model
    if ( get_Altitude() < -9000.0 ) {
	save_alt = get_Altitude();
	set_Altitude( 0.0 );
    }

    // copy control positions into the LaRCsim structure
    Lat_control = controls.get_aileron() / current_options.get_speed_up();
    Long_control = controls.get_elevator();
    Long_trim = controls.get_elevator_trim();
    Rudder_pedal = controls.get_rudder() / current_options.get_speed_up();
    Flap_handle = 30.0 * controls.get_flaps();
#ifdef USE_NEW_ENGINE_CODE
    Throttle_pct = -1.0;	// tells engine model to use propellor thrust
#else
    Throttle_pct = controls.get_throttle( 0 ) * 1.0;
#endif
    Brake_pct[0] = controls.get_brake( 1 );
    Brake_pct[1] = controls.get_brake( 0 );

    // Inform LaRCsim of the local terrain altitude
    Runway_altitude = get_Runway_altitude();

    // Weather
    V_north_airmass = get_V_north_airmass();
    V_east_airmass =  get_V_east_airmass();
    V_down_airmass =  get_V_down_airmass();

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

    double end_elev = get_Altitude();
    if ( time_step > 0.0 ) {
	// feet per second
	set_Climb_Rate( (end_elev - start_elev) / time_step );
    }

    return 1;
}


// Convert from the FGInterface struct to the LaRCsim generic_ struct
int FGLaRCsim::copy_to_LaRCsim () {
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
    V_north =   get_V_north();
    V_east =    get_V_east();
    V_down =    get_V_down();
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
    P_body =    get_P_body();
    Q_body =    get_Q_body();
    R_body =    get_R_body();
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
    Lat_geocentric =    get_Lat_geocentric();
    Lon_geocentric =    get_Lon_geocentric();
    Radius_to_vehicle = get_Radius_to_vehicle();
    Latitude =  get_Latitude();
    Longitude = get_Longitude();
    Altitude =  get_Altitude();
    Phi =       get_Phi();
    Theta =     get_Theta();
    Psi =       get_Psi();
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
    Sea_level_radius =  get_Sea_level_radius();
    Earth_position_angle =      get_Earth_position_angle();
    Runway_altitude =   get_Runway_altitude();
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

    return 1;
}


// Convert from the LaRCsim generic_ struct to the FGInterface struct
int FGLaRCsim::copy_from_LaRCsim() {

    // Mass properties and geometry values
    set_Inertias( Mass, I_xx, I_yy, I_zz, I_xz );
    // set_Pilot_Location( Dx_pilot, Dy_pilot, Dz_pilot );
    set_CG_Position( Dx_cg, Dy_cg, Dz_cg );

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
    set_Accels_Local( V_dot_north, V_dot_east, V_dot_down );
    set_Accels_Body( U_dot_body, V_dot_body, W_dot_body );
    set_Accels_CG_Body( A_X_cg, A_Y_cg, A_Z_cg );
    set_Accels_Pilot_Body( A_X_pilot, A_Y_pilot, A_Z_pilot );
    // set_Accels_CG_Body_N( N_X_cg, N_Y_cg, N_Z_cg );
    // set_Accels_Pilot_Body_N( N_X_pilot, N_Y_pilot, N_Z_pilot );
    // set_Accels_Omega( P_dot_body, Q_dot_body, R_dot_body );

    // Velocities
    set_Velocities_Local( V_north, V_east, V_down );
    // set_Velocities_Ground( V_north_rel_ground, V_east_rel_ground, 
    // 		     V_down_rel_ground );
    // set_Velocities_Local_Airmass( V_north_airmass, V_east_airmass,
    // 			    V_down_airmass );
    // set_Velocities_Local_Rel_Airmass( V_north_rel_airmass, 
    //                          V_east_rel_airmass, V_down_rel_airmass );
    // set_Velocities_Gust( U_gust, V_gust, W_gust );
    set_Velocities_Wind_Body( U_body, V_body, W_body );

    // set_V_rel_wind( V_rel_wind );
    // set_V_true_kts( V_true_kts );
    // set_V_rel_ground( V_rel_ground );
    // set_V_inertial( V_inertial );
    set_V_ground_speed( V_ground_speed );
    // set_V_equiv( V_equiv );
    set_V_equiv_kts( V_equiv_kts );
    // set_V_calibrated( V_calibrated );
    set_V_calibrated_kts( V_calibrated_kts );

    set_Omega_Body( P_body, Q_body, R_body );
    // set_Omega_Local( P_local, Q_local, R_local );
    // set_Omega_Total( P_total, Q_total, R_total );
    
    set_Euler_Rates( Phi_dot, Theta_dot, Psi_dot );
    set_Geocentric_Rates( Latitude_dot, Longitude_dot, Radius_dot );

    set_Mach_number( Mach_number );

    FG_LOG( FG_FLIGHT, FG_DEBUG, "lon = " << Longitude 
	    << " lat_geoc = " << Lat_geocentric << " lat_geod = " << Latitude 
	    << " alt = " << Altitude << " sl_radius = " << Sea_level_radius 
	    << " radius_to_vehicle = " << Radius_to_vehicle );

    double tmp_lon_geoc = Lon_geocentric;
    while ( tmp_lon_geoc < -FG_PI ) { tmp_lon_geoc += FG_2PI; }
    while ( tmp_lon_geoc > FG_PI ) { tmp_lon_geoc -= FG_2PI; }

    double tmp_lon = Longitude;
    while ( tmp_lon < -FG_PI ) { tmp_lon += FG_2PI; }
    while ( tmp_lon > FG_PI ) { tmp_lon -= FG_2PI; }

    // Positions
    set_Geocentric_Position( Lat_geocentric, tmp_lon_geoc, 
                               Radius_to_vehicle );
    set_Geodetic_Position( Latitude, tmp_lon, Altitude );
    set_Euler_Angles( Phi, Theta, Psi );

    // Miscellaneous quantities
    set_T_Local_to_Body(T_local_to_body_m);
    // set_Gravity( Gravity );
    // set_Centrifugal_relief( Centrifugal_relief );

    set_Alpha( Alpha );
    set_Beta( Beta );
    // set_Alpha_dot( Alpha_dot );
    // set_Beta_dot( Beta_dot );

    // set_Cos_alpha( Cos_alpha );
    // set_Sin_alpha( Sin_alpha );
    // set_Cos_beta( Cos_beta );
    // set_Sin_beta( Sin_beta );

    set_Cos_phi( Cos_phi );
    // set_Sin_phi( Sin_phi );
    set_Cos_theta( Cos_theta );
    // set_Sin_theta( Sin_theta );
    // set_Cos_psi( Cos_psi );
    // set_Sin_psi( Sin_psi );

    set_Gamma_vert_rad( Gamma_vert_rad );
    // set_Gamma_horiz_rad( Gamma_horiz_rad );

    // set_Sigma( Sigma );
    set_Density( Density );
    // set_V_sound( V_sound );
    // set_Mach_number( Mach_number );

    set_Static_pressure( Static_pressure );
    // set_Total_pressure( Total_pressure );
    // set_Impact_pressure( Impact_pressure );
    // set_Dynamic_pressure( Dynamic_pressure );

    set_Static_temperature( Static_temperature );
    // set_Total_temperature( Total_temperature );

    set_Sea_level_radius( Sea_level_radius );
    set_Earth_position_angle( Earth_position_angle );

    set_Runway_altitude( Runway_altitude );
    // set_Runway_latitude( Runway_latitude );
    // set_Runway_longitude( Runway_longitude );
    // set_Runway_heading( Runway_heading );
    // set_Radius_to_rwy( Radius_to_rwy );

    // set_CG_Rwy_Local( D_cg_north_of_rwy, D_cg_east_of_rwy, D_cg_above_rwy);
    // set_CG_Rwy_Rwy( X_cg_rwy, Y_cg_rwy, H_cg_rwy );
    // set_Pilot_Rwy_Local( D_pilot_north_of_rwy, D_pilot_east_of_rwy, 
    //                        D_pilot_above_rwy );
    // set_Pilot_Rwy_Rwy( X_pilot_rwy, Y_pilot_rwy, H_pilot_rwy );

    set_sin_lat_geocentric(Lat_geocentric);
    set_cos_lat_geocentric(Lat_geocentric);
    set_sin_cos_longitude(Longitude);
    set_sin_cos_latitude(Latitude);

    // printf("sin_lat_geo %f  cos_lat_geo %f\n", sin_Lat_geoc, cos_Lat_geoc);
    // printf("sin_lat     %f  cos_lat     %f\n", 
    //        get_sin_latitude(), get_cos_latitude());
    // printf("sin_lon     %f  cos_lon     %f\n",
    //        get_sin_longitude(), get_cos_longitude());

    return 1;
}
