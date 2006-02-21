/********************************************************************** 
 
 FILENAME:     uiuc_map_CD.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the CD map

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      04/08/2000   initial release
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (CXfxxf0)
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

#include "uiuc_map_CD.h"


void uiuc_map_CD()
{
  CD_map["CDo"]                   =      CDo_flag                   ;
  CD_map["CDK"]                   =      CDK_flag                   ;
  CD_map["CLK"]                   =      CLK_flag                   ;
  CD_map["CD_a"]                  =      CD_a_flag                  ;
  CD_map["CD_adot"]               =      CD_adot_flag               ;
  CD_map["CD_q"]                  =      CD_q_flag                  ;
  CD_map["CD_ih"]                 =      CD_ih_flag                 ;
  CD_map["CD_de"]                 =      CD_de_flag                 ;
  CD_map["CD_dr"]                 =      CD_dr_flag                 ;
  CD_map["CD_da"]                 =      CD_da_flag                 ;
  CD_map["CD_beta"]               =      CD_beta_flag               ;
  CD_map["CD_df"]                 =      CD_df_flag                 ;
  CD_map["CD_ds"]                 =      CD_ds_flag                 ;
  CD_map["CD_dg"]                 =      CD_dg_flag                 ;
  CD_map["CDfa"]                  =      CDfa_flag                  ;
  CD_map["CDfCL"]                 =      CDfCL_flag                 ;
  CD_map["CDfade"]                =      CDfade_flag                ;
  CD_map["CDfdf"]                 =      CDfdf_flag                 ;
  CD_map["CDfadf"]                =      CDfadf_flag                ;
  CD_map["CXo"]                   =      CXo_flag                   ;
  CD_map["CXK"]                   =      CXK_flag                   ;
  CD_map["CX_a"]                  =      CX_a_flag                  ;
  CD_map["CX_a2"]                 =      CX_a2_flag                 ;
  CD_map["CX_a3"]                 =      CX_a3_flag                 ;
  CD_map["CX_adot"]               =      CX_adot_flag               ;
  CD_map["CX_q"]                  =      CX_q_flag                  ;
  CD_map["CX_de"]                 =      CX_de_flag                 ;
  CD_map["CX_dr"]                 =      CX_dr_flag                 ;
  CD_map["CX_df"]                 =      CX_df_flag                 ;
  CD_map["CX_adf"]                =      CX_adf_flag                ;
  CD_map["CXfabetaf"]             =      CXfabetaf_flag             ;
  CD_map["CXfadef"]               =      CXfadef_flag               ;
  CD_map["CXfaqf"]                =      CXfaqf_flag                ;
}

// end uiuc_map_CD.cpp
