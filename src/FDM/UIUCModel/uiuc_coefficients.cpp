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
	       06/18/2001   (RD) Added initialization of Std_Alpha and
	                    Std_Beta.  Added aileron_input and rudder_input.
			    Added pilot_elev_no, pilot_ail_no, and
			    pilot_rud_no.
	       07/05/2001   (RD) Added pilot_(elev,ail,rud)_no=false
	       01/11/2002   (AP) Added call to uiuc_iceboot()
               12/02/2002   (RD) Moved icing demo interpolations to its
                            own function
	       
----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
	       Robert Deters      <rdeters@uiuc.edu>
	       Ann Peedikayil     <peedikay@uiuc.edu>
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include "uiuc_coefficients.h"

void uiuc_coefficients(double dt)
{
  static string uiuc_coefficients_error = " (from uiuc_coefficients.cpp) ";
  double l_trim, l_defl;
  double V_rel_wind_dum, U_body_dum;

  if (Alpha_init_true && Simtime==0)
    Std_Alpha = Alpha_init;

  if (Beta_init_true && Simtime==0)
    Std_Beta = Beta_init;

  // calculate rate derivative nondimensionalization factors
  // check if speed is sufficient to compute dynamic pressure terms
  if (dyn_on_speed==0) 
    {
      uiuc_warnings_errors(5, uiuc_coefficients_error);
    }
  if (nondim_rate_V_rel_wind || use_V_rel_wind_2U)         // c172_aero uses V_rel_wind
    {
      if (V_rel_wind > dyn_on_speed)
	{
	  cbar_2U = cbar / (2.0 * V_rel_wind);
	  b_2U    = bw /   (2.0 * V_rel_wind);
	  // chord_h is the horizontal tail chord
	  ch_2U   = chord_h /   (2.0 * V_rel_wind);
	}
      else if (use_dyn_on_speed_curve1)
  	{
  	  V_rel_wind_dum = dyn_on_speed_zero + V_rel_wind * (dyn_on_speed - dyn_on_speed_zero)/dyn_on_speed;
  	  cbar_2U        = cbar / (2.0 * V_rel_wind_dum);
  	  b_2U           = bw /   (2.0 * V_rel_wind_dum);
  	  ch_2U          = chord_h /   (2.0 * V_rel_wind_dum);
	  Std_Alpha_dot      = 0.0;
  	}
      else
	{
	  cbar_2U   = 0.0;
	  b_2U      = 0.0;
	  ch_2U     = 0.0;
	  Std_Alpha_dot = 0.0;
	}
    }
  else if(use_abs_U_body_2U)      // use absolute(U_body)
    {
      if (fabs(U_body) > dyn_on_speed)
	{
	  cbar_2U = cbar / (2.0 * fabs(U_body));
	  b_2U    = bw /   (2.0 * fabs(U_body));
	  ch_2U   = chord_h /   (2.0 * fabs(U_body));
	}
      else if (use_dyn_on_speed_curve1)
	{
	  U_body_dum = dyn_on_speed_zero + fabs(U_body) * (dyn_on_speed - dyn_on_speed_zero)/dyn_on_speed;
	  cbar_2U    = cbar / (2.0 * U_body_dum);
	  b_2U       = bw /   (2.0 * U_body_dum);
	  ch_2U      = chord_h /   (2.0 * U_body_dum);
	  Std_Alpha_dot  = 0.0;
	}
      else
	{
	  cbar_2U   = 0.0;
	  b_2U      = 0.0;
	  ch_2U     = 0.0;
	  Std_Alpha_dot = 0.0;
	}
    }
  else      // use U_body
    {
      if (U_body > dyn_on_speed)
	{
	  cbar_2U = cbar / (2.0 * U_body);
	  b_2U    = bw /   (2.0 * U_body);
	  ch_2U   = chord_h /   (2.0 * U_body);
	}
      else if (use_dyn_on_speed_curve1)
	{
	  U_body_dum = dyn_on_speed_zero + U_body * (dyn_on_speed - dyn_on_speed_zero)/dyn_on_speed;
	  cbar_2U    = cbar / (2.0 * U_body_dum);
	  b_2U       = bw /   (2.0 * U_body_dum);
	  ch_2U      = chord_h /   (2.0 * U_body_dum);
	  Std_Alpha_dot  = 0.0;
	  beta_flow_clean_tail = cbar_2U;
	}
      else
	{
	  cbar_2U   = 0.0;
	  b_2U      = 0.0;
	  ch_2U     = 0.0;
	  Std_Alpha_dot = 0.0;
	}
    }

  // check if speed is sufficient to "trust" Std_Alpha_dot value
  if (use_Alpha_dot_on_speed)
    {
      if (V_rel_wind     < Alpha_dot_on_speed)
	  Std_Alpha_dot = 0.0;
    }


  // check to see if icing model engaged
  if (ice_model)
    {
      uiuc_iceboot(dt);
      uiuc_ice_eta();
    }

  // check to see if data files are used for control deflections
  if (outside_control == false)
    {
      pilot_elev_no = false;
      pilot_ail_no = false;
      pilot_rud_no = false;
    }
  if (elevator_step || elevator_singlet || elevator_doublet || elevator_input || aileron_input || rudder_input || flap_pos_input)
    {
      uiuc_controlInput();
    }

  //if (Simtime >=10.0)
  //  {
  //    ap_hh_on = 1;
  //    ap_Psi_ref_rad = 0*DEG_TO_RAD;
  //  }

  if (ap_pah_on || ap_alh_on || ap_rah_on || ap_hh_on)
    {
      uiuc_auto_pilot(dt);
    }

  CD = CX = CL = CZ = Cm = CY = Cl = Cn = 0.0;
  CLclean_wing = CLiced_wing = CLclean_tail = CLiced_tail = 0.0;
  CZclean_wing = CZiced_wing = CZclean_tail = CZiced_tail = 0.0;
  CXclean_wing = CXiced_wing = CXclean_tail = CXiced_tail = 0.0;
  CL_clean = CL_iced = 0.0;
  CY_clean = CY_iced = 0.0;
  CD_clean = CD_iced = 0.0;
  Cm_iced = Cm_clean = 0.0;
  Cl_iced = Cl_clean = 0.0;
  Cn_iced = Cn_clean = 0.0;

  uiuc_coef_lift();
  uiuc_coef_drag();
  uiuc_coef_pitch();
  uiuc_coef_sideforce();
  uiuc_coef_roll();
  uiuc_coef_yaw();

  //uncomment next line to always run icing functions
  //nonlin_ice_case = 1;

  if (nonlin_ice_case)
    {
      if (eta_from_file)
	{
	  if (eta_tail_input) {
	    double time = Simtime - eta_tail_input_startTime;
	    eta_tail = uiuc_1Dinterpolation(eta_tail_input_timeArray,
					    eta_tail_input_daArray,
					    eta_tail_input_ntime,
					    time);
	  }
	  if (eta_wing_left_input) {
	    double time = Simtime - eta_wing_left_input_startTime;
	    eta_wing_left = uiuc_1Dinterpolation(eta_wing_left_input_timeArray,
						 eta_wing_left_input_daArray,
						 eta_wing_left_input_ntime,
						 time);
	  }
	  if (eta_wing_right_input) {
	    double time = Simtime - eta_wing_right_input_startTime;
	    eta_wing_right = uiuc_1Dinterpolation(eta_wing_right_input_timeArray,
						  eta_wing_right_input_daArray,
						  eta_wing_right_input_ntime,
						  time);
	  }
	}

      delta_CL = delta_CD = delta_Cm = delta_Cl = delta_Cn = 0.0;
      Calc_Iced_Forces();
      add_ice_effects();
      tactilefadefI = 0.0;
      if (eta_tail == 0.2 && tactile_pitch && tactilefadef)
	{
	  if (tactilefadef_nice == 1)
	    tactilefadefI = uiuc_3Dinterp_quick(tactilefadef_fArray,
					   tactilefadef_aArray_nice,
					   tactilefadef_deArray_nice,
					   tactilefadef_tactileArray,
					   tactilefadef_na_nice,
					   tactilefadef_nde_nice,
					   tactilefadef_nf,
					   flap_pos,
					   Std_Alpha,
					   elevator);
	  else
	    tactilefadefI = uiuc_3Dinterpolation(tactilefadef_fArray,
					    tactilefadef_aArray,
					    tactilefadef_deArray,
					    tactilefadef_tactileArray,
					    tactilefadef_nAlphaArray,
					    tactilefadef_nde,
					    tactilefadef_nf,
					    flap_pos,
					    Std_Alpha,
					    elevator);
	}
      else if (demo_tactile)
	{
	  double time = Simtime - demo_tactile_startTime;
	  tactilefadefI = uiuc_1Dinterpolation(demo_tactile_timeArray,
					       demo_tactile_daArray,
					       demo_tactile_ntime,
					       time);
	}
      if (icing_demo)
	uiuc_icing_demo();
    }

  if (pilot_ail_no)
    {
      if (aileron <=0)
	Lat_control = - aileron / damax * RAD_TO_DEG;
      else
	Lat_control = - aileron / damin * RAD_TO_DEG;
    }

  // can go past real limits
  // add flag to behave like trim_case2 later
  if (pilot_elev_no)
    {
      if (outside_control == false)
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
