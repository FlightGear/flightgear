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

void uiuc_initializemaps4 ()
{
        record_map["Dx_pilot"] =                      Dx_pilot_record         ;
        record_map["Dy_pilot"] =                      Dy_pilot_record         ;
        record_map["Dz_pilot"] =                      Dz_pilot_record         ;
        record_map["V_north"] =                       V_north_record          ;
        record_map["V_east"] =                        V_east_record           ;
        record_map["V_down"] =                        V_down_record           ;
        record_map["V_rel_wind"] =                    V_rel_wind_record       ;
        record_map["Dynamic_pressure"] =              Dynamic_pressure_record ;
        record_map["Alpha"] =                         Alpha_record            ;
        record_map["Alpha_dot"] =                     Alpha_dot_record        ;
        record_map["Beta"] =                          Beta_record             ;
        record_map["Beta_dot"] =                      Beta_dot_record         ;
        record_map["Gamma"] =                         Gamma_record            ;
        record_map["P_body"] =                        P_body_record           ;
        record_map["Q_body"] =                        Q_body_record           ;
        record_map["R_body"] =                        R_body_record           ;
        record_map["Phi"] =                           Phi_record              ;
        record_map["Theta"] =                         Theta_record            ;
        record_map["Psi"] =                           Psi_record              ;
        record_map["Theta_dot"] =                     Theta_dot_record        ;
        record_map["density"] =                       density_record          ;
        record_map["Mass"] =                          Mass_record             ;
        record_map["Simtime"] =                       Simtime_record          ;
        record_map["dt"] =                            dt_record               ;
        record_map["elevator"] =                      elevator_record         ;
        record_map["aileron"] =                       aileron_record          ;
        record_map["rudder"] =                        rudder_record           ;
        record_map["CD"] =                            CD_record               ;
        record_map["CDfaI"] =                         CDfaI_record            ;
        record_map["CDfadeI"] =                       CDfadeI_record          ;
        record_map["CL"] =                            CL_record               ;
        record_map["CLfaI"] =                         CLfaI_record            ;
        record_map["CLfadeI"] =                       CLfadeI_record          ;
        record_map["Cm"] =                            Cm_record               ;
        record_map["CmfadeI"] =                       CmfadeI_record          ;
        record_map["CY"] =                            CY_record               ;
        record_map["CYfadaI"] =                       CYfadaI_record          ;
        record_map["CYfbetadrI"] =                    CYfbetadrI_record       ;
        record_map["Cl"] =                            Cl_record               ;
        record_map["ClfadaI"] =                       ClfadaI_record          ;
        record_map["ClfbetadrI"] =                    ClfbetadrI_record       ;
        record_map["Cn"] =                            Cn_record               ;
        record_map["CnfadaI"] =                       CnfadaI_record          ;
        record_map["CnfbetadrI"] =                    CnfbetadrI_record       ;
        record_map["F_X_wind"] =                      F_X_wind_record         ;
        record_map["F_Y_wind"] =                      F_Y_wind_record         ;
        record_map["F_Z_wind"] =                      F_Z_wind_record         ;
        record_map["F_X_aero"] =                      F_X_aero_record         ;
        record_map["F_Y_aero"] =                      F_Y_aero_record         ;
        record_map["F_Z_aero"] =                      F_Z_aero_record         ;
        record_map["F_X_engine"] =                    F_X_engine_record       ;
        record_map["F_Y_engine"] =                    F_Y_engine_record       ;
        record_map["F_Z_engine"] =                    F_Z_engine_record       ;
        record_map["F_X_gear"] =                      F_X_gear_record         ;
        record_map["F_Y_gear"] =                      F_Y_gear_record         ;
        record_map["F_Z_gear"] =                      F_Z_gear_record         ;
        record_map["F_X"] =                           F_X_record              ;
        record_map["F_Y"] =                           F_Y_record              ;
        record_map["F_Z"] =                           F_Z_record              ;
        record_map["M_l_aero"] =                      M_l_aero_record         ;
        record_map["M_m_aero"] =                      M_m_aero_record         ;
        record_map["M_n_aero"] =                      M_n_aero_record         ;
        record_map["M_l_engine"] =                    M_l_engine_record       ;
        record_map["M_m_engine"] =                    M_m_engine_record       ;
        record_map["M_n_engine"] =                    M_n_engine_record       ;
        record_map["M_l_gear"] =                      M_l_gear_record         ;
        record_map["M_m_gear"] =                      M_m_gear_record         ;
        record_map["M_n_gear"] =                      M_n_gear_record         ;
        record_map["M_l_rp"] =                        M_l_rp_record           ;
        record_map["M_m_rp"] =                        M_m_rp_record           ;
        record_map["M_n_rp"] =                        M_n_rp_record           ;
}

// end uiuc_initializemaps.cpp
