/********************************************************************** 
 
 FILENAME:     uiuc_map_ice.cpp 

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the ice map

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

#include "uiuc_map_ice.h"


void uiuc_map_ice()
{
  ice_map["iceTime"]              =      iceTime_flag               ;
  ice_map["transientTime"]        =      transientTime_flag         ;
  ice_map["eta_ice_final"]        =      eta_ice_final_flag         ;
  ice_map["kCDo"]                 =      kCDo_flag                  ;
  ice_map["kCDK"]                 =      kCDK_flag                  ;
  ice_map["kCD_a"]                =      kCD_a_flag                 ;
  ice_map["kCD_adot"]             =      kCD_adot_flag              ;
  ice_map["kCD_q"]                =      kCD_q_flag                 ;
  ice_map["kCD_de"]               =      kCD_de_flag                ;
  ice_map["kCXo"]                 =      kCXo_flag                  ;
  ice_map["kCXK"]                 =      kCXK_flag                  ;
  ice_map["kCX_a"]                =      kCX_a_flag                 ;
  ice_map["kCX_a2"]               =      kCX_a2_flag                ;
  ice_map["kCX_a3"]               =      kCX_a3_flag                ;
  ice_map["kCX_adot"]             =      kCX_adot_flag              ;
  ice_map["kCX_q"]                =      kCX_q_flag                 ;
  ice_map["kCX_de"]               =      kCX_de_flag                ;
  ice_map["kCX_dr"]               =      kCX_dr_flag                ;
  ice_map["kCX_df"]               =      kCX_df_flag                ;
  ice_map["kCX_adf"]              =      kCX_adf_flag               ;
  ice_map["kCLo"]                 =      kCLo_flag                  ;
  ice_map["kCL_a"]                =      kCL_a_flag                 ;
  ice_map["kCL_adot"]             =      kCL_adot_flag              ;
  ice_map["kCL_q"]                =      kCL_q_flag                 ;
  ice_map["kCL_de"]               =      kCL_de_flag                ;
  ice_map["kCZo"]                 =      kCZo_flag                  ;
  ice_map["kCZ_a"]                =      kCZ_a_flag                 ;
  ice_map["kCZ_a2"]               =      kCZ_a2_flag                ;
  ice_map["kCZ_a3"]               =      kCZ_a3_flag                ;
  ice_map["kCZ_adot"]             =      kCZ_adot_flag              ;
  ice_map["kCZ_q"]                =      kCZ_q_flag                 ;
  ice_map["kCZ_de"]               =      kCZ_de_flag                ;
  ice_map["kCZ_deb2"]             =      kCZ_deb2_flag              ;
  ice_map["kCZ_df"]               =      kCZ_df_flag                ;
  ice_map["kCZ_adf"]              =      kCZ_adf_flag               ;
  ice_map["kCmo"]                 =      kCmo_flag                  ;
  ice_map["kCm_a"]                =      kCm_a_flag                 ;
  ice_map["kCm_a2"]               =      kCm_a2_flag                ;
  ice_map["kCm_adot"]             =      kCm_adot_flag              ;
  ice_map["kCm_q"]                =      kCm_q_flag                 ;
  ice_map["kCm_de"]               =      kCm_de_flag                ;
  ice_map["kCm_b2"]               =      kCm_b2_flag                ;
  ice_map["kCm_r"]                =      kCm_r_flag                 ;
  ice_map["kCm_df"]               =      kCm_df_flag                ;
  ice_map["kCYo"]                 =      kCYo_flag                  ;
  ice_map["kCY_beta"]             =      kCY_beta_flag              ;
  ice_map["kCY_p"]                =      kCY_p_flag                 ;
  ice_map["kCY_r"]                =      kCY_r_flag                 ;
  ice_map["kCY_da"]               =      kCY_da_flag                ;
  ice_map["kCY_dr"]               =      kCY_dr_flag                ;
  ice_map["kCY_dra"]              =      kCY_dra_flag               ;
  ice_map["kCY_bdot"]             =      kCY_bdot_flag              ;
  ice_map["kClo"]                 =      kClo_flag                  ;
  ice_map["kCl_beta"]             =      kCl_beta_flag              ;
  ice_map["kCl_p"]                =      kCl_p_flag                 ;
  ice_map["kCl_r"]                =      kCl_r_flag                 ;
  ice_map["kCl_da"]               =      kCl_da_flag                ;
  ice_map["kCl_dr"]               =      kCl_dr_flag                ;
  ice_map["kCl_daa"]              =      kCl_daa_flag               ;
  ice_map["kCno"]                 =      kCno_flag                  ;
  ice_map["kCn_beta"]             =      kCn_beta_flag              ;
  ice_map["kCn_p"]                =      kCn_p_flag                 ;
  ice_map["kCn_r"]                =      kCn_r_flag                 ;
  ice_map["kCn_da"]               =      kCn_da_flag                ;
  ice_map["kCn_dr"]               =      kCn_dr_flag                ;
  ice_map["kCn_q"]                =      kCn_q_flag                 ;
  ice_map["kCn_b3"]               =      kCn_b3_flag                ;
  ice_map["beta_probe_wing"]      =      beta_probe_wing_flag       ;
  ice_map["beta_probe_tail"]      =      beta_probe_tail_flag       ;
}

// end uiuc_map_ice.cpp
