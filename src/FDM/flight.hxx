// flight.hxx -- define shared flight model parameters
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#ifndef _FLIGHT_HXX
#define _FLIGHT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


/* Required get_()

   `FGInterface::get_Longitude ()'
   `FGInterface::get_Latitude ()'
   `FGInterface::get_Altitude ()'
   `FGInterface::get_Phi ()'
   `FGInterface::get_Theta ()'
   `FGInterface::get_Psi ()'
   `FGInterface::get_V_equiv_kts ()'

   `FGInterface::get_Mass ()'
   `FGInterface::get_I_xx ()'
   `FGInterface::get_I_yy ()'
   `FGInterface::get_I_zz ()'
   `FGInterface::get_I_xz ()'
   
   `FGInterface::get_V_north ()'
   `FGInterface::get_V_east ()'
   `FGInterface::get_V_down ()'

   `FGInterface::get_P_Body ()'
   `FGInterface::get_Q_Body ()'
   `FGInterface::get_R_Body ()'

   `FGInterface::get_Gamma_vert_rad ()'
   `FGInterface::get_Climb_Rate ()'
   `FGInterface::get_Alpha ()'
   `FGInterface::get_Beta ()'

   `FGInterface::get_Runway_altitude ()'

   `FGInterface::get_Lon_geocentric ()'
   `FGInterface::get_Lat_geocentric ()'
   `FGInterface::get_Sea_level_radius ()'
   `FGInterface::get_Earth_position_angle ()'

   `FGInterface::get_Latitude_dot()'
   `FGInterface::get_Longitude_dot()'
   `FGInterface::get_Radius_dot()'

   `FGInterface::get_Dx_cg ()'
   `FGInterface::get_Dy_cg ()'
   `FGInterface::get_Dz_cg ()'

   `FGInterface::get_T_local_to_body_11 ()' ... `FGInterface::get_T_local_to_body_33 ()'

   `FGInterface::get_Radius_to_vehicle ()'

 */


#include <Include/compiler.h>

#include <math.h>

#include <list>

#include <Time/timestamp.hxx>

FG_USING_STD(list);


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


typedef double FG_VECTOR_3[3];


// This is based heavily on LaRCsim/ls_generic.h
class FGInterface {

public:

    virtual int init( double dt );
    virtual int update( int multi_loop );
    virtual ~FGInterface();

    // Define the various supported flight models (many not yet implemented)
    enum {
	// Slew (in MS terminology)
	FG_SLEW = 0,
	
	// The NASA LaRCsim (Navion) flight model
	FG_LARCSIM = 1,

	// Jon S. Berndt's new FDM written from the ground up in C++
	FG_JSBSIM = 2,

	// The following aren't implemented but are here to spark
	// thoughts and discussions, and maybe even action.
	FG_ACM = 3,
	FG_SUPER_SONIC = 4,
	FG_HELICOPTER = 5,
	FG_AUTOGYRO = 6,
	FG_BALLOON = 7,
	FG_PARACHUTE = 8,

	// Driven externally via a serial port, net, file, etc.
	FG_EXTERNAL = 9
    };

/*================== Mass properties and geometry values ==================*/

    // Inertias
    double mass, i_xx, i_yy, i_zz, i_xz;
    inline double get_Mass() const { return mass; }
    inline double get_I_xx() const { return i_xx; }
    inline double get_I_yy() const { return i_yy; }
    inline double get_I_zz() const { return i_zz; }
    inline double get_I_xz() const { return i_xz; }
    inline void set_Inertias( double m, double xx, double yy, 
			      double zz, double xz)
    {
	mass = m;
	i_xx = xx;
	i_yy = yy;
	i_zz = zz;
	i_xz = xz;
    }
    
    // Pilot location rel to ref pt
    FG_VECTOR_3 d_pilot_rp_body_v;
    // inline double * get_D_pilot_rp_body_v() { 
    //  return d_pilot_rp_body_v; 
    // }
    // inline double get_Dx_pilot() const { return d_pilot_rp_body_v[0]; }
    // inline double get_Dy_pilot() const { return d_pilot_rp_body_v[1]; }
    // inline double get_Dz_pilot() const { return d_pilot_rp_body_v[2]; }
    /* inline void set_Pilot_Location( double dx, double dy, double dz ) {
	d_pilot_rp_body_v[0] = dx;
	d_pilot_rp_body_v[1] = dy;
	d_pilot_rp_body_v[2] = dz;
    } */

    // CG position w.r.t. ref. point
    FG_VECTOR_3 d_cg_rp_body_v;
    // inline double * get_D_cg_rp_body_v() { return d_cg_rp_body_v; }
    inline double get_Dx_cg() const { return d_cg_rp_body_v[0]; }
    inline double get_Dy_cg() const { return d_cg_rp_body_v[1]; }
    inline double get_Dz_cg() const { return d_cg_rp_body_v[2]; }
    inline void set_CG_Position( double dx, double dy, double dz ) {
	d_cg_rp_body_v[0] = dx;
	d_cg_rp_body_v[1] = dy;
	d_cg_rp_body_v[2] = dz;
    }

/*================================ Forces =================================*/

    FG_VECTOR_3 f_body_total_v;
    // inline double * get_F_body_total_v() { return f_body_total_v; }
    // inline double get_F_X() const { return f_body_total_v[0]; }
    // inline double get_F_Y() const { return f_body_total_v[1]; }
    // inline double get_F_Z() const { return f_body_total_v[2]; }
    /* inline void set_Forces_Body_Total( double x, double y, double z ) {
	f_body_total_v[0] = x;
	f_body_total_v[1] = y;
	f_body_total_v[2] = z;
    } */

    FG_VECTOR_3 f_local_total_v;
    // inline double * get_F_local_total_v() { return f_local_total_v; }
    // inline double get_F_north() const { return f_local_total_v[0]; }
    // inline double get_F_east() const { return f_local_total_v[1]; }
    // inline double get_F_down() const { return f_local_total_v[2]; }
    /* inline void set_Forces_Local_Total( double x, double y, double z ) {
	f_local_total_v[0] = x;
	f_local_total_v[1] = y;
	f_local_total_v[2] = z;
    } */

    FG_VECTOR_3 f_aero_v;
    // inline double * get_F_aero_v() { return f_aero_v; }
    // inline double get_F_X_aero() const { return f_aero_v[0]; }
    // inline double get_F_Y_aero() const { return f_aero_v[1]; }
    // inline double get_F_Z_aero() const { return f_aero_v[2]; }
    /* inline void set_Forces_Aero( double x, double y, double z ) {
	f_aero_v[0] = x;
	f_aero_v[1] = y;
	f_aero_v[2] = z;
    } */
    
    FG_VECTOR_3 f_engine_v;
    // inline double * get_F_engine_v() { return f_engine_v; }
    // inline double get_F_X_engine() const { return f_engine_v[0]; }
    // inline double get_F_Y_engine() const { return f_engine_v[1]; }
    // inline double get_F_Z_engine() const { return f_engine_v[2]; }
    /* inline void set_Forces_Engine( double x, double y, double z ) {
	f_engine_v[0] = x;
	f_engine_v[1] = y;
	f_engine_v[2] = z;
    } */

    FG_VECTOR_3 f_gear_v;
    // inline double * get_F_gear_v() { return f_gear_v; }
    // inline double get_F_X_gear() const { return f_gear_v[0]; }
    // inline double get_F_Y_gear() const { return f_gear_v[1]; }
    // inline double get_F_Z_gear() const { return f_gear_v[2]; }
    /* inline void set_Forces_Gear( double x, double y, double z ) {
	f_gear_v[0] = x;
	f_gear_v[1] = y;
	f_gear_v[2] = z;
    } */

    /*================================ Moments ================================*/

    FG_VECTOR_3 m_total_rp_v;
    // inline double * get_M_total_rp_v() { return m_total_rp_v; }
    // inline double get_M_l_rp() const { return m_total_rp_v[0]; }
    // inline double get_M_m_rp() const { return m_total_rp_v[1]; }
    // inline double get_M_n_rp() const { return m_total_rp_v[2]; }
    /* inline void set_Moments_Total_RP( double l, double m, double n ) {
	m_total_rp_v[0] = l;
	m_total_rp_v[1] = m;
	m_total_rp_v[2] = n;
    } */

    FG_VECTOR_3 m_total_cg_v;
    // inline double * get_M_total_cg_v() { return m_total_cg_v; }
    // inline double get_M_l_cg() const { return m_total_cg_v[0]; }
    // inline double get_M_m_cg() const { return m_total_cg_v[1]; }
    // inline double get_M_n_cg() const { return m_total_cg_v[2]; }
    /* inline void set_Moments_Total_CG( double l, double m, double n ) {
	m_total_cg_v[0] = l;
	m_total_cg_v[1] = m;
	m_total_cg_v[2] = n;
    } */

    FG_VECTOR_3 m_aero_v;
    // inline double * get_M_aero_v() { return m_aero_v; }
    // inline double get_M_l_aero() const { return m_aero_v[0]; }
    // inline double get_M_m_aero() const { return m_aero_v[1]; }
    // inline double get_M_n_aero() const { return m_aero_v[2]; }
    /* inline void set_Moments_Aero( double l, double m, double n ) {
	m_aero_v[0] = l;
	m_aero_v[1] = m;
	m_aero_v[2] = n;
    } */

    FG_VECTOR_3    m_engine_v;
    // inline double * get_M_engine_v() { return m_engine_v; }
    // inline double get_M_l_engine() const { return m_engine_v[0]; }
    // inline double get_M_m_engine() const { return m_engine_v[1]; }
    // inline double get_M_n_engine() const { return m_engine_v[2]; }
    /* inline void set_Moments_Engine( double l, double m, double n ) {
	m_engine_v[0] = l;
	m_engine_v[1] = m;
	m_engine_v[2] = n;
    } */

    FG_VECTOR_3    m_gear_v;
    // inline double * get_M_gear_v() { return m_gear_v; }
    // inline double get_M_l_gear() const { return m_gear_v[0]; }
    // inline double get_M_m_gear() const { return m_gear_v[1]; }
    // inline double get_M_n_gear() const { return m_gear_v[2]; }
    /* inline void set_Moments_Gear( double l, double m, double n ) {
	m_gear_v[0] = l;
	m_gear_v[1] = m;
	m_gear_v[2] = n;
    } */

    /*============================== Accelerations ============================*/

    FG_VECTOR_3    v_dot_local_v;
    // inline double * get_V_dot_local_v() { return v_dot_local_v; }
    // inline double get_V_dot_north() const { return v_dot_local_v[0]; }
    // inline double get_V_dot_east() const { return v_dot_local_v[1]; }
    // inline double get_V_dot_down() const { return v_dot_local_v[2]; }
    /* inline void set_Accels_Local( double north, double east, double down ) {
	v_dot_local_v[0] = north;
	v_dot_local_v[1] = east;
	v_dot_local_v[2] = down;
    } */

    FG_VECTOR_3    v_dot_body_v;
    // inline double * get_V_dot_body_v() { return v_dot_body_v; }
    inline double get_U_dot_body() const { return v_dot_body_v[0]; }
    inline double get_V_dot_body() const { return v_dot_body_v[1]; }
    inline double get_W_dot_body() const { return v_dot_body_v[2]; }
    inline void set_Accels_Body( double u, double v, double w ) {
	v_dot_body_v[0] = u;
	v_dot_body_v[1] = v;
	v_dot_body_v[2] = w;
    }

    FG_VECTOR_3    a_cg_body_v;
    // inline double * get_A_cg_body_v() { return a_cg_body_v; }
    inline double get_A_X_cg() const { return a_cg_body_v[0]; }
    inline double get_A_Y_cg() const { return a_cg_body_v[1]; }
    inline double get_A_Z_cg() const { return a_cg_body_v[2]; }
    inline void set_Accels_CG_Body( double x, double y, double z ) {
	a_cg_body_v[0] = x;
	a_cg_body_v[1] = y;
	a_cg_body_v[2] = z;
    }

    FG_VECTOR_3    a_pilot_body_v;
    // inline double * get_A_pilot_body_v() { return a_pilot_body_v; }
    inline double get_A_X_pilot() const { return a_pilot_body_v[0]; }
    inline double get_A_Y_pilot() const { return a_pilot_body_v[1]; }
    inline double get_A_Z_pilot() const { return a_pilot_body_v[2]; }
    inline void set_Accels_Pilot_Body( double x, double y, double z ) {
	a_pilot_body_v[0] = x;
	a_pilot_body_v[1] = y;
	a_pilot_body_v[2] = z;
    }

    FG_VECTOR_3    n_cg_body_v;
    // inline double * get_N_cg_body_v() { return n_cg_body_v; }
    // inline double get_N_X_cg() const { return n_cg_body_v[0]; }
    // inline double get_N_Y_cg() const { return n_cg_body_v[1]; }
    // inline double get_N_Z_cg() const { return n_cg_body_v[2]; }
    /* inline void set_Accels_CG_Body_N( double x, double y, double z ) {
	n_cg_body_v[0] = x;
	n_cg_body_v[1] = y;
	n_cg_body_v[2] = z;
    } */

    FG_VECTOR_3    n_pilot_body_v;
    // inline double * get_N_pilot_body_v() { return n_pilot_body_v; }
    // inline double get_N_X_pilot() const { return n_pilot_body_v[0]; }
    // inline double get_N_Y_pilot() const { return n_pilot_body_v[1]; }
    // inline double get_N_Z_pilot() const { return n_pilot_body_v[2]; }
    /* inline void set_Accels_Pilot_Body_N( double x, double y, double z ) {
	n_pilot_body_v[0] = x;
	n_pilot_body_v[1] = y;
	n_pilot_body_v[2] = z;
    } */

    FG_VECTOR_3    omega_dot_body_v;
    // inline double * get_Omega_dot_body_v() { return omega_dot_body_v; }
    // inline double get_P_dot_body() const { return omega_dot_body_v[0]; }
    // inline double get_Q_dot_body() const { return omega_dot_body_v[1]; }
    // inline double get_R_dot_body() const { return omega_dot_body_v[2]; }
    /* inline void set_Accels_Omega( double p, double q, double r ) {
	omega_dot_body_v[0] = p;
	omega_dot_body_v[1] = q;
	omega_dot_body_v[2] = r;
    } */


    /*============================== Velocities ===============================*/

    FG_VECTOR_3    v_local_v;
    // inline double * get_V_local_v() { return v_local_v; }
    inline double get_V_north() const { return v_local_v[0]; }
    inline double get_V_east() const { return v_local_v[1]; }
    inline double get_V_down() const { return v_local_v[2]; }
    inline void set_Velocities_Local( double north, double east, double down ) {
	v_local_v[0] = north;
	v_local_v[1] = east;
	v_local_v[2] = down;
    }

    FG_VECTOR_3    v_local_rel_ground_v; // V rel w.r.t. earth surface  
    // inline double * get_V_local_rel_ground_v() { return v_local_rel_ground_v; }
    // inline double get_V_north_rel_ground() const {
    //	return v_local_rel_ground_v[0];
    //    }
    // inline double get_V_east_rel_ground() const {
    //	return v_local_rel_ground_v[1];
    //    }
    // inline double get_V_down_rel_ground() const {
    //	return v_local_rel_ground_v[2];
    //    }
    /* inline void set_Velocities_Ground(double north, double east, double down) {
	v_local_rel_ground_v[0] = north;
	v_local_rel_ground_v[1] = east;
	v_local_rel_ground_v[2] = down;
    } */

    FG_VECTOR_3    v_local_airmass_v;   // velocity of airmass (steady winds)
    // inline double * get_V_local_airmass_v() { return v_local_airmass_v; }
    // inline double get_V_north_airmass() const { return v_local_airmass_v[0]; }
    // inline double get_V_east_airmass() const { return v_local_airmass_v[1]; }
    // inline double get_V_down_airmass() const { return v_local_airmass_v[2]; }
    /* inline void set_Velocities_Local_Airmass( double north, double east, 
					      double down)
    {
	v_local_airmass_v[0] = north;
	v_local_airmass_v[1] = east;
	v_local_airmass_v[2] = down;
    } */

    FG_VECTOR_3    v_local_rel_airmass_v;  // velocity of veh. relative to
    // airmass
    // inline double * get_V_local_rel_airmass_v() {
	//return v_local_rel_airmass_v;
    //}
    // inline double get_V_north_rel_airmass() const {
	//return v_local_rel_airmass_v[0];
    //}
    // inline double get_V_east_rel_airmass() const {
	//return v_local_rel_airmass_v[1];
    //}
    // inline double get_V_down_rel_airmass() const {
	//return v_local_rel_airmass_v[2];
    //}
    /* inline void set_Velocities_Local_Rel_Airmass( double north, double east, 
						  double down)
    {
	v_local_rel_airmass_v[0] = north;
	v_local_rel_airmass_v[1] = east;
	v_local_rel_airmass_v[2] = down;
    } */

    FG_VECTOR_3    v_local_gust_v; // linear turbulence components, L frame
    // inline double * get_V_local_gust_v() { return v_local_gust_v; }
    // inline double get_U_gust() const { return v_local_gust_v[0]; }
    // inline double get_V_gust() const { return v_local_gust_v[1]; }
    // inline double get_W_gust() const { return v_local_gust_v[2]; }
    /* inline void set_Velocities_Gust( double u, double v, double w)
    {
	v_local_gust_v[0] = u;
	v_local_gust_v[1] = v;
	v_local_gust_v[2] = w;
    } */
    
    FG_VECTOR_3    v_wind_body_v;  // Wind-relative velocities in body axis
    // inline double * get_V_wind_body_v() { return v_wind_body_v; }
    inline double get_U_body() const { return v_wind_body_v[0]; }
    inline double get_V_body() const { return v_wind_body_v[1]; }
    inline double get_W_body() const { return v_wind_body_v[2]; }
    inline void set_Velocities_Wind_Body( double u, double v, double w)
    {
	v_wind_body_v[0] = u;
	v_wind_body_v[1] = v;
	v_wind_body_v[2] = w;
    }

    double    v_rel_wind, v_true_kts, v_rel_ground, v_inertial;
    double    v_ground_speed, v_equiv, v_equiv_kts;
    double    v_calibrated, v_calibrated_kts;

    // inline double get_V_rel_wind() const { return v_rel_wind; }
    // inline void set_V_rel_wind(double wind) { v_rel_wind = wind; }

    // inline double get_V_true_kts() const { return v_true_kts; }
    // inline void set_V_true_kts(double kts) { v_true_kts = kts; }

    // inline double get_V_rel_ground() const { return v_rel_ground; }
    // inline void set_V_rel_ground( double v ) { v_rel_ground = v; }

    // inline double get_V_inertial() const { return v_inertial; }
    // inline void set_V_inertial(double v) { v_inertial = v; }

    inline double get_V_ground_speed() const { return v_ground_speed; }
    inline void set_V_ground_speed( double v) { v_ground_speed = v; }

    // inline double get_V_equiv() const { return v_equiv; }
    // inline void set_V_equiv( double v ) { v_equiv = v; }

    inline double get_V_equiv_kts() const { return v_equiv_kts; }
    inline void set_V_equiv_kts( double kts ) { v_equiv_kts = kts; }

    // inline double get_V_calibrated() const { return v_calibrated; }
    // inline void set_V_calibrated( double v ) { v_calibrated = v; }

    // inline double get_V_calibrated_kts() const { return v_calibrated_kts; }
    // inline void set_V_calibrated_kts( double kts ) { v_calibrated_kts = kts; }

    FG_VECTOR_3    omega_body_v;   // Angular B rates     
    // inline double * get_Omega_body_v() { return omega_body_v; }
    inline double get_P_body() const { return omega_body_v[0]; }
    inline double get_Q_body() const { return omega_body_v[1]; }
    inline double get_R_body() const { return omega_body_v[2]; }
    inline void set_Omega_Body( double p, double q, double r ) {
	omega_body_v[0] = p;
	omega_body_v[1] = q;
	omega_body_v[2] = r;
    }

    FG_VECTOR_3    omega_local_v;  // Angular L rates     
    // inline double * get_Omega_local_v() { return omega_local_v; }
    // inline double get_P_local() const { return omega_local_v[0]; }
    // inline double get_Q_local() const { return omega_local_v[1]; }
    // inline double get_R_local() const { return omega_local_v[2]; }
    /* inline void set_Omega_Local( double p, double q, double r ) {
	omega_local_v[0] = p;
	omega_local_v[1] = q;
	omega_local_v[2] = r;
    } */

    FG_VECTOR_3    omega_total_v;  // Diff btw B & L      
    // inline double * get_Omega_total_v() { return omega_total_v; }
    // inline double get_P_total() const { return omega_total_v[0]; }
    // inline double get_Q_total() const { return omega_total_v[1]; }
    // inline double get_R_total() const { return omega_total_v[2]; }
    /* inline void set_Omega_Total( double p, double q, double r ) {
	omega_total_v[0] = p;
	omega_total_v[1] = q;
	omega_total_v[2] = r;
    } */

    FG_VECTOR_3    euler_rates_v;
    // inline double * get_Euler_rates_v() { return euler_rates_v; }
    // inline double get_Phi_dot() const { return euler_rates_v[0]; }
    // inline double get_Theta_dot() const { return euler_rates_v[1]; }
    // inline double get_Psi_dot() const { return euler_rates_v[2]; }
    /* inline void set_Euler_Rates( double phi, double theta, double psi ) {
	euler_rates_v[0] = phi;
	euler_rates_v[1] = theta;
	euler_rates_v[2] = psi;
    } */

    FG_VECTOR_3    geocentric_rates_v;     // Geocentric linear velocities
    // inline double * get_Geocentric_rates_v() { return geocentric_rates_v; }
    inline double get_Latitude_dot() const { return geocentric_rates_v[0]; }
    inline double get_Longitude_dot() const { return geocentric_rates_v[1]; }
    inline double get_Radius_dot() const { return geocentric_rates_v[2]; }
    inline void set_Geocentric_Rates( double lat, double lon, double rad ) {
	geocentric_rates_v[0] = lat;
	geocentric_rates_v[1] = lon;
	geocentric_rates_v[2] = rad;
    }
    
    /*=============================== Positions ===============================*/

    FG_VECTOR_3    geocentric_position_v;
    // inline double * get_Geocentric_position_v() {
    //    return geocentric_position_v;
    // }
    inline double get_Lat_geocentric() const {
	return geocentric_position_v[0];
    }
    inline double get_Lon_geocentric() const { 
	return geocentric_position_v[1];
    }
    inline double get_Radius_to_vehicle() const {
	return geocentric_position_v[2];
    }
    inline void set_Radius_to_vehicle(double radius) {
	geocentric_position_v[2] = radius;
    }

    inline void set_Geocentric_Position( double lat, double lon, double rad ) {
	geocentric_position_v[0] = lat;
	geocentric_position_v[1] = lon;
	geocentric_position_v[2] = rad;
    }

    FG_VECTOR_3    geodetic_position_v;
    // inline double * get_Geodetic_position_v() { return geodetic_position_v; }
    inline double get_Latitude() const { return geodetic_position_v[0]; }
    inline void set_Latitude(double lat) { geodetic_position_v[0] = lat; }
    inline double get_Longitude() const { return geodetic_position_v[1]; }
    inline void set_Longitude(double lon) { geodetic_position_v[1] = lon; }
    inline double get_Altitude() const { return geodetic_position_v[2]; }
    inline void set_Altitude(double altitude) {
	geodetic_position_v[2] = altitude;
    }
    inline void set_Geodetic_Position( double lat, double lon, double alt ) {
	geodetic_position_v[0] = lat;
	geodetic_position_v[1] = lon;
	geodetic_position_v[2] = alt;
    }

    FG_VECTOR_3 euler_angles_v;
    // inline double * get_Euler_angles_v() { return euler_angles_v; }
    inline double get_Phi() const { return euler_angles_v[0]; }
    inline double get_Theta() const { return euler_angles_v[1]; }
    inline double get_Psi() const { return euler_angles_v[2]; }
    inline void set_Euler_Angles( double phi, double theta, double psi ) {
	euler_angles_v[0] = phi;
	euler_angles_v[1] = theta;
	euler_angles_v[2] = psi;
    }


    /*======================= Miscellaneous quantities ========================*/

    double    t_local_to_body_m[3][3];    // Transformation matrix L to B
    // inline double * get_T_local_to_body_m() { return t_local_to_body_m; }
    inline double get_T_local_to_body_11() const {
	return t_local_to_body_m[0][0];
    }
    inline double get_T_local_to_body_12() const {
	return t_local_to_body_m[0][1];
    }
    inline double get_T_local_to_body_13() const {
	return t_local_to_body_m[0][2];
    }
    inline double get_T_local_to_body_21() const {
	return t_local_to_body_m[1][0];
    }
    inline double get_T_local_to_body_22() const {
	return t_local_to_body_m[1][1];
    }
    inline double get_T_local_to_body_23() const {
	return t_local_to_body_m[1][2];
    }
    inline double get_T_local_to_body_31() const {
	return t_local_to_body_m[2][0];
    }
    inline double get_T_local_to_body_32() const {
	return t_local_to_body_m[2][1];
    }
    inline double get_T_local_to_body_33() const {
	return t_local_to_body_m[2][2];
    }
    inline void set_T_Local_to_Body( double m[3][3] ) {
	int i, j;
	for ( i = 0; i < 3; i++ ) {
	    for ( j = 0; j < 3; j++ ) {
		t_local_to_body_m[i][j] = m[i][j];
	    }
	}
    }

    double    gravity;            // Local acceleration due to G 
    // inline double get_Gravity() const { return gravity; }
    // inline void set_Gravity(double g) { gravity = g; }
    
    double    centrifugal_relief; // load factor reduction due to speed
    // inline double get_Centrifugal_relief() const { return centrifugal_relief; }
    // inline void set_Centrifugal_relief(double cr) { centrifugal_relief = cr; }

    double    alpha, beta, alpha_dot, beta_dot;   // in radians  
    inline double get_Alpha() const { return alpha; }
    inline void set_Alpha( double a ) { alpha = a; }
    inline double get_Beta() const { return beta; }
    inline void set_Beta( double b ) { beta = b; }
    // inline double get_Alpha_dot() const { return alpha_dot; }
    // inline void set_Alpha_dot( double ad ) { alpha_dot = ad; }
    // inline double get_Beta_dot() const { return beta_dot; }
    // inline void set_Beta_dot( double bd ) { beta_dot = bd; }

    double    cos_alpha, sin_alpha, cos_beta, sin_beta;
    // inline double get_Cos_alpha() const { return cos_alpha; }
    // inline void set_Cos_alpha( double ca ) { cos_alpha = ca; }
    // inline double get_Sin_alpha() const { return sin_alpha; }
    // inline void set_Sin_alpha( double sa ) { sin_alpha = sa; }
    // inline double get_Cos_beta() const { return cos_beta; }
    // inline void set_Cos_beta( double cb ) { cos_beta = cb; }
    // inline double get_Sin_beta() const { return sin_beta; }
    // inline void set_Sin_beta( double sb ) { sin_beta = sb; }

    double    cos_phi, sin_phi, cos_theta, sin_theta, cos_psi, sin_psi;
    inline double get_Cos_phi() const { return cos_phi; }
    inline void set_Cos_phi( double cp ) { cos_phi = cp; }
    // inline double get_Sin_phi() const { return sin_phi; }
    // inline void set_Sin_phi( double sp ) { sin_phi = sp; }
    inline double get_Cos_theta() const { return cos_theta; }
    inline void set_Cos_theta( double ct ) { cos_theta = ct; }
    // inline double get_Sin_theta() const { return sin_theta; }
    // inline void set_Sin_theta( double st ) { sin_theta = st; }
    // inline double get_Cos_psi() const { return cos_psi; }
    // inline void set_Cos_psi( double cp ) { cos_psi = cp; }
    // inline double get_Sin_psi() const { return sin_psi; }
    // inline void set_Sin_psi( double sp ) { sin_psi = sp; }

    double    gamma_vert_rad, gamma_horiz_rad;    // Flight path angles  
    inline double get_Gamma_vert_rad() const { return gamma_vert_rad; }
    inline void set_Gamma_vert_rad( double gv ) { gamma_vert_rad = gv; }
    // inline double get_Gamma_horiz_rad() const { return gamma_horiz_rad; }
    // inline void set_Gamma_horiz_rad( double gh ) { gamma_horiz_rad = gh; }

    double    sigma, density, v_sound, mach_number;
    // inline double get_Sigma() const { return sigma; }
    // inline void set_Sigma( double s ) { sigma = s; }
    // inline double get_Density() const { return density; }
    // inline void set_Density( double d ) { density = d; }
    // inline double get_V_sound() const { return v_sound; }
    // inline void set_V_sound( double v ) { v_sound = v; }
    // inline double get_Mach_number() const { return mach_number; }
    // inline void set_Mach_number( double m ) { mach_number = m; }

    double    static_pressure, total_pressure, impact_pressure;
    double    dynamic_pressure;
    // inline double get_Static_pressure() const { return static_pressure; }
    // inline void set_Static_pressure( double sp ) { static_pressure = sp; }
    // inline double get_Total_pressure() const { return total_pressure; }
    // inline void set_Total_pressure( double tp ) { total_pressure = tp; }
    // inline double get_Impact_pressure() const { return impact_pressure; }
    // inline void set_Impact_pressure( double ip ) { impact_pressure = ip; }
    // inline double get_Dynamic_pressure() const { return dynamic_pressure; }
    // inline void set_Dynamic_pressure( double dp ) { dynamic_pressure = dp; }

    double    static_temperature, total_temperature;
    // inline double get_Static_temperature() const { return static_temperature; }
    // inline void set_Static_temperature( double t ) { static_temperature = t; }
    // inline double get_Total_temperature() const { return total_temperature; }
    // inline void set_Total_temperature( double t ) { total_temperature = t; }

    double    sea_level_radius, earth_position_angle;
    inline double get_Sea_level_radius() const { return sea_level_radius; }
    inline void set_Sea_level_radius( double r ) { sea_level_radius = r; }
    inline double get_Earth_position_angle() const {
	return earth_position_angle;
    }
    inline void set_Earth_position_angle(double a) { 
	earth_position_angle = a;
    }

    double    runway_altitude, runway_latitude, runway_longitude;
    double    runway_heading;
    inline double get_Runway_altitude() const { return runway_altitude; }
    inline void set_Runway_altitude( double alt ) { runway_altitude = alt; }
    // inline double get_Runway_latitude() const { return runway_latitude; }
    // inline void set_Runway_latitude( double lat ) { runway_latitude = lat; }
    // inline double get_Runway_longitude() const { return runway_longitude; }
    // inline void set_Runway_longitude( double lon ) { runway_longitude = lon; }
    // inline double get_Runway_heading() const { return runway_heading; }
    // inline void set_Runway_heading( double h ) { runway_heading = h; }

    double    radius_to_rwy;
    // inline double get_Radius_to_rwy() const { return radius_to_rwy; }
    // inline void set_Radius_to_rwy( double r ) { radius_to_rwy = r; }

    FG_VECTOR_3    d_cg_rwy_local_v;       // CG rel. to rwy in local coords
    // inline double * get_D_cg_rwy_local_v() { return d_cg_rwy_local_v; }
    // inline double get_D_cg_north_of_rwy() const { return d_cg_rwy_local_v[0]; }
    // inline double get_D_cg_east_of_rwy() const { return d_cg_rwy_local_v[1]; }
    // inline double get_D_cg_above_rwy() const { return d_cg_rwy_local_v[2]; }
    /* inline void set_CG_Rwy_Local( double north, double east, double above )
    {
	d_cg_rwy_local_v[0] = north;
	d_cg_rwy_local_v[1] = east;
	d_cg_rwy_local_v[2] = above;
    } */

    FG_VECTOR_3    d_cg_rwy_rwy_v; // CG relative to rwy, in rwy coordinates
    // inline double * get_D_cg_rwy_rwy_v() { return d_cg_rwy_rwy_v; }
    // inline double get_X_cg_rwy() const { return d_cg_rwy_rwy_v[0]; }
    // inline double get_Y_cg_rwy() const { return d_cg_rwy_rwy_v[1]; }
    // inline double get_H_cg_rwy() const { return d_cg_rwy_rwy_v[2]; }
    /* inline void set_CG_Rwy_Rwy( double x, double y, double h )
    {
	d_cg_rwy_rwy_v[0] = x;
	d_cg_rwy_rwy_v[1] = y;
	d_cg_rwy_rwy_v[2] = h;
    } */

    FG_VECTOR_3    d_pilot_rwy_local_v;  // pilot rel. to rwy in local coords
    // inline double * get_D_pilot_rwy_local_v() { return d_pilot_rwy_local_v; }
    // inline double get_D_pilot_north_of_rwy() const {
	//return d_pilot_rwy_local_v[0];
  //  }
    // inline double get_D_pilot_east_of_rwy() const {
//	return d_pilot_rwy_local_v[1];
//    }
    // inline double get_D_pilot_above_rwy() const {
	//return d_pilot_rwy_local_v[2];
 //   }
    /* inline void set_Pilot_Rwy_Local( double north, double east, double above )
    {
	d_pilot_rwy_local_v[0] = north;
	d_pilot_rwy_local_v[1] = east;
	d_pilot_rwy_local_v[2] = above;
    } */

    FG_VECTOR_3   d_pilot_rwy_rwy_v;   // pilot rel. to rwy, in rwy coords.
    // inline double * get_D_pilot_rwy_rwy_v() { return d_pilot_rwy_rwy_v; }
    // inline double get_X_pilot_rwy() const { return d_pilot_rwy_rwy_v[0]; }
    // inline double get_Y_pilot_rwy() const { return d_pilot_rwy_rwy_v[1]; }
    // inline double get_H_pilot_rwy() const { return d_pilot_rwy_rwy_v[2]; }
    /* inline void set_Pilot_Rwy_Rwy( double x, double y, double h )
    {
	d_pilot_rwy_rwy_v[0] = x;
	d_pilot_rwy_rwy_v[1] = y;
	d_pilot_rwy_rwy_v[2] = h;
    } */

    double        climb_rate;           // in feet per second
    inline double get_Climb_Rate() const { return climb_rate; }
    inline void set_Climb_Rate(double rate) { climb_rate = rate; }

    FGTimeStamp valid_stamp;       // time this record is valid
    FGTimeStamp next_stamp;       // time this record is valid
    inline FGTimeStamp get_time_stamp() const { return valid_stamp; }
    inline void stamp_time() { valid_stamp = next_stamp; next_stamp.stamp(); }

    // Extrapolate FDM based on time_offset (in usec)
    void extrapolate( int time_offset );

    // sin/cos lat_geocentric
    double sin_lat_geocentric;
    double cos_lat_geocentric;
    inline void set_sin_lat_geocentric(double parm) {
	sin_lat_geocentric = sin(parm);
    }
    inline void set_cos_lat_geocentric(double parm) {
	cos_lat_geocentric = cos(parm);
    }
    inline double get_sin_lat_geocentric(void) const {
	return sin_lat_geocentric;
    }
    inline double get_cos_lat_geocentric(void) const {
	return cos_lat_geocentric;
    }

    double sin_longitude;
    double cos_longitude;
    inline void set_sin_cos_longitude(double parm) {
	sin_longitude = sin(parm);
	cos_longitude = cos(parm);
    }
    inline double get_sin_longitude(void) const {
	return sin_longitude;
    }
    inline double get_cos_longitude(void) const {
	return cos_longitude;
    }
	
    double sin_latitude;
    double cos_latitude;
    inline void set_sin_cos_latitude(double parm) {
	sin_latitude = sin(parm);
	cos_latitude = cos(parm);
    }
    inline double get_sin_latitude(void) const {
	return sin_latitude;
    }
    inline double get_cos_latitude(void) const {
	return cos_latitude;
    }
};


typedef list < FGInterface > fdm_state_list;
typedef fdm_state_list::iterator fdm_state_list_iterator;
typedef fdm_state_list::const_iterator const_fdm_state_list_iterator;


extern FGInterface * cur_fdm_state;


// General interface to the flight model routines

// Initialize the flight model parameters
int fgFDMInit(int model, FGInterface& f, double dt);

// Run multiloop iterations of the flight model
int fgFDMUpdate(int model, FGInterface& f, int multiloop, int jitter);

// Set the altitude (force)
void fgFDMForceAltitude(int model, double alt_meters);

// Set the local ground elevation
void fgFDMSetGroundElevation(int model, double alt_meters);


#endif // _FLIGHT_HXX


