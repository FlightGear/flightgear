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

void uiuc_initializemaps2 ()
{
        CL_map["CLo"] = CLo_flag;
        CL_map["CL_a"] = CL_a_flag;
        CL_map["CL_adot"] = CL_adot_flag;
        CL_map["CL_q"] = CL_q_flag;
        CL_map["CL_de"] = CL_de_flag;
        CL_map["CLfa"] = CLfa_flag;
        CL_map["CLfade"] = CLfade_flag; 


        Cm_map["Cmo"] = Cmo_flag;
        Cm_map["Cm_a"] = Cm_a_flag;
        Cm_map["Cm_adot"] = Cm_adot_flag;
        Cm_map["Cm_q"] = Cm_q_flag;
        Cm_map["Cm_de"] = Cm_de_flag;
        Cm_map["Cmfade"] = Cmfade_flag;


        CY_map["CYo"] = CYo_flag;
        CY_map["CY_beta"] = CY_beta_flag;
        CY_map["CY_p"] = CY_p_flag;
        CY_map["CY_r"] = CY_r_flag;
        CY_map["CY_da"] = CY_da_flag;
        CY_map["CY_dr"] = CY_dr_flag;
        CY_map["CYfada"] = CYfada_flag;
        CY_map["CYfbetadr"] = CYfbetadr_flag;


        Cl_map["Clo"] = Clo_flag;
        Cl_map["Cl_beta"] = Cl_beta_flag;
        Cl_map["Cl_p"] = Cl_p_flag;
        Cl_map["Cl_r"] = Cl_r_flag;
        Cl_map["Cl_da"] = Cl_da_flag;
        Cl_map["Cl_dr"] = Cl_dr_flag;
        Cl_map["Clfada"] = Clfada_flag;
        Cl_map["Clfbetadr"] = Clfbetadr_flag;
        
        Cn_map["Cno"] = Cno_flag;
        Cn_map["Cn_beta"] = Cn_beta_flag;
        Cn_map["Cn_p"] = Cn_p_flag;
        Cn_map["Cn_r"] = Cn_r_flag;
        Cn_map["Cn_da"] = Cn_da_flag;
        Cn_map["Cn_dr"] = Cn_dr_flag;
        Cn_map["Cnfada"] = Cnfada_flag;
        Cn_map["Cnfbetadr"] = Cnfbetadr_flag;
}

// end uiuc_initializemaps.cpp
