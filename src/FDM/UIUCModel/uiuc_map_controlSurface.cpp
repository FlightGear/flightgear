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
               06/18/2001   (RD) Added aileron_input, rudder_input,
	                    pilot_elev_no, pilot_ail_no, and 
			    pilot_rud_no
	       11/12/2001   (RD) Added flap_max, flap_rate, and

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
  controlSurface_map["aileron_input"]    = aileron_input_flag       ;
  controlSurface_map["rudder_input"]     = rudder_input_flag        ;
  controlSurface_map["flap_pos_input"]   = flap_pos_input_flag      ;
  controlSurface_map["pilot_elev_no"]    = pilot_elev_no_flag       ;
  controlSurface_map["pilot_ail_no"]     = pilot_ail_no_flag        ;
  controlSurface_map["pilot_rud_no"]     = pilot_rud_no_flag        ;
  controlSurface_map["flap_max"]         = flap_max_flag            ;
  controlSurface_map["flap_rate"]        = flap_rate_flag           ;
}

// end uiuc_map_controlSurface.cpp
