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
	       02/24/2002   (GD) Added uiuc_network_routine()
	       03/27/2002   (RD) Changed how forces are calculated when
	                    body-axis is used
               12/11/2002   (RD) Divided uiuc_network_routine into
                            uiuc_network_recv_routine and
                            uiuc_network_send_routine
               03/16/2003   (RD) Added trigger lines in recorder area

----------------------------------------------------------------------
 
 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Robert Deters      <rdeters@uiuc.edu>
	       Glen Dimock        <dimock@uiuc.edu>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>
#include <Aircraft/aircraft.hxx>
#include <Main/fg_props.hxx>

#include "uiuc_aircraft.h"
#include "uiuc_aircraftdir.h"
#include "uiuc_coefficients.h"
#include "uiuc_getwind.h"
#include "uiuc_engine.h"
#include "uiuc_gear.h"
#include "uiuc_aerodeflections.h"
#include "uiuc_recorder.h"
#include "uiuc_menu.h"
#include "uiuc_betaprobe.h"
#include <FDM/LaRCsim/ls_generic.h>
//#include "Main/simple_udp.h"
#include "uiuc_fog.h" //321654
//#include "uiuc_network.h"
#include "uiuc_get_flapper.h"

SG_USING_STD(cout);
SG_USING_STD(endl);

extern "C" void uiuc_initial_init ();
extern "C" void uiuc_defaults_inits ();
extern "C" void uiuc_vel_init ();
extern "C" void uiuc_init_aeromodel ();
extern "C" void uiuc_force_moment(double dt);
extern "C" void uiuc_engine_routine();
extern "C" void uiuc_wind_routine();
extern "C" void uiuc_gear_routine();
extern "C" void uiuc_record_routine(double dt);
extern "C" void uiuc_network_recv_routine();
extern "C" void uiuc_network_send_routine();

AIRCRAFT *aircraft_ = new AIRCRAFT;
AIRCRAFTDIR *aircraftdir_ = new AIRCRAFTDIR;

// SendArray testarray(4950);

/* Convert float to string */
//string ftoa(double in)
//{
//  static char temp[20];
//  sprintf(temp,"%g",in);
//  return (string)temp;
//}

void uiuc_initial_init ()
{
  // This function called from both ls_step and ls_model(uiuc model side).
  // Apply brute force initializations, which override ls_step and ls_aux values
  // for the first time step.
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

void uiuc_defaults_inits ()
{
  // set defaults and initialize (called from ls_step.c at Simtime=0)

  //fog inits 
  fog_field = 0;
  fog_segments = 0;
  fog_point_index = -1;
  fog_time = NULL;
  fog_value = NULL;
  fog_next_time = 0.0;
  fog_current_segment = 0;
  Fog = 0;

  use_V_rel_wind_2U = 0;
  nondim_rate_V_rel_wind = 0;
  use_abs_U_body_2U = 0;
  use_dyn_on_speed_curve1 = 0;
  use_Alpha_dot_on_speed = 0;
  use_gamma_horiz_on_speed = 0;
  b_downwashMode = 0;
  P_body_init_true = 0;
  Q_body_init_true = 0;
  R_body_init_true = 0;
  Phi_init_true = 0;
  Theta_init_true = 0;
  Psi_init_true = 0;
  Alpha_init_true = 0;
  Beta_init_true = 0;
  U_body_init_true = 0;
  V_body_init_true = 0;
  W_body_init_true = 0;
  trim_case_2 = 0;
  use_uiuc_network = 0;
  old_flap_routine = 0;
  icing_demo = 0;
  outside_control = 0;
  set_Long_trim = 0;
  zero_Long_trim = 0;
  elevator_step = 0;
  elevator_singlet = 0;
  elevator_doublet = 0;
  elevator_input = 0;
  aileron_input = 0;
  rudder_input = 0;
  pilot_elev_no = 0;
  pilot_elev_no_check = 0;
  pilot_ail_no = 0;
  pilot_ail_no_check = 0;
  pilot_rud_no = 0;
  pilot_rud_no_check = 0;
  use_flaps = 0;
  use_spoilers = 0;
  flap_pos_input = 0;
  use_elevator_sas_type1 = 0;
  use_elevator_sas_max = 0;
  use_elevator_sas_min = 0;
  use_elevator_stick_gain = 0;
  use_aileron_sas_type1 = 0;
  use_aileron_sas_max = 0;
  use_aileron_stick_gain = 0;
  use_rudder_sas_type1 = 0;
  use_rudder_sas_max = 0;
  use_rudder_stick_gain = 0;
  simpleSingleModel = 0;
  Throttle_pct_input = 0;
  gyroForce_Q_body = 0;
  gyroForce_R_body = 0;
  b_slipstreamEffects = 0;
  Xp_input = 0;
  Zp_input = 0;
  Mp_input = 0;
  b_CLK = 0;
  //  gear_model[MAX_GEAR] = 0;
  use_gear = 0;
  ice_model = 0;
  ice_on = 0;
  beta_model = 0;
  //  bootTrue[20] = 0;
  eta_from_file = 0;
  eta_wing_left_input = 0;
  eta_wing_right_input = 0;
  eta_tail_input = 0;
  demo_eps_alpha_max = 0;
  demo_eps_pitch_max = 0;
  demo_eps_pitch_min = 0;
  demo_eps_roll_max = 0;
  demo_eps_thrust_min = 0;
  demo_eps_airspeed_max = 0;
  demo_eps_airspeed_min = 0;
  demo_eps_flap_max = 0;
  demo_boot_cycle_tail = 0;
  demo_boot_cycle_wing_left = 0;
  demo_boot_cycle_wing_right = 0;
  demo_eps_pitch_input = 0;
  tactilefadef = 0;
  demo_ap_pah_on = 0;
  demo_ap_Theta_ref_deg = 0;
  demo_tactile = 0;
  demo_ice_tail = 0;
  demo_ice_left = 0;
  demo_ice_right = 0;
  flapper_model = 0;
  ignore_unknown_keywords = 0;
  pilot_throttle_no = 0;

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
  SGPath path(globals->get_fg_root());
  path.append(aircraft_dir);
  path.append("aircraft.dat");
  cout << "We are using "<< path.str() << endl;
  uiuc_initializemaps(); // Initialize the <string,int> maps
  uiuc_menu(path.str());   // Read the specified aircraft file 
}

void uiuc_force_moment(double dt)
{
  double qS = Dynamic_pressure * Sw;
  double qScbar = qS * cbar;
  double qSb = qS * bw;

  uiuc_aerodeflections(dt);
  uiuc_coefficients(dt);

  /* Calculate the forces */
  if (CX && CZ)
    {
      F_X_aero = CX * qS;
      F_Y_aero = CY * qS;
      F_Z_aero = CZ * qS;
    }
  else
    {
      // Cos_beta * Cos_beta corrects V_rel_wind to get normal q onto wing, 
      // hence Cos_beta * Cos_beta term included.
      // Same thing is done w/ moments below.
      // Without this "die-off" function, lift would be produced in a 90 deg sideslip, when
      // that should not be the case.  See FGFS notes 021105
      F_X_wind = -CD * qS  * Cos_beta * Cos_beta;
      F_Y_wind =  CY * qS;
      F_Z_wind = -CL * qS  * Cos_beta * Cos_beta;
      // F_X_wind = -CD * qS  * Cos_beta * Cos_beta;
      // F_Y_wind =  CY * qS  * Cos_beta * Cos_beta;
      // F_Z_wind = -CL * qS  * Cos_beta * Cos_beta;

      // wind-axis to body-axis transformation 
      F_X_aero = F_X_wind * Cos_alpha * Cos_beta - F_Y_wind * Cos_alpha * Sin_beta - F_Z_wind * Sin_alpha;
      F_Y_aero = F_X_wind * Sin_beta + F_Y_wind * Cos_beta;
      F_Z_aero = F_X_wind * Sin_alpha * Cos_beta - F_Y_wind * Sin_alpha * Sin_beta + F_Z_wind * Cos_alpha;
    }
  // Moment calculations
  M_l_aero = Cl * qSb    ;
  M_m_aero = Cm * qScbar * Cos_beta * Cos_beta;
  M_n_aero = Cn * qSb    ;
  // M_l_aero = Cl * qSb    * Cos_beta * Cos_beta;
  // M_m_aero = Cm * qScbar * Cos_beta * Cos_beta;
  // M_n_aero = Cn * qSb    * Cos_beta * Cos_beta;

  // Adding in apparent mass effects
  if (Mass_appMass_ratio)
    F_Z_aero += -(Mass_appMass_ratio * Mass) * W_dot_body;
  if (I_xx_appMass_ratio)
    M_l_aero += -(I_xx_appMass_ratio * I_xx) * P_dot_body;
  if (I_yy_appMass_ratio)
    M_m_aero += -(I_yy_appMass_ratio * I_yy) * Q_dot_body;
  if (I_zz_appMass_ratio)
    M_n_aero += -(I_zz_appMass_ratio * I_zz) * R_dot_body;

  if (Mass_appMass)
    F_Z_aero += -Mass_appMass * W_dot_body;
  if (I_xx_appMass)
    M_l_aero += -I_xx_appMass * P_dot_body;
  if (I_yy_appMass)
    M_m_aero += -I_yy_appMass * Q_dot_body;
  if (I_zz_appMass)
    M_n_aero += -I_zz_appMass * R_dot_body;

  // gyroscopic moments
  // engineOmega is positive when rotation is ccw when viewed from the front
  if (gyroForce_Q_body)
    M_n_aero +=  polarInertia * engineOmega * Q_body;
  if (gyroForce_R_body)
    M_m_aero += -polarInertia * engineOmega * R_body;

  // ornithopter support
  if (flapper_model)
    {
      uiuc_get_flapper(dt);
      F_X_aero += F_X_aero_flapper;
      F_Z_aero += F_Z_aero_flapper;
      M_m_aero += flapper_Moment;
    }

  // fog field update
   Fog = 0;
   if (fog_field)
     uiuc_fog();

   double vis;
   if (Fog != 0)
   {
     vis = fgGetDouble("/environment/visibility-m");
     if (Fog > 0)
       vis /= 1.01;
     else
       vis *= 1.01;
     fgSetDouble("/environment/visibility-m", vis);
   }
 

  /* Send data on the network to the Glass Cockpit */
 
   //  string input="";

   //  input += " stick_right " + ftoa(Lat_control);
   //  input += " rudder_left " + ftoa(-Rudder_pedal);
   //  input += " stick_forward " + ftoa(Long_control);
   //  input += " stick_trim_forward " + ftoa(Long_trim);
   //  input += " vehicle_pitch " + ftoa(Theta * 180.0 / 3.14);
   //  input += " vehicle_roll " + ftoa(Phi * 180.0 / 3.14);
   //  input += " vehicle_speed " + ftoa(V_rel_wind);
   //  input += " throttle_forward " + ftoa(Throttle_pct);
   //  input += " altitude " + ftoa(Altitude);
   //  input += " climb_rate " + ftoa(-1.0*V_down_rel_ground);
 
   //  testarray.getHello();
   //  testarray.sendData(input);
  
  /* End of Networking */ 

}

void uiuc_wind_routine()
{
  uiuc_getwind();
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
  if (trigger_last_time_step == 0 && trigger_on == 1) {
    if (trigger_toggle == 0)
      trigger_toggle = 1;
    else
      trigger_toggle = 0;
    trigger_num++;
    if (trigger_num % 2 != 0)
      trigger_counter++;
  }

  if (Simtime >= recordStartTime)
    uiuc_recorder(dt);

  trigger_last_time_step = trigger_on;
}

void uiuc_network_recv_routine()
{
  //if (use_uiuc_network)
    //uiuc_network(1);
}

void uiuc_network_send_routine()
{
  //if (use_uiuc_network)
    //uiuc_network(2);
}
//end uiuc_wrapper.cpp
