/**************************************************************************
 * flight.h -- define shared flight model parameters
 *
 * Written by Curtis Olson, started May 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef _FLIGHT_H
#define _FLIGHT_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


#include <Flight/Slew/slew.h>
/* #include <Flight/LaRCsim/ls_interface.h> */


/* Define the various supported flight models (most not yet implemented) */
#define FG_LARCSIM     0   /* The only one that is currently implemented */
#define FG_ACM         1
#define FG_SUPER_SONIC 2
#define FG_HELICOPTER  3
#define FG_AUTOGYRO    4
#define FG_BALLOON     5
#define FG_PARACHUTE   6
#define FG_SLEW        7    /* Slew (in MS terminology) */
#define FG_EXTERN_GPS  8    /* Driven via a serially connected GPS */
#define FG_EXTERN_NET  9    /* Driven externally via the net */
#define FG_EXTERN_NASA 10   /* Track the space shuttle ? */


typedef double FG_VECTOR_3[3];

/* This is based heavily on LaRCsim/ls_generic.h */
typedef struct {

/*================== Mass properties and geometry values ==================*/

    double    mass, i_xx, i_yy, i_zz, i_xz; /* Inertias */
#define FG_Mass                    f->mass
#define FG_I_xx                    f->i_xx
#define FG_I_yy                    f->i_yy
#define FG_I_zz                    f->i_zz
#define FG_I_xz                    f->i_xz

    FG_VECTOR_3    d_pilot_rp_body_v;      /* Pilot location rel to ref pt */
#define FG_D_pilot_rp_body_v       f->d_pilot_rp_body_v
#define FG_Dx_pilot                f->d_pilot_rp_body_v[0]
#define FG_Dy_pilot                f->d_pilot_rp_body_v[1]
#define FG_Dz_pilot                f->d_pilot_rp_body_v[2]

    FG_VECTOR_3    d_cg_rp_body_v; /* CG position w.r.t. ref. point */
#define FG_D_cg_rp_body_v          f->d_cg_rp_body_v
#define FG_Dx_cg                   f->d_cg_rp_body_v[0]
#define FG_Dy_cg                   f->d_cg_rp_body_v[1]
#define FG_Dz_cg                   f->d_cg_rp_body_v[2]

/*================================ Forces =================================*/

    FG_VECTOR_3    f_body_total_v;
#define FG_F_body_total_v          f->f_body_total_v
#define FG_F_X                     f->f_body_total_v[0]
#define FG_F_Y                     f->f_body_total_v[1]
#define FG_F_Z                     f->f_body_total_v[2]

    FG_VECTOR_3    f_local_total_v;
#define FG_F_local_total_v         f->f_local_total_v
#define FG_F_north                 f->f_local_total_v[0]
#define FG_F_east                  f->f_local_total_v[1]
#define FG_F_down                  f->f_local_total_v[2]

    FG_VECTOR_3    f_aero_v;
#define FG_F_aero_v                f->f_aero_v
#define FG_F_X_aero                f->f_aero_v[0]
#define FG_F_Y_aero                f->f_aero_v[1]
#define FG_F_Z_aero                f->f_aero_v[2]

    FG_VECTOR_3    f_engine_v;
#define FG_F_engine_v              f->f_engine_v
#define FG_F_X_engine              f->f_engine_v[0]
#define FG_F_Y_engine              f->f_engine_v[1]
#define FG_F_Z_engine              f->f_engine_v[2]

    FG_VECTOR_3    f_gear_v;
#define FG_F_gear_v                f->f_gear_v
#define FG_F_X_gear                f->f_gear_v[0]
#define FG_F_Y_gear                f->f_gear_v[1]
#define FG_F_Z_gear                f->f_gear_v[2]

/*================================ Moments ================================*/

    FG_VECTOR_3    m_total_rp_v;
#define FG_M_total_rp_v            f->m_total_rp_v
#define FG_M_l_rp                  f->m_total_rp_v[0]
#define FG_M_m_rp                  f->m_total_rp_v[1]
#define FG_M_n_rp                  f->m_total_rp_v[2]

    FG_VECTOR_3    m_total_cg_v;
#define FG_M_total_cg_v            f->m_total_cg_v
#define FG_M_l_cg                  f->m_total_cg_v[0]
#define FG_M_m_cg                  f->m_total_cg_v[1]
#define FG_M_n_cg                  f->m_total_cg_v[2]

    FG_VECTOR_3    m_aero_v;
#define FG_M_aero_v                f->m_aero_v
#define FG_M_l_aero                f->m_aero_v[0]
#define FG_M_m_aero                f->m_aero_v[1]
#define FG_M_n_aero                f->m_aero_v[2]

    FG_VECTOR_3    m_engine_v;
#define FG_M_engine_v              f->m_engine_v
#define FG_M_l_engine              f->m_engine_v[0]
#define FG_M_m_engine              f->m_engine_v[1]
#define FG_M_n_engine              f->m_engine_v[2]

    FG_VECTOR_3    m_gear_v;
#define FG_M_gear_v                f->m_gear_v
#define FG_M_l_gear                f->m_gear_v[0]
#define FG_M_m_gear                f->m_gear_v[1]
#define FG_M_n_gear                f->m_gear_v[2]

/*============================== Accelerations ============================*/

    FG_VECTOR_3    v_dot_local_v;
#define FG_V_dot_local_v           f->v_dot_local_v
#define FG_V_dot_north             f->v_dot_local_v[0]
#define FG_V_dot_east              f->v_dot_local_v[1]
#define FG_V_dot_down              f->v_dot_local_v[2]

    FG_VECTOR_3    v_dot_body_v;
#define FG_V_dot_body_v            f->v_dot_body_v
#define FG_U_dot_body              f->v_dot_body_v[0]
#define FG_V_dot_body              f->v_dot_body_v[1]
#define FG_W_dot_body              f->v_dot_body_v[2]

    FG_VECTOR_3    a_cg_body_v;
#define FG_A_cg_body_v             f->a_cg_body_v
#define FG_A_X_cg                  f->a_cg_body_v[0]
#define FG_A_Y_cg                  f->a_cg_body_v[1]
#define FG_A_Z_cg                  f->a_cg_body_v[2]

    FG_VECTOR_3    a_pilot_body_v;
#define FG_A_pilot_body_v          f->a_pilot_body_v
#define FG_A_X_pilot               f->a_pilot_body_v[0]
#define FG_A_Y_pilot               f->a_pilot_body_v[1]
#define FG_A_Z_pilot               f->a_pilot_body_v[2]

    FG_VECTOR_3    n_cg_body_v;
#define FG_N_cg_body_v             f->n_cg_body_v
#define FG_N_X_cg                  f->n_cg_body_v[0]
#define FG_N_Y_cg                  f->n_cg_body_v[1]
#define FG_N_Z_cg                  f->n_cg_body_v[2]

    FG_VECTOR_3    n_pilot_body_v;
#define FG_N_pilot_body_v          f->n_pilot_body_v
#define FG_N_X_pilot               f->n_pilot_body_v[0]
#define FG_N_Y_pilot               f->n_pilot_body_v[1]
#define FG_N_Z_pilot               f->n_pilot_body_v[2]

    FG_VECTOR_3    omega_dot_body_v;
#define FG_Omega_dot_body_v        f->omega_dot_body_v
#define FG_P_dot_body              f->omega_dot_body_v[0]
#define FG_Q_dot_body              f->omega_dot_body_v[1]
#define FG_R_dot_body              f->omega_dot_body_v[2]


/*============================== Velocities ===============================*/

    FG_VECTOR_3    v_local_v;
#define FG_V_local_v               f->v_local_v
#define FG_V_north                 f->v_local_v[0]
#define FG_V_east                  f->v_local_v[1]
#define FG_V_down                  f->v_local_v[2]

    FG_VECTOR_3    v_local_rel_ground_v; /* V rel w.r.t. earth surface   */
#define FG_V_local_rel_ground_v    f->v_local_rel_ground_v
#define FG_V_north_rel_ground      f->v_local_rel_ground_v[0]
#define FG_V_east_rel_ground       f->v_local_rel_ground_v[1]
#define FG_V_down_rel_ground       f->v_local_rel_ground_v[2]

    FG_VECTOR_3    v_local_airmass_v;   /* velocity of airmass (steady winds) */
#define FG_V_local_airmass_v       f->v_local_airmass_v
#define FG_V_north_airmass         f->v_local_airmass_v[0]
#define FG_V_east_airmass          f->v_local_airmass_v[1]
#define FG_V_down_airmass          f->v_local_airmass_v[2]

    FG_VECTOR_3    v_local_rel_airmass_v;  /* velocity of veh. relative to */
                                           /* airmass */
#define FG_V_local_rel_airmass_v   f->v_local_rel_airmass_v
#define FG_V_north_rel_airmass     f->v_local_rel_airmass_v[0]
#define FG_V_east_rel_airmass      f->v_local_rel_airmass_v[1]
#define FG_V_down_rel_airmass      f->v_local_rel_airmass_v[2]

    FG_VECTOR_3    v_local_gust_v; /* linear turbulence components, L frame */
#define FG_V_local_gust_v          f->v_local_gust_v
#define FG_U_gust                  f->v_local_gust_v[0]
#define FG_V_gust                  f->v_local_gust_v[1]
#define FG_W_gust                  f->v_local_gust_v[2]

    FG_VECTOR_3    v_wind_body_v;  /* Wind-relative velocities in body axis */
#define FG_V_wind_body_v           f->v_wind_body_v
#define FG_U_body                  f->v_wind_body_v[0]
#define FG_V_body                  f->v_wind_body_v[1]
#define FG_W_body                  f->v_wind_body_v[2]

    double    v_rel_wind, v_true_kts, v_rel_ground, v_inertial;
    double    v_ground_speed, v_equiv, v_equiv_kts;
    double    v_calibrated, v_calibrated_kts;
#define FG_V_rel_wind              f->v_rel_wind
#define FG_V_true_kts              f->v_true_kts
#define FG_V_rel_ground            f->v_rel_ground
#define FG_V_inertial              f->v_inertial
#define FG_V_ground_speed          f->v_ground_speed
#define FG_V_equiv                 f->v_equiv
#define FG_V_equiv_kts             f->v_equiv_kts
#define FG_V_calibrated            f->v_calibrated
#define FG_V_calibrated_kts        f->v_calibrated_kts

    FG_VECTOR_3    omega_body_v;   /* Angular B rates      */
#define FG_Omega_body_v            f->omega_body_v
#define FG_P_body                  f->omega_body_v[0]
#define FG_Q_body                  f->omega_body_v[1]
#define FG_R_body                  f->omega_body_v[2]

    FG_VECTOR_3    omega_local_v;  /* Angular L rates      */
#define FG_Omega_local_v           f->omega_local_v
#define FG_P_local                 f->omega_local_v[0]
#define FG_Q_local                 f->omega_local_v[1]
#define FG_R_local                 f->omega_local_v[2]

    FG_VECTOR_3    omega_total_v;  /* Diff btw B & L       */
#define FG_Omega_total_v           f->omega_total_v
#define FG_P_total                 f->omega_total_v[0]
#define FG_Q_total                 f->omega_total_v[1]
#define FG_R_total                 f->omega_total_v[2]

    FG_VECTOR_3    euler_rates_v;
#define FG_Euler_rates_v           f->euler_rates_v
#define FG_Phi_dot                 f->euler_rates_v[0]
#define FG_Theta_dot               f->euler_rates_v[1]
#define FG_Psi_dot                 f->euler_rates_v[2]

    FG_VECTOR_3    geocentric_rates_v;     /* Geocentric linear velocities */
#define FG_Geocentric_rates_v      f->geocentric_rates_v
#define FG_Latitude_dot            f->geocentric_rates_v[0]
#define FG_Longitude_dot           f->geocentric_rates_v[1]
#define FG_Radius_dot              f->geocentric_rates_v[2]

/*=============================== Positions ===============================*/

    FG_VECTOR_3    geocentric_position_v;
#define FG_Geocentric_position_v   f->geocentric_position_v
#define FG_Lat_geocentric          f->geocentric_position_v[0]
#define FG_Lon_geocentric          f->geocentric_position_v[1]
#define FG_Radius_to_vehicle       f->geocentric_position_v[2]

    FG_VECTOR_3    geodetic_position_v;
#define FG_Geodetic_position_v     f->geodetic_position_v
#define FG_Latitude                f->geodetic_position_v[0]
#define FG_Longitude               f->geodetic_position_v[1]
#define FG_Altitude                f->geodetic_position_v[2]

    FG_VECTOR_3    euler_angles_v;
#define FG_Euler_angles_v          f->euler_angles_v
#define FG_Phi                     f->euler_angles_v[0]
#define FG_Theta                   f->euler_angles_v[1]
#define FG_Psi                     f->euler_angles_v[2]

/*======================= Miscellaneous quantities ========================*/

    double    t_local_to_body_m[3][3];    /* Transformation matrix L to B */
#define FG_T_local_to_body_m       f->t_local_to_body_m
#define FG_T_local_to_body_11      f->t_local_to_body_m[0][0]
#define FG_T_local_to_body_12      f->t_local_to_body_m[0][1]
#define FG_T_local_to_body_13      f->t_local_to_body_m[0][2]
#define FG_T_local_to_body_21      f->t_local_to_body_m[1][0]
#define FG_T_local_to_body_22      f->t_local_to_body_m[1][1]
#define FG_T_local_to_body_23      f->t_local_to_body_m[1][2]
#define FG_T_local_to_body_31      f->t_local_to_body_m[2][0]
#define FG_T_local_to_body_32      f->t_local_to_body_m[2][1]
#define FG_T_local_to_body_33      f->t_local_to_body_m[2][2]

    double    gravity;            /* Local acceleration due to G  */
#define FG_Gravity                 f->gravity

    double    centrifugal_relief; /* load factor reduction due to speed */
#define FG_Centrifugal_relief      f->centrifugal_relief

    double    alpha, beta, alpha_dot, beta_dot;   /* in radians   */
#define FG_Alpha                   f->alpha
#define FG_Beta                    f->beta
#define FG_Alpha_dot               f->alpha_dot
#define FG_Beta_dot                f->beta_dot

    double    cos_alpha, sin_alpha, cos_beta, sin_beta;
#define FG_Cos_alpha               f->cos_alpha
#define FG_Sin_alpha               f->sin_alpha
#define FG_Cos_beta                f->cos_beta
#define FG_Sin_beta                f->sin_beta

    double    cos_phi, sin_phi, cos_theta, sin_theta, cos_psi, sin_psi;
#define FG_Cos_phi                 f->cos_phi
#define FG_Sin_phi                 f->sin_phi
#define FG_Cos_theta               f->cos_theta
#define FG_Sin_theta               f->sin_theta
#define FG_Cos_psi                 f->cos_psi
#define FG_Sin_psi                 f->sin_psi

    double    gamma_vert_rad, gamma_horiz_rad;    /* Flight path angles   */
#define FG_Gamma_vert_rad          f->gamma_vert_rad
#define FG_Gamma_horiz_rad         f->gamma_horiz_rad

    double    sigma, density, v_sound, mach_number;
#define FG_Sigma                   f->sigma
#define FG_Density                 f->density
#define FG_V_sound                 f->v_sound
#define FG_Mach_number             f->mach_number

    double    static_pressure, total_pressure, impact_pressure;
    double    dynamic_pressure;
#define FG_Static_pressure         f->static_pressure
#define FG_Total_pressure          f->total_pressure
#define FG_Impact_pressure         f->impact_pressure
#define FG_Dynamic_pressure        f->dynamic_pressure

    double    static_temperature, total_temperature;
#define FG_Static_temperature      f->static_temperature
#define FG_Total_temperature       f->total_temperature

    double    sea_level_radius, earth_position_angle;
#define FG_Sea_level_radius        f->sea_level_radius
#define FG_Earth_position_angle    f->earth_position_angle

    double    runway_altitude, runway_latitude, runway_longitude;
    double    runway_heading;
#define FG_Runway_altitude         f->runway_altitude
#define FG_Runway_latitude         f->runway_latitude
#define FG_Runway_longitude        f->runway_longitude
#define FG_Runway_heading          f->runway_heading

    double    radius_to_rwy;
#define FG_Radius_to_rwy           f->radius_to_rwy

    FG_VECTOR_3    d_cg_rwy_local_v;       /* CG rel. to rwy in local coords */
#define FG_D_cg_rwy_local_v        f->d_cg_rwy_local_v
#define FG_D_cg_north_of_rwy       f->d_cg_rwy_local_v[0]
#define FG_D_cg_east_of_rwy        f->d_cg_rwy_local_v[1]
#define FG_D_cg_above_rwy          f->d_cg_rwy_local_v[2]

    FG_VECTOR_3    d_cg_rwy_rwy_v; /* CG relative to rwy, in rwy coordinates */
#define FG_D_cg_rwy_rwy_v          f->d_cg_rwy_rwy_v
#define FG_X_cg_rwy                f->d_cg_rwy_rwy_v[0]
#define FG_Y_cg_rwy                f->d_cg_rwy_rwy_v[1]
#define FG_H_cg_rwy                f->d_cg_rwy_rwy_v[2]

    FG_VECTOR_3    d_pilot_rwy_local_v;  /* pilot rel. to rwy in local coords */
#define FG_D_pilot_rwy_local_v     f->d_pilot_rwy_local_v
#define FG_D_pilot_north_of_rwy    f->d_pilot_rwy_local_v[0]
#define FG_D_pilot_east_of_rwy     f->d_pilot_rwy_local_v[1]
#define FG_D_pilot_above_rwy       f->d_pilot_rwy_local_v[2]

    FG_VECTOR_3   d_pilot_rwy_rwy_v;   /* pilot rel. to rwy, in rwy coords. */
#define FG_D_pilot_rwy_rwy_v       f->d_pilot_rwy_rwy_v
#define FG_X_pilot_rwy             f->d_pilot_rwy_rwy_v[0]
#define FG_Y_pilot_rwy             f->d_pilot_rwy_rwy_v[1]
#define FG_H_pilot_rwy             f->d_pilot_rwy_rwy_v[2]

} fgFLIGHT, *pfgFlight;


extern fgFLIGHT cur_flight_params;


/* General interface to the flight model routines */

/* Initialize the flight model parameters */
int fgFlightModelInit(int model, fgFLIGHT *f, double dt);

/* Run multiloop iterations of the flight model */
int fgFlightModelUpdate(int model, fgFLIGHT *f, int multiloop);


#ifdef __cplusplus
}
#endif


#endif /* _FLIGHT_H */


/* $Log$
/* Revision 1.15  1998/04/21 16:59:33  curt
/* Integrated autopilot.
/* Prepairing for C++ integration.
/*
 * Revision 1.14  1998/02/07 15:29:37  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.13  1998/01/24 00:04:59  curt
 * misc. tweaks.
 *
 * Revision 1.12  1998/01/22 02:59:32  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.11  1998/01/19 19:27:03  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.10  1997/12/10 22:37:43  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.9  1997/09/04 02:17:33  curt
 * Shufflin' stuff.
 *
 * Revision 1.8  1997/08/27 03:30:06  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.7  1997/07/23 21:52:19  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.6  1997/06/21 17:52:22  curt
 * Continue directory shuffling ... everything should be compilable/runnable
 * again.
 *
 * Revision 1.5  1997/06/21 17:12:49  curt
 * Capitalized subdirectory names.
 *
 * Revision 1.4  1997/05/29 22:39:57  curt
 * Working on incorporating the LaRCsim flight model.
 *
 * Revision 1.3  1997/05/29 02:32:25  curt
 * Starting to build generic flight model interface.
 *
 * Revision 1.2  1997/05/23 15:40:37  curt
 * Added GNU copyright headers.
 *
 * Revision 1.1  1997/05/16 16:04:45  curt
 * Initial revision.
 *
 */
