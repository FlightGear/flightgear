// flight.cxx -- a general interface to the various flight models
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "flight.hxx"

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/timing/timestamp.hxx>

#include <Scenery/scenery.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <FDM/groundcache.hxx>


static inline void assign(double* ptr, const SGVec3d& vec)
{
  ptr[0] = vec[0];
  ptr[1] = vec[1];
  ptr[2] = vec[2];
}

// Constructor
FGInterface::FGInterface()
{
    _setup();
}

FGInterface::FGInterface( double dt )
{
    _setup();
}

// Destructor
FGInterface::~FGInterface() {
    // unbind();                   // FIXME: should be called explicitly
}

int
FGInterface::_calc_multiloop (double dt)
{
  // Since some time the simulation time increments we get here are
  // already a multiple of the basic update frequency.
  // So, there is no need to do our own multiloop rounding with all bad
  // roundoff problems when we already have nearly accurate values.
  // Only the speedup thing must be still handled here
  int hz = fgGetInt("/sim/model-hz");
  int multiloop = SGMiscd::roundToInt(dt*hz);
  int speedup = fgGetInt("/sim/speed-up");
  return multiloop * speedup;
}


/**
 * Set default values for the state of the FDM.
 *
 * This method is invoked by the constructors.
 */
void
FGInterface::_setup ()
{
    inited = false;
    bound = false;

    d_cg_rp_body_v = SGVec3d::zeros();
    v_dot_local_v = SGVec3d::zeros();
    v_dot_body_v = SGVec3d::zeros();
    a_cg_body_v = SGVec3d::zeros();
    a_pilot_body_v = SGVec3d::zeros();
    n_cg_body_v = SGVec3d::zeros();
    v_local_v = SGVec3d::zeros();
    v_local_rel_ground_v = SGVec3d::zeros();
    v_local_airmass_v = SGVec3d::zeros();
    v_wind_body_v = SGVec3d::zeros();
    omega_body_v = SGVec3d::zeros();
    euler_rates_v = SGVec3d::zeros();
    geocentric_rates_v = SGVec3d::zeros();
    geodetic_position_v = SGGeod::fromRadM(0, 0, 0);
    cartesian_position_v = SGVec3d::fromGeod(geodetic_position_v);
    geocentric_position_v = SGGeoc::fromCart(cartesian_position_v);
    euler_angles_v = SGVec3d::zeros();
    
    nlf=0;
    v_rel_wind=v_true_kts=0;
    v_ground_speed=v_equiv_kts=0;
    v_calibrated_kts=0;
    alpha=beta=0;
    gamma_vert_rad=0;
    density=mach_number=0;
    static_pressure=total_pressure=0;
    dynamic_pressure=0;
    static_temperature=total_temperature=0;
    sea_level_radius=earth_position_angle=0;
    runway_altitude=0;
    climb_rate=0;
    altitude_agl=0;
    track=0;
}

void
FGInterface::init () {}

/**
 * Initialize the state of the FDM.
 *
 * Subclasses of FGInterface may do their own, additional initialization,
 * but there is some that is common to all.  Normally, they should call
 * this before they begin their own init to make sure the basic structures
 * are set up properly.
 */
void
FGInterface::common_init ()
{
    SG_LOG( SG_FLIGHT, SG_INFO, "Start common FDM init" );

    set_inited( true );

    ground_cache.set_cache_time_offset(globals->get_sim_time_sec());

    // Set initial position
    SG_LOG( SG_FLIGHT, SG_INFO, "...initializing position..." );
    double lon = fgGetDouble("/sim/presets/longitude-deg")
      * SGD_DEGREES_TO_RADIANS;
    double lat = fgGetDouble("/sim/presets/latitude-deg")
      * SGD_DEGREES_TO_RADIANS;
    double alt_ft = fgGetDouble("/sim/presets/altitude-ft");
    double alt_m = alt_ft * SG_FEET_TO_METER;
    set_Longitude( lon );
    set_Latitude( lat );
    SG_LOG( SG_FLIGHT, SG_INFO, "Checking for lon = "
            << lon*SGD_RADIANS_TO_DEGREES << "deg, lat = "
            << lat*SGD_RADIANS_TO_DEGREES << "deg, alt = "
            << alt_ft << "ft");

    double ground_elev_m = get_groundlevel_m(lat, lon, alt_m);
    double ground_elev_ft = ground_elev_m * SG_METER_TO_FEET;
    _set_Runway_altitude ( ground_elev_ft );

    // Set aircraft altitude
    if ( fgGetBool("/sim/presets/onground") || alt_ft < ground_elev_ft ) {
        fgSetDouble("/position/altitude-ft", ground_elev_ft + 0.1);
        set_Altitude( ground_elev_ft + 0.1);
    } else {
        set_Altitude( alt_ft );
    }

    // Set ground elevation
    SG_LOG( SG_FLIGHT, SG_INFO,
            "...initializing ground elevation to " << ground_elev_ft
            << "ft..." );

    // Set sea-level radius
    SG_LOG( SG_FLIGHT, SG_INFO, "...initializing sea-level radius..." );
    SG_LOG( SG_FLIGHT, SG_INFO, " lat = "
            << fgGetDouble("/sim/presets/latitude-deg")
            << " alt = " << get_Altitude() );
    double slr = SGGeodesy::SGGeodToSeaLevelRadius(geodetic_position_v);
    _set_Sea_level_radius( slr * SG_METER_TO_FEET );

    // Set initial Euler angles
    SG_LOG( SG_FLIGHT, SG_INFO, "...initializing Euler angles..." );
    set_Euler_Angles( fgGetDouble("/sim/presets/roll-deg")
                        * SGD_DEGREES_TO_RADIANS,
                      fgGetDouble("/sim/presets/pitch-deg")
                        * SGD_DEGREES_TO_RADIANS,
                      fgGetDouble("/sim/presets/heading-deg")
                        * SGD_DEGREES_TO_RADIANS );

    // Set initial velocities
    SG_LOG( SG_FLIGHT, SG_INFO, "...initializing velocities..." );
    if ( !fgHasNode("/sim/presets/speed-set") ) {
        set_V_calibrated_kts(0.0);
    } else {
        const string speedset = fgGetString("/sim/presets/speed-set");
        if ( speedset == "knots" || speedset == "KNOTS" ) {
            set_V_calibrated_kts( fgGetDouble("/sim/presets/airspeed-kt") );
        } else if ( speedset == "mach" || speedset == "MACH" ) {
            set_Mach_number( fgGetDouble("/sim/presets/mach") );
        } else if ( speedset == "UVW" || speedset == "uvw" ) {
            set_Velocities_Wind_Body(
                                     fgGetDouble("/sim/presets/uBody-fps"),
                                     fgGetDouble("/sim/presets/vBody-fps"),
                                     fgGetDouble("/sim/presets/wBody-fps") );
        } else if ( speedset == "NED" || speedset == "ned" ) {
            set_Velocities_Local(
                                 fgGetDouble("/sim/presets/speed-north-fps"),
                                 fgGetDouble("/sim/presets/speed-east-fps"),
                                 fgGetDouble("/sim/presets/speed-down-fps") );
        } else {
            SG_LOG( SG_FLIGHT, SG_ALERT,
                    "Unrecognized value for /sim/presets/speed-set: "
                    << speedset);
            set_V_calibrated_kts( 0.0 );
        }
    }

    if ( fgHasNode("/sim/presets/glideslope-deg") )
        set_Gamma_vert_rad( fgGetDouble("/sim/presets/glideslope-deg")
                              * SGD_DEGREES_TO_RADIANS );
    else if ( fgHasNode("/sim/presets/speed-set") &&
              fgHasNode( "/sim/presets/vertical-speed-fps") )
    {
        set_Climb_Rate( fgGetDouble("/sim/presets/vertical-speed-fps") );
    }

    SG_LOG( SG_FLIGHT, SG_INFO, "End common FDM init" );
}


/**
 * Bind getters and setters to properties.
 *
 * The bind() method will be invoked after init().  Note that unlike
 * the usual implementations of FGSubsystem::bind(), this method does
 * not automatically pick up existing values for the properties at
 * bind time; instead, all values are set explicitly in the init()
 * method.
 */
void
FGInterface::bind ()
{
  bound = true;

  _tiedProperties.setRoot(globals->get_props());
  // Aircraft position
  _tiedProperties.Tie("/position/latitude-deg", this,
                      &FGInterface::get_Latitude_deg,
                      &FGInterface::set_Latitude_deg,
                      false);
  fgSetArchivable("/position/latitude-deg");
  _tiedProperties.Tie("/position/longitude-deg", this,
                      &FGInterface::get_Longitude_deg,
                      &FGInterface::set_Longitude_deg,
                      false);
  fgSetArchivable("/position/longitude-deg");
  _tiedProperties.Tie("/position/altitude-ft", this,
                      &FGInterface::get_Altitude,
                      &FGInterface::set_Altitude,
                      false);
  fgSetArchivable("/position/altitude-ft");
  _tiedProperties.Tie("/position/altitude-agl-ft", this,
                      &FGInterface::get_Altitude_AGL, &FGInterface::set_AltitudeAGL, false);
  fgSetArchivable("/position/ground-elev-ft");
  _tiedProperties.Tie("/position/ground-elev-ft", this,
                      &FGInterface::get_Runway_altitude); // read-only
  fgSetArchivable("/position/ground-elev-m");
  _tiedProperties.Tie("/position/ground-elev-m", this,
                      &FGInterface::get_Runway_altitude_m); // read-only
  _tiedProperties.Tie("/environment/ground-elevation-m", this,
                      &FGInterface::get_Runway_altitude_m); // read-only
  fgSetArchivable("/position/sea-level-radius-ft");
  _tiedProperties.Tie("/position/sea-level-radius-ft", this,
                      &FGInterface::get_Sea_level_radius,
                      &FGInterface::_set_Sea_level_radius, false);

  // Orientation
  _tiedProperties.Tie("/orientation/roll-deg", this,
                      &FGInterface::get_Phi_deg,
                      &FGInterface::set_Phi_deg, false);
  fgSetArchivable("/orientation/roll-deg");
  _tiedProperties.Tie("/orientation/pitch-deg", this,
                      &FGInterface::get_Theta_deg,
                      &FGInterface::set_Theta_deg, false);
  fgSetArchivable("/orientation/pitch-deg");
  _tiedProperties.Tie("/orientation/heading-deg", this,
                      &FGInterface::get_Psi_deg,
                      &FGInterface::set_Psi_deg, false);
  fgSetArchivable("/orientation/heading-deg");
  _tiedProperties.Tie("/orientation/track-deg", this,
                      &FGInterface::get_Track); // read-only

  // Body-axis "euler rates" (rotation speed, but in a funny
  // representation).
  _tiedProperties.Tie("/orientation/roll-rate-degps", this,
                      &FGInterface::get_Phi_dot_degps,
                      &FGInterface::set_Phi_dot_degps, false);
  _tiedProperties.Tie("/orientation/pitch-rate-degps", this,
                      &FGInterface::get_Theta_dot_degps,
                      &FGInterface::set_Theta_dot_degps, false);
  _tiedProperties.Tie("/orientation/yaw-rate-degps", this,
                      &FGInterface::get_Psi_dot_degps,
                      &FGInterface::set_Psi_dot_degps, false);

  _tiedProperties.Tie("/orientation/p-body", this, &FGInterface::get_P_body); // read-only
  _tiedProperties.Tie("/orientation/q-body", this, &FGInterface::get_Q_body); // read-only
  _tiedProperties.Tie("/orientation/r-body", this, &FGInterface::get_R_body); // read-only

  // Ground speed knots
  _tiedProperties.Tie("/velocities/groundspeed-kt", this,
                      &FGInterface::get_V_ground_speed_kt,
                      &FGInterface::set_V_ground_speed_kt); // read-only

  // Calibrated airspeed
  _tiedProperties.Tie("/velocities/airspeed-kt", this,
                      &FGInterface::get_V_calibrated_kts,
                      &FGInterface::set_V_calibrated_kts,
                      false);

  _tiedProperties.Tie("/velocities/equivalent-kt", this,
                      &FGInterface::get_V_equiv_kts); // read-only

  // Mach number
  _tiedProperties.Tie("/velocities/mach", this,
                      &FGInterface::get_Mach_number,
                      &FGInterface::set_Mach_number,
                      false);

  // Local velocities
//   _tiedProperties.Tie("/velocities/speed-north-fps", this,
//                       &FGInterface::get_V_north,
//                       &FGInterface::set_V_north);
//   fgSetArchivable("/velocities/speed-north-fps");
//   _tiedProperties.Tie("/velocities/speed-east-fps", this,
//                       &FGInterface::get_V_east,
//                       &FGInterface::set_V_east);
//   fgSetArchivable("/velocities/speed-east-fps");
//   _tiedProperties.Tie("/velocities/speed-down-fps", this,
//                       &FGInterface::get_V_down,
//                       &FGInterface::set_V_down);
//   fgSetArchivable("/velocities/speed-down-fps");

  // FIXME: Temporarily read-only, until the
  // incompatibilities between JSBSim and
  // LaRCSim are fixed (LaRCSim adds the
  // earth's rotation to the east velocity).
  _tiedProperties.Tie("/velocities/speed-north-fps", this,
                      &FGInterface::get_V_north, &FGInterface::set_V_north, false);
  _tiedProperties.Tie("/velocities/speed-east-fps", this,
                      &FGInterface::get_V_east, &FGInterface::set_V_east, false);
  _tiedProperties.Tie("/velocities/speed-down-fps", this,
                      &FGInterface::get_V_down, &FGInterface::set_V_down, false);

  _tiedProperties.Tie("/velocities/north-relground-fps", this,
                      &FGInterface::get_V_north_rel_ground); // read-only
  _tiedProperties.Tie("/velocities/east-relground-fps", this,
                      &FGInterface::get_V_east_rel_ground); // read-only
  _tiedProperties.Tie("/velocities/down-relground-fps", this,
                      &FGInterface::get_V_down_rel_ground); // read-only

  // Relative wind
  // FIXME: temporarily archivable, until the NED problem is fixed.
  _tiedProperties.Tie("/velocities/uBody-fps", this,
                      &FGInterface::get_uBody,
                      &FGInterface::set_uBody,
                      false);
  fgSetArchivable("/velocities/uBody-fps");
  _tiedProperties.Tie("/velocities/vBody-fps", this,
                      &FGInterface::get_vBody,
                      &FGInterface::set_vBody,
                      false);
  fgSetArchivable("/velocities/vBody-fps");
  _tiedProperties.Tie("/velocities/wBody-fps", this,
                      &FGInterface::get_wBody,
                      &FGInterface::set_wBody,
                      false);
  fgSetArchivable("/velocities/wBody-fps");

  // Climb and slip (read-only)
  _tiedProperties.Tie("/velocities/vertical-speed-fps", this,
                      &FGInterface::get_Climb_Rate,
                      &FGInterface::set_Climb_Rate, false );
  _tiedProperties.Tie("/velocities/glideslope", this,
                      &FGInterface::get_Gamma_vert_rad,
                      &FGInterface::set_Gamma_vert_rad, false );
  _tiedProperties.Tie("/orientation/side-slip-rad", this,
                      &FGInterface::get_Beta, &FGInterface::_set_Beta, false);
  _tiedProperties.Tie("/orientation/side-slip-deg", this,
                      &FGInterface::get_Beta_deg); // read-only
  _tiedProperties.Tie("/orientation/alpha-deg", this,
                      &FGInterface::get_Alpha_deg, &FGInterface::set_Alpha_deg, false);
  _tiedProperties.Tie("/accelerations/nlf", this,
                      &FGInterface::get_Nlf); // read-only

  // NED accelerations
  _tiedProperties.Tie("/accelerations/ned/north-accel-fps_sec",
                      this, &FGInterface::get_V_dot_north); // read-only
  _tiedProperties.Tie("/accelerations/ned/east-accel-fps_sec",
                      this, &FGInterface::get_V_dot_east); // read-only
  _tiedProperties.Tie("/accelerations/ned/down-accel-fps_sec",
                      this, &FGInterface::get_V_dot_down); // read-only

  // Pilot accelerations
  _tiedProperties.Tie("/accelerations/pilot/x-accel-fps_sec",
                      this, &FGInterface::get_A_X_pilot, &FGInterface::set_A_X_pilot, false);
  _tiedProperties.Tie("/accelerations/pilot/y-accel-fps_sec",
                      this, &FGInterface::get_A_Y_pilot, &FGInterface::set_A_Y_pilot, false);
  _tiedProperties.Tie("/accelerations/pilot/z-accel-fps_sec",
                      this, &FGInterface::get_A_Z_pilot, &FGInterface::set_A_Z_pilot, false);

  _tiedProperties.Tie("/accelerations/n-z-cg-fps_sec",
                      this, &FGInterface::get_N_Z_cg); // read-only
}


/**
 * Unbind any properties bound to this FDM.
 *
 * This method allows the FDM to release properties so that a new
 * FDM can bind them instead.
 */
void
FGInterface::unbind ()
{
  _tiedProperties.Untie();
  bound = false;
}

/**
 * Update the state of the FDM (i.e. run the equations of motion).
 */
void
FGInterface::update (double dt)
{
    SG_LOG(SG_FLIGHT, SG_ALERT, "dummy update() ... SHOULDN'T BE CALLED!");
}


void FGInterface::_updatePositionM(const SGVec3d& cartPos)
{
    TrackComputer tracker( track, geodetic_position_v );
    cartesian_position_v = cartPos;
    geodetic_position_v = SGGeod::fromCart(cartesian_position_v);
    geocentric_position_v = SGGeoc::fromCart(cartesian_position_v);
    _set_Sea_level_radius( SGGeodesy::SGGeodToSeaLevelRadius(geodetic_position_v)*SG_METER_TO_FEET );
    _update_ground_elev_at_pos();
}


void FGInterface::_updatePosition(const SGGeod& geod)
{
    TrackComputer tracker( track, geodetic_position_v );
    geodetic_position_v = geod;
    cartesian_position_v = SGVec3d::fromGeod(geodetic_position_v);
    geocentric_position_v = SGGeoc::fromCart(cartesian_position_v);

    _set_Sea_level_radius( SGGeodesy::SGGeodToSeaLevelRadius(geodetic_position_v)*SG_METER_TO_FEET );
    _update_ground_elev_at_pos();
}


void FGInterface::_updatePosition(const SGGeoc& geoc)
{
    TrackComputer tracker( track, geodetic_position_v );
    geocentric_position_v = geoc;
    cartesian_position_v = SGVec3d::fromGeoc(geocentric_position_v);
    geodetic_position_v = SGGeod::fromCart(cartesian_position_v);

    _set_Sea_level_radius( SGGeodesy::SGGeodToSeaLevelRadius(geodetic_position_v)*SG_METER_TO_FEET );
    _update_ground_elev_at_pos();
}


void FGInterface::_updateGeodeticPosition( double lat, double lon, double alt )
{
    _updatePosition(SGGeod::fromRadFt(lon, lat, alt));
}


void FGInterface::_updateGeocentricPosition( double lat, double lon,
                                             double alt )
{
    _updatePosition(SGGeoc::fromRadFt(lon, lat, get_Sea_level_radius() + alt));
}

void FGInterface::_update_ground_elev_at_pos( void ) {
    double groundlevel_m = get_groundlevel_m(geodetic_position_v);
    _set_Runway_altitude( groundlevel_m * SG_METER_TO_FEET );
}

// Positions
void FGInterface::set_Latitude(double lat) {
    geodetic_position_v.setLatitudeRad(lat);
}

void FGInterface::set_Longitude(double lon) {
    geodetic_position_v.setLongitudeRad(lon);
}

void FGInterface::set_Altitude(double alt) {
    geodetic_position_v.setElevationFt(alt);
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


void FGInterface::_busdump(void)
{
    SG_LOG(SG_FLIGHT,SG_INFO,"d_cg_rp_body_v: " << d_cg_rp_body_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"v_dot_local_v: " << v_dot_local_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"v_dot_body_v: " << v_dot_body_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"a_cg_body_v: " << a_cg_body_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"a_pilot_body_v: " << a_pilot_body_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"n_cg_body_v: " << n_cg_body_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"v_local_v: " << v_local_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"v_local_rel_ground_v: " << v_local_rel_ground_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"v_local_airmass_v: " << v_local_airmass_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"v_wind_body_v: " << v_wind_body_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"omega_body_v: " << omega_body_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"euler_rates_v: " << euler_rates_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"geocentric_rates_v: " << geocentric_rates_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"geocentric_position_v: " << geocentric_position_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"geodetic_position_v: " << geodetic_position_v);
    SG_LOG(SG_FLIGHT,SG_INFO,"euler_angles_v: " << euler_angles_v);

    SG_LOG(SG_FLIGHT,SG_INFO,"nlf: " << nlf );
    SG_LOG(SG_FLIGHT,SG_INFO,"v_rel_wind: " << v_rel_wind );
    SG_LOG(SG_FLIGHT,SG_INFO,"v_true_kts: " << v_true_kts );
    SG_LOG(SG_FLIGHT,SG_INFO,"v_ground_speed: " << v_ground_speed );
    SG_LOG(SG_FLIGHT,SG_INFO,"v_equiv_kts: " << v_equiv_kts );
    SG_LOG(SG_FLIGHT,SG_INFO,"v_calibrated_kts: " << v_calibrated_kts );
    SG_LOG(SG_FLIGHT,SG_INFO,"alpha: " << alpha );
    SG_LOG(SG_FLIGHT,SG_INFO,"beta: " << beta );
    SG_LOG(SG_FLIGHT,SG_INFO,"gamma_vert_rad: " << gamma_vert_rad );
    SG_LOG(SG_FLIGHT,SG_INFO,"density: " << density );
    SG_LOG(SG_FLIGHT,SG_INFO,"mach_number: " << mach_number );
    SG_LOG(SG_FLIGHT,SG_INFO,"static_pressure: " << static_pressure );
    SG_LOG(SG_FLIGHT,SG_INFO,"total_pressure: " << total_pressure );
    SG_LOG(SG_FLIGHT,SG_INFO,"dynamic_pressure: " << dynamic_pressure );
    SG_LOG(SG_FLIGHT,SG_INFO,"static_temperature: " << static_temperature );
    SG_LOG(SG_FLIGHT,SG_INFO,"total_temperature: " << total_temperature );
    SG_LOG(SG_FLIGHT,SG_INFO,"sea_level_radius: " << sea_level_radius );
    SG_LOG(SG_FLIGHT,SG_INFO,"earth_position_angle: " << earth_position_angle );
    SG_LOG(SG_FLIGHT,SG_INFO,"runway_altitude: " << runway_altitude );
    SG_LOG(SG_FLIGHT,SG_INFO,"climb_rate: " << climb_rate );
    SG_LOG(SG_FLIGHT,SG_INFO,"altitude_agl: " << altitude_agl );
}

bool
FGInterface::prepare_ground_cache_m(double startSimTime, double endSimTime,
                                    const double pt[3], double rad)
{
  return ground_cache.prepare_ground_cache(startSimTime, endSimTime,
                                           SGVec3d(pt), rad);
}

bool
FGInterface::prepare_ground_cache_ft(double startSimTime, double endSimTime,
                                     const double pt[3], double rad)
{
  // Convert units and do the real work.
  SGVec3d pt_ft = SG_FEET_TO_METER*SGVec3d(pt);
  return ground_cache.prepare_ground_cache(startSimTime, endSimTime,
                                           pt_ft, rad*SG_FEET_TO_METER);
}

bool
FGInterface::is_valid_m(double *ref_time, double pt[3], double *rad)
{
  SGVec3d _pt;
  bool valid = ground_cache.is_valid(*ref_time, _pt, *rad);
  assign(pt, _pt);
  return valid;
}

bool FGInterface::is_valid_ft(double *ref_time, double pt[3], double *rad)
{
  // Convert units and do the real work.
  SGVec3d _pt;
  bool found_ground = ground_cache.is_valid(*ref_time, _pt, *rad);
  assign(pt, SG_METER_TO_FEET*_pt);
  *rad *= SG_METER_TO_FEET;
  return found_ground;
}

double
FGInterface::get_cat_m(double t, const double pt[3],
                       double end[2][3], double vel[2][3])
{
  SGVec3d _end[2], _vel[2];
  double dist = ground_cache.get_cat(t, SGVec3d(pt), _end, _vel);
  for (int k=0; k<2; ++k) {
    assign( end[k], _end[k] );
    assign( vel[k], _vel[k] );
  }
  return dist;
}

double
FGInterface::get_cat_ft(double t, const double pt[3],
                        double end[2][3], double vel[2][3])
{
  // Convert units and do the real work.
  SGVec3d pt_m = SG_FEET_TO_METER*SGVec3d(pt);
  SGVec3d _end[2], _vel[2];
  double dist = ground_cache.get_cat(t, pt_m, _end, _vel);
  for (int k=0; k<2; ++k) {
    assign( end[k], SG_METER_TO_FEET*_end[k] );
    assign( vel[k], SG_METER_TO_FEET*_vel[k] );
  }
  return dist*SG_METER_TO_FEET;
}

bool
FGInterface::get_body_m(double t, simgear::BVHNode::Id id,
                        double bodyToWorld[16], double linearVel[3],
                        double angularVel[3])
{
  SGMatrixd _bodyToWorld;
  SGVec3d _linearVel, _angularVel;
  if (!ground_cache.get_body(t, _bodyToWorld, _linearVel, _angularVel, id))
    return false;

  assign(linearVel, _linearVel);
  assign(angularVel, _angularVel);
  for (unsigned i = 0; i < 16; ++i)
      bodyToWorld[i] = _bodyToWorld.data()[i];

  return true;
}

bool
FGInterface::get_agl_m(double t, const double pt[3], double max_altoff,
                       double contact[3], double normal[3],
                       double linearVel[3], double angularVel[3],
                       SGMaterial const*& material, simgear::BVHNode::Id& id)
{
  SGVec3d pt_m = SGVec3d(pt) - max_altoff*ground_cache.get_down();
  SGVec3d _contact, _normal, _linearVel, _angularVel;
  material = 0;
  bool ret = ground_cache.get_agl(t, pt_m, _contact, _normal, _linearVel,
                                  _angularVel, id, material);
  // correct the linear velocity, since the line intersector delivers
  // values for the start point and the get_agl function should
  // traditionally deliver for the contact point
  _linearVel += cross(_angularVel, _contact - pt_m);

  assign(contact, _contact);
  assign(normal, _normal);
  assign(linearVel, _linearVel);
  assign(angularVel, _angularVel);
  return ret;
}

bool
FGInterface::get_agl_ft(double t, const double pt[3], double max_altoff,
                        double contact[3], double normal[3],
                        double linearVel[3], double angularVel[3],
                        SGMaterial const*& material, simgear::BVHNode::Id& id)
{
  // Convert units and do the real work.
  SGVec3d pt_m = SGVec3d(pt) - max_altoff*ground_cache.get_down();
  pt_m *= SG_FEET_TO_METER;
  SGVec3d _contact, _normal, _linearVel, _angularVel;
  material = 0;
  bool ret = ground_cache.get_agl(t, pt_m, _contact, _normal, _linearVel,
                                  _angularVel, id, material);
  // correct the linear velocity, since the line intersector delivers
  // values for the start point and the get_agl function should
  // traditionally deliver for the contact point
  _linearVel += cross(_angularVel, _contact - pt_m);

  // Convert units back ...
  assign( contact, SG_METER_TO_FEET*_contact );
  assign( normal, _normal );
  assign( linearVel, SG_METER_TO_FEET*_linearVel );
  assign( angularVel, _angularVel );
  return ret;
}

bool
FGInterface::get_nearest_m(double t, const double pt[3], double maxDist,
                           double contact[3], double normal[3],
                           double linearVel[3], double angularVel[3],
                           SGMaterial const*& material,
                           simgear::BVHNode::Id& id)
{
  SGVec3d _contact, _linearVel, _angularVel;
  if (!ground_cache.get_nearest(t, SGVec3d(pt), maxDist, _contact, _linearVel,
                                _angularVel, id, material))
      return false;

  assign(contact, _contact);
  assign(linearVel, _linearVel);
  assign(angularVel, _angularVel);
  return true;
}

bool
FGInterface::get_nearest_ft(double t, const double pt[3], double maxDist,
                            double contact[3], double normal[3],
                            double linearVel[3], double angularVel[3],
                            SGMaterial const*& material,
                            simgear::BVHNode::Id& id)
{
  SGVec3d _contact, _linearVel, _angularVel;
  if (!ground_cache.get_nearest(t, SG_FEET_TO_METER*SGVec3d(pt),
                                SG_FEET_TO_METER*maxDist, _contact, _linearVel,
                                _angularVel, id, material))
      return false;

  assign(contact, SG_METER_TO_FEET*_contact);
  assign(linearVel, SG_METER_TO_FEET*_linearVel);
  assign(angularVel, _angularVel);
  return true;
}

double
FGInterface::get_groundlevel_m(double lat, double lon, double alt)
{
  return get_groundlevel_m(SGGeod::fromRadM(lon, lat, alt));
}

double
FGInterface::get_groundlevel_m(const SGGeod& geod)
{
  // Compute the cartesian position of the given lat/lon/alt.
  SGVec3d pos = SGVec3d::fromGeod(geod);

  // FIXME: how to handle t - ref_time differences ???
  SGVec3d cpos;
  double ref_time = 0, radius;
  // Prepare the ground cache for that position.
  if (!is_valid_m(&ref_time, cpos.data(), &radius)) {
    double startTime = ref_time;
    double endTime = startTime + 1;
    bool ok = prepare_ground_cache_m(startTime, endTime, pos.data(), 10);
    /// This is most likely the case when the given altitude is
    /// too low, try with a new altitude of 10000m, that should be
    /// sufficient to find a ground level below everywhere on our planet
    if (!ok) {
      pos = SGVec3d::fromGeod(SGGeod::fromGeodM(geod, 10000));
      /// If there is still no ground, return sea level radius
      if (!prepare_ground_cache_m(startTime, endTime, pos.data(), 10))
        return 0;
    }
  } else if (radius*radius <= distSqr(pos, cpos)) {
    double startTime = ref_time;
    double endTime = startTime + 1;

    /// We reuse the old radius value, but only if it is at least 10 Meters ..
    if (!(10 < radius)) // Well this strange compare is nan safe
      radius = 10;

    bool ok = prepare_ground_cache_m(startTime, endTime, pos.data(), radius);
    /// This is most likely the case when the given altitude is
    /// too low, try with a new altitude of 10000m, that should be
    /// sufficient to find a ground level below everywhere on our planet
    if (!ok) {
      pos = SGVec3d::fromGeod(SGGeod::fromGeodM(geod, 10000));
      /// If there is still no ground, return sea level radius
      if (!prepare_ground_cache_m(startTime, endTime, pos.data(), radius))
        return 0;
    }
  }
  
  double contact[3], normal[3], vel[3], angvel[3];
  const SGMaterial* material;
  simgear::BVHNode::Id id;
  // Ignore the return value here, since it just tells us if
  // the returns stem from the groundcache or from the coarse
  // computations below the groundcache. The contact point is still something
  // valid, the normals and the other returns just contain some defaults.
  get_agl_m(ref_time, pos.data(), 2.0, contact, normal, vel, angvel,
            material, id);
  return SGGeod::fromCart(SGVec3d(contact)).getElevationM();
}
  
bool
FGInterface::caught_wire_m(double t, const double pt[4][3])
{
  SGVec3d pt_m[4];
  for (int i=0; i<4; ++i)
    pt_m[i] = SGVec3d(pt[i]);
  
  return ground_cache.caught_wire(t, pt_m);
}

bool
FGInterface::caught_wire_ft(double t, const double pt[4][3])
{
  // Convert units and do the real work.
  SGVec3d pt_m[4];
  for (int i=0; i<4; ++i)
    pt_m[i] = SG_FEET_TO_METER*SGVec3d(pt[i]);
    
  return ground_cache.caught_wire(t, pt_m);
}
  
bool
FGInterface::get_wire_ends_m(double t, double end[2][3], double vel[2][3])
{
  SGVec3d _end[2], _vel[2];
  bool ret = ground_cache.get_wire_ends(t, _end, _vel);
  for (int k=0; k<2; ++k) {
    assign( end[k], _end[k] );
    assign( vel[k], _vel[k] );
  }
  return ret;
}

bool
FGInterface::get_wire_ends_ft(double t, double end[2][3], double vel[2][3])
{
  // Convert units and do the real work.
  SGVec3d _end[2], _vel[2];
  bool ret = ground_cache.get_wire_ends(t, _end, _vel);
  for (int k=0; k<2; ++k) {
    assign( end[k], SG_METER_TO_FEET*_end[k] );
    assign( vel[k], SG_METER_TO_FEET*_vel[k] );
  }
  return ret;
}

void
FGInterface::release_wire(void)
{
  ground_cache.release_wire();
}

