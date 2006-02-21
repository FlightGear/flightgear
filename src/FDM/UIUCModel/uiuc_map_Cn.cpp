/********************************************************************** 
 
 FILENAME:     uiuc_map_Cn.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the Cn map

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      04/08/2000   initial release
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (Cnfxxf0)
	       11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (CXfxxf).  Zero flap vairables removed.

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

#include "uiuc_map_Cn.h"


void uiuc_map_Cn()
{
  Cn_map["Cno"]                   =      Cno_flag                   ;
  Cn_map["Cn_beta"]               =      Cn_beta_flag               ;
  Cn_map["Cn_p"]                  =      Cn_p_flag                  ;
  Cn_map["Cn_r"]                  =      Cn_r_flag                  ;
  Cn_map["Cn_da"]                 =      Cn_da_flag                 ;
  Cn_map["Cn_dr"]                 =      Cn_dr_flag                 ;
  Cn_map["Cn_q"]                  =      Cn_q_flag                  ;
  Cn_map["Cn_b3"]                 =      Cn_b3_flag                 ;
  Cn_map["Cnfada"]                =      Cnfada_flag                ;
  Cn_map["Cnfbetadr"]             =      Cnfbetadr_flag             ;
  Cn_map["Cnfabetaf"]             =      Cnfabetaf_flag             ;
  Cn_map["Cnfadaf"]               =      Cnfadaf_flag               ;
  Cn_map["Cnfadrf"]               =      Cnfadrf_flag               ;
  Cn_map["Cnfapf"]                =      Cnfapf_flag                ;
  Cn_map["Cnfarf"]                =      Cnfarf_flag                ;
}

// end uiuc_map_Cn.cpp
