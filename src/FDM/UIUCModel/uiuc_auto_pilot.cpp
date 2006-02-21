/**********************************************************************

 FILENAME:     uiuc_auto_pilot.cpp

----------------------------------------------------------------------

 DESCRIPTION:  calls autopilot routines

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      09/04/2003   initial release (with PAH)
               10/31/2003   added ALH autopilot
               11/04/2003   added RAH and HH autopilots
	       
----------------------------------------------------------------------

 AUTHOR(S):    Robert Deters      <rdeters@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -V_rel_wind (or U_body)
               -dyn_on_speed
               -ice on/off
	       -phugoid on/off

----------------------------------------------------------------------

 OUTPUTS:      -CL
               -CD
               -Cm
               -CY
               -Cl
               -Cn

----------------------------------------------------------------------

 CALLED BY:    uiuc_coefficients

----------------------------------------------------------------------

 CALLS TO:     uiuc_coef_lift
               uiuc_coef_drag

----------------------------------------------------------------------

 COPYRIGHT:    (C) 2003 by Michael Selig

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

#include "uiuc_auto_pilot.h"
//#include <stdio.h>
void uiuc_auto_pilot(double dt)
{
  double V_rel_wind_ms;
  double Altitude_m;
  //static bool ap_pah_on_prev = false;
  static int ap_pah_init = 0;
  //static bool ap_alh_on_prev = false;
  static int ap_alh_init = 0;
  static int ap_rah_init = 0;
  static int ap_hh_init = 0;
  double ap_alt_ref_m;
  double bw_m;
  double ail_rud[2];
  V_rel_wind_ms = V_rel_wind * 0.3048;
  Altitude_m = Altitude * 0.3048;

  if (ap_pah_on && icing_demo==false && Simtime>=ap_pah_start_time)
    {
      //ap_Theta_ref_rad = ap_Theta_ref_deg * DEG_TO_RAD;
      //if (ap_pah_on_prev == false) {
      //ap_pah_init = 1;
      //ap_pah_on_prev = true;
      //}
      elevator = pah_ap(Theta, Theta_dot, ap_Theta_ref_rad, V_rel_wind_ms, dt, 
			ap_pah_init);
      if (elevator*RAD_TO_DEG <= -1*demax)
	elevator = -1*demax * DEG_TO_RAD;
      if (elevator*RAD_TO_DEG >= demin)
	elevator = demin * DEG_TO_RAD;
      pilot_elev_no=true;
      ap_pah_init=1;
      //printf("elv1=%f\n",elevator);
    }

  if (ap_alh_on && icing_demo==false && Simtime>=ap_alh_start_time)
    {
      ap_alt_ref_m = ap_alt_ref_ft * 0.3048;
      //if (ap_alh_on_prev == false)
      //ap_alh_init = 0;
      elevator = alh_ap(Theta, Theta_dot, ap_alt_ref_m, Altitude_m, 
			V_rel_wind_ms, dt, ap_alh_init);
      if (elevator*RAD_TO_DEG <= -1*demax)
	elevator = -1*demax * DEG_TO_RAD;
      if (elevator*RAD_TO_DEG >= demin)
	elevator = demin * DEG_TO_RAD;
      pilot_elev_no=true;
      ap_alh_init = 1;
    }

  if (ap_rah_on && icing_demo==false && Simtime>=ap_rah_start_time)
    {
      bw_m = bw * 0.3048;
      rah_ap(Phi, Phi_dot, ap_Phi_ref_rad, V_rel_wind_ms, dt,
	     bw_m, Psi_dot, ail_rud, ap_rah_init);
      aileron = ail_rud[0];
      rudder = ail_rud[1];
      if (aileron*RAD_TO_DEG <= -1*damax)
	aileron = -1*damax * DEG_TO_RAD;
      if (aileron*RAD_TO_DEG >= damin)
	aileron = damin * DEG_TO_RAD;
      if (rudder*RAD_TO_DEG <= -1*drmax)
	rudder = -1*drmax * DEG_TO_RAD;
      if (rudder*RAD_TO_DEG >= drmin)
	rudder = drmin * DEG_TO_RAD;
      pilot_ail_no=true;
      pilot_rud_no=true;
      ap_rah_init = 1;
    }

  if (ap_hh_on && icing_demo==false && Simtime>=ap_hh_start_time)
    {
      bw_m = bw * 0.3048;
      hh_ap(Phi, Psi, Phi_dot, ap_Psi_ref_rad, V_rel_wind_ms, dt,
	    bw_m, Psi_dot, ail_rud, ap_hh_init);
      aileron = ail_rud[0];
      rudder = ail_rud[1];
      if (aileron*RAD_TO_DEG <= -1*damax)
	aileron = -1*damax * DEG_TO_RAD;
      if (aileron*RAD_TO_DEG >= damin)
	aileron = damin * DEG_TO_RAD;
      if (rudder*RAD_TO_DEG <= -1*drmax)
	rudder = -1*drmax * DEG_TO_RAD;
      if (rudder*RAD_TO_DEG >= drmin)
	rudder = drmin * DEG_TO_RAD;
      pilot_ail_no=true;
      pilot_rud_no=true;
      ap_hh_init = 1;
    }
}
