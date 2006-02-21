/********************************************************************** 
 
 FILENAME:     uiuc_map_CL.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the CL map

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      04/08/2000   initial release
               06/18/2001   Added CZfa
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (CZfxxf0)
	       11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (CZfxxf).  Zero flap vairables removed.

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

#include "uiuc_map_CL.h"


void uiuc_map_CL()
{
  CL_map["CLo"]                   =      CLo_flag                   ;
  CL_map["CL_a"]                  =      CL_a_flag                  ;
  CL_map["CL_adot"]               =      CL_adot_flag               ;
  CL_map["CL_q"]                  =      CL_q_flag                  ;
  CL_map["CL_ih"]                 =      CL_ih_flag                 ;
  CL_map["CL_de"]                 =      CL_de_flag                 ;
  CL_map["CL_df"]                 =      CL_df_flag                 ;
  CL_map["CL_ds"]                 =      CL_ds_flag                 ;
  CL_map["CL_dg"]                 =      CL_dg_flag                 ;
  CL_map["CLfa"]                  =      CLfa_flag                  ;
  CL_map["CLfade"]                =      CLfade_flag                ; 
  CL_map["CLfdf"]                 =      CLfdf_flag                 ;
  CL_map["CLfadf"]                =      CLfadf_flag                ; 
  CL_map["CZo"]                   =      CZo_flag                   ;
  CL_map["CZ_a"]                  =      CZ_a_flag                  ;
  CL_map["CZ_a2"]                 =      CZ_a2_flag                 ;
  CL_map["CZ_a3"]                 =      CZ_a3_flag                 ;
  CL_map["CZ_adot"]               =      CZ_adot_flag               ;
  CL_map["CZ_q"]                  =      CZ_q_flag                  ;
  CL_map["CZ_de"]                 =      CZ_de_flag                 ;
  CL_map["CZ_deb2"]               =      CZ_deb2_flag               ;
  CL_map["CZ_df"]                 =      CZ_df_flag                 ;
  CL_map["CZ_adf"]                =      CZ_adf_flag                ;
  CL_map["CZfa"]                  =      CZfa_flag                  ;
  CL_map["CZfabetaf"]             =      CZfabetaf_flag             ;
  CL_map["CZfadef"]               =      CZfadef_flag               ;
  CL_map["CZfaqf"]                =      CZfaqf_flag                ;
}

// end uiuc_map_CL.cpp
