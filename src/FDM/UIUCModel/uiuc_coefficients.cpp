/**********************************************************************

 FILENAME:     uiuc_coefficients.cpp

----------------------------------------------------------------------

 DESCRIPTION:  computes aggregated aerodynamic coefficients

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      01/29/2000   initial release
               02/01/2000   (JS) changed map name from aeroData to 
                            aeroPart
               02/18/2000   (JS) added calls to 1Dinterpolation 
                            function from CLfa and CDfa switches
               02/24/2000   added icing model functions
               02/29/2000   (JS) added calls to 2Dinterpolation 
                            function from CLfade, CDfade, Cmfade, 
                            CYfada, CYfbetadr, Clfada, Clfbetadr, 
                            Cnfada, and Cnfbetadr switches
	       04/15/2000   (JS) broke up into multiple 
	                    uiuc_coef_xxx functions
	       06/18/2001   (RD) Added initialization of Alpha and
	                    Beta.  Added aileron_input and rudder_input.
			    Added pilot_elev_no, pilot_ail_no, and
			    pilot_rud_no.
	       07/05/2001   (RD) Added pilot_(elev,ail,rud)_no=false

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
	       Robert Deters      <rdeters@uiuc.edu>

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

 CALLED BY:    uiuc_wrapper

----------------------------------------------------------------------

 CALLS TO:     uiuc_coef_lift
               uiuc_coef_drag
               uiuc_coef_pitch
               uiuc_coef_sideforce
               uiuc_coef_roll
               uiuc_coef_yaw
	       uiuc_controlInput

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

#include "uiuc_coefficients.h"


void uiuc_coefficients()
{
  double l_trim, l_defl;

  if (Alpha_init_true && Simtime==0)
    Alpha = Alpha_init;

  if (Beta_init_true && Simtime==0)
    Beta = Beta_init;

  // calculate rate derivative nondimensionalization factors
  // check if speed is sufficient to compute dynamic pressure terms
  if (nondim_rate_V_rel_wind)         // c172_aero uses V_rel_wind
    {
      if (V_rel_wind > dyn_on_speed)
	{
	  cbar_2U = cbar / (2.0 * V_rel_wind);
	  b_2U = bw / (2.0 * V_rel_wind);
	  ch_2U = ch / (2.0 * V_rel_wind);
	}
      else
	{
	  cbar_2U = 0.0;
	  b_2U = 0.0;
	  ch_2U = 0.0;
	}
    }
  else      // use U_body which is probably more correct
    {
      if (U_body > dyn_on_speed)
	{
	  cbar_2U = cbar / (2.0 * U_body);
	  b_2U = bw / (2.0 * U_body);
	  ch_2U = ch / (2.0 * U_body);
	}
      else
	{
	  cbar_2U = 0.0;
	  b_2U = 0.0;
	  ch_2U = 0.0;
	}
    }

  // check to see if icing model engaged
  if (ice_model)
    {
      uiuc_ice_eta();
    }

  // check to see if data files are used for control deflections
  pilot_elev_no = false;
  pilot_ail_no = false;
  pilot_rud_no = false;
  if (elevator_step || elevator_singlet || elevator_doublet || elevator_input || aileron_input || rudder_input)
    {
      uiuc_controlInput();
    }

  CD = CX = CL = CZ = Cm = CY = Cl = Cn = 0.0;
  CLclean_wing = CLiced_wing = CLclean_tail = CLiced_tail = 0.0;
  CZclean_wing = CZiced_wing = CZclean_tail = CZiced_tail = 0.0;
  CXclean_wing = CXiced_wing = CXclean_tail = CXiced_tail = 0.0;

  uiuc_coef_lift();
  uiuc_coef_drag();
  uiuc_coef_pitch();
  uiuc_coef_sideforce();
  uiuc_coef_roll();
  uiuc_coef_yaw();

  if (pilot_ail_no)
    {
      if (aileron <=0)
	Lat_control = - aileron / damax * RAD_TO_DEG;
      else
	Lat_control = - aileron / damin * RAD_TO_DEG;
    }

  if (pilot_elev_no)
    {
      l_trim = elevator_tab;
      l_defl = elevator - elevator_tab;
      if (l_trim <=0 )
	Long_trim = l_trim / demax * RAD_TO_DEG;
      else
	Long_trim = l_trim / demin * RAD_TO_DEG;
      if (l_defl <= 0)
	Long_control = l_defl / demax * RAD_TO_DEG;
      else
	Long_control = l_defl / demin * RAD_TO_DEG;
    }

  if (pilot_rud_no)
    {
      if (rudder <=0)
	Rudder_pedal = - rudder / drmax * RAD_TO_DEG;
      else
	Rudder_pedal = - rudder / drmin * RAD_TO_DEG;
    }

  return;
}

// end uiuc_coefficients.cpp
