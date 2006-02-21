/********************************************************************** 
 
 FILENAME:     uiuc_map_geometry.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the geometry map

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

#include "uiuc_map_geometry.h"


void uiuc_map_geometry()
{
  geometry_map["bw"]              =      bw_flag                    ;
  geometry_map["cbar"]            =      cbar_flag                  ;
  geometry_map["Sw"]              =      Sw_flag                    ;
  geometry_map["ih"]              =      ih_flag                    ;
  geometry_map["bh"]              =      bh_flag                    ;
  geometry_map["chord_h"]         =      ch_flag                    ;
  geometry_map["Sh"]              =      Sh_flag                    ;
}

// end uiuc_map_geometry.cpp
