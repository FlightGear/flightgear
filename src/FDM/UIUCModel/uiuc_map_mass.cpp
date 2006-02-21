/********************************************************************** 
 
 FILENAME:     uiuc_map_mass.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the mass map

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

#include "uiuc_map_mass.h"


void uiuc_map_mass()
{
  mass_map["Weight"]              =      Weight_flag                ;
  mass_map["Mass"]                =      Mass_flag                  ;
  mass_map["I_xx"]                =      I_xx_flag                  ;
  mass_map["I_yy"]                =      I_yy_flag                  ;
  mass_map["I_zz"]                =      I_zz_flag                  ;
  mass_map["I_xz"]                =      I_xz_flag                  ;
  mass_map["Mass_appMass_ratio"]  =      Mass_appMass_ratio_flag    ;
  mass_map["I_xx_appMass_ratio"]  =      I_xx_appMass_ratio_flag    ;
  mass_map["I_yy_appMass_ratio"]  =      I_yy_appMass_ratio_flag    ;
  mass_map["I_zz_appMass_ratio"]  =      I_zz_appMass_ratio_flag    ;
  mass_map["Mass_appMass"]        =      Mass_appMass_flag          ;
  mass_map["I_xx_appMass"]        =      I_xx_appMass_flag          ;
  mass_map["I_yy_appMass"]        =      I_yy_appMass_flag          ;
  mass_map["I_zz_appMass"]        =      I_zz_appMass_flag          ;
}

// end uiuc_map_mass.cpp
