/********************************************************************** 
 * 
 * FILENAME:     uiuc_initializemaps.cpp 
 *
 * ---------------------------------------------------------------------- 
 *
 * DESCRIPTION:  Initializes the maps for various keywords 
 *
 * ----------------------------------------------------------------------
 * 
 * STATUS:       alpha version
 *
 * ----------------------------------------------------------------------
 * 
 * REFERENCES:   
 * 
 * ----------------------------------------------------------------------
 * 
 * HISTORY:      01/26/2000   initial release
 * 
 * ----------------------------------------------------------------------
 * 
 * AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
 * 
 * ----------------------------------------------------------------------
 * 
 * VARIABLES:
 * 
 * ----------------------------------------------------------------------
 * 
 * INPUTS:       *
 * 
 * ----------------------------------------------------------------------
 * 
 * OUTPUTS:      *
 * 
 * ----------------------------------------------------------------------
 * 
 * CALLED BY:    uiuc_wrapper.cpp
 * 
 * ----------------------------------------------------------------------
 * 
 * CALLS TO:     *
 * 
 * ----------------------------------------------------------------------
 * 
 * COPYRIGHT:    (C) 2000 by Michael Selig
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA or view http://www.gnu.org/copyleft/gpl.html.
 * 
 ***********************************************************************/


#include "uiuc_initializemaps.h"

void uiuc_initializemaps3 ()
{
        ice_map["iceTime"] = iceTime_flag;
        ice_map["transientTime"] =  transientTime_flag;
        ice_map["eta_final"] = eta_final_flag;
        ice_map["kCDo"] = kCDo_flag;
        ice_map["kCDK"] = kCDK_flag;
        ice_map["kCD_a"] = kCD_a_flag;
        ice_map["kCD_de"] = kCD_de_flag;
        ice_map["kCLo"] = kCLo_flag;
        ice_map["kCL_a"] = kCL_a_flag;
        ice_map["kCL_adot"] = kCL_adot_flag;
        ice_map["kCL_q"] = kCL_q_flag;
        ice_map["kCL_de"] = kCL_de_flag;
        ice_map["kCmo"] = kCmo_flag;
        ice_map["kCm_a"] = kCm_a_flag;
        ice_map["kCm_adot"] = kCm_adot_flag;
        ice_map["kCm_q"] = kCm_q_flag;
        ice_map["kCm_de"] = kCm_de_flag;
        ice_map["kCYo"] = kCYo_flag;
        ice_map["kCY_beta"] = kCY_beta_flag;
        ice_map["kCY_p"] = kCY_p_flag;
        ice_map["kCY_r"] = kCY_r_flag;
        ice_map["kCY_da"] = kCY_da_flag;
        ice_map["kCY_dr"] = kCY_dr_flag;
        ice_map["kClo"] = kClo_flag;
        ice_map["kCl_beta"] = kCl_beta_flag;
        ice_map["kCl_p"] = kCl_p_flag;
        ice_map["kCl_r"] = kCl_r_flag;
        ice_map["kCl_da"] = kCl_da_flag;
        ice_map["kCl_dr"] = kCl_dr_flag;
        ice_map["kCno"] = kCno_flag;
        ice_map["kCn_beta"] = kCn_beta_flag;
        ice_map["kCn_p"] = kCn_p_flag;
        ice_map["kCn_r"] = kCn_r_flag;
        ice_map["kCn_da"] = kCn_da_flag;
        ice_map["kCn_dr"] = kCn_dr_flag;
}

// end uiuc_initializemaps.cpp
