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
// (Log is kept at end of this file)


#include "LaRCsim.hxx"

#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <Flight/flight.hxx>
#include <Flight/LaRCsim/ls_cockpit.h>
#include <Flight/LaRCsim/ls_generic.h>
#include <Flight/LaRCsim/ls_interface.h>


// Initialize the LaRCsim flight model, dt is the time increment for
// each subsequent iteration through the EOM
int fgLaRCsimInit(double dt) {
    ls_toplevel_init(dt);

    return(1);
}


// Run an iteration of the EOM (equations of motion)
int fgLaRCsimUpdate(FGState& f, int multiloop) {
    double save_alt = 0.0;

    // lets try to avoid really screwing up the LaRCsim model
    if ( f.get_Altitude() < -9000 ) {
	save_alt = f.get_Altitude();
	f.set_Altitude( 0.0 );
    }

    // copy control positions into the LaRCsim structure
    Lat_control = controls.get_aileron();
    Long_control = controls.get_elevator();
    Long_trim = controls.get_elevator_trim();
    Rudder_pedal = controls.get_rudder();
    Throttle_pct = controls.get_throttle( 0 );
    Brake_pct = controls.get_brake( 0 );

    // Inform LaRCsim of the local terrain altitude
    Runway_altitude =   f.get_Runway_altitude();

    // old -- FGstate_2_LaRCsim() not needed except for Init()
    // translate FG to LaRCsim structure
    // FGState_2_LaRCsim(f);
    // printf("FG_Altitude = %.2f\n", FG_Altitude * 0.3048);
    // printf("Altitude = %.2f\n", Altitude * 0.3048);
    // printf("Radius to Vehicle = %.2f\n", Radius_to_vehicle * 0.3048);

    ls_update(multiloop);

    // printf("%d FG_Altitude = %.2f\n", i, FG_Altitude * 0.3048);
    // printf("%d Altitude = %.2f\n", i, Altitude * 0.3048);
    
    // translate LaRCsim back to FG structure so that the
    // autopilot (and the rest of the sim can use the updated
    // values
    fgLaRCsim_2_FGState(f);

    // but lets restore our original bogus altitude when we are done
    if ( save_alt < -9000.0 ) {
	f.set_Altitude( save_alt );
    }

    return 1;
}


// Convert from the FGState struct to the LaRCsim generic_ struct
int FGState_2_LaRCsim (FGState& f) {

    Mass =      f.get_Mass();
    I_xx =      f.get_I_xx();
    I_yy =      f.get_I_yy();
    I_zz =      f.get_I_zz();
    I_xz =      f.get_I_xz();
    // Dx_pilot =  f.get_Dx_pilot();
    // Dy_pilot =  f.get_Dy_pilot();
    // Dz_pilot =  f.get_Dz_pilot();
    Dx_cg =     f.get_Dx_cg();
    Dy_cg =     f.get_Dy_cg();
    Dz_cg =     f.get_Dz_cg();
    // F_X =       f.get_F_X();
    // F_Y =       f.get_F_Y();
    // F_Z =       f.get_F_Z();
    // F_north =   f.get_F_north();
    // F_east =    f.get_F_east();
    // F_down =    f.get_F_down();
    // F_X_aero =  f.get_F_X_aero();
    // F_Y_aero =  f.get_F_Y_aero();
    // F_Z_aero =  f.get_F_Z_aero();
    // F_X_engine =        f.get_F_X_engine();
    // F_Y_engine =        f.get_F_Y_engine();
    // F_Z_engine =        f.get_F_Z_engine();
    // F_X_gear =  f.get_F_X_gear();
    // F_Y_gear =  f.get_F_Y_gear();
    // F_Z_gear =  f.get_F_Z_gear();
    // M_l_rp =    f.get_M_l_rp();
    // M_m_rp =    f.get_M_m_rp();
    // M_n_rp =    f.get_M_n_rp();
    // M_l_cg =    f.get_M_l_cg();
    // M_m_cg =    f.get_M_m_cg();
    // M_n_cg =    f.get_M_n_cg();
    // M_l_aero =  f.get_M_l_aero();
    // M_m_aero =  f.get_M_m_aero();
    // M_n_aero =  f.get_M_n_aero();
    // M_l_engine =        f.get_M_l_engine();
    // M_m_engine =        f.get_M_m_engine();
    // M_n_engine =        f.get_M_n_engine();
    // M_l_gear =  f.get_M_l_gear();
    // M_m_gear =  f.get_M_m_gear();
    // M_n_gear =  f.get_M_n_gear();
    // V_dot_north =       f.get_V_dot_north();
    // V_dot_east =        f.get_V_dot_east();
    // V_dot_down =        f.get_V_dot_down();
    // U_dot_body =        f.get_U_dot_body();
    // V_dot_body =        f.get_V_dot_body();
    // W_dot_body =        f.get_W_dot_body();
    // A_X_cg =    f.get_A_X_cg();
    // A_Y_cg =    f.get_A_Y_cg();
    // A_Z_cg =    f.get_A_Z_cg();
    // A_X_pilot = f.get_A_X_pilot();
    // A_Y_pilot = f.get_A_Y_pilot();
    // A_Z_pilot = f.get_A_Z_pilot();
    // N_X_cg =    f.get_N_X_cg();
    // N_Y_cg =    f.get_N_Y_cg();
    // N_Z_cg =    f.get_N_Z_cg();
    // N_X_pilot = f.get_N_X_pilot();
    // N_Y_pilot = f.get_N_Y_pilot();
    // N_Z_pilot = f.get_N_Z_pilot();
    // P_dot_body =        f.get_P_dot_body();
    // Q_dot_body =        f.get_Q_dot_body();
    // R_dot_body =        f.get_R_dot_body();
    V_north =   f.get_V_north();
    V_east =    f.get_V_east();
    V_down =    f.get_V_down();
    // V_north_rel_ground =        f.get_V_north_rel_ground();
    // V_east_rel_ground = f.get_V_east_rel_ground();
    // V_down_rel_ground = f.get_V_down_rel_ground();
    // V_north_airmass =   f.get_V_north_airmass();
    // V_east_airmass =    f.get_V_east_airmass();
    // V_down_airmass =    f.get_V_down_airmass();
    // V_north_rel_airmass =       f.get_V_north_rel_airmass();
    // V_east_rel_airmass =        f.get_V_east_rel_airmass();
    // V_down_rel_airmass =        f.get_V_down_rel_airmass();
    // U_gust =    f.get_U_gust();
    // V_gust =    f.get_V_gust();
    // W_gust =    f.get_W_gust();
    // U_body =    f.get_U_body();
    // V_body =    f.get_V_body();
    // W_body =    f.get_W_body();
    // V_rel_wind =        f.get_V_rel_wind();
    // V_true_kts =        f.get_V_true_kts();
    // V_rel_ground =      f.get_V_rel_ground();
    // V_inertial =        f.get_V_inertial();
    // V_ground_speed =    f.get_V_ground_speed();
    // V_equiv =   f.get_V_equiv();
    // V_equiv_kts =       f.get_V_equiv_kts();
    // V_calibrated =      f.get_V_calibrated();
    // V_calibrated_kts =  f.get_V_calibrated_kts();
    P_body =    f.get_P_body();
    Q_body =    f.get_Q_body();
    R_body =    f.get_R_body();
    // P_local =   f.get_P_local();
    // Q_local =   f.get_Q_local();
    // R_local =   f.get_R_local();
    // P_total =   f.get_P_total();
    // Q_total =   f.get_Q_total();
    // R_total =   f.get_R_total();
    // Phi_dot =   f.get_Phi_dot();
    // Theta_dot = f.get_Theta_dot();
    // Psi_dot =   f.get_Psi_dot();
    // Latitude_dot =      f.get_Latitude_dot();
    // Longitude_dot =     f.get_Longitude_dot();
    // Radius_dot =        f.get_Radius_dot();
    Lat_geocentric =    f.get_Lat_geocentric();
    Lon_geocentric =    f.get_Lon_geocentric();
    Radius_to_vehicle = f.get_Radius_to_vehicle();
    Latitude =  f.get_Latitude();
    Longitude = f.get_Longitude();
    Altitude =  f.get_Altitude();
    Phi =       f.get_Phi();
    Theta =     f.get_Theta();
    Psi =       f.get_Psi();
    // T_local_to_body_11 =        f.get_T_local_to_body_11();
    // T_local_to_body_12 =        f.get_T_local_to_body_12();
    // T_local_to_body_13 =        f.get_T_local_to_body_13();
    // T_local_to_body_21 =        f.get_T_local_to_body_21();
    // T_local_to_body_22 =        f.get_T_local_to_body_22();
    // T_local_to_body_23 =        f.get_T_local_to_body_23();
    // T_local_to_body_31 =        f.get_T_local_to_body_31();
    // T_local_to_body_32 =        f.get_T_local_to_body_32();
    // T_local_to_body_33 =        f.get_T_local_to_body_33();
    // Gravity =   f.get_Gravity();
    // Centrifugal_relief =        f.get_Centrifugal_relief();
    // Alpha =     f.get_Alpha();
    // Beta =      f.get_Beta();
    // Alpha_dot = f.get_Alpha_dot();
    // Beta_dot =  f.get_Beta_dot();
    // Cos_alpha = f.get_Cos_alpha();
    // Sin_alpha = f.get_Sin_alpha();
    // Cos_beta =  f.get_Cos_beta();
    // Sin_beta =  f.get_Sin_beta();
    // Cos_phi =   f.get_Cos_phi();
    // Sin_phi =   f.get_Sin_phi();
    // Cos_theta = f.get_Cos_theta();
    // Sin_theta = f.get_Sin_theta();
    // Cos_psi =   f.get_Cos_psi();
    // Sin_psi =   f.get_Sin_psi();
    // Gamma_vert_rad =    f.get_Gamma_vert_rad();
    // Gamma_horiz_rad =   f.get_Gamma_horiz_rad();
    // Sigma =     f.get_Sigma();
    // Density =   f.get_Density();
    // V_sound =   f.get_V_sound();
    // Mach_number =       f.get_Mach_number();
    // Static_pressure =   f.get_Static_pressure();
    // Total_pressure =    f.get_Total_pressure();
    // Impact_pressure =   f.get_Impact_pressure();
    // Dynamic_pressure =  f.get_Dynamic_pressure();
    // Static_temperature =        f.get_Static_temperature();
    // Total_temperature = f.get_Total_temperature();
    Sea_level_radius =  f.get_Sea_level_radius();
    Earth_position_angle =      f.get_Earth_position_angle();
    Runway_altitude =   f.get_Runway_altitude();
    // Runway_latitude =   f.get_Runway_latitude();
    // Runway_longitude =  f.get_Runway_longitude();
    // Runway_heading =    f.get_Runway_heading();
    // Radius_to_rwy =     f.get_Radius_to_rwy();
    // D_cg_north_of_rwy = f.get_D_cg_north_of_rwy();
    // D_cg_east_of_rwy =  f.get_D_cg_east_of_rwy();
    // D_cg_above_rwy =    f.get_D_cg_above_rwy();
    // X_cg_rwy =  f.get_X_cg_rwy();
    // Y_cg_rwy =  f.get_Y_cg_rwy();
    // H_cg_rwy =  f.get_H_cg_rwy();
    // D_pilot_north_of_rwy =      f.get_D_pilot_north_of_rwy();
    // D_pilot_east_of_rwy =       f.get_D_pilot_east_of_rwy();
    // D_pilot_above_rwy = f.get_D_pilot_above_rwy();
    // X_pilot_rwy =       f.get_X_pilot_rwy();
    // Y_pilot_rwy =       f.get_Y_pilot_rwy();
    // H_pilot_rwy =       f.get_H_pilot_rwy();

    return( 0 );
}


// Convert from the LaRCsim generic_ struct to the FGState struct
int fgLaRCsim_2_FGState (FGState& f) {

    // Mass properties and geometry values
    f.set_Inertias( Mass, I_xx, I_yy, I_zz, I_xz );
    // f.set_Pilot_Location( Dx_pilot, Dy_pilot, Dz_pilot );
    f.set_CG_Position( Dx_cg, Dy_cg, Dz_cg );

    // Forces
    // f.set_Forces_Body_Total( F_X, F_Y, F_Z );
    // f.set_Forces_Local_Total( F_north, F_east, F_down );
    // f.set_Forces_Aero( F_X_aero, F_Y_aero, F_Z_aero );
    // f.set_Forces_Engine( F_X_engine, F_Y_engine, F_Z_engine );
    // f.set_Forces_Gear( F_X_gear, F_Y_gear, F_Z_gear );

    // Moments
    // f.set_Moments_Total_RP( M_l_rp, M_m_rp, M_n_rp );
    // f.set_Moments_Total_CG( M_l_cg, M_m_cg, M_n_cg );
    // f.set_Moments_Aero( M_l_aero, M_m_aero, M_n_aero );
    // f.set_Moments_Engine( M_l_engine, M_m_engine, M_n_engine );
    // f.set_Moments_Gear( M_l_gear, M_m_gear, M_n_gear );

    // Accelerations
    // f.set_Accels_Local( V_dot_north, V_dot_east, V_dot_down );
    // f.set_Accels_Body( U_dot_body, V_dot_body, W_dot_body );
    // f.set_Accels_CG_Body( A_X_cg, A_Y_cg, A_Z_cg );
    // f.set_Accels_Pilot_Body( A_X_pilot, A_Y_pilot, A_Z_pilot );
    // f.set_Accels_CG_Body_N( N_X_cg, N_Y_cg, N_Z_cg );
    // f.set_Accels_Pilot_Body_N( N_X_pilot, N_Y_pilot, N_Z_pilot );
    // f.set_Accels_Omega( P_dot_body, Q_dot_body, R_dot_body );

    // Velocities
    f.set_Velocities_Local( V_north, V_east, V_down );
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
    f.set_V_equiv_kts( V_equiv_kts );
    // f.set_V_calibrated( V_calibrated );
    // f.set_V_calibrated_kts( V_calibrated_kts );

    f.set_Omega_Body( P_body, Q_body, R_body );
    // f.set_Omega_Local( P_local, Q_local, R_local );
    // f.set_Omega_Total( P_total, Q_total, R_total );
    
    // f.set_Euler_Rates( Phi_dot, Theta_dot, Psi_dot );
    f.set_Geocentric_Rates( Latitude_dot, Longitude_dot, Radius_dot );

    // Positions
    f.set_Geocentric_Position( Lat_geocentric, Lon_geocentric, 
                               Radius_to_vehicle );
    f.set_Geodetic_Position( Latitude, Longitude, Altitude );
    f.set_Euler_Angles( Phi, Theta, Psi );

    // Miscellaneous quantities
    f.set_T_Local_to_Body(T_local_to_body_m);
    // f.set_Gravity( Gravity );
    // f.set_Centrifugal_relief( Centrifugal_relief );

    f.set_Alpha( Alpha );
    f.set_Beta( Beta );
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

    f.set_Gamma_vert_rad( Gamma_vert_rad );
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

    f.set_Sea_level_radius( Sea_level_radius );
    f.set_Earth_position_angle( Earth_position_angle );

    f.set_Runway_altitude( Runway_altitude );
    // f.set_Runway_latitude( Runway_latitude );
    // f.set_Runway_longitude( Runway_longitude );
    // f.set_Runway_heading( Runway_heading );
    // f.set_Radius_to_rwy( Radius_to_rwy );

    // f.set_CG_Rwy_Local( D_cg_north_of_rwy, D_cg_east_of_rwy, D_cg_above_rwy);
    // f.set_CG_Rwy_Rwy( X_cg_rwy, Y_cg_rwy, H_cg_rwy );
    // f.set_Pilot_Rwy_Local( D_pilot_north_of_rwy, D_pilot_east_of_rwy, 
    //                        D_pilot_above_rwy );
    // f.set_Pilot_Rwy_Rwy( X_pilot_rwy, Y_pilot_rwy, H_pilot_rwy );


    return 0;
}


// $Log$
// Revision 1.9  1999/01/08 19:27:36  curt
// Fixed AOA reading on HUD.
// Continued work on time jitter compensation.
//
// Revision 1.8  1998/12/18 23:37:06  curt
// Collapsed out the FGState variables not currently needed.  They are just
// commented out and can be readded easily at any time.  The point of this
// exersize is to determine which variables were or were not currently being
// used.
//
// Revision 1.7  1998/12/14 13:31:06  curt
// LaRCsim maintains all it's variables internally.  I had been copying all of
// them back and forth to the FG struture everytime I updated the flight model.
// However, I have realized that this is not necessary.  I just need to copy
// the control positions and environmental parameters into the LaRCsim structure
// before updating the FDM, then copy every thing back out into the publick FGFS
// structure afterwords.  This seems to solve (or at least help) a westward
// drift problem some poeple had been observing.
//
// Revision 1.6  1998/12/05 15:54:08  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.5  1998/12/03 04:25:02  curt
// Working on fixing up new fgFLIGHT class.
//
// Revision 1.4  1998/12/03 01:16:37  curt
// Converted fgFLIGHT to a class.
//
// Revision 1.3  1998/10/25 14:08:43  curt
// Turned "struct fgCONTROLS" into a class, with inlined accessor functions.
//
// Revision 1.2  1998/10/17 01:34:11  curt
// C++ ifying ...
//
// Revision 1.1  1998/10/17 00:43:58  curt
// Initial revision.
//
//
