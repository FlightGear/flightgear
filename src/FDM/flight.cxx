// flight.c -- a general interface to the various flight models
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


#include <stdio.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <FDM/LaRCsim/ls_interface.h>
#include <Main/globals.hxx>
#include <Time/timestamp.hxx>

#include "External.hxx"
#include "flight.hxx"
#include "JSBSim.hxx"
#include "LaRCsim.hxx"
#include "Balloon.h"


// base_fdm_state is the internal state that is updated in integer
// multiples of "dt".  This leads to "jitter" with respect to the real
// world time, so we introduce cur_fdm_state which is extrapolated by
// the difference between sim time and real world time

FGInterface *cur_fdm_state;
FGInterface base_fdm_state;

inline void init_vec(FG_VECTOR_3 vec) {
    vec[0] = 0.0; vec[1] = 0.0; vec[2] = 0.0;
}  

FGEngInterface::FGEngInterface(void) {
    
    // inputs
    Throttle=0;
    Mixture=0;
    Prop_Advance=0;

    // outputs
    RPM=0;
    Manifold_Pressure=0;
    MaxHP=0;
    Percentage_Power=0;
    EGT=0;
    prop_thrust=0;
}

FGEngInterface::~FGEngInterface(void) {
}


// Constructor
FGInterface::FGInterface(void) {
    init_vec( d_pilot_rp_body_v );
    init_vec( d_cg_rp_body_v );
    init_vec( f_body_total_v );
    init_vec( f_local_total_v );
    init_vec( f_aero_v );
    init_vec( f_engine_v );
    init_vec( f_gear_v );
    init_vec( m_total_rp_v );
    init_vec( m_total_cg_v );
    init_vec( m_aero_v );
    init_vec( m_engine_v );
    init_vec( m_gear_v );
    init_vec( v_dot_local_v );
    init_vec( v_dot_body_v );
    init_vec( a_cg_body_v );
    init_vec( a_pilot_body_v );
    init_vec( n_cg_body_v );
    init_vec( n_pilot_body_v );
    init_vec( omega_dot_body_v );
    init_vec( v_local_v );
    init_vec( v_local_rel_ground_v ); 
    init_vec( v_local_airmass_v );    
    init_vec( v_local_rel_airmass_v );  
    init_vec( v_local_gust_v ); 
    init_vec( v_wind_body_v );     
    init_vec( omega_body_v );         
    init_vec( omega_local_v );        
    init_vec( omega_total_v );       
    init_vec( euler_rates_v );
    init_vec( geocentric_rates_v );   
    init_vec( geocentric_position_v );
    init_vec( geodetic_position_v );
    init_vec( euler_angles_v );
    init_vec( d_cg_rwy_local_v );     
    init_vec( d_cg_rwy_rwy_v );       
    init_vec( d_pilot_rwy_local_v );  
    init_vec( d_pilot_rwy_rwy_v );    
    init_vec( t_local_to_body_m[0] );
    init_vec( t_local_to_body_m[1] );
    init_vec( t_local_to_body_m[2] );
    
    mass=i_xx=i_yy=i_zz=i_xz=0;
    nlf=0;  
    v_rel_wind=v_true_kts=v_rel_ground=v_inertial=0;
    v_ground_speed=v_equiv=v_equiv_kts=0;
    v_calibrated=v_calibrated_kts=0;
    gravity=0;            
    centrifugal_relief=0; 
    alpha=beta=alpha_dot=beta_dot=0;   
    cos_alpha=sin_alpha=cos_beta=sin_beta=0;
    cos_phi=sin_phi=cos_theta=sin_theta=cos_psi=sin_psi=0;
    gamma_vert_rad=gamma_horiz_rad=0;    
    sigma=density=v_sound=mach_number=0;
    static_pressure=total_pressure=impact_pressure=0;
    dynamic_pressure=0;
    static_temperature=total_temperature=0;
    sea_level_radius=earth_position_angle=0;
    runway_altitude=runway_latitude=runway_longitude=0;
    runway_heading=0;
    radius_to_rwy=0;
    climb_rate=0;           
    sin_lat_geocentric=cos_lat_geocentric=0;
    sin_latitude=cos_latitude=0;
    sin_longitude=cos_longitude=0;
    altitude_agl=0;

    resetNeeded=false;
}  


// Destructor
FGInterface::~FGInterface() {
}


bool FGInterface::init( double dt ) {
    cout << "dummy init() ... SHOULDN'T BE CALLED!" << endl;
    return false;
}


bool FGInterface::update( int multi_loop ) {
    cout << "dummy update() ... SHOULDN'T BE CALLED!" << endl;
    return false;
}


// Extrapolate fdm based on time_offset (in usec)
void FGInterface::extrapolate( int time_offset ) {
    double dt = time_offset / 1000000.0;

    // -dw- metrowerks complains about ambiguous access, not critical
    // to keep this ;)
#ifndef __MWERKS__
    cout << "extrapolating FDM by dt = " << dt << endl;
#endif

    double lat = geodetic_position_v[0] + geocentric_rates_v[0] * dt;
    double lat_geoc = geocentric_position_v[0] + geocentric_rates_v[0] * dt;

    double lon = geodetic_position_v[1] + geocentric_rates_v[1] * dt;
    double lon_geoc = geocentric_position_v[1] + geocentric_rates_v[1] * dt;

    double alt = geodetic_position_v[2] + geocentric_rates_v[2] * dt;
    double radius = geocentric_position_v[2] + geocentric_rates_v[2] * dt;

    geodetic_position_v[0] = lat;
    geocentric_position_v[0] = lat_geoc;

    geodetic_position_v[1] = lon;
    geocentric_position_v[1] = lon_geoc;

    geodetic_position_v[2] = alt;
    geocentric_position_v[2] = radius;
}


// Set the altitude (force)
void fgFDMForceAltitude(int model, double alt_meters) {
    double sea_level_radius_meters;
    double lat_geoc;

    // Set the FG variables first
    sgGeodToGeoc( base_fdm_state.get_Latitude(), alt_meters, 
		  &sea_level_radius_meters, &lat_geoc);

    base_fdm_state.set_Altitude( alt_meters * METER_TO_FEET );
    base_fdm_state.set_Sea_level_radius( sea_level_radius_meters *
					 METER_TO_FEET ); 
					  

    // additional work needed for some flight models
    if ( model == FGInterface::FG_LARCSIM ) {
	ls_ForceAltitude( base_fdm_state.get_Altitude() );
    }
}


// Set the local ground elevation
void fgFDMSetGroundElevation(int model, double ground_meters) {
    FG_LOG( FG_FLIGHT,FG_INFO, "fgFDMSetGroundElevation: "
	    << ground_meters*METER_TO_FEET ); 
    base_fdm_state.set_Runway_altitude( ground_meters * METER_TO_FEET );
    cur_fdm_state->set_Runway_altitude( ground_meters * METER_TO_FEET );
}


// Positions
void FGInterface::set_Latitude(double lat) { 
    geodetic_position_v[0] = lat;
}  

void FGInterface::set_Longitude(double lon) {
    geodetic_position_v[1] = lon;
}       

void FGInterface::set_Altitude(double alt) {
    geodetic_position_v[2] = alt;
}          

void FGInterface::set_AltitudeAGL(double altagl) {
    altitude_agl=altagl;
}  

// Velocities
void FGInterface::set_V_calibrated_kts(double vc) {
    v_calibrated_kts = vc;
}  

void FGInterface::set_Mach_number(double mach) {
    mach_number = mach;
}  

void FGInterface::set_Velocities_Local( double north, 
					double east, 
					double down ){
    v_local_v[0] = north;
    v_local_v[1] = east;
    v_local_v[2] = down;                                                 
}  

void FGInterface::set_Velocities_Wind_Body( double u, 
					    double v, 
					    double w){
    v_wind_body_v[0] = u;
    v_wind_body_v[1] = v;
    v_wind_body_v[2] = w;
} 

// Euler angles 
void FGInterface::set_Euler_Angles( double phi, 
				    double theta, 
				    double psi ) {
    euler_angles_v[0] = phi;
    euler_angles_v[1] = theta;
    euler_angles_v[2] = psi;                                            
}  

// Flight Path
void FGInterface::set_Climb_Rate( double roc) {
    climb_rate = roc;
}  

void FGInterface::set_Gamma_vert_rad( double gamma) {
    gamma_vert_rad = gamma;
}  

// Earth
void FGInterface::set_Sea_level_radius(double slr) {
    sea_level_radius = slr;
}  

void FGInterface::set_Runway_altitude(double ralt) {
    runway_altitude = ralt;
}  

void FGInterface::set_Static_pressure(double p) { static_pressure = p; }
void FGInterface::set_Static_temperature(double T) { static_temperature = T; }
void FGInterface::set_Density(double rho) { density = rho; }

void FGInterface::set_Velocities_Local_Airmass (double wnorth, 
						double weast, 
						double wdown ) {
    v_local_airmass_v[0] = wnorth;
    v_local_airmass_v[1] = weast;
    v_local_airmass_v[2] = wdown;
}     


void FGInterface::busdump(void) {

    FG_LOG(FG_FLIGHT,FG_INFO,"d_pilot_rp_body_v[3]: " << d_pilot_rp_body_v[0] << ", " << d_pilot_rp_body_v[1] << ", " << d_pilot_rp_body_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"d_cg_rp_body_v[3]: " << d_cg_rp_body_v[0] << ", " << d_cg_rp_body_v[1] << ", " << d_cg_rp_body_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"f_body_total_v[3]: " << f_body_total_v[0] << ", " << f_body_total_v[1] << ", " << f_body_total_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"f_local_total_v[3]: " << f_local_total_v[0] << ", " << f_local_total_v[1] << ", " << f_local_total_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"f_aero_v[3]: " << f_aero_v[0] << ", " << f_aero_v[1] << ", " << f_aero_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"f_engine_v[3]: " << f_engine_v[0] << ", " << f_engine_v[1] << ", " << f_engine_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"f_gear_v[3]: " << f_gear_v[0] << ", " << f_gear_v[1] << ", " << f_gear_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"m_total_rp_v[3]: " << m_total_rp_v[0] << ", " << m_total_rp_v[1] << ", " << m_total_rp_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"m_total_cg_v[3]: " << m_total_cg_v[0] << ", " << m_total_cg_v[1] << ", " << m_total_cg_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"m_aero_v[3]: " << m_aero_v[0] << ", " << m_aero_v[1] << ", " << m_aero_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"m_engine_v[3]: " << m_engine_v[0] << ", " << m_engine_v[1] << ", " << m_engine_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"m_gear_v[3]: " << m_gear_v[0] << ", " << m_gear_v[1] << ", " << m_gear_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"v_dot_local_v[3]: " << v_dot_local_v[0] << ", " << v_dot_local_v[1] << ", " << v_dot_local_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"v_dot_body_v[3]: " << v_dot_body_v[0] << ", " << v_dot_body_v[1] << ", " << v_dot_body_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"a_cg_body_v[3]: " << a_cg_body_v[0] << ", " << a_cg_body_v[1] << ", " << a_cg_body_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"a_pilot_body_v[3]: " << a_pilot_body_v[0] << ", " << a_pilot_body_v[1] << ", " << a_pilot_body_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"n_cg_body_v[3]: " << n_cg_body_v[0] << ", " << n_cg_body_v[1] << ", " << n_cg_body_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"n_pilot_body_v[3]: " << n_pilot_body_v[0] << ", " << n_pilot_body_v[1] << ", " << n_pilot_body_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"omega_dot_body_v[3]: " << omega_dot_body_v[0] << ", " << omega_dot_body_v[1] << ", " << omega_dot_body_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"v_local_v[3]: " << v_local_v[0] << ", " << v_local_v[1] << ", " << v_local_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"v_local_rel_ground_v[3]: " << v_local_rel_ground_v[0] << ", " << v_local_rel_ground_v[1] << ", " << v_local_rel_ground_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"v_local_airmass_v[3]: " << v_local_airmass_v[0] << ", " << v_local_airmass_v[1] << ", " << v_local_airmass_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"v_local_rel_airmass_v[3]: " << v_local_rel_airmass_v[0] << ", " << v_local_rel_airmass_v[1] << ", " << v_local_rel_airmass_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"v_local_gust_v[3]: " << v_local_gust_v[0] << ", " << v_local_gust_v[1] << ", " << v_local_gust_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"v_wind_body_v[3]: " << v_wind_body_v[0] << ", " << v_wind_body_v[1] << ", " << v_wind_body_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"omega_body_v[3]: " << omega_body_v[0] << ", " << omega_body_v[1] << ", " << omega_body_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"omega_local_v[3]: " << omega_local_v[0] << ", " << omega_local_v[1] << ", " << omega_local_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"omega_total_v[3]: " << omega_total_v[0] << ", " << omega_total_v[1] << ", " << omega_total_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"euler_rates_v[3]: " << euler_rates_v[0] << ", " << euler_rates_v[1] << ", " << euler_rates_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"geocentric_rates_v[3]: " << geocentric_rates_v[0] << ", " << geocentric_rates_v[1] << ", " << geocentric_rates_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"geocentric_position_v[3]: " << geocentric_position_v[0] << ", " << geocentric_position_v[1] << ", " << geocentric_position_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"geodetic_position_v[3]: " << geodetic_position_v[0] << ", " << geodetic_position_v[1] << ", " << geodetic_position_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"euler_angles_v[3]: " << euler_angles_v[0] << ", " << euler_angles_v[1] << ", " << euler_angles_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"d_cg_rwy_local_v[3]: " << d_cg_rwy_local_v[0] << ", " << d_cg_rwy_local_v[1] << ", " << d_cg_rwy_local_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"d_cg_rwy_rwy_v[3]: " << d_cg_rwy_rwy_v[0] << ", " << d_cg_rwy_rwy_v[1] << ", " << d_cg_rwy_rwy_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"d_pilot_rwy_local_v[3]: " << d_pilot_rwy_local_v[0] << ", " << d_pilot_rwy_local_v[1] << ", " << d_pilot_rwy_local_v[2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"d_pilot_rwy_rwy_v[3]: " << d_pilot_rwy_rwy_v[0] << ", " << d_pilot_rwy_rwy_v[1] << ", " << d_pilot_rwy_rwy_v[2]);
  
    FG_LOG(FG_FLIGHT,FG_INFO,"t_local_to_body_m[0][3]: " << t_local_to_body_m[0][0] << ", " << t_local_to_body_m[0][1] << ", " << t_local_to_body_m[0][2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"t_local_to_body_m[1][3]: " << t_local_to_body_m[1][0] << ", " << t_local_to_body_m[1][1] << ", " << t_local_to_body_m[1][2]);
    FG_LOG(FG_FLIGHT,FG_INFO,"t_local_to_body_m[2][3]: " << t_local_to_body_m[2][0] << ", " << t_local_to_body_m[2][1] << ", " << t_local_to_body_m[2][2]);

    FG_LOG(FG_FLIGHT,FG_INFO,"mass: " << mass );
    FG_LOG(FG_FLIGHT,FG_INFO,"i_xx: " << i_xx );
    FG_LOG(FG_FLIGHT,FG_INFO,"i_yy: " << i_yy );
    FG_LOG(FG_FLIGHT,FG_INFO,"i_zz: " << i_zz );
    FG_LOG(FG_FLIGHT,FG_INFO,"i_xz: " << i_xz );
    FG_LOG(FG_FLIGHT,FG_INFO,"nlf: " << nlf );
    FG_LOG(FG_FLIGHT,FG_INFO,"v_rel_wind: " << v_rel_wind );
    FG_LOG(FG_FLIGHT,FG_INFO,"v_true_kts: " << v_true_kts );
    FG_LOG(FG_FLIGHT,FG_INFO,"v_rel_ground: " << v_rel_ground );
    FG_LOG(FG_FLIGHT,FG_INFO,"v_inertial: " << v_inertial );
    FG_LOG(FG_FLIGHT,FG_INFO,"v_ground_speed: " << v_ground_speed );
    FG_LOG(FG_FLIGHT,FG_INFO,"v_equiv: " << v_equiv );
    FG_LOG(FG_FLIGHT,FG_INFO,"v_equiv_kts: " << v_equiv_kts );
    FG_LOG(FG_FLIGHT,FG_INFO,"v_calibrated: " << v_calibrated );
    FG_LOG(FG_FLIGHT,FG_INFO,"v_calibrated_kts: " << v_calibrated_kts );
    FG_LOG(FG_FLIGHT,FG_INFO,"gravity: " << gravity );
    FG_LOG(FG_FLIGHT,FG_INFO,"centrifugal_relief: " << centrifugal_relief );
    FG_LOG(FG_FLIGHT,FG_INFO,"alpha: " << alpha );
    FG_LOG(FG_FLIGHT,FG_INFO,"beta: " << beta );
    FG_LOG(FG_FLIGHT,FG_INFO,"alpha_dot: " << alpha_dot );
    FG_LOG(FG_FLIGHT,FG_INFO,"beta_dot: " << beta_dot );
    FG_LOG(FG_FLIGHT,FG_INFO,"cos_alpha: " << cos_alpha );
    FG_LOG(FG_FLIGHT,FG_INFO,"sin_alpha: " << sin_alpha );
    FG_LOG(FG_FLIGHT,FG_INFO,"cos_beta: " << cos_beta );
    FG_LOG(FG_FLIGHT,FG_INFO,"sin_beta: " << sin_beta );
    FG_LOG(FG_FLIGHT,FG_INFO,"cos_phi: " << cos_phi );
    FG_LOG(FG_FLIGHT,FG_INFO,"sin_phi: " << sin_phi );
    FG_LOG(FG_FLIGHT,FG_INFO,"cos_theta: " << cos_theta );
    FG_LOG(FG_FLIGHT,FG_INFO,"sin_theta: " << sin_theta );
    FG_LOG(FG_FLIGHT,FG_INFO,"cos_psi: " << cos_psi );
    FG_LOG(FG_FLIGHT,FG_INFO,"sin_psi: " << sin_psi );
    FG_LOG(FG_FLIGHT,FG_INFO,"gamma_vert_rad: " << gamma_vert_rad );
    FG_LOG(FG_FLIGHT,FG_INFO,"gamma_horiz_rad: " << gamma_horiz_rad );
    FG_LOG(FG_FLIGHT,FG_INFO,"sigma: " << sigma );
    FG_LOG(FG_FLIGHT,FG_INFO,"density: " << density );
    FG_LOG(FG_FLIGHT,FG_INFO,"v_sound: " << v_sound );
    FG_LOG(FG_FLIGHT,FG_INFO,"mach_number: " << mach_number );
    FG_LOG(FG_FLIGHT,FG_INFO,"static_pressure: " << static_pressure );
    FG_LOG(FG_FLIGHT,FG_INFO,"total_pressure: " << total_pressure );
    FG_LOG(FG_FLIGHT,FG_INFO,"impact_pressure: " << impact_pressure );
    FG_LOG(FG_FLIGHT,FG_INFO,"dynamic_pressure: " << dynamic_pressure );
    FG_LOG(FG_FLIGHT,FG_INFO,"static_temperature: " << static_temperature );
    FG_LOG(FG_FLIGHT,FG_INFO,"total_temperature: " << total_temperature );
    FG_LOG(FG_FLIGHT,FG_INFO,"sea_level_radius: " << sea_level_radius );
    FG_LOG(FG_FLIGHT,FG_INFO,"earth_position_angle: " << earth_position_angle );
    FG_LOG(FG_FLIGHT,FG_INFO,"runway_altitude: " << runway_altitude );
    FG_LOG(FG_FLIGHT,FG_INFO,"runway_latitude: " << runway_latitude );
    FG_LOG(FG_FLIGHT,FG_INFO,"runway_longitude: " << runway_longitude );
    FG_LOG(FG_FLIGHT,FG_INFO,"runway_heading: " << runway_heading );
    FG_LOG(FG_FLIGHT,FG_INFO,"radius_to_rwy: " << radius_to_rwy );
    FG_LOG(FG_FLIGHT,FG_INFO,"climb_rate: " << climb_rate );
    FG_LOG(FG_FLIGHT,FG_INFO,"sin_lat_geocentric: " << sin_lat_geocentric );
    FG_LOG(FG_FLIGHT,FG_INFO,"cos_lat_geocentric: " << cos_lat_geocentric );
    FG_LOG(FG_FLIGHT,FG_INFO,"sin_longitude: " << sin_longitude );
    FG_LOG(FG_FLIGHT,FG_INFO,"cos_longitude: " << cos_longitude );
    FG_LOG(FG_FLIGHT,FG_INFO,"sin_latitude: " << sin_latitude );
    FG_LOG(FG_FLIGHT,FG_INFO,"cos_latitude: " << cos_latitude );
    FG_LOG(FG_FLIGHT,FG_INFO,"altitude_agl: " << altitude_agl );
}  

