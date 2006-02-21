/********************************************************************** 
 
 FILENAME:     uiuc_map_init.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the init map

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      04/08/2000   initial release
               06/18/2001   (RD) Added Alpha, Beta, U_body
	                    V_body, and W_body.
               08/20/2003   (RD) Removed old_flap_routine

----------------------------------------------------------------------
 
 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
	       Robert Deters      <rdeters@uiuc.edu>
 
----------------------------------------------------------------------
 
 VARIABLES:
 
----------------------------------------------------------------------
 
 INPUTS:       none
 
----------------------------------------------------------------------
 
 OUTPUTS:      none
 
----------------------------------------------------------------------
 
 CALLED BY:    uiuc_initializemaps.cpp
 
----------------------------------------------------------------------
 
 CALLS TO:     none
 
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

#include "uiuc_map_init.h"


void uiuc_map_init()
{
  init_map["Dx_pilot"]            =      Dx_pilot_flag              ;
  init_map["Dy_pilot"]            =      Dy_pilot_flag              ;
  init_map["Dz_pilot"]            =      Dz_pilot_flag              ;
  init_map["Dx_cg"]               =      Dx_cg_flag                 ;
  init_map["Dy_cg"]               =      Dy_cg_flag                 ;
  init_map["Dz_cg"]               =      Dz_cg_flag                 ;
  init_map["Altitude"]            =      Altitude_flag              ;
  init_map["V_north"]             =      V_north_flag               ;
  init_map["V_east"]              =      V_east_flag                ;
  init_map["V_down"]              =      V_down_flag                ;
  init_map["P_body"]              =      P_body_flag                ;
  init_map["Q_body"]              =      Q_body_flag                ;
  init_map["R_body"]              =      R_body_flag                ;
  init_map["Phi"]                 =      Phi_flag                   ;
  init_map["Theta"]               =      Theta_flag                 ;
  init_map["Psi"]                 =      Psi_flag                   ;
  init_map["Long_trim"]           =      Long_trim_flag             ;
  init_map["recordRate"]          =      recordRate_flag            ;
  init_map["recordStartTime"]     =      recordStartTime_flag       ;
  init_map["use_V_rel_wind_2U"]   =      use_V_rel_wind_2U_flag     ;
  init_map["nondim_rate_V_rel_wind"]=    nondim_rate_V_rel_wind_flag;
  init_map["use_abs_U_body_2U"]   =      use_abs_U_body_2U_flag     ;
  init_map["dyn_on_speed"]        =      dyn_on_speed_flag          ;
  init_map["dyn_on_speed_zero"]   =      dyn_on_speed_zero_flag     ;
  init_map["use_dyn_on_speed_curve1"] =  use_dyn_on_speed_curve1_flag;
  init_map["use_Alpha_dot_on_speed"]  =  use_Alpha_dot_on_speed_flag;
  init_map["use_gamma_horiz_on_speed"]  =  use_gamma_horiz_on_speed_flag;
  init_map["downwashMode"]        =      downwashMode_flag          ;
  init_map["downwashCoef"]        =      downwashCoef_flag          ;
  init_map["Alpha"]               =      Alpha_flag                 ;
  init_map["Beta"]                =      Beta_flag                  ;
  init_map["U_body"]              =      U_body_flag                ;
  init_map["V_body"]              =      V_body_flag                ;
  init_map["W_body"]              =      W_body_flag                ;
  init_map["ignore_unknown_keywords"] = ignore_unknown_keywords_flag;
  init_map["trim_case_2"]         =      trim_case_2_flag           ;
  init_map["use_uiuc_network"]    =      use_uiuc_network_flag      ;
  init_map["icing_demo"]          =      icing_demo_flag            ;
  init_map["outside_control"]     =      outside_control_flag       ;
}

// end uiuc_map_init.cpp
