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

----------------------------------------------------------------------
 
 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
 
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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.
 
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
  init_map["nondim_rate_V_rel_wind"]=    nondim_rate_V_rel_wind_flag;
  init_map["dyn_on_speed"]        =      dyn_on_speed_flag          ;
}

// end uiuc_map_init.cpp
