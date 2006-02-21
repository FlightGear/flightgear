/********************************************************************** 
 
 FILENAME:     uiuc_map_CY.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the CY map

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      04/08/2000   initial release
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (CYfxxf0)
	       11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (CYfxxf).  Zero flap vairables removed.

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

#include "uiuc_map_CY.h"


void uiuc_map_CY()
{
  CY_map["CYo"]                   =      CYo_flag                   ;
  CY_map["CY_beta"]               =      CY_beta_flag               ;
  CY_map["CY_p"]                  =      CY_p_flag                  ;
  CY_map["CY_r"]                  =      CY_r_flag                  ;
  CY_map["CY_da"]                 =      CY_da_flag                 ;
  CY_map["CY_dr"]                 =      CY_dr_flag                 ;
  CY_map["CY_dra"]                =      CY_dra_flag                ;
  CY_map["CY_bdot"]               =      CY_bdot_flag               ;
  CY_map["CYfada"]                =      CYfada_flag                ;
  CY_map["CYfbetadr"]             =      CYfbetadr_flag             ;
  CY_map["CYfabetaf"]             =      CYfabetaf_flag             ;
  CY_map["CYfadaf"]               =      CYfadaf_flag               ;
  CY_map["CYfadrf"]               =      CYfadrf_flag               ;
  CY_map["CYfapf"]                =      CYfapf_flag                ;
  CY_map["CYfarf"]                =      CYfarf_flag                ;
}

// end uiuc_map_CY.cpp
