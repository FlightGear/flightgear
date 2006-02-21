/********************************************************************** 
 
 FILENAME:     uiuc_map_keyword.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the main keyword map

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

#include "uiuc_map_keyword.h"


void uiuc_map_keyword()
{
  Keyword_map["init"]             =      init_flag                  ;
  Keyword_map["geometry"]         =      geometry_flag              ;
  Keyword_map["controlSurface"]   =      controlSurface_flag        ;
  Keyword_map["mass"]             =      mass_flag                  ;
  Keyword_map["engine"]           =      engine_flag                ;
  Keyword_map["CD"]               =      CD_flag                    ;
  Keyword_map["CL"]               =      CL_flag                    ;
  Keyword_map["Cm"]               =      Cm_flag                    ;
  Keyword_map["CY"]               =      CY_flag                    ;
  Keyword_map["Cl"]               =      Cl_flag                    ;
  Keyword_map["Cn"]               =      Cn_flag                    ;
  Keyword_map["gear"]             =      gear_flag                  ;
  Keyword_map["ice"]              =      ice_flag                   ;
  Keyword_map["record"]           =      record_flag                ;
  Keyword_map["misc"]             =      misc_flag                  ;
  Keyword_map["fog"]		  =	 fog_flag		    ;
}

// end uiuc_map_keyword.cpp
