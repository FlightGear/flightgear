// flightProperties.cxx -- expose FDM properties via helper methods
//
// Written by James Turner, started June 2010.
//
// Copyright (C) 2010  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <FDM/flightProperties.hxx>

#include <simgear/props/props.hxx>
#include <simgear/math/SGMath.hxx>

#include <Main/globals.hxx>

FlightProperties::FlightProperties(SGPropertyNode* root) :
  _root(root)
{
  if (!_root) {
    _root = globals->get_props();
  }
}

FlightProperties::~FlightProperties()
{
}

double FlightProperties::get_V_north() const
{
  return _root->getDoubleValue("velocities/speed-north-fps", 0.0);
}

double FlightProperties::get_V_east() const
{
  return _root->getDoubleValue("velocities/speed-east-fps", 0.0);
}

double FlightProperties::get_V_down() const
{
  return _root->getDoubleValue("velocities/speed-down-fps", 0.0);
}

double FlightProperties::get_uBody () const
{
  return _root->getDoubleValue("velocities/uBody-fps", 0.0);
}

double FlightProperties::get_vBody () const
{
  return _root->getDoubleValue("velocities/vBody-fps", 0.0);
}

double FlightProperties::get_wBody () const
{
  return _root->getDoubleValue("velocities/wBody-fps", 0.0);
}

double FlightProperties::get_A_X_pilot() const
{
  return _root->getDoubleValue("accelerations/pilot/x-accel-fps_sec", 0.0);
}

double FlightProperties::get_A_Y_pilot() const
{
  return _root->getDoubleValue("/accelerations/pilot/y-accel-fps_sec", 0.0);
}

double FlightProperties::get_A_Z_pilot() const
{
  return _root->getDoubleValue("/accelerations/pilot/z-accel-fps_sec", 0.0);
}

SGGeod FlightProperties::getPosition() const
{
  return SGGeod::fromDegFt(get_Longitude_deg(), get_Latitude_deg(), get_Altitude());
}

double FlightProperties::get_Latitude() const
{
  return get_Latitude_deg() * SG_DEGREES_TO_RADIANS;
}

double FlightProperties::get_Longitude() const
{
  return get_Longitude_deg() * SG_DEGREES_TO_RADIANS;
}

double FlightProperties::get_Altitude() const
{
  return _root->getDoubleValue("position/altitude-ft");
}

double FlightProperties::get_Altitude_AGL(void) const
{
  return _root->getDoubleValue("position/altitude-agl-ft");
}

double FlightProperties::get_Latitude_deg () const
{
  return _root->getDoubleValue("position/latitude-deg");
}

double FlightProperties::get_Longitude_deg () const
{
  return _root->getDoubleValue("position/longitude-deg");
}

double FlightProperties::get_Track(void) const
{
  return _root->getDoubleValue("orientation/track-deg");
}

double FlightProperties::get_Phi_deg() const
{
  return _root->getDoubleValue("orientation/roll-deg");
}

double FlightProperties::get_Theta_deg() const
{
  return _root->getDoubleValue("orientation/pitch-deg");
}

double FlightProperties::get_Psi_deg() const
{
  return _root->getDoubleValue("orientation/heading-deg");
}

double FlightProperties::get_Phi_dot() const
{
  return get_Phi_dot_degps() * SG_DEGREES_TO_RADIANS;
}

double FlightProperties::get_Theta_dot() const
{
   return get_Theta_dot_degps() * SG_DEGREES_TO_RADIANS;
}

double FlightProperties::get_Psi_dot() const
{
   return get_Psi_dot_degps() * SG_DEGREES_TO_RADIANS;
}

double FlightProperties::get_Alpha() const
{
  return _root->getDoubleValue("orientation/alpha-deg") * SG_DEGREES_TO_RADIANS;
}

double FlightProperties::get_Beta() const
{
  return _root->getDoubleValue("orientation/beta-deg") * SG_DEGREES_TO_RADIANS;
}

double FlightProperties::get_Phi_dot_degps() const
{
  return _root->getDoubleValue("orientation/roll-rate-degps");
}

double FlightProperties::get_Theta_dot_degps() const
{
  return _root->getDoubleValue("orientation/pitch-rate-degps");
}

double FlightProperties::get_Psi_dot_degps() const
{
  return _root->getDoubleValue("orientation/yaw-rate-degps");
}
  
double FlightProperties::get_Total_temperature() const
{
  return 0.0;
}

double FlightProperties::get_Total_pressure() const
{
  return 0.0;
}

double FlightProperties::get_Dynamic_pressure() const
{
  return 0.0;
}

void FlightProperties::set_Longitude(double l)
{
  _root->setDoubleValue("position/longitude-deg", l * SG_RADIANS_TO_DEGREES);
}

void FlightProperties::set_Latitude(double l)
{
  _root->setDoubleValue("position/latitude-deg", l * SG_RADIANS_TO_DEGREES);
}

void FlightProperties::set_Altitude(double ft)
{
  _root->setDoubleValue("position/altitude-ft", ft);
}

void FlightProperties::set_Euler_Angles(double phi, double theta, double psi)
{
  _root->setDoubleValue("orientation/roll-deg", phi * SG_RADIANS_TO_DEGREES);
  _root->setDoubleValue("orientation/pitch-deg", theta * SG_RADIANS_TO_DEGREES);
  _root->setDoubleValue("orientation/heading-deg", psi * SG_RADIANS_TO_DEGREES);
}

void FlightProperties::set_V_calibrated_kts(double kts)
{
  _root->setDoubleValue("velocities/airspeed-kt", kts);
}

void FlightProperties::set_Climb_Rate(double fps)
{
  _root->setDoubleValue("velocities/vertical-speed-fps", fps);
}

double FlightProperties::get_V_ground_speed() const
{
  const double KNOTS_TO_FTS = (SG_NM_TO_METER * SG_METER_TO_FEET)/ 3600.0;
  return _root->getDoubleValue("velocities/groundspeed-kt") * KNOTS_TO_FTS;
}

double FlightProperties::get_V_calibrated_kts() const
{
  return _root->getDoubleValue("velocities/airspeed-kt");
}

double FlightProperties::get_V_equiv_kts() const
{
  return _root->getDoubleValue("velocities/equivalent-kt");
}

double FlightProperties::get_Climb_Rate() const
{
  return _root->getDoubleValue("velocities/vertical-speed-fps");
}

double FlightProperties::get_Runway_altitude_m() const
{
  return _root->getDoubleValue("environment/ground-elevation-m");
}

void FlightProperties::set_Accels_Pilot_Body(double x, double y, double z)
{
  _root->setDoubleValue("accelerations/pilot/x-accel-fps_sec", x);
  _root->setDoubleValue("accelerations/pilot/y-accel-fps_sec", y);
  _root->setDoubleValue("accelerations/pilot/z-accel-fps_sec", z);
}

void FlightProperties::set_Velocities_Local(double x, double y, double z)
{
  _root->setDoubleValue("velocities/speed-north-fps", x);
  _root->setDoubleValue("velocities/speed-east-fps", y);
  _root->setDoubleValue("velocities/speed-down-fps", z);
}

void FlightProperties::set_Velocities_Wind_Body(double x, double y, double z)
{
  _root->setDoubleValue("velocities/vBody-fps", x);
  _root->setDoubleValue("velocities/uBody-fps", y);
  _root->setDoubleValue("velocities/wBody-fps", z);
}

void FlightProperties::set_Euler_Rates(double x, double y, double z)
{
  _root->setDoubleValue("orientation/roll-rate-degps", x * SG_RADIANS_TO_DEGREES);
  _root->setDoubleValue("orientation/pitch-rate-degps", y * SG_RADIANS_TO_DEGREES);
  _root->setDoubleValue("orientation/yaw-rate-degps", z * SG_RADIANS_TO_DEGREES);
}

void FlightProperties::set_Alpha(double a)
{
  _root->setDoubleValue("orientation/alpha-deg", a * SG_RADIANS_TO_DEGREES);
}

void FlightProperties::set_Beta(double b)
{
  _root->setDoubleValue("orientation/side-slip-rad", b);
}

void FlightProperties::set_Altitude_AGL(double ft)
{
  _root->setDoubleValue("position/altitude-agl-ft", ft);
}

double FlightProperties::get_P_body() const
{
  return _root->getDoubleValue("orientation/p-body", 0.0);
}

double FlightProperties::get_Q_body() const
{
  return _root->getDoubleValue("orientation/q-body", 0.0);
}

double FlightProperties::get_R_body() const
{
  return _root->getDoubleValue("orientation/r-body", 0.0);
}
