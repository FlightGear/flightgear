/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_ice.cpp

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

#include "uiuc_menu_ice.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_ice( const string& linetoken2, const string& linetoken3,
                const string& linetoken4, const string& linetoken5,
		const string& linetoken6, const string& linetoken7, 
		const string& linetoken8, const string& linetoken9,
		const string& linetoken10, const string& aircraft_directory,
		LIST command_line ) {
    double token_value;
    int token_value_convert1, token_value_convert2, token_value_convert3;
    int token_value_convert4;
    double datafile_xArray[100][100], datafile_yArray[100];
    double datafile_zArray[100][100];
    double convert_f;
    int datafile_nxArray[100], datafile_ny;
    istringstream token3(linetoken3.c_str());
    istringstream token4(linetoken4.c_str());
    istringstream token5(linetoken5.c_str());
    istringstream token6(linetoken6.c_str());
    istringstream token7(linetoken7.c_str());
    istringstream token8(linetoken8.c_str());
    istringstream token9(linetoken9.c_str());
    istringstream token10(linetoken10.c_str());

    static bool tactilefadef_first = true;

    switch(ice_map[linetoken2])
      {
      case iceTime_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  ice_model = true;
	  iceTime = token_value;
	  break;
	}
      case transientTime_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  transientTime = token_value;
	  break;
	}
      case eta_ice_final_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  eta_ice_final = token_value;
	  break;
	}
      case beta_probe_wing_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  beta_model = true;
	  x_probe_wing = token_value;
	  break;
	}
      case beta_probe_tail_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  beta_model = true;
	  x_probe_tail = token_value;
	  break;
	}
      case kCDo_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCDo = token_value;
	  break;
	}
      case kCDK_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCDK = token_value;
	  break;
	}
      case kCD_a_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCD_a = token_value;
	  break;
	}
      case kCD_adot_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCD_adot = token_value;
	  break;
	}
      case kCD_q_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCD_q = token_value;
	  break;
	}
      case kCD_de_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCD_de = token_value;
	  break;
	}
      case kCXo_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCXo = token_value;
	  break;
	}
      case kCXK_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCXK = token_value;
	  break;
	}
      case kCX_a_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCX_a = token_value;
	  break;
	}
      case kCX_a2_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCX_a2 = token_value;
	  break;
	}
      case kCX_a3_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCX_a3 = token_value;
	  break;
	}
      case kCX_adot_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCX_adot = token_value;
	  break;
	}
      case kCX_q_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCX_q = token_value;
	  break;
	}
      case kCX_de_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCX_de = token_value;
	  break;
	}
      case kCX_dr_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCX_dr = token_value;
	  break;
	}
      case kCX_df_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCX_df = token_value;
	  break;
	}
      case kCX_adf_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCX_adf = token_value;
	  break;
	}
      case kCLo_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCLo = token_value;
	  break;
	}
      case kCL_a_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCL_a = token_value;
	  break;
	}
      case kCL_adot_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCL_adot = token_value;
	  break;
	}
      case kCL_q_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCL_q = token_value;
	  break;
	}
      case kCL_de_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCL_de = token_value;
	  break;
	}
      case kCZo_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCZo = token_value;
	  break;
	}
      case kCZ_a_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCZ_a = token_value;
	  break;
	}
      case kCZ_a2_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCZ_a2 = token_value;
	  break;
	}
      case kCZ_a3_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCZ_a3 = token_value;
	  break;
	}
      case kCZ_adot_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCZ_adot = token_value;
	  break;
	}
      case kCZ_q_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCZ_q = token_value;
	  break;
	}
      case kCZ_de_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCZ_de = token_value;
	  break;
	}
      case kCZ_deb2_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCZ_deb2 = token_value;
	  break;
	}
      case kCZ_df_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCZ_df = token_value;
	  break;
	}
      case kCZ_adf_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCZ_adf = token_value;
	  break;
	}
      case kCmo_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCmo = token_value;
	  break;
	}
      case kCm_a_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCm_a = token_value;
	  break;
	}
      case kCm_a2_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCm_a2 = token_value;
	  break;
	}
      case kCm_adot_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCm_adot = token_value;
	  break;
	}
      case kCm_q_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCm_q = token_value;
	  break;
	}
      case kCm_de_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCm_de = token_value;
	  break;
	}
      case kCm_b2_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCm_b2 = token_value;
	  break;
	}
      case kCm_r_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCm_r = token_value;
	  break;
	}
      case kCm_df_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCm_df = token_value;
	  break;
	}
      case kCYo_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCYo = token_value;
	  break;
	}
      case kCY_beta_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCY_beta = token_value;
	  break;
	}
      case kCY_p_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCY_p = token_value;
	  break;
	}
      case kCY_r_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCY_r = token_value;
	  break;
	}
      case kCY_da_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCY_da = token_value;
	  break;
	}
      case kCY_dr_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCY_dr = token_value;
	  break;
	}
      case kCY_dra_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCY_dra = token_value;
	  break;
	}
      case kCY_bdot_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCY_bdot = token_value;
	  break;
	}
      case kClo_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kClo = token_value;
	  break;
	}
      case kCl_beta_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCl_beta = token_value;
	  break;
	}
      case kCl_p_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCl_p = token_value;
	  break;
	}
      case kCl_r_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCl_r = token_value;
	  break;
	}
      case kCl_da_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCl_da = token_value;
	  break;
	}
      case kCl_dr_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCl_dr = token_value;
	  break;
	}
      case kCl_daa_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCl_daa = token_value;
	  break;
	}
      case kCno_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCno = token_value;
	  break;
	}
      case kCn_beta_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCn_beta = token_value;
	  break;
	}
      case kCn_p_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCn_p = token_value;
	  break;
	}
      case kCn_r_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCn_r = token_value;
	  break;
	}
      case kCn_da_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCn_da = token_value;
	  break;
	}
      case kCn_dr_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCn_dr = token_value;
	  break;
	}
      case kCn_q_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCn_q = token_value;
	  break;
	}
      case kCn_b3_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  kCn_b3 = token_value;
	  break;
	}
      case bootTime_flag:
	{
	  int index;
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  token3 >> index;
	  if (index < 0 || index >= 20)
	    uiuc_warnings_errors(1, *command_line);
	  bootTime[index] = token_value;
	  bootTrue[index] = true;
	  break;
	}  
      case eta_wing_left_input_flag:
	{
	  eta_from_file = true;
	  eta_wing_left_input = true;
	  eta_wing_left_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(eta_wing_left_input_file,
				eta_wing_left_input_timeArray,
				eta_wing_left_input_daArray,
				eta_wing_left_input_ntime);
	  token6 >> token_value;
	  eta_wing_left_input_startTime = token_value;
	  break;
	}
      case eta_wing_right_input_flag:
	{
	  eta_from_file = true;
	  eta_wing_right_input = true;
	  eta_wing_right_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(eta_wing_right_input_file,
				eta_wing_right_input_timeArray,
				eta_wing_right_input_daArray,
				eta_wing_right_input_ntime);
	  token6 >> token_value;
	  eta_wing_right_input_startTime = token_value;
	  break;
	}
      case eta_tail_input_flag:
	{
	  eta_from_file = true;
	  eta_tail_input = true;
	  eta_tail_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(eta_tail_input_file,
				eta_tail_input_timeArray,
				eta_tail_input_daArray,
				eta_tail_input_ntime);
	  token6 >> token_value;
	  eta_tail_input_startTime = token_value;
	  break;
	}
      case nonlin_ice_case_flag:
	{
	  token3 >> nonlin_ice_case;
	  break;
	}
      case eta_tail_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);

	  eta_tail = token_value;
	  break;
	}
      case eta_wing_left_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);

	  eta_wing_left = token_value;
	  break;
	}
      case eta_wing_right_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);

	  eta_wing_right = token_value;
	  break;
	}
      case demo_eps_alpha_max_flag:
	{
	  demo_eps_alpha_max = true;
	  demo_eps_alpha_max_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_eps_alpha_max_file,
				demo_eps_alpha_max_timeArray,
				demo_eps_alpha_max_daArray,
				demo_eps_alpha_max_ntime);
	  token6 >> token_value;
	  demo_eps_alpha_max_startTime = token_value;
	  break;
	}
      case demo_eps_pitch_max_flag:
	{
	  demo_eps_pitch_max = true;
	  demo_eps_pitch_max_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_eps_pitch_max_file,
				demo_eps_pitch_max_timeArray,
				demo_eps_pitch_max_daArray,
				demo_eps_pitch_max_ntime);
	  token6 >> token_value;
	  demo_eps_pitch_max_startTime = token_value;
	  break;
	}
      case demo_eps_pitch_min_flag:
	{
	  demo_eps_pitch_min = true;
	  demo_eps_pitch_min_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_eps_pitch_min_file,
				demo_eps_pitch_min_timeArray,
				demo_eps_pitch_min_daArray,
				demo_eps_pitch_min_ntime);
	  token6 >> token_value;
	  demo_eps_pitch_min_startTime = token_value;
	  break;
	}
      case demo_eps_roll_max_flag:
	{
	  demo_eps_roll_max = true;
	  demo_eps_roll_max_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_eps_roll_max_file,
				demo_eps_roll_max_timeArray,
				demo_eps_roll_max_daArray,
				demo_eps_roll_max_ntime);
	  token6 >> token_value;
	  demo_eps_roll_max_startTime = token_value;
	  break;
	}
      case demo_eps_thrust_min_flag:
	{
	  demo_eps_thrust_min = true;
	  demo_eps_thrust_min_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_eps_thrust_min_file,
				demo_eps_thrust_min_timeArray,
				demo_eps_thrust_min_daArray,
				demo_eps_thrust_min_ntime);
	  token6 >> token_value;
	  demo_eps_thrust_min_startTime = token_value;
	  break;
	}
      case demo_eps_airspeed_max_flag:
	{
	  demo_eps_airspeed_max = true;
	  demo_eps_airspeed_max_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_eps_airspeed_max_file,
				demo_eps_airspeed_max_timeArray,
				demo_eps_airspeed_max_daArray,
				demo_eps_airspeed_max_ntime);
	  token6 >> token_value;
	  demo_eps_airspeed_max_startTime = token_value;
	  break;
	}
      case demo_eps_airspeed_min_flag:
	{
	  demo_eps_airspeed_min = true;
	  demo_eps_airspeed_min_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_eps_airspeed_min_file,
				demo_eps_airspeed_min_timeArray,
				demo_eps_airspeed_min_daArray,
				demo_eps_airspeed_min_ntime);
	  token6 >> token_value;
	  demo_eps_airspeed_min_startTime = token_value;
	  break;
	}
      case demo_eps_flap_max_flag:
	{
	  demo_eps_flap_max = true;
	  demo_eps_flap_max_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_eps_flap_max_file,
				demo_eps_flap_max_timeArray,
				demo_eps_flap_max_daArray,
				demo_eps_flap_max_ntime);
	  token6 >> token_value;
	  demo_eps_flap_max_startTime = token_value;
	  break;
	}
      case demo_boot_cycle_tail_flag:
	{
	  demo_boot_cycle_tail = true;
	  demo_boot_cycle_tail_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_boot_cycle_tail_file,
				demo_boot_cycle_tail_timeArray,
				demo_boot_cycle_tail_daArray,
				demo_boot_cycle_tail_ntime);
	  token6 >> token_value;
	  demo_boot_cycle_tail_startTime = token_value;
	  break;
	}
      case demo_boot_cycle_wing_left_flag:
	{
	  demo_boot_cycle_wing_left = true;
	  demo_boot_cycle_wing_left_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_boot_cycle_wing_left_file,
				demo_boot_cycle_wing_left_timeArray,
				demo_boot_cycle_wing_left_daArray,
				demo_boot_cycle_wing_left_ntime);
	  token6 >> token_value;
	  demo_boot_cycle_wing_left_startTime = token_value;
	  break;
	}
      case demo_boot_cycle_wing_right_flag:
	{
	  demo_boot_cycle_wing_right = true;
	  demo_boot_cycle_wing_right_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_boot_cycle_wing_right_file,
				demo_boot_cycle_wing_right_timeArray,
				demo_boot_cycle_wing_right_daArray,
				demo_boot_cycle_wing_right_ntime);
	  token6 >> token_value;
	  demo_boot_cycle_wing_right_startTime = token_value;
	  break;
	}
      case demo_eps_pitch_input_flag:
	{
	  demo_eps_pitch_input = true;
	  demo_eps_pitch_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_eps_pitch_input_file,
				demo_eps_pitch_input_timeArray,
				demo_eps_pitch_input_daArray,
				demo_eps_pitch_input_ntime);
	  token6 >> token_value;
	  demo_eps_pitch_input_startTime = token_value;
	  break;
	}
      case demo_ap_pah_on_flag:
	{
	  demo_ap_pah_on = true;
	  demo_ap_pah_on_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ap_pah_on_file,
				demo_ap_pah_on_timeArray,
				demo_ap_pah_on_daArray,
				demo_ap_pah_on_ntime);
	  token6 >> token_value;
	  demo_ap_pah_on_startTime = token_value;
	  break;
	}
      case demo_ap_alh_on_flag:
	{
	  demo_ap_alh_on = true;
	  demo_ap_alh_on_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ap_alh_on_file,
				demo_ap_alh_on_timeArray,
				demo_ap_alh_on_daArray,
				demo_ap_alh_on_ntime);
	  token6 >> token_value;
	  demo_ap_alh_on_startTime = token_value;
	  break;
	}
      case demo_ap_rah_on_flag:
	{
	  demo_ap_rah_on = true;
	  demo_ap_rah_on_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ap_rah_on_file,
				demo_ap_rah_on_timeArray,
				demo_ap_rah_on_daArray,
				demo_ap_rah_on_ntime);
	  token6 >> token_value;
	  demo_ap_rah_on_startTime = token_value;
	  break;
	}
       case demo_ap_hh_on_flag:
	{
	  demo_ap_hh_on = true;
	  demo_ap_hh_on_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ap_hh_on_file,
				demo_ap_hh_on_timeArray,
				demo_ap_hh_on_daArray,
				demo_ap_hh_on_ntime);
	  token6 >> token_value;
	  demo_ap_hh_on_startTime = token_value;
	  break;
	}
     case demo_ap_Theta_ref_flag:
	{
	  demo_ap_Theta_ref = true;
	  demo_ap_Theta_ref_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ap_Theta_ref_file,
				demo_ap_Theta_ref_timeArray,
				demo_ap_Theta_ref_daArray,
				demo_ap_Theta_ref_ntime);
	  token6 >> token_value;
	  demo_ap_Theta_ref_startTime = token_value;
	  break;
	}
      case demo_ap_alt_ref_flag:
	{
	  demo_ap_alt_ref = true;
	  demo_ap_alt_ref_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ap_alt_ref_file,
				demo_ap_alt_ref_timeArray,
				demo_ap_alt_ref_daArray,
				demo_ap_alt_ref_ntime);
	  token6 >> token_value;
	  demo_ap_alt_ref_startTime = token_value;
	  break;
	}
      case demo_ap_Phi_ref_flag:
	{
	  demo_ap_Phi_ref = true;
	  demo_ap_Phi_ref_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ap_Phi_ref_file,
				demo_ap_Phi_ref_timeArray,
				demo_ap_Phi_ref_daArray,
				demo_ap_Phi_ref_ntime);
	  token6 >> token_value;
	  demo_ap_Phi_ref_startTime = token_value;
	  break;
	}
      case demo_ap_Psi_ref_flag:
	{
	  demo_ap_Psi_ref = true;
	  demo_ap_Psi_ref_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ap_Psi_ref_file,
				demo_ap_Psi_ref_timeArray,
				demo_ap_Psi_ref_daArray,
				demo_ap_Psi_ref_ntime);
	  token6 >> token_value;
	  demo_ap_Psi_ref_startTime = token_value;
	  break;
	}
      case tactilefadef_flag:
	{
	  int tactilefadef_index, i;
	  string tactilefadef_file;
	  double flap_value;
	  tactilefadef = true;
	  tactilefadef_file = aircraft_directory + linetoken3;
	  token4 >> tactilefadef_index;
	  if (tactilefadef_index < 0 || tactilefadef_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (tactilefadef_index > tactilefadef_nf)
	    tactilefadef_nf = tactilefadef_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> tactilefadef_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  tactilefadef_fArray[tactilefadef_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (tactilefadef_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(tactilefadef_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, tactilefadef_aArray, tactilefadef_index);
	  d_1_to_2(datafile_yArray, tactilefadef_deArray, tactilefadef_index);
	  d_2_to_3(datafile_zArray, tactilefadef_tactileArray, tactilefadef_index);
	  i_1_to_2(datafile_nxArray, tactilefadef_nAlphaArray, tactilefadef_index);
	  tactilefadef_nde[tactilefadef_index] = datafile_ny;
	  if (tactilefadef_first==true)
	    {
	      if (tactilefadef_nice == 1)
		{
		  tactilefadef_na_nice = datafile_nxArray[1];
		  tactilefadef_nde_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, tactilefadef_deArray_nice);
		  for (i=1; i<=tactilefadef_na_nice; i++)
		    tactilefadef_aArray_nice[i] = datafile_xArray[1][i];
		}
	      tactilefadef_first=false;
	    }
	  break;
	}
      case tactile_pitch_flag:
	{
	  tactile_pitch = 1;
	  break;
	}
      case demo_tactile_flag:
	{
	  demo_tactile = true;
	  demo_tactile_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_tactile_file,
				demo_tactile_timeArray,
				demo_tactile_daArray,
				demo_tactile_ntime);
	  token6 >> token_value;
	  demo_tactile_startTime = token_value;
	  break;
	}
      case demo_ice_tail_flag:
	{
	  demo_ice_tail = true;
	  demo_ice_tail_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ice_tail_file,
				demo_ice_tail_timeArray,
				demo_ice_tail_daArray,
				demo_ice_tail_ntime);
	  token6 >> token_value;
	  demo_ice_tail_startTime = token_value;
	  break;
	}
      case demo_ice_left_flag:
	{
	  demo_ice_left = true;
	  demo_ice_left_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ice_left_file,
				demo_ice_left_timeArray,
				demo_ice_left_daArray,
				demo_ice_left_ntime);
	  token6 >> token_value;
	  demo_ice_left_startTime = token_value;
	  break;
	}
      case demo_ice_right_flag:
	{
	  demo_ice_right = true;
	  demo_ice_right_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(demo_ice_right_file,
				demo_ice_right_timeArray,
				demo_ice_right_daArray,
				demo_ice_right_ntime);
	  token6 >> token_value;
	  demo_ice_right_startTime = token_value;
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
