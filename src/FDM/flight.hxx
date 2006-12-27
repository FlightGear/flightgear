// flight.hxx -- define shared flight model parameters
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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


#include <math.h>

#include <list>
#include <vector>
#include <string>

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <FDM/groundcache.hxx>

SG_USING_STD(list);
SG_USING_STD(vector);
SG_USING_STD(string);

typedef double FG_VECTOR_3[3];

// This is based heavily on LaRCsim/ls_generic.h
class FGInterface : public SGSubsystem {

private:
  
    // Has the init() method been called.  This is used to delay
    // initialization until scenery can be loaded and we know the true
    // ground elevation.
    bool inited;

    // Have we bound to the property system
    bool bound; 

    // periodic update management variable.  This is a scheme to run
    // the fdm with a fixed delta-t.  We control how many iteration of
    // the fdm to run with the fixed dt based on the elapsed time from
    // the last update.  This allows us to maintain sync with the real
    // time clock, even though each frame could take a random amount
    // of time.  Since "dt" is unlikely to divide evenly into the
    // elapse time, we keep track of the remainder and add it into the
    // next elapsed time.  This yields a small amount of temporal
    // jitter ( < dt ) but in practice seems to work well.

//     double delta_t;		// delta "t"
//     SGTimeStamp time_stamp;	// time stamp of last run
//     long elapsed;		// time elapsed since last run
    double remainder;		// remainder time from last run
//     int multi_loop;		// number of iterations of "delta_t" to run

    // Pilot location rel to ref pt
    FG_VECTOR_3 d_pilot_rp_body_v;

    // CG position w.r.t. ref. point
    FG_VECTOR_3 d_cg_rp_body_v;

    // Forces
    FG_VECTOR_3 f_body_total_v;
    FG_VECTOR_3 f_local_total_v;
    FG_VECTOR_3 f_aero_v;
    FG_VECTOR_3 f_engine_v;
    FG_VECTOR_3 f_gear_v;

    // Moments
    FG_VECTOR_3 m_total_rp_v;
    FG_VECTOR_3 m_total_cg_v;
    FG_VECTOR_3 m_aero_v;
    FG_VECTOR_3 m_engine_v;
    FG_VECTOR_3 m_gear_v;

    // Accelerations
    FG_VECTOR_3 v_dot_local_v;
    FG_VECTOR_3 v_dot_body_v;
    FG_VECTOR_3 a_cg_body_v;
    FG_VECTOR_3 a_pilot_body_v;
    FG_VECTOR_3 n_cg_body_v;
    FG_VECTOR_3 n_pilot_body_v;
    FG_VECTOR_3 omega_dot_body_v;

    // Velocities
    FG_VECTOR_3 v_local_v;
    FG_VECTOR_3 v_local_rel_ground_v; // V rel w.r.t. earth surface
    FG_VECTOR_3 v_local_airmass_v;    // velocity of airmass (steady winds)
    FG_VECTOR_3 v_local_rel_airmass_v;  // velocity of veh. relative to airmass
    FG_VECTOR_3 v_local_gust_v;       // linear turbulence components, L frame
    FG_VECTOR_3 v_wind_body_v;        // Wind-relative velocities in body axis

    FG_VECTOR_3 omega_body_v;         // Angular B rates
    FG_VECTOR_3 omega_local_v;        // Angular L rates
    FG_VECTOR_3 omega_total_v;        // Diff btw B & L
    FG_VECTOR_3 euler_rates_v;
    FG_VECTOR_3 geocentric_rates_v;   // Geocentric linear velocities

    // Positions
    FG_VECTOR_3 geocentric_position_v;
    FG_VECTOR_3 geodetic_position_v;
    FG_VECTOR_3 euler_angles_v;

    // Miscellaneous Quantities
    FG_VECTOR_3 d_cg_rwy_local_v;     // CG rel. to rwy in local coords
    FG_VECTOR_3 d_cg_rwy_rwy_v;       // CG relative to rwy, in rwy coordinates
    FG_VECTOR_3 d_pilot_rwy_local_v;  // pilot rel. to rwy in local coords
    FG_VECTOR_3 d_pilot_rwy_rwy_v;    // pilot rel. to rwy, in rwy coords.

    // Inertias
    double mass, i_xx, i_yy, i_zz, i_xz;

    // Normal Load Factor
    double nlf;

    // Velocities
    double v_rel_wind, v_true_kts, v_rel_ground, v_inertial;
    double v_ground_speed, v_equiv, v_equiv_kts;
    double v_calibrated, v_calibrated_kts;

    // Miscellaneious Quantities
    double t_local_to_body_m[3][3];   // Transformation matrix L to B
    double gravity;                   // Local acceleration due to G
    double centrifugal_relief;        // load factor reduction due to speed
    double alpha, beta, alpha_dot, beta_dot;  // in radians
    double cos_alpha, sin_alpha, cos_beta, sin_beta;
    double cos_phi, sin_phi, cos_theta, sin_theta, cos_psi, sin_psi;
    double gamma_vert_rad, gamma_horiz_rad;  // Flight path angles
    double sigma, density, v_sound, mach_number;
    double static_pressure, total_pressure, impact_pressure;
    double dynamic_pressure;
    double static_temperature, total_temperature;
    double sea_level_radius, earth_position_angle;
    double runway_altitude, runway_latitude, runway_longitude;
    double runway_heading;
    double radius_to_rwy;
    double climb_rate;                // in feet per second
    double sin_lat_geocentric, cos_lat_geocentric;
    double sin_longitude, cos_longitude;
    double sin_latitude, cos_latitude;
    double altitude_agl;

    double daux[16];		// auxilliary doubles
    float  faux[16];		// auxilliary floats
    int    iaux[16];		// auxilliary ints

    // SGTimeStamp valid_stamp;          // time this record is valid
    // SGTimeStamp next_stamp;           // time this record is valid

    // the ground cache object itself.
    FGGroundCache ground_cache;

protected:

    int _calc_multiloop (double dt);

public:

				// deliberately not virtual so that
				// FGInterface constructor will call
				// the right version
    void _setup();

    void _busdump(void);
    void _updateGeodeticPosition( double lat, double lon, double alt );
    void _updateGeocentricPosition( double lat_geoc, double lon, double alt );
    void _update_ground_elev_at_pos( void );
    void _updateWeather( void );

    inline void _set_Inertias( double m, double xx, double yy, 
			      double zz, double xz)
    {
	mass = m;
	i_xx = xx;
	i_yy = yy;
	i_zz = zz;
	i_xz = xz;
    }
    inline void _set_CG_Position( double dx, double dy, double dz ) {
	d_cg_rp_body_v[0] = dx;
	d_cg_rp_body_v[1] = dy;
	d_cg_rp_body_v[2] = dz;
    }
    inline void _set_Accels_Local( double north, double east, double down ) {
	v_dot_local_v[0] = north;
	v_dot_local_v[1] = east;
	v_dot_local_v[2] = down;
    }
    inline void _set_Accels_Body( double u, double v, double w ) {
	v_dot_body_v[0] = u;
	v_dot_body_v[1] = v;
	v_dot_body_v[2] = w;
    }
    inline void _set_Accels_CG_Body( double x, double y, double z ) {
	a_cg_body_v[0] = x;
	a_cg_body_v[1] = y;
	a_cg_body_v[2] = z;
    }
    inline void _set_Accels_Pilot_Body( double x, double y, double z ) {
	a_pilot_body_v[0] = x;
	a_pilot_body_v[1] = y;
	a_pilot_body_v[2] = z;
    }
    inline void _set_Accels_CG_Body_N( double x, double y, double z ) {
	n_cg_body_v[0] = x;
	n_cg_body_v[1] = y;
	n_cg_body_v[2] = z;
    }
    void _set_Nlf(double n) { nlf=n;  }
    inline void _set_Velocities_Local( double north, double east, double down ){
	v_local_v[0] = north;
	v_local_v[1] = east;
	v_local_v[2] = down;
    }
    inline void _set_Velocities_Ground(double north, double east, double down) {
	v_local_rel_ground_v[0] = north;
	v_local_rel_ground_v[1] = east;
	v_local_rel_ground_v[2] = down;
    }
    inline void _set_Velocities_Local_Airmass( double north, double east, 
					      double down)
    {
	v_local_airmass_v[0] = north;
	v_local_airmass_v[1] = east;
	v_local_airmass_v[2] = down;
    }
    inline void _set_Velocities_Wind_Body( double u, double v, double w) {
	v_wind_body_v[0] = u;
	v_wind_body_v[1] = v;
	v_wind_body_v[2] = w;
    }
    inline void _set_V_rel_wind(double vt) { v_rel_wind = vt; }
    inline void _set_V_ground_speed( double v) { v_ground_speed = v; }
    inline void _set_V_equiv_kts( double kts ) { v_equiv_kts = kts; }
    inline void _set_V_calibrated_kts( double kts ) { v_calibrated_kts = kts; }
    inline void _set_Omega_Body( double p, double q, double r ) {
	omega_body_v[0] = p;
	omega_body_v[1] = q;
	omega_body_v[2] = r;
    }
    inline void _set_Euler_Rates( double phi, double theta, double psi ) {
	euler_rates_v[0] = phi;
	euler_rates_v[1] = theta;
	euler_rates_v[2] = psi;
    }
    inline void _set_Geocentric_Rates( double lat, double lon, double rad ) {
	geocentric_rates_v[0] = lat;
	geocentric_rates_v[1] = lon;
	geocentric_rates_v[2] = rad;
    }
#if 0
    inline void _set_Radius_to_vehicle(double radius) {
	geocentric_position_v[2] = radius;
    }
#endif
    inline void _set_Geocentric_Position( double lat, double lon, double rad ) {
	geocentric_position_v[0] = lat;
	geocentric_position_v[1] = lon;
	geocentric_position_v[2] = rad;
    }
    inline void _set_Latitude(double lat) { geodetic_position_v[0] = lat; }
    inline void _set_Longitude(double lon) { geodetic_position_v[1] = lon; }
    inline void _set_Altitude(double altitude) {
	geodetic_position_v[2] = altitude;
    }
    inline void _set_Altitude_AGL(double agl) {
	altitude_agl = agl;
    }
    inline void _set_Geodetic_Position( double lat, double lon, double alt ) {
	geodetic_position_v[0] = lat;
	geodetic_position_v[1] = lon;
	geodetic_position_v[2] = alt;
    }
    inline void _set_Euler_Angles( double phi, double theta, double psi ) {
	euler_angles_v[0] = phi;
	euler_angles_v[1] = theta;
	euler_angles_v[2] = psi;
    }
    inline void _set_T_Local_to_Body( int i, int j, double value) {
	t_local_to_body_m[i-1][j-1] = value;
    }
    inline void _set_T_Local_to_Body( double m[3][3] ) {
	int i, j;
	for ( i = 0; i < 3; i++ ) {
	    for ( j = 0; j < 3; j++ ) {
		t_local_to_body_m[i][j] = m[i][j];
	    }
	}
    }
    inline void _set_Alpha( double a ) { alpha = a; }
    inline void _set_Beta( double b ) { beta = b; }
    inline void _set_Cos_phi( double cp ) { cos_phi = cp; }
    inline void _set_Cos_theta( double ct ) { cos_theta = ct; }
    inline void _set_Gamma_vert_rad( double gv ) { gamma_vert_rad = gv; }
    inline void _set_Density( double d ) { density = d; }
    inline void _set_Mach_number( double m ) { mach_number = m; }
    inline void _set_Static_pressure( double sp ) { static_pressure = sp; }
    inline void _set_Static_temperature( double t ) { static_temperature = t; } 
    inline void _set_Total_temperature( double tat ) { total_temperature = tat; } //JW
    inline void _set_Sea_level_radius( double r ) { sea_level_radius = r; }
    inline void _set_Earth_position_angle(double a) { earth_position_angle = a; }
    inline void _set_Runway_altitude( double alt ) { runway_altitude = alt; }
    inline void _set_Climb_Rate(double rate) { climb_rate = rate; }
    inline void _set_sin_lat_geocentric(double parm) {
	sin_lat_geocentric = sin(parm);
    }
    inline void _set_cos_lat_geocentric(double parm) {
	cos_lat_geocentric = cos(parm);
    }
    inline void _set_sin_cos_longitude(double parm) {
	sin_longitude = sin(parm);
	cos_longitude = cos(parm);
    }
    inline void _set_sin_cos_latitude(double parm) {
	sin_latitude = sin(parm);
	cos_latitude = cos(parm);
    }

    inline void _set_daux( int n, double value ) { daux[n] = value; }
    inline void _set_faux( int n, float value ) { faux[n] = value; }
    inline void _set_iaux( int n, int value ) { iaux[n] = value; }

public:
  
    FGInterface();
    FGInterface( double dt );
    virtual ~FGInterface();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update(double dt);
    virtual bool ToggleDataLogging(bool state) { return false; }
    virtual bool ToggleDataLogging(void) { return false; }

    // Define the various supported flight models (many not yet implemented)
    enum {
	// Magic Carpet mode
	FG_MAGICCARPET = 0,

	// The NASA LaRCsim (Navion) flight model
	FG_LARCSIM = 1,

	// Jon S. Berndt's new FDM written from the ground up in C++
	FG_JSBSIM = 2,

	// Christian's hot air balloon simulation
	FG_BALLOONSIM = 3,

	// Aeronautical DEvelopment AGEncy, Bangalore India
	FG_ADA = 4,

	// The following aren't implemented but are here to spark
	// thoughts and discussions, and maybe even action.
	FG_ACM = 5,
	FG_SUPER_SONIC = 6,
	FG_HELICOPTER = 7,
	FG_AUTOGYRO = 8,
	FG_PARACHUTE = 9,

	// Driven externally via a serial port, net, file, etc.
	FG_EXTERNAL = 10
    };

    // initialization
    inline bool get_inited() const { return inited; }
    inline void set_inited( bool value ) { inited = value; }

    inline bool get_bound() const { return bound; }

    //perform initializion that is common to all FDM's
    void common_init();

    // time and update management values
//     inline double get_delta_t() const { return delta_t; }
//     inline void set_delta_t( double dt ) { delta_t = dt; }
//     inline SGTimeStamp get_time_stamp() const { return time_stamp; }
//     inline void set_time_stamp( SGTimeStamp s ) { time_stamp = s; }
//     inline void stamp() { time_stamp.stamp(); }
//     inline long get_elapsed() const { return elapsed; }
//     inline void set_elapsed( long e ) { elapsed = e; }
//     inline long get_remainder() const { return remainder; }
//     inline void set_remainder( long r ) { remainder = r; }
//     inline int get_multi_loop() const { return multi_loop; }
//     inline void set_multi_loop( int ml ) { multi_loop = ml; }

    // Positions
    virtual void set_Latitude(double lat);       // geocentric
    virtual void set_Longitude(double lon);    
    virtual void set_Altitude(double alt);  // triggers re-calc of AGL altitude
    virtual void set_AltitudeAGL(double altagl); // and vice-versa
    virtual void set_Latitude_deg (double lat) {
      set_Latitude(lat * SGD_DEGREES_TO_RADIANS);
    }
    virtual void set_Longitude_deg (double lon) {
      set_Longitude(lon * SGD_DEGREES_TO_RADIANS);
    }
    
    // Speeds -- setting any of these will trigger a re-calc of the rest
    virtual void set_V_calibrated_kts(double vc);
    virtual void set_Mach_number(double mach);
    virtual void set_Velocities_Local( double north, double east, double down );
    inline void set_V_north (double north) { 
      set_Velocities_Local(north, v_local_v[1], v_local_v[2]);
    }
    inline void set_V_east (double east) { 
      set_Velocities_Local(v_local_v[0], east, v_local_v[2]);
    }
    inline void set_V_down (double down) { 
      set_Velocities_Local(v_local_v[0], v_local_v[1], down);
    }
    virtual void set_Velocities_Wind_Body( double u, double v, double w);
    virtual void set_uBody (double uBody) { 
      set_Velocities_Wind_Body(uBody, v_wind_body_v[1], v_wind_body_v[2]);
    }
    virtual void set_vBody (double vBody) { 
      set_Velocities_Wind_Body(v_wind_body_v[0], vBody, v_wind_body_v[2]);
    }
    virtual void set_wBody (double wBody) {
      set_Velocities_Wind_Body(v_wind_body_v[0], v_wind_body_v[1], wBody);
    }
    
    // Euler angles 
    virtual void set_Euler_Angles( double phi, double theta, double psi );
    virtual void set_Phi (double phi) {
      set_Euler_Angles(phi, get_Theta(), get_Psi());
    }
    virtual void set_Theta (double theta) {
      set_Euler_Angles(get_Phi(), theta, get_Psi());
    }
    virtual void set_Psi (double psi) { 
      set_Euler_Angles(get_Phi(), get_Theta(), psi);
    }
    virtual void set_Phi_deg (double phi) {
      set_Phi(phi * SGD_DEGREES_TO_RADIANS);
    }
    virtual void set_Theta_deg (double theta) {
      set_Theta(theta * SGD_DEGREES_TO_RADIANS); 
    }
    virtual void set_Psi_deg (double psi) {
      set_Psi(psi * SGD_DEGREES_TO_RADIANS);
    }
    
    // Flight Path
    virtual void set_Climb_Rate( double roc);
    virtual void set_Gamma_vert_rad( double gamma);
    
    // Earth
    
    virtual void set_Static_pressure(double p);
    virtual void set_Static_temperature(double T);
    virtual void set_Density(double rho);
    
    virtual void set_Velocities_Local_Airmass (double wnorth, 
					       double weast, 
					       double wdown );

    // ========== Mass properties and geometry values ==========

    // Inertias
    inline double get_Mass() const { return mass; }
    inline double get_I_xx() const { return i_xx; }
    inline double get_I_yy() const { return i_yy; }
    inline double get_I_zz() const { return i_zz; }
    inline double get_I_xz() const { return i_xz; }

    // Pilot location rel to ref pt
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
    // inline double * get_D_cg_rp_body_v() { return d_cg_rp_body_v; }
    inline double get_Dx_cg() const { return d_cg_rp_body_v[0]; }
    inline double get_Dy_cg() const { return d_cg_rp_body_v[1]; }
    inline double get_Dz_cg() const { return d_cg_rp_body_v[2]; }

    // ========== Forces ==========

    // inline double * get_F_body_total_v() { return f_body_total_v; }
    // inline double get_F_X() const { return f_body_total_v[0]; }
    // inline double get_F_Y() const { return f_body_total_v[1]; }
    // inline double get_F_Z() const { return f_body_total_v[2]; }
    /* inline void set_Forces_Body_Total( double x, double y, double z ) {
	f_body_total_v[0] = x;
	f_body_total_v[1] = y;
	f_body_total_v[2] = z;
    } */

    // inline double * get_F_local_total_v() { return f_local_total_v; }
    // inline double get_F_north() const { return f_local_total_v[0]; }
    // inline double get_F_east() const { return f_local_total_v[1]; }
    // inline double get_F_down() const { return f_local_total_v[2]; }
    /* inline void set_Forces_Local_Total( double x, double y, double z ) {
	f_local_total_v[0] = x;
	f_local_total_v[1] = y;
	f_local_total_v[2] = z;
    } */

    // inline double * get_F_aero_v() { return f_aero_v; }
    // inline double get_F_X_aero() const { return f_aero_v[0]; }
    // inline double get_F_Y_aero() const { return f_aero_v[1]; }
    // inline double get_F_Z_aero() const { return f_aero_v[2]; }
    /* inline void set_Forces_Aero( double x, double y, double z ) {
	f_aero_v[0] = x;
	f_aero_v[1] = y;
	f_aero_v[2] = z;
    } */
    
    // inline double * get_F_engine_v() { return f_engine_v; }
    // inline double get_F_X_engine() const { return f_engine_v[0]; }
    // inline double get_F_Y_engine() const { return f_engine_v[1]; }
    // inline double get_F_Z_engine() const { return f_engine_v[2]; }
    /* inline void set_Forces_Engine( double x, double y, double z ) {
	f_engine_v[0] = x;
	f_engine_v[1] = y;
	f_engine_v[2] = z;
    } */

    // inline double * get_F_gear_v() { return f_gear_v; }
    // inline double get_F_X_gear() const { return f_gear_v[0]; }
    // inline double get_F_Y_gear() const { return f_gear_v[1]; }
    // inline double get_F_Z_gear() const { return f_gear_v[2]; }
    /* inline void set_Forces_Gear( double x, double y, double z ) {
	f_gear_v[0] = x;
	f_gear_v[1] = y;
	f_gear_v[2] = z;
    } */

    // ========== Moments ==========

    // inline double * get_M_total_rp_v() { return m_total_rp_v; }
    // inline double get_M_l_rp() const { return m_total_rp_v[0]; }
    // inline double get_M_m_rp() const { return m_total_rp_v[1]; }
    // inline double get_M_n_rp() const { return m_total_rp_v[2]; }
    /* inline void set_Moments_Total_RP( double l, double m, double n ) {
	m_total_rp_v[0] = l;
	m_total_rp_v[1] = m;
	m_total_rp_v[2] = n;
    } */

    // inline double * get_M_total_cg_v() { return m_total_cg_v; }
    // inline double get_M_l_cg() const { return m_total_cg_v[0]; }
    // inline double get_M_m_cg() const { return m_total_cg_v[1]; }
    // inline double get_M_n_cg() const { return m_total_cg_v[2]; }
    /* inline void set_Moments_Total_CG( double l, double m, double n ) {
	m_total_cg_v[0] = l;
	m_total_cg_v[1] = m;
	m_total_cg_v[2] = n;
    } */

    // inline double * get_M_aero_v() { return m_aero_v; }
    // inline double get_M_l_aero() const { return m_aero_v[0]; }
    // inline double get_M_m_aero() const { return m_aero_v[1]; }
    // inline double get_M_n_aero() const { return m_aero_v[2]; }
    /* inline void set_Moments_Aero( double l, double m, double n ) {
	m_aero_v[0] = l;
	m_aero_v[1] = m;
	m_aero_v[2] = n;
    } */

    // inline double * get_M_engine_v() { return m_engine_v; }
    // inline double get_M_l_engine() const { return m_engine_v[0]; }
    // inline double get_M_m_engine() const { return m_engine_v[1]; }
    // inline double get_M_n_engine() const { return m_engine_v[2]; }
    /* inline void set_Moments_Engine( double l, double m, double n ) {
	m_engine_v[0] = l;
	m_engine_v[1] = m;
	m_engine_v[2] = n;
    } */

    // inline double * get_M_gear_v() { return m_gear_v; }
    // inline double get_M_l_gear() const { return m_gear_v[0]; }
    // inline double get_M_m_gear() const { return m_gear_v[1]; }
    // inline double get_M_n_gear() const { return m_gear_v[2]; }
    /* inline void set_Moments_Gear( double l, double m, double n ) {
	m_gear_v[0] = l;
	m_gear_v[1] = m;
	m_gear_v[2] = n;
    } */

    // ========== Accelerations ==========

    // inline double * get_V_dot_local_v() { return v_dot_local_v; }
    inline double get_V_dot_north() const { return v_dot_local_v[0]; }
    inline double get_V_dot_east() const { return v_dot_local_v[1]; }
    inline double get_V_dot_down() const { return v_dot_local_v[2]; }

    // inline double * get_V_dot_body_v() { return v_dot_body_v; }
    inline double get_U_dot_body() const { return v_dot_body_v[0]; }
    inline double get_V_dot_body() const { return v_dot_body_v[1]; }
    inline double get_W_dot_body() const { return v_dot_body_v[2]; }

    // inline double * get_A_cg_body_v() { return a_cg_body_v; }
    inline double get_A_X_cg() const { return a_cg_body_v[0]; }
    inline double get_A_Y_cg() const { return a_cg_body_v[1]; }
    inline double get_A_Z_cg() const { return a_cg_body_v[2]; }

    // inline double * get_A_pilot_body_v() { return a_pilot_body_v; }
    inline double get_A_X_pilot() const { return a_pilot_body_v[0]; }
    inline double get_A_Y_pilot() const { return a_pilot_body_v[1]; }
    inline double get_A_Z_pilot() const { return a_pilot_body_v[2]; }

    // inline double * get_N_cg_body_v() { return n_cg_body_v; }
    inline double get_N_X_cg() const { return n_cg_body_v[0]; }
    inline double get_N_Y_cg() const { return n_cg_body_v[1]; }
    inline double get_N_Z_cg() const { return n_cg_body_v[2]; }

    // inline double * get_N_pilot_body_v() { return n_pilot_body_v; }
    // inline double get_N_X_pilot() const { return n_pilot_body_v[0]; }
    // inline double get_N_Y_pilot() const { return n_pilot_body_v[1]; }
    // inline double get_N_Z_pilot() const { return n_pilot_body_v[2]; }
    // inline void set_Accels_Pilot_Body_N( double x, double y, double z ) {
    //    n_pilot_body_v[0] = x;
    //    n_pilot_body_v[1] = y;
    //    n_pilot_body_v[2] = z;
    // }

    inline double get_Nlf(void) const { return nlf; }

    // inline double * get_Omega_dot_body_v() { return omega_dot_body_v; }
    // inline double get_P_dot_body() const { return omega_dot_body_v[0]; }
    // inline double get_Q_dot_body() const { return omega_dot_body_v[1]; }
    // inline double get_R_dot_body() const { return omega_dot_body_v[2]; }
    /* inline void set_Accels_Omega( double p, double q, double r ) {
	omega_dot_body_v[0] = p;
	omega_dot_body_v[1] = q;
	omega_dot_body_v[2] = r;
    } */


    // ========== Velocities ==========

    // inline double * get_V_local_v() { return v_local_v; }
    inline double get_V_north() const { return v_local_v[0]; }
    inline double get_V_east() const { return v_local_v[1]; }
    inline double get_V_down() const { return v_local_v[2]; }
    inline double get_uBody () const { return v_wind_body_v[0]; }
    inline double get_vBody () const { return v_wind_body_v[1]; }
    inline double get_wBody () const { return v_wind_body_v[2]; }

    // Please dont comment these out. fdm=ada uses these (see
    // cockpit.cxx) --->
    inline double * get_V_local_rel_ground_v() {
        return v_local_rel_ground_v;
    }
    inline double get_V_north_rel_ground() const {
        return v_local_rel_ground_v[0];
    }
    inline double get_V_east_rel_ground() const {
        return v_local_rel_ground_v[1];
    }
    inline double get_V_down_rel_ground() const {
        return v_local_rel_ground_v[2];
    }
    // <--- fdm=ada uses these (see cockpit.cxx)

    // inline double * get_V_local_airmass_v() { return v_local_airmass_v; }
    inline double get_V_north_airmass() const { return v_local_airmass_v[0]; }
    inline double get_V_east_airmass() const { return v_local_airmass_v[1]; }
    inline double get_V_down_airmass() const { return v_local_airmass_v[2]; }

    // airmass
    // inline double * get_V_local_rel_airmass_v() {
    //   return v_local_rel_airmass_v;
    // }
    // inline double get_V_north_rel_airmass() const {
    //   return v_local_rel_airmass_v[0];
    // }
    // inline double get_V_east_rel_airmass() const {
    //   return v_local_rel_airmass_v[1];
    // }
    // inline double get_V_down_rel_airmass() const {
    //   return v_local_rel_airmass_v[2];
    // }
    /* inline void set_Velocities_Local_Rel_Airmass( double north, double east, 
						  double down)
    {
	v_local_rel_airmass_v[0] = north;
	v_local_rel_airmass_v[1] = east;
	v_local_rel_airmass_v[2] = down;
    } */

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
    
    // inline double * get_V_wind_body_v() { return v_wind_body_v; }
    inline double get_U_body() const { return v_wind_body_v[0]; }
    inline double get_V_body() const { return v_wind_body_v[1]; }
    inline double get_W_body() const { return v_wind_body_v[2]; }

    inline double get_V_rel_wind() const { return v_rel_wind; }
    // inline void set_V_rel_wind(double wind) { v_rel_wind = wind; }

    inline double get_V_true_kts() const { return v_true_kts; }
    // inline void set_V_true_kts(double kts) { v_true_kts = kts; }

    // inline double get_V_rel_ground() const { return v_rel_ground; }
    // inline void set_V_rel_ground( double v ) { v_rel_ground = v; }

    // inline double get_V_inertial() const { return v_inertial; }
    // inline void set_V_inertial(double v) { v_inertial = v; }

    inline double get_V_ground_speed() const { return v_ground_speed; }

    // inline double get_V_equiv() const { return v_equiv; }
    // inline void set_V_equiv( double v ) { v_equiv = v; }

    inline double get_V_equiv_kts() const { return v_equiv_kts; }

    //inline double get_V_calibrated() const { return v_calibrated; }
    //inline void set_V_calibrated( double v ) { v_calibrated = v; }

    inline double get_V_calibrated_kts() const { return v_calibrated_kts; }

    // inline double * get_Omega_body_v() { return omega_body_v; }
    inline double get_P_body() const { return omega_body_v[0]; }
    inline double get_Q_body() const { return omega_body_v[1]; }
    inline double get_R_body() const { return omega_body_v[2]; }

    // inline double * get_Omega_local_v() { return omega_local_v; }
    // inline double get_P_local() const { return omega_local_v[0]; }
    // inline double get_Q_local() const { return omega_local_v[1]; }
    // inline double get_R_local() const { return omega_local_v[2]; }
    /* inline void set_Omega_Local( double p, double q, double r ) {
	omega_local_v[0] = p;
	omega_local_v[1] = q;
	omega_local_v[2] = r;
    } */

    // inline double * get_Omega_total_v() { return omega_total_v; }
    // inline double get_P_total() const { return omega_total_v[0]; }
    // inline double get_Q_total() const { return omega_total_v[1]; }
    // inline double get_R_total() const { return omega_total_v[2]; }
    /* inline void set_Omega_Total( double p, double q, double r ) {
	omega_total_v[0] = p;
	omega_total_v[1] = q;
	omega_total_v[2] = r;
    } */

    // inline double * get_Euler_rates_v() { return euler_rates_v; }
    inline double get_Phi_dot() const { return euler_rates_v[0]; }
    inline double get_Theta_dot() const { return euler_rates_v[1]; }
    inline double get_Psi_dot() const { return euler_rates_v[2]; }
    inline double get_Phi_dot_degps() const { return euler_rates_v[0] * SGD_RADIANS_TO_DEGREES; }
    inline double get_Theta_dot_degps() const { return euler_rates_v[1] * SGD_RADIANS_TO_DEGREES; }
    inline double get_Psi_dot_degps() const { return euler_rates_v[2] * SGD_RADIANS_TO_DEGREES; }

    // inline double * get_Geocentric_rates_v() { return geocentric_rates_v; }
    inline double get_Latitude_dot() const { return geocentric_rates_v[0]; }
    inline double get_Longitude_dot() const { return geocentric_rates_v[1]; }
    inline double get_Radius_dot() const { return geocentric_rates_v[2]; }

    // ========== Positions ==========

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

    // inline double * get_Geodetic_position_v() { return geodetic_position_v; }
    inline double get_Latitude() const { return geodetic_position_v[0]; }
    inline double get_Longitude() const { return geodetic_position_v[1]; }
    inline double get_Altitude() const { return geodetic_position_v[2]; }
    inline double get_Altitude_AGL(void) const { return altitude_agl; }

    inline double get_Latitude_deg () const {
      return get_Latitude() * SGD_RADIANS_TO_DEGREES;
    }
    inline double get_Longitude_deg () const {
      return get_Longitude() * SGD_RADIANS_TO_DEGREES;
    }

    // inline double * get_Euler_angles_v() { return euler_angles_v; }
    inline double get_Phi() const { return euler_angles_v[0]; }
    inline double get_Theta() const { return euler_angles_v[1]; }
    inline double get_Psi() const { return euler_angles_v[2]; }
    inline double get_Phi_deg () const { return get_Phi() * SGD_RADIANS_TO_DEGREES; }
    inline double get_Theta_deg () const { return get_Theta() * SGD_RADIANS_TO_DEGREES; }
    inline double get_Psi_deg () const { return get_Psi() * SGD_RADIANS_TO_DEGREES; }


    // ========== Miscellaneous quantities ==========

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

    // inline double get_Gravity() const { return gravity; }
    // inline void set_Gravity(double g) { gravity = g; }

    // inline double get_Centrifugal_relief() const {
    //   return centrifugal_relief;
    // }
    // inline void set_Centrifugal_relief(double cr) {
    //   centrifugal_relief = cr;
    // }

    inline double get_Alpha() const { return alpha; }
    inline double get_Alpha_deg() const { return alpha * SGD_RADIANS_TO_DEGREES; }
    inline double get_Beta() const { return beta; }
    inline double get_Beta_deg() const { return beta * SGD_RADIANS_TO_DEGREES; }
    inline double get_Alpha_dot() const { return alpha_dot; }
    // inline void set_Alpha_dot( double ad ) { alpha_dot = ad; }
    inline double get_Beta_dot() const { return beta_dot; }
    // inline void set_Beta_dot( double bd ) { beta_dot = bd; }

    // inline double get_Cos_alpha() const { return cos_alpha; }
    // inline void set_Cos_alpha( double ca ) { cos_alpha = ca; }
    // inline double get_Sin_alpha() const { return sin_alpha; }
    // inline void set_Sin_alpha( double sa ) { sin_alpha = sa; }
    // inline double get_Cos_beta() const { return cos_beta; }
    // inline void set_Cos_beta( double cb ) { cos_beta = cb; }
    // inline double get_Sin_beta() const { return sin_beta; }
    // inline void set_Sin_beta( double sb ) { sin_beta = sb; }

    inline double get_Cos_phi() const { return cos_phi; }
    // inline double get_Sin_phi() const { return sin_phi; }
    // inline void set_Sin_phi( double sp ) { sin_phi = sp; }
    inline double get_Cos_theta() const { return cos_theta; }
    // inline double get_Sin_theta() const { return sin_theta; }
    // inline void set_Sin_theta( double st ) { sin_theta = st; }
    // inline double get_Cos_psi() const { return cos_psi; }
    // inline void set_Cos_psi( double cp ) { cos_psi = cp; }
    // inline double get_Sin_psi() const { return sin_psi; }
    // inline void set_Sin_psi( double sp ) { sin_psi = sp; }

    inline double get_Gamma_vert_rad() const { return gamma_vert_rad; }
    // inline double get_Gamma_horiz_rad() const { return gamma_horiz_rad; }
    // inline void set_Gamma_horiz_rad( double gh ) { gamma_horiz_rad = gh; }

    // inline double get_Sigma() const { return sigma; }
    // inline void set_Sigma( double s ) { sigma = s; }
    inline double get_Density() const { return density; }
    // inline double get_V_sound() const { return v_sound; }
    // inline void set_V_sound( double v ) { v_sound = v; }
    inline double get_Mach_number() const { return mach_number; }

    inline double get_Static_pressure() const { return static_pressure; }
    inline double get_Total_pressure() const { return total_pressure; }
    // inline void set_Total_pressure( double tp ) { total_pressure = tp; }
    // inline double get_Impact_pressure() const { return impact_pressure; }
    // inline void set_Impact_pressure( double ip ) { impact_pressure = ip; }
    inline double get_Dynamic_pressure() const { return dynamic_pressure; }
    // inline void set_Dynamic_pressure( double dp ) { dynamic_pressure = dp; }

    inline double get_Static_temperature() const { return static_temperature; }
    inline double get_Total_temperature() const { return total_temperature; }
    // inline void set_Total_temperature( double t ) { total_temperature = t; }

    inline double get_Sea_level_radius() const { return sea_level_radius; }
    inline double get_Earth_position_angle() const {
	return earth_position_angle;
    }

    inline double get_Runway_altitude() const { return runway_altitude; }
    inline double get_Runway_altitude_m() const { return SG_FEET_TO_METER * runway_altitude; }
    // inline double get_Runway_latitude() const { return runway_latitude; }
    // inline void set_Runway_latitude( double lat ) { runway_latitude = lat; }
    // inline double get_Runway_longitude() const { return runway_longitude; }
    // inline void set_Runway_longitude( double lon ) {
    //   runway_longitude = lon;
    // }
    // inline double get_Runway_heading() const { return runway_heading; }
    // inline void set_Runway_heading( double h ) { runway_heading = h; }

    // inline double get_Radius_to_rwy() const { return radius_to_rwy; }
    // inline void set_Radius_to_rwy( double r ) { radius_to_rwy = r; }

    // inline double * get_D_cg_rwy_local_v() { return d_cg_rwy_local_v; }
    // inline double get_D_cg_north_of_rwy() const {
    //   return d_cg_rwy_local_v[0];
    // }
    // inline double get_D_cg_east_of_rwy() const {
    //   return d_cg_rwy_local_v[1];
    // }
    // inline double get_D_cg_above_rwy() const { return d_cg_rwy_local_v[2]; }
    /* inline void set_CG_Rwy_Local( double north, double east, double above )
    {
	d_cg_rwy_local_v[0] = north;
	d_cg_rwy_local_v[1] = east;
	d_cg_rwy_local_v[2] = above;
    } */

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

    // inline double * get_D_pilot_rwy_local_v() { return d_pilot_rwy_local_v; }
    // inline double get_D_pilot_north_of_rwy() const {
    //   return d_pilot_rwy_local_v[0];
    // }
    // inline double get_D_pilot_east_of_rwy() const {
    //   return d_pilot_rwy_local_v[1];
    // }
    // inline double get_D_pilot_above_rwy() const {
    //   return d_pilot_rwy_local_v[2];
    // }
    /* inline void set_Pilot_Rwy_Local( double north, double east, double above )
    {
	d_pilot_rwy_local_v[0] = north;
	d_pilot_rwy_local_v[1] = east;
	d_pilot_rwy_local_v[2] = above;
    } */

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

    inline double get_Climb_Rate() const { return climb_rate; }

    // inline SGTimeStamp get_time_stamp() const { return valid_stamp; }
    // inline void stamp_time() { valid_stamp = next_stamp; next_stamp.stamp(); }

    // Extrapolate FDM based on time_offset (in usec)
    void extrapolate( int time_offset );

    // sin/cos lat_geocentric
    inline double get_sin_lat_geocentric(void) const {
	return sin_lat_geocentric;
    }
    inline double get_cos_lat_geocentric(void) const {
	return cos_lat_geocentric;
    }

    inline double get_sin_longitude(void) const {
	return sin_longitude;
    }
    inline double get_cos_longitude(void) const {
	return cos_longitude;
    }

    inline double get_sin_latitude(void) const {
	return sin_latitude;
    }
    inline double get_cos_latitude(void) const {
	return cos_latitude;
    }

    // Auxilliary variables
    inline double get_daux( int n ) const { return daux[n]; }
    inline float  get_faux( int n ) const { return faux[n]; }
    inline int    get_iaux( int n ) const { return iaux[n]; }

    // Note that currently this is the "same" value runway altitude...
    inline double get_ground_elev_ft() const { return runway_altitude; }


    //////////////////////////////////////////////////////////////////////////
    // Ground handling routines
    //////////////////////////////////////////////////////////////////////////

    enum GroundType {
      Unknown = 0, //??
      Solid, // Whatever we will roll on with infinite load factor.
      Water, // For the beaver ...
      Catapult, // Carrier cats.
      Wire // Carrier wires.
    };

    // Prepare the ground cache for the wgs84 position pt_*.
    // That is take all vertices in the ball with radius rad around the
    // position given by the pt_* and store them in a local scene graph.
    bool prepare_ground_cache_m(double ref_time, const double pt[3],
                                double rad);
    bool prepare_ground_cache_ft(double ref_time, const double pt[3],
                                 double rad);


    // Returns true if the cache is valid.
    // Also the reference time, point and radius values where the cache
    // is valid for are returned.
    bool is_valid_m(double *ref_time, double pt[3], double *rad);
    bool is_valid_ft(double *ref_time, double pt[3], double *rad);

    // Return the nearest catapult to the given point
    // pt in wgs84 coordinates.
    double get_cat_m(double t, const double pt[3],
                     double end[2][3], double vel[2][3]);
    double get_cat_ft(double t, const double pt[3],
                      double end[2][3], double vel[2][3]);
  

    // Return the altitude above ground below the wgs84 point pt
    // Search for the nearest triangle to pt.
    // Return ground properties like the ground type, the maximum load
    // this kind kind of ground can carry, the friction factor between
    // 0 and 1 which can be used to model lower friction with wet runways
    // and finally the altitude above ground.
    bool get_agl_m(double t, const double pt[3],
                   double contact[3], double normal[3], double vel[3],
                   int *type, double *loadCapacity,
                   double *frictionFactor, double *agl);
    bool get_agl_m(double t, const double pt[3],
                       double contact[3], double normal[3], double vel[3],
                       int *type, const SGMaterial **material,double *agl);
    bool get_agl_ft(double t, const double pt[3],
                    double contact[3], double normal[3], double vel[3],
                    int *type, double *loadCapacity,
                    double *frictionFactor, double *agl);

    // Return the altitude above ground below the wgs84 point pt
    // Search for the nearest triangle to pt.
    // Return ground properties like the ground type, a pointer to the
    // material and finally the altitude above ground.
    bool get_agl_m(double t, const double pt[3], double max_altoff,
                   double contact[3], double normal[3], double vel[3],
                   int *type, const SGMaterial** material, double *agl);
    bool get_agl_ft(double t, const double pt[3], double max_altoff,
                    double contact[3], double normal[3], double vel[3],
                    int *type, const SGMaterial** material, double *agl);
    double get_groundlevel_m(double lat, double lon, double alt);


    // Return 1 if the hook intersects with a wire.
    // That test is done by checking if the quad spanned by the points pt*
    // intersects with the line representing the wire.
    // If the wire is caught, the cache will trace this wires endpoints until
    // the FDM calls release_wire().
    bool caught_wire_m(double t, const double pt[4][3]);
    bool caught_wire_ft(double t, const double pt[4][3]);
  
    // Return the location and speed of the wire endpoints.
    bool get_wire_ends_m(double t, double end[2][3], double vel[2][3]);
    bool get_wire_ends_ft(double t, double end[2][3], double vel[2][3]);

    // Tell the cache code that it does no longer need to care for
    // the wire end position.
    void release_wire(void);
};

extern FGInterface * cur_fdm_state;

// Toggle data logging on/off
void fgToggleFDMdataLogging(void);


#endif // _FLIGHT_HXX
