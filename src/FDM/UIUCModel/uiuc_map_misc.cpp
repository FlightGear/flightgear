/********************************************************************** 
 
 FILENAME:     uiuc_map_misc.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the misc map

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      04/08/2000   initial release
               --/--/2002   (RD) added flapper

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

#include "uiuc_map_misc.h"


void uiuc_map_misc()
{
  misc_map["simpleHingeMomentCoef"] =    simpleHingeMomentCoef_flag ;
  //misc_map["dfTimefdf"]             =    dfTimefdf_flag             ;
  misc_map["flapper"]               =    flapper_flag               ;
  misc_map["flapper_phi_init"]      =    flapper_phi_init_flag      ;
}

// end uiuc_map_misc.cpp
