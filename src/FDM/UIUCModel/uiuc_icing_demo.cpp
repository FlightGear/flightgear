/**********************************************************************

 FILENAME:     uiuc_icing_demo.cpp

----------------------------------------------------------------------

 DESCRIPTION:  

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      12/02/2002   initial release - originally in
                            uiuc_coefficients()

----------------------------------------------------------------------

 AUTHOR(S):    Robert Deters      <rdeters@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       

----------------------------------------------------------------------

 OUTPUTS:      

----------------------------------------------------------------------

 CALLED BY:    uiuc_coefficients

----------------------------------------------------------------------

 CALLS TO:     

----------------------------------------------------------------------

 COPYRIGHT:    (C) 2002 by Michael Selig

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include "uiuc_icing_demo.h"

void uiuc_icing_demo()
{
  // Envelope protection values
  if (demo_eps_alpha_max) {
    double time = Simtime - demo_eps_alpha_max_startTime;
    eps_alpha_max = uiuc_1Dinterpolation(demo_eps_alpha_max_timeArray,
					 demo_eps_alpha_max_daArray,
					 demo_eps_alpha_max_ntime,
					 time);
  }
  if (demo_eps_pitch_max) {
    double time = Simtime - demo_eps_pitch_max_startTime;
    eps_pitch_max = uiuc_1Dinterpolation(demo_eps_pitch_max_timeArray,
					 demo_eps_pitch_max_daArray,
					 demo_eps_pitch_max_ntime,
					 time);
  }
  if (demo_eps_pitch_min) {
    double time = Simtime - demo_eps_pitch_min_startTime;
    eps_pitch_min = uiuc_1Dinterpolation(demo_eps_pitch_min_timeArray,
					 demo_eps_pitch_min_daArray,
					 demo_eps_pitch_min_ntime,
					 time);
  }
  if (demo_eps_roll_max) {
    double time = Simtime - demo_eps_roll_max_startTime;
    eps_roll_max = uiuc_1Dinterpolation(demo_eps_roll_max_timeArray,
					demo_eps_roll_max_daArray,
					demo_eps_roll_max_ntime,
					time);
  }
  if (demo_eps_thrust_min) {
    double time = Simtime - demo_eps_thrust_min_startTime;
    eps_thrust_min = uiuc_1Dinterpolation(demo_eps_thrust_min_timeArray,
					  demo_eps_thrust_min_daArray,
					  demo_eps_thrust_min_ntime,
					  time);
  }
  if (demo_eps_airspeed_max) {
    double time = Simtime - demo_eps_airspeed_max_startTime;
    eps_airspeed_max = uiuc_1Dinterpolation(demo_eps_airspeed_max_timeArray,
					    demo_eps_airspeed_max_daArray,
					    demo_eps_airspeed_max_ntime,
					    time);
  }
  if (demo_eps_airspeed_min) {
    double time = Simtime - demo_eps_airspeed_min_startTime;
    eps_airspeed_min = uiuc_1Dinterpolation(demo_eps_airspeed_min_timeArray,
					    demo_eps_airspeed_min_daArray,
					    demo_eps_airspeed_min_ntime,
					    time);
  }
  if (demo_eps_flap_max) {
    double time = Simtime - demo_eps_flap_max_startTime;
    eps_flap_max = uiuc_1Dinterpolation(demo_eps_flap_max_timeArray,
					demo_eps_flap_max_daArray,
					demo_eps_flap_max_ntime,
					time);
  }
  if (demo_eps_pitch_input) {
    double time = Simtime - demo_eps_pitch_input_startTime;
    eps_pitch_input = uiuc_1Dinterpolation(demo_eps_pitch_input_timeArray,
					   demo_eps_pitch_input_daArray,
					   demo_eps_pitch_input_ntime,
					   time);
  }

  // Boot cycle values
  if (demo_boot_cycle_tail) {
    double time = Simtime - demo_boot_cycle_tail_startTime;
    boot_cycle_tail = uiuc_1Dinterpolation(demo_boot_cycle_tail_timeArray,
					   demo_boot_cycle_tail_daArray,
					   demo_boot_cycle_tail_ntime,
					   time);
  }
  if (demo_boot_cycle_wing_left) {
    double time = Simtime - demo_boot_cycle_wing_left_startTime;
    boot_cycle_wing_left = uiuc_1Dinterpolation(demo_boot_cycle_wing_left_timeArray,
						demo_boot_cycle_wing_left_daArray,
						demo_boot_cycle_wing_left_ntime,
						time);
  }
  if (demo_boot_cycle_wing_right) {
    double time = Simtime - demo_boot_cycle_wing_right_startTime;
    boot_cycle_wing_right = uiuc_1Dinterpolation(demo_boot_cycle_wing_right_timeArray,
						 demo_boot_cycle_wing_right_daArray,
						 demo_boot_cycle_wing_right_ntime,
						 time);
  }

  // Ice values
  if (demo_ice_tail) {
    double time = Simtime - demo_ice_tail_startTime;
    ice_tail = uiuc_1Dinterpolation(demo_ice_tail_timeArray,
				    demo_ice_tail_daArray,
				    demo_ice_tail_ntime,
				    time);
  }
  if (demo_ice_left) {
    double time = Simtime - demo_ice_left_startTime;
    ice_left = uiuc_1Dinterpolation(demo_ice_left_timeArray,
				    demo_ice_left_daArray,
				    demo_ice_left_ntime,
				    time);
  }
  if (demo_ice_right) {
    double time = Simtime - demo_ice_right_startTime;
    ice_right = uiuc_1Dinterpolation(demo_ice_right_timeArray,
				     demo_ice_right_daArray,
				     demo_ice_right_ntime,
				     time);
  }

  // Autopilot
  if (demo_ap_pah_on){
    double time = Simtime - demo_ap_pah_on_startTime;
    ap_pah_on = uiuc_1Dinterpolation(demo_ap_pah_on_timeArray,
				     demo_ap_pah_on_daArray,
				     demo_ap_pah_on_ntime,
				     time);
  }
  if (demo_ap_alh_on){
    double time = Simtime - demo_ap_alh_on_startTime;
    ap_alh_on = uiuc_1Dinterpolation(demo_ap_alh_on_timeArray,
				     demo_ap_alh_on_daArray,
				     demo_ap_alh_on_ntime,
				     time);
  }
  if (demo_ap_rah_on){
    double time = Simtime - demo_ap_rah_on_startTime;
    ap_rah_on = uiuc_1Dinterpolation(demo_ap_rah_on_timeArray,
				     demo_ap_rah_on_daArray,
				     demo_ap_rah_on_ntime,
				     time);
  }
  if (demo_ap_hh_on){
    double time = Simtime - demo_ap_hh_on_startTime;
    ap_hh_on = uiuc_1Dinterpolation(demo_ap_hh_on_timeArray,
				    demo_ap_hh_on_daArray,
				    demo_ap_hh_on_ntime,
				    time);
  }
  if (demo_ap_Theta_ref){
    double time = Simtime - demo_ap_Theta_ref_startTime;
    ap_Theta_ref_rad = uiuc_1Dinterpolation(demo_ap_Theta_ref_timeArray,
					    demo_ap_Theta_ref_daArray,
					    demo_ap_Theta_ref_ntime,
					    time);
  }
  if (demo_ap_alt_ref){
    double time = Simtime - demo_ap_alt_ref_startTime;
    ap_alt_ref_ft = uiuc_1Dinterpolation(demo_ap_alt_ref_timeArray,
					    demo_ap_alt_ref_daArray,
					    demo_ap_alt_ref_ntime,
					    time);
  }
  if (demo_ap_Phi_ref){
    double time = Simtime - demo_ap_Phi_ref_startTime;
    ap_Phi_ref_rad = uiuc_1Dinterpolation(demo_ap_Phi_ref_timeArray,
					    demo_ap_Phi_ref_daArray,
					    demo_ap_Phi_ref_ntime,
					    time);
  }
  if (demo_ap_Psi_ref){
    double time = Simtime - demo_ap_Psi_ref_startTime;
    ap_Psi_ref_rad = uiuc_1Dinterpolation(demo_ap_Psi_ref_timeArray,
					    demo_ap_Psi_ref_daArray,
					    demo_ap_Psi_ref_ntime,
					    time);
  }

  return;
}
