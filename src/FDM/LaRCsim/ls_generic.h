/***************************************************************************

	TITLE:	ls_generic.h
	
----------------------------------------------------------------------------

	FUNCTION:	LaRCSim generic parameters header file

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Created 15 DEC 1993 by Bruce Jackson;
			was part of old ls_eom.h header

----------------------------------------------------------------------------

	DESIGNED BY:	B. Jackson
	
	CODED BY:	B. Jackson
	
	MAINTAINED BY:	guess who

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY
	
		
----------------------------------------------------------------------------

	REFERENCES:
	
		[ 1]	McFarland, Richard E.: "A Standard Kinematic Model
			for Flight Simulation at NASA-Ames", NASA CR-2497,
			January 1975
			
		[ 2]	ANSI/AIAA R-004-1992 "Recommended Practice: Atmos-
			pheric and Space Flight Vehicle Coordinate Systems",
			February 1992
			
		[ 3]	Beyer, William H., editor: "CRC Standard Mathematical
			Tables, 28th edition", CRC Press, Boca Raton, FL, 1987,
			ISBN 0-8493-0628-0
			
		[ 4]	Dowdy, M. C.; Jackson, E. B.; and Nichols, J. H.:
			"Controls Analysis and Simulation Test Loop Environ-
			ment (CASTLE) Programmer's Guide, Version 1.3", 
			NATC TM 89-11, 30 March 1989.
			
		[ 5]	Halliday, David; and Resnick, Robert: "Fundamentals
			of Physics, Revised Printing", Wiley and Sons, 1974.
			ISBN 0-471-34431-1

		[ 6]	Anon: "U. S. Standard Atmosphere, 1962"
		
		[ 7]	Anon: "Aeronautical Vest Pocket Handbook, 17th edition",
			Pratt & Whitney Aircraft Group, Dec. 1977
			
		[ 8]	Stevens, Brian L.; and Lewis, Frank L.: "Aircraft 
			Control and Simulation", Wiley and Sons, 1992.
			ISBN 0-471-61397-5			

--------------------------------------------------------------------------*/


#ifndef _LS_GENERIC_H
#define _LS_GENERIC_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


#include "ls_types.h"


typedef struct {

/*================== Mass properties and geometry values ==================*/
	
    DATA    mass, i_xx, i_yy, i_zz, i_xz;	/* Inertias */
#define Mass		    	generic_.mass
#define I_xx		    	generic_.i_xx
#define I_yy		    	generic_.i_yy
#define I_zz		    	generic_.i_zz
#define I_xz		    	generic_.i_xz
	
    VECTOR_3    d_pilot_rp_body_v;	/* Pilot location rel to ref pt	*/
#define D_pilot_rp_body_v   	generic_.d_pilot_rp_body_v
#define Dx_pilot	    	generic_.d_pilot_rp_body_v[0]
#define	Dy_pilot	    	generic_.d_pilot_rp_body_v[1]
#define	Dz_pilot	    	generic_.d_pilot_rp_body_v[2]

    VECTOR_3    d_cg_rp_body_v;	/* CG position w.r.t. ref. point */
#define D_cg_rp_body_v	    	generic_.d_cg_rp_body_v
#define Dx_cg		    	generic_.d_cg_rp_body_v[0]
#define Dy_cg		    	generic_.d_cg_rp_body_v[1]
#define Dz_cg		    	generic_.d_cg_rp_body_v[2]
	
/*================================ Forces =================================*/

    VECTOR_3    f_body_total_v;
#define F_body_total_v	    	generic_.f_body_total_v
#define	F_X		    	generic_.f_body_total_v[0]
#define F_Y		    	generic_.f_body_total_v[1]
#define F_Z		    	generic_.f_body_total_v[2]

    VECTOR_3    f_local_total_v;
#define F_local_total_v		generic_.f_local_total_v
#define	F_north			generic_.f_local_total_v[0]
#define F_east			generic_.f_local_total_v[1]
#define F_down			generic_.f_local_total_v[2]

    VECTOR_3    f_aero_v;
#define F_aero_v		generic_.f_aero_v
#define	F_X_aero		generic_.f_aero_v[0]
#define	F_Y_aero		generic_.f_aero_v[1]
#define	F_Z_aero		generic_.f_aero_v[2]

    VECTOR_3    f_engine_v;
#define F_engine_v		generic_.f_engine_v
#define	F_X_engine		generic_.f_engine_v[0]
#define	F_Y_engine		generic_.f_engine_v[1]
#define	F_Z_engine		generic_.f_engine_v[2]

    int         use_external_engine;
#define Use_External_Engine     generic_.use_external_engine

    VECTOR_3    f_gear_v;
#define F_gear_v		generic_.f_gear_v
#define	F_X_gear		generic_.f_gear_v[0]
#define	F_Y_gear		generic_.f_gear_v[1]
#define	F_Z_gear		generic_.f_gear_v[2]

/*================================ Moments ================================*/

    VECTOR_3    m_total_rp_v;
#define M_total_rp_v		generic_.m_total_rp_v
#define	M_l_rp			generic_.m_total_rp_v[0]
#define M_m_rp			generic_.m_total_rp_v[1]
#define M_n_rp			generic_.m_total_rp_v[2]

    VECTOR_3    m_total_cg_v;
#define M_total_cg_v		generic_.m_total_cg_v
#define	M_l_cg			generic_.m_total_cg_v[0]
#define M_m_cg			generic_.m_total_cg_v[1]
#define M_n_cg			generic_.m_total_cg_v[2]

    VECTOR_3    m_aero_v;
#define M_aero_v		generic_.m_aero_v
#define	M_l_aero		generic_.m_aero_v[0]
#define	M_m_aero		generic_.m_aero_v[1]
#define	M_n_aero		generic_.m_aero_v[2]

    VECTOR_3    m_engine_v;
#define M_engine_v		generic_.m_engine_v
#define	M_l_engine		generic_.m_engine_v[0]
#define	M_m_engine		generic_.m_engine_v[1]
#define	M_n_engine		generic_.m_engine_v[2]

    VECTOR_3    m_gear_v;
#define M_gear_v		generic_.m_gear_v
#define	M_l_gear		generic_.m_gear_v[0]
#define	M_m_gear		generic_.m_gear_v[1]
#define	M_n_gear		generic_.m_gear_v[2]

/*============================== Accelerations ============================*/

    VECTOR_3    v_dot_local_v;
#define V_dot_local_v		generic_.v_dot_local_v
#define	V_dot_north		generic_.v_dot_local_v[0]
#define	V_dot_east		generic_.v_dot_local_v[1]
#define	V_dot_down		generic_.v_dot_local_v[2]

    VECTOR_3    v_dot_body_v;
#define V_dot_body_v		generic_.v_dot_body_v
#define	U_dot_body		generic_.v_dot_body_v[0]
#define	V_dot_body		generic_.v_dot_body_v[1]
#define	W_dot_body		generic_.v_dot_body_v[2]

    VECTOR_3    a_cg_body_v;
#define A_cg_body_v		generic_.a_cg_body_v
#define	A_X_cg			generic_.a_cg_body_v[0]
#define A_Y_cg			generic_.a_cg_body_v[1]
#define	A_Z_cg			generic_.a_cg_body_v[2]

    VECTOR_3    a_pilot_body_v;
#define A_pilot_body_v		generic_.a_pilot_body_v
#define	A_X_pilot		generic_.a_pilot_body_v[0]
#define	A_Y_pilot		generic_.a_pilot_body_v[1]
#define	A_Z_pilot		generic_.a_pilot_body_v[2]

    VECTOR_3    n_cg_body_v;
#define N_cg_body_v		generic_.n_cg_body_v
#define	N_X_cg			generic_.n_cg_body_v[0]
#define N_Y_cg			generic_.n_cg_body_v[1]
#define	N_Z_cg			generic_.n_cg_body_v[2]

    VECTOR_3    n_pilot_body_v;
#define N_pilot_body_v		generic_.n_pilot_body_v
#define	N_X_pilot		generic_.n_pilot_body_v[0]
#define	N_Y_pilot		generic_.n_pilot_body_v[1]
#define	N_Z_pilot		generic_.n_pilot_body_v[2]

    VECTOR_3    omega_dot_body_v;
#define Omega_dot_body_v	generic_.omega_dot_body_v
#define P_dot_body		generic_.omega_dot_body_v[0]
#define Q_dot_body		generic_.omega_dot_body_v[1]
#define	R_dot_body		generic_.omega_dot_body_v[2]


/*============================== Velocities ===============================*/

    VECTOR_3    v_local_v;
#define V_local_v		generic_.v_local_v
#define	V_north			generic_.v_local_v[0]
#define	V_east			generic_.v_local_v[1]
#define	V_down			generic_.v_local_v[2]

    VECTOR_3    v_local_rel_ground_v;	/* V rel w.r.t. earth surface	*/
#define V_local_rel_ground_v	generic_.v_local_rel_ground_v
#define	V_north_rel_ground	generic_.v_local_rel_ground_v[0]
#define V_east_rel_ground	generic_.v_local_rel_ground_v[1]
#define	V_down_rel_ground	generic_.v_local_rel_ground_v[2]

    VECTOR_3    v_local_airmass_v;	/* velocity of airmass (steady winds)	*/
#define V_local_airmass_v	generic_.v_local_airmass_v
#define V_north_airmass		generic_.v_local_airmass_v[0]
#define	V_east_airmass		generic_.v_local_airmass_v[1]
#define	V_down_airmass		generic_.v_local_airmass_v[2]

    VECTOR_3    v_local_rel_airmass_v;	/* velocity of veh. relative to airmass	*/
#define V_local_rel_airmass_v	generic_.v_local_rel_airmass_v
#define	V_north_rel_airmass	generic_.v_local_rel_airmass_v[0]
#define	V_east_rel_airmass	generic_.v_local_rel_airmass_v[1]
#define	V_down_rel_airmass	generic_.v_local_rel_airmass_v[2]

    VECTOR_3    v_local_gust_v; /* linear turbulence components, L frame */
#define V_local_gust_v		generic_.v_local_gust_v
#define	U_gust			generic_.v_local_gust_v[0]
#define	V_gust			generic_.v_local_gust_v[1]
#define	W_gust			generic_.v_local_gust_v[2]

    VECTOR_3    v_wind_body_v;	/* Wind-relative velocities in body axis	*/
#define V_wind_body_v		generic_.v_wind_body_v
#define	U_body			generic_.v_wind_body_v[0]
#define V_body			generic_.v_wind_body_v[1]
#define	W_body			generic_.v_wind_body_v[2]

    DATA    v_rel_wind, v_true_kts, v_rel_ground, v_inertial;
    DATA    v_ground_speed, v_equiv, v_equiv_kts;
    DATA    v_calibrated, v_calibrated_kts;
#define V_rel_wind		generic_.v_rel_wind
#define V_true_kts		generic_.v_true_kts
#define V_rel_ground		generic_.v_rel_ground
#define V_inertial		generic_.v_inertial
#define V_ground_speed		generic_.v_ground_speed
#define V_equiv			generic_.v_equiv
#define V_equiv_kts		generic_.v_equiv_kts
#define V_calibrated		generic_.v_calibrated
#define V_calibrated_kts	generic_.v_calibrated_kts

    VECTOR_3    omega_body_v;	/* Angular B rates	*/
#define Omega_body_v		generic_.omega_body_v
#define	P_body			generic_.omega_body_v[0]
#define	Q_body			generic_.omega_body_v[1]
#define	R_body			generic_.omega_body_v[2]
			
    VECTOR_3    omega_local_v;	/* Angular L rates	*/
#define Omega_local_v		generic_.omega_local_v
#define	P_local			generic_.omega_local_v[0]
#define	Q_local			generic_.omega_local_v[1]
#define	R_local			generic_.omega_local_v[2]

    VECTOR_3    omega_total_v;	/* Diff btw B & L	*/	
#define Omega_total_v		generic_.omega_total_v
#define	P_total			generic_.omega_total_v[0]
#define	Q_total			generic_.omega_total_v[1]
#define	R_total			generic_.omega_total_v[2]

    VECTOR_3    euler_rates_v;
#define Euler_rates_v		generic_.euler_rates_v
#define	Phi_dot			generic_.euler_rates_v[0]
#define	Theta_dot		generic_.euler_rates_v[1]
#define	Psi_dot			generic_.euler_rates_v[2]

    VECTOR_3    geocentric_rates_v;	/* Geocentric linear velocities */
#define Geocentric_rates_v	generic_.geocentric_rates_v
#define	Latitude_dot		generic_.geocentric_rates_v[0]
#define	Longitude_dot		generic_.geocentric_rates_v[1]
#define	Radius_dot		generic_.geocentric_rates_v[2]

/*=============================== Positions ===============================*/

    VECTOR_3    geocentric_position_v;
#define Geocentric_position_v	generic_.geocentric_position_v
#define Lat_geocentric 		generic_.geocentric_position_v[0]
#define	Lon_geocentric 		generic_.geocentric_position_v[1]
#define	Radius_to_vehicle	generic_.geocentric_position_v[2]

    VECTOR_3    geodetic_position_v;
#define Geodetic_position_v	generic_.geodetic_position_v
#define Latitude		generic_.geodetic_position_v[0]
#define	Longitude		generic_.geodetic_position_v[1]
#define Altitude       		generic_.geodetic_position_v[2]

    VECTOR_3    euler_angles_v;
#define Euler_angles_v		generic_.euler_angles_v
#define	Phi			generic_.euler_angles_v[0]
#define	Theta			generic_.euler_angles_v[1]
#define	Psi			generic_.euler_angles_v[2]

/*======================= Miscellaneous quantities ========================*/
	
    DATA    t_local_to_body_m[3][3];	/* Transformation matrix L to B */
#define T_local_to_body_m	generic_.t_local_to_body_m
#define	T_local_to_body_11	generic_.t_local_to_body_m[0][0]
#define	T_local_to_body_12	generic_.t_local_to_body_m[0][1]
#define	T_local_to_body_13	generic_.t_local_to_body_m[0][2]
#define	T_local_to_body_21	generic_.t_local_to_body_m[1][0]
#define	T_local_to_body_22	generic_.t_local_to_body_m[1][1]
#define	T_local_to_body_23	generic_.t_local_to_body_m[1][2]
#define	T_local_to_body_31	generic_.t_local_to_body_m[2][0]
#define	T_local_to_body_32	generic_.t_local_to_body_m[2][1]
#define	T_local_to_body_33	generic_.t_local_to_body_m[2][2]

    DATA    gravity;		/* Local acceleration due to G	*/
#define Gravity			generic_.gravity

    DATA    centrifugal_relief;	/* load factor reduction due to speed */
#define Centrifugal_relief	generic_.centrifugal_relief

    DATA    alpha, beta, alpha_dot, beta_dot;	/* in radians	*/
#define Std_Alpha		generic_.alpha
#define Std_Beta		generic_.beta
#define Std_Alpha_dot		generic_.alpha_dot
#define Std_Beta_dot		generic_.beta_dot

    DATA    cos_alpha, sin_alpha, cos_beta, sin_beta;
#define Cos_alpha		generic_.cos_alpha
#define Sin_alpha		generic_.sin_alpha
#define Cos_beta		generic_.cos_beta
#define Sin_beta		generic_.sin_beta

    DATA    cos_phi, sin_phi, cos_theta, sin_theta, cos_psi, sin_psi;
#define Cos_phi			generic_.cos_phi
#define Sin_phi			generic_.sin_phi
#define Cos_theta		generic_.cos_theta
#define Sin_theta		generic_.sin_theta
#define Cos_psi			generic_.cos_psi
#define Sin_psi			generic_.sin_psi
	
    DATA    gamma_vert_rad, gamma_horiz_rad;	/* Flight path angles	*/
#define Gamma_vert_rad		generic_.gamma_vert_rad
#define Gamma_horiz_rad		generic_.gamma_horiz_rad
	
    DATA    sigma, density, v_sound, mach_number;
#define Sigma			generic_.sigma
#define Density			generic_.density
#define V_sound			generic_.v_sound
#define Mach_number		generic_.mach_number
	
    DATA    static_pressure, total_pressure, impact_pressure, dynamic_pressure;
#define Static_pressure		generic_.static_pressure
#define Total_pressure		generic_.total_pressure
#define Impact_pressure		generic_.impact_pressure
#define Dynamic_pressure	generic_.dynamic_pressure

    DATA    static_temperature, total_temperature;
#define Static_temperature	generic_.static_temperature
#define Total_temperature	generic_.total_temperature
	
    DATA    sea_level_radius, earth_position_angle;
#define Sea_level_radius	generic_.sea_level_radius
#define Earth_position_angle	generic_.earth_position_angle
	
    DATA    runway_altitude, runway_latitude, runway_longitude, runway_heading;
#define Runway_altitude		generic_.runway_altitude
#define Runway_latitude		generic_.runway_latitude
#define Runway_longitude	generic_.runway_longitude
#define Runway_heading		generic_.runway_heading

    DATA    radius_to_rwy;
#define Radius_to_rwy		generic_.radius_to_rwy
	
    VECTOR_3    d_cg_rwy_local_v;       /* CG rel. to rwy in local coords */
#define D_cg_rwy_local_v	generic_.d_cg_rwy_local_v
#define	D_cg_north_of_rwy	generic_.d_cg_rwy_local_v[0]
#define	D_cg_east_of_rwy	generic_.d_cg_rwy_local_v[1]
#define	D_cg_above_rwy		generic_.d_cg_rwy_local_v[2]

    VECTOR_3    d_cg_rwy_rwy_v;	/* CG relative to runway, in rwy coordinates */
#define D_cg_rwy_rwy_v		generic_.d_cg_rwy_rwy_v
#define X_cg_rwy		generic_.d_cg_rwy_rwy_v[0]
#define Y_cg_rwy		generic_.d_cg_rwy_rwy_v[1]
#define H_cg_rwy		generic_.d_cg_rwy_rwy_v[2]

    VECTOR_3    d_pilot_rwy_local_v;	/* pilot rel. to rwy in local coords */
#define D_pilot_rwy_local_v	generic_.d_pilot_rwy_local_v
#define	D_pilot_north_of_rwy	generic_.d_pilot_rwy_local_v[0]
#define	D_pilot_east_of_rwy	generic_.d_pilot_rwy_local_v[1]
#define	D_pilot_above_rwy	generic_.d_pilot_rwy_local_v[2]

    VECTOR_3   d_pilot_rwy_rwy_v;	/* pilot rel. to rwy, in rwy coords. */
#define D_pilot_rwy_rwy_v	generic_.d_pilot_rwy_rwy_v
#define X_pilot_rwy		generic_.d_pilot_rwy_rwy_v[0]
#define Y_pilot_rwy		generic_.d_pilot_rwy_rwy_v[1]
#define H_pilot_rwy		generic_.d_pilot_rwy_rwy_v[2]


} GENERIC;

extern GENERIC generic_;	/* usually defined in ls_main.c */


#ifdef __cplusplus
}
#endif


#endif /* _LS_GENERIC_H */


/*---------------------------  end of ls_generic.h  ------------------------*/
