// flightProperties.hxx -- expose FDM properties via helper methods
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

#ifndef FG_FLIGHT_PROPERTIES_HXX
#define FG_FLIGHT_PROPERTIES_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstring>
#include <simgear/math/SGMathFwd.hxx> // for SGVec3d 
#include <simgear/math/SGMisc.hxx>

// forward decls
class SGPropertyNode;
class SGGeoc;
class SGGeod;

/**
 * Encapsulate the FDM properties in some getter/setter helpers.
 * This class intentionally mimics portions of
 * @FGInterface, to permit easy migration of code outside the FDMs,
 * to use properties instead of global variables.
 */
class FlightProperties
{
public:
  FlightProperties(SGPropertyNode* aRoot = NULL);
  ~FlightProperties();

  double get_V_north() const;
  double get_V_east() const;
  double get_V_down() const;
  double get_uBody () const;
  double get_vBody () const;
  double get_wBody () const;
  
  double get_A_X_pilot() const;
  double get_A_Y_pilot() const;
  double get_A_Z_pilot() const;
  
  double get_P_body() const;
  double get_Q_body() const;
  double get_R_body() const;
    
  SGGeod getPosition() const;

  double get_Latitude() const;
  double get_Longitude() const;
  double get_Altitude() const;
  
  double get_Altitude_AGL(void) const;
  double get_Track(void) const;

  double get_Latitude_deg () const;
  double get_Longitude_deg () const;
  
  double get_Phi_deg() const;
  double get_Theta_deg() const;
  double get_Psi_deg() const;
  
  double get_Phi() const { return SGMiscd::deg2rad(get_Phi_deg()); }
  double get_Theta() const { return SGMiscd::deg2rad(get_Theta_deg()); }
  double get_Psi() const { return SGMiscd::deg2rad(get_Psi_deg()); }

  double get_Phi_dot() const;
  double get_Theta_dot() const;
  double get_Psi_dot() const;
  double get_Alpha() const;
  double get_Beta() const;
  
  double get_Phi_dot_degps() const;
  double get_Theta_dot_degps() const;
  double get_Psi_dot_degps() const;
  
  double get_V_ground_speed() const; // in feet/s
  double get_V_equiv_kts() const;
  double get_V_calibrated_kts() const;
  double get_Climb_Rate() const;
  double get_Runway_altitude_m() const;
  
  double get_Total_temperature() const;
  double get_Total_pressure() const;
  double get_Dynamic_pressure() const;
  
  void set_Longitude(double l); // radians
  void set_Latitude(double l); // radians
  void set_Altitude(double ft); // feet
    
  void set_Euler_Angles(double phi, double theta, double psi);
  void set_Euler_Rates(double x, double y, double z);
  
  void set_Alpha(double a);
  void set_Beta(double b);
  
  void set_Altitude_AGL(double ft);
  
  void set_V_calibrated_kts(double kts);
  void set_Climb_Rate(double fps);
  
  void set_Velocities_Local(double x, double y, double z);
  void set_Velocities_Wind_Body(double x, double y, double z);
  void set_Accels_Pilot_Body(double x, double y, double z);
private:
  SGPropertyNode* _root;
};

#endif // of FG_FLIGHT_PROPERTIES_HXX
