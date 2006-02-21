/********************************************************************** 
 
 FILENAME:     uiuc_map_Croll.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the Cl map

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      04/08/2000   initial release
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (Clfxxf0)
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

#include "uiuc_map_Croll.h"


void uiuc_map_Croll()
{
  Cl_map["Clo"]                   =      Clo_flag                   ;
  Cl_map["Cl_beta"]               =      Cl_beta_flag               ;
  Cl_map["Cl_p"]                  =      Cl_p_flag                  ;
  Cl_map["Cl_r"]                  =      Cl_r_flag                  ;
  Cl_map["Cl_da"]                 =      Cl_da_flag                 ;
  Cl_map["Cl_dr"]                 =      Cl_dr_flag                 ;
  Cl_map["Cl_daa"]                =      Cl_daa_flag                ;
  Cl_map["Clfada"]                =      Clfada_flag                ;
  Cl_map["Clfbetadr"]             =      Clfbetadr_flag             ;
  Cl_map["Clfabetaf"]             =      Clfabetaf_flag             ;
  Cl_map["Clfadaf"]               =      Clfadaf_flag               ;
  Cl_map["Clfadrf"]               =      Clfadrf_flag               ;
  Cl_map["Clfapf"]                =      Clfapf_flag                ;
  Cl_map["Clfarf"]                =      Clfarf_flag                ;
}

// end uiuc_map_Croll.cpp
