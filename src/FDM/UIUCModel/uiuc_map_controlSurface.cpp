/********************************************************************** 
 
 FILENAME:     uiuc_map_controlSurface.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the control surface map

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

#include "uiuc_map_controlSurface.h"


void uiuc_map_controlSurface()
{
  controlSurface_map["de"]               = de_flag                  ;
  controlSurface_map["da"]               = da_flag                  ;
  controlSurface_map["dr"]               = dr_flag                  ;
  controlSurface_map["set_Long_trim"]    = set_Long_trim_flag       ;
  controlSurface_map["set_Long_trim_deg"]= set_Long_trim_deg_flag   ;
  controlSurface_map["zero_Long_trim"]   = zero_Long_trim_flag      ;
  controlSurface_map["elevator_step"]    = elevator_step_flag       ;
  controlSurface_map["elevator_singlet"] = elevator_singlet_flag    ;
  controlSurface_map["elevator_doublet"] = elevator_doublet_flag    ;
  controlSurface_map["elevator_input"]   = elevator_input_flag      ;
}

// end uiuc_map_controlSurface.cpp
