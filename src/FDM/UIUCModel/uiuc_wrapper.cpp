/********************************************************************** 
 
 FILENAME:     uiuc_wrapper.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  A wrapper(interface) between the UIUC Aeromodel (C++ files) 
               and the LaRCsim FDM (C files)

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------

 HISTORY:      01/26/2000   initial release
               03/09/2001   (DPM) added support for gear
               06/18/2001   (RD) Made uiuc_recorder its own routine.
	       07/19/2001   (RD) Added uiuc_vel_init() to initialize
	                    the velocities.
	       08/27/2001   (RD) Added uiuc_initial_init() to help
	                    in starting an A/C at an initial condition
 
----------------------------------------------------------------------
 
 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Robert Deters      <rdeters@uiuc.edu>
               David Megginson    <david@megginson.com>
 
----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       *

----------------------------------------------------------------------

 OUTPUTS:      *

----------------------------------------------------------------------
 
 CALLED BY:    *
 
----------------------------------------------------------------------
 
 CALLS TO:     *
 
----------------------------------------------------------------------
 
 COPYRIGHT:    (C) 2000 by Michael Selig
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.
 
**********************************************************************/

#include <simgear/compiler.h>
#include <Aircraft/aircraft.hxx>

#ifndef FG_OLD_WEATHER
#include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#include <Weather/weather.hxx>
#endif

#include "uiuc_aircraft.h"
#include "uiuc_aircraftdir.h"
#include "uiuc_coefficients.h"
#include "uiuc_engine.h"
#include "uiuc_gear.h"
#include "uiuc_aerodeflections.h"
#include "uiuc_recorder.h"
#include "uiuc_menu.h"
#include "uiuc_betaprobe.h"
#include <FDM/LaRCsim/ls_generic.h>
// #include "Main/simple_udp.h"
#include "uiuc_fog.h" //321654

#if !defined (SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cout);
SG_USING_STD(endl);
#endif

extern "C" void uiuc_init_aeromodel ();
extern "C" void uiuc_force_moment(double dt);
extern "C" void uiuc_engine_routine();
extern "C" void uiuc_gear_routine();
extern "C" void uiuc_record_routine(double dt);
extern "C" void uiuc_vel_init ();
extern "C" void uiuc_initial_init ();

AIRCRAFT *aircraft_ = new AIRCRAFT;
AIRCRAFTDIR *aircraftdir_ = new AIRCRAFTDIR;

// SendArray testarray(4950);

/* Convert float to string */
string ftoa(double in)
{
  static char temp[20];
  sprintf(temp,"%g",in);
  return (string)temp;
}

void uiuc_initial_init ()
{
  if (P_body_init_true)
    P_body = P_body_init;
  if (Q_body_init_true)
    Q_body = Q_body_init;
  if (R_body_init_true)
    R_body = R_body_init;

  if (Phi_init_true)
    Phi = Phi_init;
  if (Theta_init_true)
    Theta = Theta_init;
  if (Psi_init_true)
    Psi = Psi_init;

  if (U_body_init_true)
    U_body = U_body_init;
  if (V_body_init_true)
    V_body = V_body_init;
  if (W_body_init_true)
    W_body = W_body_init;

}

void uiuc_vel_init ()
{
  if (U_body_init_true && V_body_init_true && W_body_init_true)
    {
  double det_T_l_to_b, cof11, cof12, cof13, cof21, cof22, cof23, cof31, cof32, cof33;

  det_T_l_to_b = T_local_to_body_11*(T_local_to_body_22*T_local_to_body_33-T_local_to_body_23*T_local_to_body_32) - T_local_to_body_12*(T_local_to_body_21*T_local_to_body_33-T_local_to_body_23*T_local_to_body_31) + T_local_to_body_13*(T_local_to_body_21*T_local_to_body_32-T_local_to_body_22*T_local_to_body_31);
  cof11 = T_local_to_body_22 * T_local_to_body_33 - T_local_to_body_23 * T_local_to_body_32;
  cof12 = T_local_to_body_23 * T_local_to_body_31 - T_local_to_body_21 * T_local_to_body_33;
  cof13 = T_local_to_body_21 * T_local_to_body_32 - T_local_to_body_22 * T_local_to_body_31;
  cof21 = T_local_to_body_13 * T_local_to_body_32 - T_local_to_body_12 * T_local_to_body_33;
  cof22 = T_local_to_body_11 * T_local_to_body_33 - T_local_to_body_13 * T_local_to_body_31;
  cof23 = T_local_to_body_12 * T_local_to_body_31 - T_local_to_body_11 * T_local_to_body_32;
  cof31 = T_local_to_body_12 * T_local_to_body_23 - T_local_to_body_13 * T_local_to_body_22;
  cof32 = T_local_to_body_13 * T_local_to_body_21 - T_local_to_body_11 * T_local_to_body_23;
  cof33 = T_local_to_body_11 * T_local_to_body_22 - T_local_to_body_12 * T_local_to_body_21;

  V_north = (cof11*U_body+cof21*V_body+cof31*W_body)/det_T_l_to_b;
  V_east_rel_ground = (cof12*U_body+cof22*V_body+cof32*W_body)/det_T_l_to_b;
  V_down = (cof13*U_body+cof23*V_body+cof33*W_body)/det_T_l_to_b;

  V_east = V_east_rel_ground + OMEGA_EARTH*Sea_level_radius*cos(Lat_geocentric);
    }
}

void uiuc_init_aeromodel ()
{
  string aircraft;

  if (aircraft_dir != (string)"")
    aircraft = aircraft_dir + "/";

  aircraft += "aircraft.dat";
  cout << "We are using "<< aircraft << endl;
  uiuc_initializemaps(); // Initialize the <string,int> maps
  uiuc_menu(aircraft);   // Read the specified aircraft file 
}

void uiuc_force_moment(double dt)
{
  double qS = Dynamic_pressure * Sw;
  double qScbar = qS * cbar;
  double qSb = qS * bw;

  uiuc_aerodeflections(dt);
  uiuc_coefficients();

  /* Calculate the wind axis forces */
  if (CX && CZ)
    {
      CD = -CX * cos(Alpha) - CZ * sin(Alpha);
      CL =  CX * sin(Alpha) - CZ * cos(Alpha);
    }
  F_X_wind = -1 * CD * qS;
  F_Y_wind = CY * qS;
  F_Z_wind = -1 * CL * qS;

  /* wind-axis to body-axis transformation */
  F_X_aero = F_X_wind * Cos_alpha * Cos_beta - F_Y_wind * Cos_alpha * Sin_beta - F_Z_wind * Sin_alpha;
  F_Y_aero = F_X_wind * Sin_beta + F_Y_wind * Cos_beta;
  F_Z_aero = F_X_wind * Sin_alpha * Cos_beta - F_Y_wind * Sin_alpha * Sin_beta + F_Z_wind * Cos_alpha;

  /* Moment calculations */
  M_l_aero = Cl * qSb;
  M_m_aero = Cm * qScbar;
  M_n_aero = Cn * qSb;

  /* Call flight data recorder */
  //  if (Simtime >= recordStartTime)
  //      uiuc_recorder(dt);
  

  // fog field update
   Fog = 0;
   if (fog_field)
     uiuc_fog();

   double vis;
   if (Fog != 0)
   {
 #ifndef FG_OLD_WEATHER
     vis = WeatherDatabase->getWeatherVisibility();
     if (Fog > 0)
       vis /= 1.01;
     else
       vis *= 1.01;
     WeatherDatabase->setWeatherVisibility( vis );
 #else
     vis = current_weather->get_visibility();
     if (Fog > 0)
       vis /= 1.01;
     else
       vis *= 1.01;
     current_weather->set_visibility( vis );
 #endif
   }
 

  /* Send data on the network to the Glass Cockpit */
 
  string input="";

  input += " stick_right " + ftoa(Lat_control);
  input += " rudder_left " + ftoa(-Rudder_pedal);
  input += " stick_forward " + ftoa(Long_control);
  input += " stick_trim_forward " + ftoa(Long_trim);
  input += " vehicle_pitch " + ftoa(Theta * 180.0 / 3.14);
  input += " vehicle_roll " + ftoa(Phi * 180.0 / 3.14);
  input += " vehicle_speed " + ftoa(V_rel_wind);
  input += " throttle_forward " + ftoa(Throttle_pct);
  input += " altitude " + ftoa(Altitude);
  input += " climb_rate " + ftoa(-1.0*V_down_rel_ground);
 
  // testarray.getHello();
  // testarray.sendData(input);
  
  /* End of Networking */ 

}

void uiuc_engine_routine()
{
  uiuc_engine();
}

void uiuc_gear_routine ()
{
  uiuc_gear();
}

void uiuc_record_routine(double dt)
{
  if (Simtime >= recordStartTime)
    uiuc_recorder(dt);
}
//end uiuc_wrapper.cpp
