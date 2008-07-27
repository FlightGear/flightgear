/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_engine.cpp

----------------------------------------------------------------------

 DESCRIPTION:  reads input data for specified aircraft and creates 
               approporiate data storage space

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   based on "menu reader" format of Michael Selig

----------------------------------------------------------------------

 HISTORY:      04/04/2003   initial release
               06/30/2003   (RD) replaced istrstream with istringstream
                            to get rid of the annoying warning about
                            using the strstream header

----------------------------------------------------------------------

 AUTHOR(S):    Robert Deters      <rdeters@uiuc.edu>
               Michael Selig      <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       n/a

----------------------------------------------------------------------

 OUTPUTS:      n/a

----------------------------------------------------------------------

 CALLED BY:    uiuc_menu()

----------------------------------------------------------------------

 CALLS TO:     check_float() if needed
	       d_2_to_3() if needed
	       d_1_to_2() if needed
	       i_1_to_2() if needed
	       d_1_to_1() if needed

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

#include <simgear/compiler.h>


#include <cstdlib>
#include <string>
#include <iostream>

#include "uiuc_menu_engine.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_engine( const string& linetoken2, const string& linetoken3,
		   const string& linetoken4, const string& linetoken5, 
		   const string& linetoken6, const string& linetoken7, 
		   const string& linetoken8, const string& linetoken9,
		   const string& linetoken10,const string& aircraft_directory,
		   LIST command_line ) {
    double token_value;
    int token_value_convert1, token_value_convert2;
    istringstream token3(linetoken3.c_str());
    istringstream token4(linetoken4.c_str());
    istringstream token5(linetoken5.c_str());
    istringstream token6(linetoken6.c_str());
    istringstream token7(linetoken7.c_str());
    istringstream token8(linetoken8.c_str());
    istringstream token9(linetoken9.c_str());
    istringstream token10(linetoken10.c_str());

    switch(engine_map[linetoken2])
      {
      case simpleSingle_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  simpleSingleMaxThrust = token_value; 
	  engineParts -> storeCommands (*command_line);
	  break;
	}
      case simpleSingleModel_flag:
	{
	  simpleSingleModel = true;
	  /* input the thrust at zero speed */
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  t_v0 = token_value; 
	  /* input slope of thrust at speed for which thrust is zero */
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  dtdv_t0 = token_value; 
	  /* input speed at which thrust is zero */
	  if (check_float(linetoken5))
	    token5 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  v_t0 = token_value; 
	  dtdvvt = -dtdv_t0 * v_t0 / t_v0;
	  engineParts -> storeCommands (*command_line);
	  break;
	}
      case c172_flag:
	{
	  engineParts -> storeCommands (*command_line);
	  break;
	}
      case cherokee_flag:
	{
	  engineParts -> storeCommands (*command_line);
	  break;
	}
      case Throttle_pct_input_flag:
	{
	  Throttle_pct_input = true;
	  Throttle_pct_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(Throttle_pct_input_file,
				Throttle_pct_input_timeArray,
				Throttle_pct_input_dTArray,
				Throttle_pct_input_ntime);
	  token6 >> token_value;
	  Throttle_pct_input_startTime = token_value;
	  break;
	}
      case gyroForce_Q_body_flag:
	{
	  /* include gyroscopic forces due to pitch */
	  gyroForce_Q_body = true;
	  break;
	}
      case gyroForce_R_body_flag:
	{
	  /* include gyroscopic forces due to yaw */
	  gyroForce_R_body = true;
	  break;
	}

      case slipstream_effects_flag:
	{
	// include slipstream effects
	  b_slipstreamEffects = true;
	  if (!simpleSingleModel)
	    uiuc_warnings_errors(3, *command_line);
	  break;
	}
      case propDia_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  propDia = token_value;
	  break;
	}
      case eta_q_Cm_q_flag:
	{
	// include slipstream effects due to Cm_q
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cm_q_fac = token_value;
	  if (eta_q_Cm_q_fac == 0.0) {eta_q_Cm_q_fac = 1.0;}
	  break;
	}
      case eta_q_Cm_adot_flag:
	{
	// include slipstream effects due to Cm_adot
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cm_adot_fac = token_value;
	  if (eta_q_Cm_adot_fac == 0.0) {eta_q_Cm_adot_fac = 1.0;}
	  break;
	}
      case eta_q_Cmfade_flag:
	{
	// include slipstream effects due to Cmfade
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cmfade_fac = token_value;
	  if (eta_q_Cmfade_fac == 0.0) {eta_q_Cmfade_fac = 1.0;}
	  break;
	}
      case eta_q_Cm_de_flag:
	{
	// include slipstream effects due to Cmfade
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cm_de_fac = token_value;
	  if (eta_q_Cm_de_fac == 0.0) {eta_q_Cm_de_fac = 1.0;}
	  break;
	}
      case eta_q_Cl_beta_flag:
	{
	// include slipstream effects due to Cl_beta
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cl_beta_fac = token_value;
	  if (eta_q_Cl_beta_fac == 0.0) {eta_q_Cl_beta_fac = 1.0;}
	  break;
	}
      case eta_q_Cl_p_flag:
	{
	// include slipstream effects due to Cl_p
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cl_p_fac = token_value;
	  if (eta_q_Cl_p_fac == 0.0) {eta_q_Cl_p_fac = 1.0;}
	  break;
	}
      case eta_q_Cl_r_flag:
	{
	// include slipstream effects due to Cl_r
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cl_r_fac = token_value;
	  if (eta_q_Cl_r_fac == 0.0) {eta_q_Cl_r_fac = 1.0;}
	  break;
	}
      case eta_q_Cl_dr_flag:
	{
	// include slipstream effects due to Cl_dr
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cl_dr_fac = token_value;
	  if (eta_q_Cl_dr_fac == 0.0) {eta_q_Cl_dr_fac = 1.0;}
	  break;
	}
      case eta_q_CY_beta_flag:
	{
	// include slipstream effects due to CY_beta
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_CY_beta_fac = token_value;
	  if (eta_q_CY_beta_fac == 0.0) {eta_q_CY_beta_fac = 1.0;}
	  break;
	}
      case eta_q_CY_p_flag:
	{
	// include slipstream effects due to CY_p
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_CY_p_fac = token_value;
	  if (eta_q_CY_p_fac == 0.0) {eta_q_CY_p_fac = 1.0;}
	  break;
	}
      case eta_q_CY_r_flag:
	{
	// include slipstream effects due to CY_r
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_CY_r_fac = token_value;
	  if (eta_q_CY_r_fac == 0.0) {eta_q_CY_r_fac = 1.0;}
	  break;
	}
      case eta_q_CY_dr_flag:
	{
	// include slipstream effects due to CY_dr
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_CY_dr_fac = token_value;
	  if (eta_q_CY_dr_fac == 0.0) {eta_q_CY_dr_fac = 1.0;}
	  break;
	}
      case eta_q_Cn_beta_flag:
	{
	// include slipstream effects due to Cn_beta
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cn_beta_fac = token_value;
	  if (eta_q_Cn_beta_fac == 0.0) {eta_q_Cn_beta_fac = 1.0;}
	  break;
	}
      case eta_q_Cn_p_flag:
	{
	// include slipstream effects due to Cn_p
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cn_p_fac = token_value;
	  if (eta_q_Cn_p_fac == 0.0) {eta_q_Cn_p_fac = 1.0;}
	  break;
	}
      case eta_q_Cn_r_flag:
	{
	// include slipstream effects due to Cn_r
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cn_r_fac = token_value;
	  if (eta_q_Cn_r_fac == 0.0) {eta_q_Cn_r_fac = 1.0;}
	  break;
	}
      case eta_q_Cn_dr_flag:
	{
	// include slipstream effects due to Cn_dr
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  eta_q_Cn_dr_fac = token_value;
	  if (eta_q_Cn_dr_fac == 0.0) {eta_q_Cn_dr_fac = 1.0;}
	  break;
	}

      case omega_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  minOmega = token_value; 
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  maxOmega = token_value; 
	  break;
	}
      case omegaRPM_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  minOmegaRPM = token_value; 
	  minOmega    = minOmegaRPM * 2.0 * LS_PI / 60;
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  maxOmegaRPM = token_value; 
	  maxOmega    = maxOmegaRPM * 2.0 * LS_PI / 60;
	  break;
	}
      case polarInertia_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  polarInertia = token_value; 
	  break;
	}
      case forcemom_flag:
	{
	  engineParts -> storeCommands (*command_line);
	  break;
	}
      case Xp_input_flag:
	{
	  Xp_input = true;
	  Xp_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(Xp_input_file,
				Xp_input_timeArray,
				Xp_input_XpArray,
				Xp_input_ntime);
	  token6 >> token_value;
	  Xp_input_startTime = token_value;
	  break;
	}
      case Zp_input_flag:
	{
	  Zp_input = true;
	  Zp_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(Zp_input_file,
				Zp_input_timeArray,
				Zp_input_ZpArray,
				Zp_input_ntime);
	  token6 >> token_value;
	  Zp_input_startTime = token_value;
	  break;
	}
      case Mp_input_flag:
	{
	  Mp_input = true;
	  Mp_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(Mp_input_file,
				Mp_input_timeArray,
				Mp_input_MpArray,
				Mp_input_ntime);
	  token6 >> token_value;
	  Mp_input_startTime = token_value;
	  break;
	}
      default:
	{
	  if (ignore_unknown_keywords) {
	    // do nothing
	  } else {
	    // print error message
	    uiuc_warnings_errors(2, *command_line);
	  }
	  break;
	}
      };
}
