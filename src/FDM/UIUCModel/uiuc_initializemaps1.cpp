/********************************************************************** 
 * 
 * FILENAME:     uiuc_initializemaps1.cpp 
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

void uiuc_initializemaps1 ()
{
        Keyword_map["init"] = init_flag;
        Keyword_map["geometry"] = geometry_flag;
        Keyword_map["controlSurface"] = controlSurface_flag;
        Keyword_map["mass"] = mass_flag;
        Keyword_map["engine"] = engine_flag;
        Keyword_map["CD"] = CD_flag;
        Keyword_map["CL"] = CL_flag;
        Keyword_map["Cm"] = Cm_flag;
        Keyword_map["CY"] = CY_flag;
        Keyword_map["Cl"] = Cl_flag;
        Keyword_map["Cn"] = Cn_flag;
        Keyword_map["gear"] = gear_flag;
        Keyword_map["ice"] = ice_flag;
        Keyword_map["record"] = record_flag;



        init_map["Dx_pilot"] = Dx_pilot_flag;
        init_map["Dy_pilot"] = Dy_pilot_flag;
        init_map["Dz_pilot"] = Dz_pilot_flag;
        init_map["V_north"] = V_north_flag;
        init_map["V_east"] = V_east_flag;
        init_map["V_down"] = V_down_flag;
        init_map["P_body"] = P_body_flag;
        init_map["Q_body"] = Q_body_flag;
        init_map["R_body"] = R_body_flag;
        init_map["Phi"] = Phi_flag;
        init_map["Theta"] = Theta_flag;
        init_map["Psi"] = Psi_flag;


        geometry_map["bw"] = bw_flag;
        geometry_map["cbar"] = cbar_flag;
        geometry_map["Sw"] = Sw_flag;


        controlSurface_map["de"] = de_flag;
        controlSurface_map["da"] = da_flag;
        controlSurface_map["dr"] = dr_flag;


        mass_map["Mass"] = Mass_flag;
        mass_map["I_xx"] = I_xx_flag;
        mass_map["I_yy"] = I_yy_flag;
        mass_map["I_zz"] = I_zz_flag;
        mass_map["I_xz"] = I_xz_flag;


        engine_map["simpleSingle"] = simpleSingle_flag;
        engine_map["c172"] = c172_flag;


        CD_map["CDo"] = CDo_flag;
        CD_map["CDK"] = CDK_flag;
        CD_map["CD_a"] = CD_a_flag;
        CD_map["CD_de"] = CD_de_flag;
        CD_map["CDfa"] = CDfa_flag;
        CD_map["CDfade"] = CDfade_flag;
}

// end uiuc_initializemaps.cpp
