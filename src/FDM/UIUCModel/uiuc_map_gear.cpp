/********************************************************************** 
 
 FILENAME:     uiuc_map_gear.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the gear map

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include "uiuc_map_gear.h"


void uiuc_map_gear()
{
  gear_map["Dx_gear"]             =      Dx_gear_flag               ;
  gear_map["Dy_gear"]             =      Dy_gear_flag               ;
  gear_map["Dz_gear"]             =      Dz_gear_flag               ;
  gear_map["cgear"]               =      cgear_flag                 ;
  gear_map["kgear"]               =      kgear_flag                 ;
  gear_map["muGear"]              =      muGear_flag                ;
  gear_map["strutLength"]         =      strutLength_flag           ;
  gear_map["gear_max"]            =      gear_max_flag              ;
  gear_map["gear_rate"]           =      gear_rate_flag             ;
  gear_map["use_gear"]            =      use_gear_flag              ;
}

// end uiuc_map_gear.cpp
