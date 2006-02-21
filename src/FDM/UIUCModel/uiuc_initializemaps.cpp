/********************************************************************** 
 
 FILENAME:     uiuc_initializemaps.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the maps for various keywords 

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      01/26/2000   initial release
               04/08/2000   broke up into multiple map_xxxx functions
               03/09/2001   (DPM) initialize gear map

----------------------------------------------------------------------
 
 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
               David Megginson    <david@megginson.com>
 
----------------------------------------------------------------------
 
 VARIABLES:
 
----------------------------------------------------------------------
 
 INPUTS:       none
 
----------------------------------------------------------------------
 
 OUTPUTS:      none
 
----------------------------------------------------------------------
 
 CALLED BY:    uiuc_wrapper.cpp
 
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

#include "uiuc_initializemaps.h"


void uiuc_initializemaps()
{
  uiuc_map_keyword();
  uiuc_map_init();
  uiuc_map_geometry();
  uiuc_map_controlSurface();
  uiuc_map_mass();
  uiuc_map_engine();
  uiuc_map_CD();
  uiuc_map_CL();
  uiuc_map_Cm();
  uiuc_map_CY();
  uiuc_map_Croll();
  uiuc_map_Cn();
  uiuc_map_gear();
  uiuc_map_fog();
  uiuc_map_ice();
  uiuc_map_record1();
  uiuc_map_record2();
  uiuc_map_record3();
  uiuc_map_record4();
  uiuc_map_record5();
  uiuc_map_record6();
  uiuc_map_misc();
}

// end uiuc_initializemaps.cpp
