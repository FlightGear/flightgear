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
#include <Controls/controls.h>
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
int fgLaRCsimUpdate(fgFLIGHT *f, int multiloop) {
    double save_alt = 0.0;

    // lets try to avoid really screwing up the LaRCsim model
    if ( FG_Altitude < -9000 ) {
	save_alt = FG_Altitude;
	FG_Altitude = 0;
    }

    // translate FG to LaRCsim structure
    fgFlight_2_LaRCsim(f);
    // printf("FG_Altitude = %.2f\n", FG_Altitude * 0.3048);
    // printf("Altitude = %.2f\n", Altitude * 0.3048);
    // printf("Radius to Vehicle = %.2f\n", Radius_to_vehicle * 0.3048);

    ls_update(multiloop);

    // printf("%d FG_Altitude = %.2f\n", i, FG_Altitude * 0.3048);
    // printf("%d Altitude = %.2f\n", i, Altitude * 0.3048);
    
    // translate LaRCsim back to FG structure so that the
    // autopilot (and the rest of the sim can use the updated
    // values
    fgLaRCsim_2_Flight(f);

    // but lets restore our original bogus altitude when we are done
    if ( save_alt < -9000 ) {
	FG_Altitude = save_alt;
    }

    return 1;
}


// Convert from the fgFLIGHT struct to the LaRCsim generic_ struct
int fgFlight_2_LaRCsim (fgFLIGHT *f) {
    Mass =      FG_Mass;
    I_xx =      FG_I_xx;
    I_yy =      FG_I_yy;
    I_zz =      FG_I_zz;
    I_xz =      FG_I_xz;
    Dx_pilot =  FG_Dx_pilot;
    Dy_pilot =  FG_Dy_pilot;
    Dz_pilot =  FG_Dz_pilot;
    Dx_cg =     FG_Dx_cg;
    Dy_cg =     FG_Dy_cg;
    Dz_cg =     FG_Dz_cg;
    F_X =       FG_F_X;
    F_Y =       FG_F_Y;
    F_Z =       FG_F_Z;
    F_north =   FG_F_north;
    F_east =    FG_F_east;
    F_down =    FG_F_down;
    F_X_aero =  FG_F_X_aero;
    F_Y_aero =  FG_F_Y_aero;
    F_Z_aero =  FG_F_Z_aero;
    F_X_engine =        FG_F_X_engine;
    F_Y_engine =        FG_F_Y_engine;
    F_Z_engine =        FG_F_Z_engine;
    F_X_gear =  FG_F_X_gear;
    F_Y_gear =  FG_F_Y_gear;
    F_Z_gear =  FG_F_Z_gear;
    M_l_rp =    FG_M_l_rp;
    M_m_rp =    FG_M_m_rp;
    M_n_rp =    FG_M_n_rp;
    M_l_cg =    FG_M_l_cg;
    M_m_cg =    FG_M_m_cg;
    M_n_cg =    FG_M_n_cg;
    M_l_aero =  FG_M_l_aero;
    M_m_aero =  FG_M_m_aero;
    M_n_aero =  FG_M_n_aero;
    M_l_engine =        FG_M_l_engine;
    M_m_engine =        FG_M_m_engine;
    M_n_engine =        FG_M_n_engine;
    M_l_gear =  FG_M_l_gear;
    M_m_gear =  FG_M_m_gear;
    M_n_gear =  FG_M_n_gear;
    V_dot_north =       FG_V_dot_north;
    V_dot_east =        FG_V_dot_east;
    V_dot_down =        FG_V_dot_down;
    U_dot_body =        FG_U_dot_body;
    V_dot_body =        FG_V_dot_body;
    W_dot_body =        FG_W_dot_body;
    A_X_cg =    FG_A_X_cg;
    A_Y_cg =    FG_A_Y_cg;
    A_Z_cg =    FG_A_Z_cg;
    A_X_pilot = FG_A_X_pilot;
    A_Y_pilot = FG_A_Y_pilot;
    A_Z_pilot = FG_A_Z_pilot;
    N_X_cg =    FG_N_X_cg;
    N_Y_cg =    FG_N_Y_cg;
    N_Z_cg =    FG_N_Z_cg;
    N_X_pilot = FG_N_X_pilot;
    N_Y_pilot = FG_N_Y_pilot;
    N_Z_pilot = FG_N_Z_pilot;
    P_dot_body =        FG_P_dot_body;
    Q_dot_body =        FG_Q_dot_body;
    R_dot_body =        FG_R_dot_body;
    V_north =   FG_V_north;
    V_east =    FG_V_east;
    V_down =    FG_V_down;
    V_north_rel_ground =        FG_V_north_rel_ground;
    V_east_rel_ground = FG_V_east_rel_ground;
    V_down_rel_ground = FG_V_down_rel_ground;
    V_north_airmass =   FG_V_north_airmass;
    V_east_airmass =    FG_V_east_airmass;
    V_down_airmass =    FG_V_down_airmass;
    V_north_rel_airmass =       FG_V_north_rel_airmass;
    V_east_rel_airmass =        FG_V_east_rel_airmass;
    V_down_rel_airmass =        FG_V_down_rel_airmass;
    U_gust =    FG_U_gust;
    V_gust =    FG_V_gust;
    W_gust =    FG_W_gust;
    U_body =    FG_U_body;
    V_body =    FG_V_body;
    W_body =    FG_W_body;
    V_rel_wind =        FG_V_rel_wind;
    V_true_kts =        FG_V_true_kts;
    V_rel_ground =      FG_V_rel_ground;
    V_inertial =        FG_V_inertial;
    V_ground_speed =    FG_V_ground_speed;
    V_equiv =   FG_V_equiv;
    V_equiv_kts =       FG_V_equiv_kts;
    V_calibrated =      FG_V_calibrated;
    V_calibrated_kts =  FG_V_calibrated_kts;
    P_body =    FG_P_body;
    Q_body =    FG_Q_body;
    R_body =    FG_R_body;
    P_local =   FG_P_local;
    Q_local =   FG_Q_local;
    R_local =   FG_R_local;
    P_total =   FG_P_total;
    Q_total =   FG_Q_total;
    R_total =   FG_R_total;
    Phi_dot =   FG_Phi_dot;
    Theta_dot = FG_Theta_dot;
    Psi_dot =   FG_Psi_dot;
    Latitude_dot =      FG_Latitude_dot;
    Longitude_dot =     FG_Longitude_dot;
    Radius_dot =        FG_Radius_dot;
    Lat_geocentric =    FG_Lat_geocentric;
    Lon_geocentric =    FG_Lon_geocentric;
    Radius_to_vehicle = FG_Radius_to_vehicle;
    Latitude =  FG_Latitude;
    Longitude = FG_Longitude;
    Altitude =  FG_Altitude;
    Phi =       FG_Phi;
    Theta =     FG_Theta;
    Psi =       FG_Psi;
    T_local_to_body_11 =        FG_T_local_to_body_11;
    T_local_to_body_12 =        FG_T_local_to_body_12;
    T_local_to_body_13 =        FG_T_local_to_body_13;
    T_local_to_body_21 =        FG_T_local_to_body_21;
    T_local_to_body_22 =        FG_T_local_to_body_22;
    T_local_to_body_23 =        FG_T_local_to_body_23;
    T_local_to_body_31 =        FG_T_local_to_body_31;
    T_local_to_body_32 =        FG_T_local_to_body_32;
    T_local_to_body_33 =        FG_T_local_to_body_33;
    Gravity =   FG_Gravity;
    Centrifugal_relief =        FG_Centrifugal_relief;
    Alpha =     FG_Alpha;
    Beta =      FG_Beta;
    Alpha_dot = FG_Alpha_dot;
    Beta_dot =  FG_Beta_dot;
    Cos_alpha = FG_Cos_alpha;
    Sin_alpha = FG_Sin_alpha;
    Cos_beta =  FG_Cos_beta;
    Sin_beta =  FG_Sin_beta;
    Cos_phi =   FG_Cos_phi;
    Sin_phi =   FG_Sin_phi;
    Cos_theta = FG_Cos_theta;
    Sin_theta = FG_Sin_theta;
    Cos_psi =   FG_Cos_psi;
    Sin_psi =   FG_Sin_psi;
    Gamma_vert_rad =    FG_Gamma_vert_rad;
    Gamma_horiz_rad =   FG_Gamma_horiz_rad;
    Sigma =     FG_Sigma;
    Density =   FG_Density;
    V_sound =   FG_V_sound;
    Mach_number =       FG_Mach_number;
    Static_pressure =   FG_Static_pressure;
    Total_pressure =    FG_Total_pressure;
    Impact_pressure =   FG_Impact_pressure;
    Dynamic_pressure =  FG_Dynamic_pressure;
    Static_temperature =        FG_Static_temperature;
    Total_temperature = FG_Total_temperature;
    Sea_level_radius =  FG_Sea_level_radius;
    Earth_position_angle =      FG_Earth_position_angle;
    Runway_altitude =   FG_Runway_altitude;
    Runway_latitude =   FG_Runway_latitude;
    Runway_longitude =  FG_Runway_longitude;
    Runway_heading =    FG_Runway_heading;
    Radius_to_rwy =     FG_Radius_to_rwy;
    D_cg_north_of_rwy = FG_D_cg_north_of_rwy;
    D_cg_east_of_rwy =  FG_D_cg_east_of_rwy;
    D_cg_above_rwy =    FG_D_cg_above_rwy;
    X_cg_rwy =  FG_X_cg_rwy;
    Y_cg_rwy =  FG_Y_cg_rwy;
    H_cg_rwy =  FG_H_cg_rwy;
    D_pilot_north_of_rwy =      FG_D_pilot_north_of_rwy;
    D_pilot_east_of_rwy =       FG_D_pilot_east_of_rwy;
    D_pilot_above_rwy = FG_D_pilot_above_rwy;
    X_pilot_rwy =       FG_X_pilot_rwy;
    Y_pilot_rwy =       FG_Y_pilot_rwy;
    H_pilot_rwy =       FG_H_pilot_rwy;

    return( 0 );
}


// Convert from the LaRCsim generic_ struct to the fgFLIGHT struct
int fgLaRCsim_2_Flight (fgFLIGHT *f) {
    fgCONTROLS *c;

    c = current_aircraft.controls;

    Lat_control = FG_Aileron;
    Long_control = FG_Elevator;
    Long_trim = FG_Elev_Trim;
    Rudder_pedal = FG_Rudder;
    Throttle_pct = FG_Throttle[0];

    FG_Mass =   Mass;
    FG_I_xx =   I_xx;
    FG_I_yy =   I_yy;
    FG_I_zz =   I_zz;
    FG_I_xz =   I_xz;
    FG_Dx_pilot =       Dx_pilot;
    FG_Dy_pilot =       Dy_pilot;
    FG_Dz_pilot =       Dz_pilot;
    FG_Dx_cg =  Dx_cg;
    FG_Dy_cg =  Dy_cg;
    FG_Dz_cg =  Dz_cg;
    FG_F_X =    F_X;
    FG_F_Y =    F_Y;
    FG_F_Z =    F_Z;
    FG_F_north =        F_north;
    FG_F_east = F_east;
    FG_F_down = F_down;
    FG_F_X_aero =       F_X_aero;
    FG_F_Y_aero =       F_Y_aero;
    FG_F_Z_aero =       F_Z_aero;
    FG_F_X_engine =     F_X_engine;
    FG_F_Y_engine =     F_Y_engine;
    FG_F_Z_engine =     F_Z_engine;
    FG_F_X_gear =       F_X_gear;
    FG_F_Y_gear =       F_Y_gear;
    FG_F_Z_gear =       F_Z_gear;
    FG_M_l_rp = M_l_rp;
    FG_M_m_rp = M_m_rp;
    FG_M_n_rp = M_n_rp;
    FG_M_l_cg = M_l_cg;
    FG_M_m_cg = M_m_cg;
    FG_M_n_cg = M_n_cg;
    FG_M_l_aero =       M_l_aero;
    FG_M_m_aero =       M_m_aero;
    FG_M_n_aero =       M_n_aero;
    FG_M_l_engine =     M_l_engine;
    FG_M_m_engine =     M_m_engine;
    FG_M_n_engine =     M_n_engine;
    FG_M_l_gear =       M_l_gear;
    FG_M_m_gear =       M_m_gear;
    FG_M_n_gear =       M_n_gear;
    FG_V_dot_north =    V_dot_north;
    FG_V_dot_east =     V_dot_east;
    FG_V_dot_down =     V_dot_down;
    FG_U_dot_body =     U_dot_body;
    FG_V_dot_body =     V_dot_body;
    FG_W_dot_body =     W_dot_body;
    FG_A_X_cg = A_X_cg;
    FG_A_Y_cg = A_Y_cg;
    FG_A_Z_cg = A_Z_cg;
    FG_A_X_pilot =      A_X_pilot;
    FG_A_Y_pilot =      A_Y_pilot;
    FG_A_Z_pilot =      A_Z_pilot;
    FG_N_X_cg = N_X_cg;
    FG_N_Y_cg = N_Y_cg;
    FG_N_Z_cg = N_Z_cg;
    FG_N_X_pilot =      N_X_pilot;
    FG_N_Y_pilot =      N_Y_pilot;
    FG_N_Z_pilot =      N_Z_pilot;
    FG_P_dot_body =     P_dot_body;
    FG_Q_dot_body =     Q_dot_body;
    FG_R_dot_body =     R_dot_body;
    FG_V_north =        V_north;
    FG_V_east = V_east;
    FG_V_down = V_down;
    FG_V_north_rel_ground =     V_north_rel_ground;
    FG_V_east_rel_ground =      V_east_rel_ground;
    FG_V_down_rel_ground =      V_down_rel_ground;
    FG_V_north_airmass =        V_north_airmass;
    FG_V_east_airmass = V_east_airmass;
    FG_V_down_airmass = V_down_airmass;
    FG_V_north_rel_airmass =    V_north_rel_airmass;
    FG_V_east_rel_airmass =     V_east_rel_airmass;
    FG_V_down_rel_airmass =     V_down_rel_airmass;
    FG_U_gust = U_gust;
    FG_V_gust = V_gust;
    FG_W_gust = W_gust;
    FG_U_body = U_body;
    FG_V_body = V_body;
    FG_W_body = W_body;
    FG_V_rel_wind =     V_rel_wind;
    FG_V_true_kts =     V_true_kts;
    FG_V_rel_ground =   V_rel_ground;
    FG_V_inertial =     V_inertial;
    FG_V_ground_speed = V_ground_speed;
    FG_V_equiv =        V_equiv;
    FG_V_equiv_kts =    V_equiv_kts;
    FG_V_calibrated =   V_calibrated;
    FG_V_calibrated_kts =       V_calibrated_kts;
    FG_P_body = P_body;
    FG_Q_body = Q_body;
    FG_R_body = R_body;
    FG_P_local =        P_local;
    FG_Q_local =        Q_local;
    FG_R_local =        R_local;
    FG_P_total =        P_total;
    FG_Q_total =        Q_total;
    FG_R_total =        R_total;
    FG_Phi_dot =        Phi_dot;
    FG_Theta_dot =      Theta_dot;
    FG_Psi_dot =        Psi_dot;
    FG_Latitude_dot =   Latitude_dot;
    FG_Longitude_dot =  Longitude_dot;
    FG_Radius_dot =     Radius_dot;
    FG_Lat_geocentric = Lat_geocentric;
    FG_Lon_geocentric = Lon_geocentric;
    FG_Radius_to_vehicle =      Radius_to_vehicle;
    FG_Latitude =       Latitude;
    FG_Longitude =      Longitude;
    FG_Altitude =       Altitude;
    FG_Phi =    Phi;
    FG_Theta =  Theta;
    FG_Psi =    Psi;
    FG_T_local_to_body_11 =     T_local_to_body_11;
    FG_T_local_to_body_12 =     T_local_to_body_12;
    FG_T_local_to_body_13 =     T_local_to_body_13;
    FG_T_local_to_body_21 =     T_local_to_body_21;
    FG_T_local_to_body_22 =     T_local_to_body_22;
    FG_T_local_to_body_23 =     T_local_to_body_23;
    FG_T_local_to_body_31 =     T_local_to_body_31;
    FG_T_local_to_body_32 =     T_local_to_body_32;
    FG_T_local_to_body_33 =     T_local_to_body_33;
    FG_Gravity =        Gravity;
    FG_Centrifugal_relief =     Centrifugal_relief;
    FG_Alpha =  Alpha;
    FG_Beta =   Beta;
    FG_Alpha_dot =      Alpha_dot;
    FG_Beta_dot =       Beta_dot;
    FG_Cos_alpha =      Cos_alpha;
    FG_Sin_alpha =      Sin_alpha;
    FG_Cos_beta =       Cos_beta;
    FG_Sin_beta =       Sin_beta;
    FG_Cos_phi =        Cos_phi;
    FG_Sin_phi =        Sin_phi;
    FG_Cos_theta =      Cos_theta;
    FG_Sin_theta =      Sin_theta;
    FG_Cos_psi =        Cos_psi;
    FG_Sin_psi =        Sin_psi;
    FG_Gamma_vert_rad = Gamma_vert_rad;
    FG_Gamma_horiz_rad =        Gamma_horiz_rad;
    FG_Sigma =  Sigma;
    FG_Density =        Density;
    FG_V_sound =        V_sound;
    FG_Mach_number =    Mach_number;
    FG_Static_pressure =        Static_pressure;
    FG_Total_pressure = Total_pressure;
    FG_Impact_pressure =        Impact_pressure;
    FG_Dynamic_pressure =       Dynamic_pressure;
    FG_Static_temperature =     Static_temperature;
    FG_Total_temperature =      Total_temperature;
    FG_Sea_level_radius =       Sea_level_radius;
    FG_Earth_position_angle =   Earth_position_angle;
    FG_Runway_altitude =        Runway_altitude;
    FG_Runway_latitude =        Runway_latitude;
    FG_Runway_longitude =       Runway_longitude;
    FG_Runway_heading = Runway_heading;
    FG_Radius_to_rwy =  Radius_to_rwy;
    FG_D_cg_north_of_rwy =      D_cg_north_of_rwy;
    FG_D_cg_east_of_rwy =       D_cg_east_of_rwy;
    FG_D_cg_above_rwy = D_cg_above_rwy;
    FG_X_cg_rwy =       X_cg_rwy;
    FG_Y_cg_rwy =       Y_cg_rwy;
    FG_H_cg_rwy =       H_cg_rwy;
    FG_D_pilot_north_of_rwy =   D_pilot_north_of_rwy;
    FG_D_pilot_east_of_rwy =    D_pilot_east_of_rwy;
    FG_D_pilot_above_rwy =      D_pilot_above_rwy;
    FG_X_pilot_rwy =    X_pilot_rwy;
    FG_Y_pilot_rwy =    Y_pilot_rwy;
    FG_H_pilot_rwy =    H_pilot_rwy;

    return ( 0 );
}


// $Log$
// Revision 1.1  1998/10/17 00:43:58  curt
// Initial revision.
//
//
