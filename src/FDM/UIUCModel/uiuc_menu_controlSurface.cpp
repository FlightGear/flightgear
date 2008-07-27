/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_controlSurface.cpp

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

#include "uiuc_menu_controlSurface.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_controlSurface( const string& linetoken2, const string& linetoken3,
                           const string& linetoken4, const string& linetoken5,
                           const string& linetoken6, const string& linetoken7, 
			   const string& linetoken8, const string& linetoken9,
			   const string& linetoken10, 
			   const string& aircraft_directory, 
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

    switch(controlSurface_map[linetoken2])
      {
      case de_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  demax = token_value;
	  
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  demin = token_value;
	  break;
	}
      case da_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  damax = token_value;
	  
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  damin = token_value;
	  break;
	}
      case dr_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  drmax = token_value;
	  
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  drmin = token_value;
	  break;
	}
      case set_Long_trim_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  set_Long_trim = true;
	  elevator_tab = token_value;
	  break;
	}
      case set_Long_trim_deg_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  set_Long_trim = true;
	  elevator_tab = token_value * DEG_TO_RAD;
	  break;
	}
      case zero_Long_trim_flag:
	{
	  zero_Long_trim = true;
	  break;
	}
      case elevator_step_flag:
	{
	  // set step input flag
	  elevator_step = true;
	  
	  // read in step angle in degrees and convert
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  elevator_step_angle = token_value * DEG_TO_RAD;
	  
	  // read in step start time
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  elevator_step_startTime = token_value;
	  break;
	}
      case elevator_singlet_flag:
	{
	  // set singlet input flag
	  elevator_singlet = true;
	  
	  // read in singlet angle in degrees and convert
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  elevator_singlet_angle = token_value * DEG_TO_RAD;
	  
	  // read in singlet start time
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  elevator_singlet_startTime = token_value;
	  
	  // read in singlet duration
	  if (check_float(linetoken5))
	    token5 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  elevator_singlet_duration = token_value;
	  break;
	}
      case elevator_doublet_flag:
	{
	  // set doublet input flag
	  elevator_doublet = true;
	  
	  // read in doublet angle in degrees and convert
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  elevator_doublet_angle = token_value * DEG_TO_RAD;
	  
	  // read in doublet start time
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  elevator_doublet_startTime = token_value;
	  
	  // read in doublet duration
	  if (check_float(linetoken5))
	    token5 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  elevator_doublet_duration = token_value;
	  break;
	}
      case elevator_input_flag:
	{
	  elevator_input = true;
	  elevator_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(elevator_input_file,
				elevator_input_timeArray,
				elevator_input_deArray,
				elevator_input_ntime);
	  token6 >> token_value;
	  elevator_input_startTime = token_value;
	  break;
	}
      case aileron_input_flag:
	{
	  aileron_input = true;
	  aileron_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(aileron_input_file,
				aileron_input_timeArray,
				aileron_input_daArray,
				aileron_input_ntime);
	  token6 >> token_value;
	  aileron_input_startTime = token_value;
	  break;
	}
      case rudder_input_flag:
	{
	  rudder_input = true;
	  rudder_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(rudder_input_file,
				rudder_input_timeArray,
				rudder_input_drArray,
				rudder_input_ntime);
	  token6 >> token_value;
	  rudder_input_startTime = token_value;
	  break;
	}
      case flap_pos_input_flag:
	{
	  flap_pos_input = true;
	  flap_pos_input_file = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  uiuc_1DdataFileReader(flap_pos_input_file,
				flap_pos_input_timeArray,
				flap_pos_input_dfArray,
				flap_pos_input_ntime);
	  token6 >> token_value;
	  flap_pos_input_startTime = token_value;
	  break;
	}
      case pilot_elev_no_flag:
	{
	  pilot_elev_no_check = true;
	  break;
	}
      case pilot_ail_no_flag:
	{
	  pilot_ail_no_check = true;
	  break;
	}
      case pilot_rud_no_flag:
	{
	  pilot_rud_no_check = true;
	  break;
	}
      case flap_max_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  use_flaps = true;
	  flap_max = token_value;
	  break;
	}
      case flap_rate_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  use_flaps = true;
	  flap_rate = token_value;
	  break;
	}
      case spoiler_max_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  use_spoilers = true;
	  spoiler_max = token_value;
	  break;
	}
      case spoiler_rate_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  use_spoilers = true;
	  spoiler_rate = token_value;
	  break;
	}
      case aileron_sas_KP_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);

	  aileron_sas_KP = token_value;
	  break;
	}
      case aileron_sas_max_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  aileron_sas_max = token_value;
	  use_aileron_sas_max = true;
	  break;
	}
      case aileron_stick_gain_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  aileron_stick_gain = token_value;
	  use_aileron_stick_gain = true;
	  break;
	}
      case elevator_sas_KQ_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  elevator_sas_KQ = token_value;
	  break;
	}
      case elevator_sas_max_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  elevator_sas_max = token_value;
	  use_elevator_sas_max = true;
	  break;
	}
      case elevator_sas_min_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  elevator_sas_min = token_value;
	  use_elevator_sas_min = true;
	  break;
	}
      case elevator_stick_gain_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  elevator_stick_gain = token_value;
	  use_elevator_stick_gain = true;
	  break;
	}
      case rudder_sas_KR_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  rudder_sas_KR = token_value;
	  break;
	}
      case rudder_sas_max_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  rudder_sas_max = token_value;
	  use_rudder_sas_max = true;
	  break;
	}
      case rudder_stick_gain_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  rudder_stick_gain = token_value;
	  use_rudder_stick_gain = true;
	  break;
	}
      case use_aileron_sas_type1_flag:
	{
	  use_aileron_sas_type1 = true;
	  break;
	}
      case use_elevator_sas_type1_flag:
	{
	  use_elevator_sas_type1 = true;
	  break;
	}
      case use_rudder_sas_type1_flag:
	{
	  use_rudder_sas_type1 = true;
	  break;
	}
      case ap_pah_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);

	  ap_pah_start_time=token_value;
	  ap_pah_on = 1;
	  break;
	}
      case ap_alh_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);

	  ap_alh_start_time=token_value;
	  ap_alh_on = 1;
	  break;
	}
      case ap_rah_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);

	  ap_rah_start_time=token_value;
	  ap_rah_on = 1;
	  break;
	}
      case ap_hh_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);

	  ap_hh_start_time=token_value;
	  ap_hh_on = 1;
	  break;
	}
      case ap_Theta_ref_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  token4 >> token_value_convert1;
	  convert_y = uiuc_convert(token_value_convert1);

	  ap_Theta_ref_rad = token_value * convert_y;
	  break;
	}
      case ap_alt_ref_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);

	  ap_alt_ref_ft = token_value;
	  break;
	}
      case ap_Phi_ref_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  token4 >> token_value_convert1;
	  convert_y = uiuc_convert(token_value_convert1);

	  ap_Phi_ref_rad = token_value * convert_y;
	  break;
	}
      case ap_Psi_ref_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  token4 >> token_value_convert1;
	  convert_y = uiuc_convert(token_value_convert1);

	  ap_Psi_ref_rad = token_value * convert_y;
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
