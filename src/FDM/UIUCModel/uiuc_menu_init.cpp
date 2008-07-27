/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_init.cpp

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
               08/20/2003   (RD) removed old_flap_routine

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

#include "uiuc_menu_init.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_init( const string& linetoken2, const string& linetoken3,
                 const string& linetoken4, const string& linetoken5, 
		 const string& linetoken6, const string& linetoken7, 
		 const string& linetoken8, const string& linetoken9,
		 const string& linetoken10, const string& aircraft_directory,
		 LIST command_line ) {
    double token_value;
    istringstream token3(linetoken3.c_str());
    istringstream token4(linetoken4.c_str());
    istringstream token5(linetoken5.c_str());
    istringstream token6(linetoken6.c_str());
    istringstream token7(linetoken7.c_str());
    istringstream token8(linetoken8.c_str());
    istringstream token9(linetoken9.c_str());
    istringstream token10(linetoken10.c_str());
    int token_value_recordRate;

    switch(init_map[linetoken2])
      {
      case Dx_pilot_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Dx_pilot = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Dy_pilot_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Dy_pilot = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Dz_pilot_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Dz_pilot = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Dx_cg_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Dx_cg = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Dy_cg_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Dy_cg = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Dz_cg_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Dz_cg = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Altitude_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Altitude = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case V_north_flag:
	{
	  if (check_float(linetoken3)) 
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  V_north = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case V_east_flag:
	{
	  initParts -> storeCommands (*command_line);
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  V_east = token_value;
	  break;
	}
      case V_down_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  V_down = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case P_body_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  P_body_init_true = true;
	  P_body_init = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Q_body_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Q_body_init_true = true;
	  Q_body_init = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case R_body_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  R_body_init_true = true;
	  R_body_init = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Phi_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Phi_init_true = true;
	  Phi_init = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Theta_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Theta_init_true = true;
	  Theta_init = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Psi_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Psi_init_true = true;
	  Psi_init = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case Long_trim_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Long_trim = token_value;
	  initParts -> storeCommands (*command_line);
	  break;
	}
      case recordRate_flag:
	{
	  //can't use check_float since variable is integer
	  token3 >> token_value_recordRate;
	  recordRate = 120 / token_value_recordRate;
	  break;
	}
      case recordStartTime_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  recordStartTime = token_value;
	  break;
	}
      case use_V_rel_wind_2U_flag:
	{
	  use_V_rel_wind_2U = true;
	  break;
	}
      case nondim_rate_V_rel_wind_flag:
	{
	  nondim_rate_V_rel_wind = true;
	  break;
	}
      case use_abs_U_body_2U_flag:
	{
	  use_abs_U_body_2U = true;
	  break;
	}
      case dyn_on_speed_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  dyn_on_speed = token_value;
	  break;
	}
      case dyn_on_speed_zero_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  dyn_on_speed_zero = token_value;
	  break;
	}
      case use_dyn_on_speed_curve1_flag:
	{
	  use_dyn_on_speed_curve1 = true;
	  break;
	}
      case use_Alpha_dot_on_speed_flag:
	{
	  use_Alpha_dot_on_speed = true;
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  Alpha_dot_on_speed = token_value;
	  break;
	}
      case use_gamma_horiz_on_speed_flag:
	{
	  use_gamma_horiz_on_speed = true;
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  gamma_horiz_on_speed = token_value;
	  break;
	}
      case downwashMode_flag:
	{
	  b_downwashMode = true;
	  token3 >> downwashMode;
	  if (downwashMode==100)
	    ;
	  // compute downwash using downwashCoef, do nothing here
	  else
	    uiuc_warnings_errors(4, *command_line); 
	  break;
	}
      case downwashCoef_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  downwashCoef = token_value;
	  break;
	}
      case Alpha_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Alpha_init_true = true;
	  Alpha_init = token_value;
	  break;
	}
      case Beta_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Beta_init_true = true;
	  Beta_init = token_value;
	  break;
	}
      case U_body_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  U_body_init_true = true;
	  U_body_init = token_value;
	  break;
	}
      case V_body_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  V_body_init_true = true;
	  V_body_init = token_value;
	  break;
	}
      case W_body_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  W_body_init_true = true;
	  W_body_init = token_value;
	  break;
	}
      case ignore_unknown_keywords_flag:
	{
	  ignore_unknown_keywords=true;
	  break;
	}
      case trim_case_2_flag:
	{
	  trim_case_2 = true;
	  break;
	}
      case use_uiuc_network_flag:
	{
	  use_uiuc_network = true;
	  server_IP = linetoken3;
	  token4 >> port_num;
	  break;
	}
      case icing_demo_flag:
	{
	  icing_demo = true;
	  break;
	}
      case outside_control_flag:
	{
	  outside_control = true;
	  break;
	}
      default:
	{
	  if (ignore_unknown_keywords){
	    // do nothing
	  } else {
	    // print error message
	    uiuc_warnings_errors(2, *command_line);
	  }
	  break;
	}
      };
}
