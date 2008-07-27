/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_gear.cpp

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

#include "uiuc_menu_gear.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_gear( const string& linetoken2, const string& linetoken3,
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

    switch(gear_map[linetoken2])
      {
      case Dx_gear_flag:
	{
	  int index;
	  token3 >> index;
	  if (index < 0 || index >= 16)
	    uiuc_warnings_errors(1, *command_line);
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  D_gear_v[index][0] = token_value;
	  gear_model[index] = true;
	  break;
	}
      case Dy_gear_flag:
	{
	  int index;
	  token3 >> index;
	  if (index < 0 || index >= 16)
	    uiuc_warnings_errors(1, *command_line);
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  D_gear_v[index][1] = token_value;
	  gear_model[index] = true;
	  break;
	}
      case Dz_gear_flag:
	{
	  int index;
	  token3 >> index;
	  if (index < 0 || index >= 16)
	    uiuc_warnings_errors(1, *command_line);
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  D_gear_v[index][2] = token_value;
	  gear_model[index] = true;
	  break;
	}
      case cgear_flag:
	{
	  int index;
	  token3 >> index;
	  if (index < 0 || index >= 16)
	    uiuc_warnings_errors(1, *command_line);
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  cgear[index] = token_value;
	  gear_model[index] = true;
	  break;
	}
      case kgear_flag:
	{
	  int index;
	  token3 >> index;
	  if (index < 0 || index >= 16)
	    uiuc_warnings_errors(1, *command_line);
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  kgear[index] = token_value;
	  gear_model[index] = true;
	  break;
	}
      case muGear_flag:
	{
	  int index;
	  token3 >> index;
	  if (index < 0 || index >= 16)
	    uiuc_warnings_errors(1, *command_line);
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  muGear[index] = token_value;
	  gear_model[index] = true;
	  break;
	}
      case strutLength_flag:
	{
	  int index;
	  token3 >> index;
	  if (index < 0 || index >= 16)
	    uiuc_warnings_errors(1, *command_line);
	  if (check_float(linetoken4))
	    token4 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  strutLength[index] = token_value;
	  gear_model[index] = true;
	  break;
	}
      case gear_max_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  use_gear = true;
	  gear_max = token_value;
	  break;
	}
      case gear_rate_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  use_gear = true;
	  gear_rate = token_value;
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
