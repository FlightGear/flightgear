/********************************************************************** 
 
 FILENAME:     uiuc_map_record4.cpp

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the record maps for aerodynamic 
               coefficients and ice detection parameters

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      06/03/2000   file creation
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (Cxfxxf0I)
               11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (CxfxxfI).  Removed zero flap vairables
	       02/13/2002   (RD) Added variables so linear aero model
	                    values can be recorded

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.
 
**********************************************************************/

#include "uiuc_map_record4.h"


void uiuc_map_record4()
{
  /****************** Aero Coefficients ******************/
  record_map["CD"]                =      CD_record                  ;
  record_map["CDfaI"]             =      CDfaI_record               ;
  record_map["CDfCLI"]            =      CDfCLI_record              ;
  record_map["CDfadeI"]           =      CDfadeI_record             ;
  record_map["CDfdfI"]            =      CDfdfI_record              ;
  record_map["CDfadfI"]           =      CDfadfI_record             ;
  record_map["CX"]                =      CX_record                  ;
  record_map["CXfabetafI"]        =      CXfabetafI_record          ;
  record_map["CXfadefI"]          =      CXfadefI_record            ;
  record_map["CXfaqfI"]           =      CXfaqfI_record             ;
  record_map["CDo_save"]          =      CDo_save_record            ;
  record_map["CDK_save"]          =      CDK_save_record            ;
  record_map["CD_a_save"]         =      CD_a_save_record           ;
  record_map["CD_adot_save"]      =      CD_adot_save_record        ;
  record_map["CD_q_save"]         =      CD_q_save_record           ;
  record_map["CD_ih_save"]        =      CD_ih_save_record          ;
  record_map["CD_de_save"]        =      CD_de_save_record          ;
  record_map["CXo_save"]          =      CXo_save_record            ;
  record_map["CXK_save"]          =      CXK_save_record            ;
  record_map["CX_a_save"]         =      CX_a_save_record           ;
  record_map["CX_a2_save"]        =      CX_a2_save_record          ;
  record_map["CX_a3_save"]        =      CX_a3_save_record          ;
  record_map["CX_adot_save"]      =      CX_adot_save_record        ;
  record_map["CX_q_save"]         =      CX_q_save_record           ;
  record_map["CX_de_save"]        =      CX_de_save_record          ;
  record_map["CX_dr_save"]        =      CX_dr_save_record          ;
  record_map["CX_df_save"]        =      CX_df_save_record          ;
  record_map["CX_adf_save"]       =      CX_adf_save_record         ;
  record_map["CL"]                =      CL_record                  ;
  record_map["CLfaI"]             =      CLfaI_record               ;
  record_map["CLfadeI"]           =      CLfadeI_record             ;
  record_map["CLfdfI"]            =      CLfdfI_record              ;
  record_map["CLfadfI"]           =      CLfadfI_record             ;
  record_map["CZ"]                =      CZ_record                  ;
  record_map["CZfaI"]             =      CZfaI_record               ;
  record_map["CZfabetafI"]        =      CZfabetafI_record          ;
  record_map["CZfadefI"]          =      CZfadefI_record            ;
  record_map["CZfaqfI"]           =      CZfaqfI_record             ;
  record_map["CLo_save"]          =      CLo_save_record            ;
  record_map["CL_a_save"]         =      CL_a_save_record           ;
  record_map["CL_adot_save"]      =      CL_adot_save_record        ;
  record_map["CL_q_save"]         =      CL_q_save_record           ;
  record_map["CL_ih_save"]        =      CL_ih_save_record          ;
  record_map["CL_de_save"]        =      CL_de_save_record          ;
  record_map["CZo_save"]          =      CZo_save_record            ;
  record_map["CZ_a_save"]         =      CZ_a_save_record           ;
  record_map["CZ_a2_save"]        =      CZ_a2_save_record          ;
  record_map["CZ_a3_save"]        =      CZ_a3_save_record          ;
  record_map["CZ_adot_save"]      =      CZ_adot_save_record        ;
  record_map["CZ_q_save"]         =      CZ_q_save_record           ;
  record_map["CZ_de_save"]        =      CZ_de_save_record          ;
  record_map["CZ_deb2_save"]      =      CZ_deb2_save_record        ;
  record_map["CZ_df_save"]        =      CZ_df_save_record          ;
  record_map["CZ_adf_save"]       =      CZ_adf_save_record         ;
  record_map["Cm"]                =      Cm_record                  ;
  record_map["CmfaI"]             =      CmfaI_record               ;
  record_map["CmfadeI"]           =      CmfadeI_record             ;
  record_map["CmfdfI"]            =      CmfdfI_record              ;
  record_map["CmfadfI"]           =      CmfadfI_record             ;
  record_map["CmfabetafI"]        =      CmfabetafI_record          ;
  record_map["CmfadefI"]          =      CmfadefI_record            ;
  record_map["CmfaqfI"]           =      CmfaqfI_record             ;
  record_map["Cmo_save"]          =      Cmo_save_record            ;
  record_map["Cm_a_save"]         =      Cm_a_save_record           ;
  record_map["Cm_a2_save"]        =      Cm_a2_save_record          ;
  record_map["Cm_adot_save"]      =      Cm_adot_save_record        ;
  record_map["Cm_q_save"]         =      Cm_q_save_record           ;
  record_map["Cm_ih_save"]        =      Cm_ih_save_record          ;
  record_map["Cm_de_save"]        =      Cm_de_save_record          ;
  record_map["Cm_b2_save"]        =      Cm_b2_save_record          ;
  record_map["Cm_r_save"]         =      Cm_r_save_record           ;
  record_map["Cm_df_save"]        =      Cm_df_save_record          ;
  record_map["CY"]                =      CY_record                  ;
  record_map["CYfadaI"]           =      CYfadaI_record             ;
  record_map["CYfbetadrI"]        =      CYfbetadrI_record          ;
  record_map["CYfabetafI"]        =      CYfabetafI_record          ;
  record_map["CYfadafI"]          =      CYfadafI_record            ;
  record_map["CYfadrfI"]          =      CYfadrfI_record            ;
  record_map["CYfapfI"]           =      CYfapfI_record             ;
  record_map["CYfarfI"]           =      CYfarfI_record             ;
  record_map["CYo_save"]          =      CYo_save_record            ;
  record_map["CY_beta_save"]      =      CY_beta_save_record        ;
  record_map["CY_p_save"]         =      CY_p_save_record           ;
  record_map["CY_r_save"]         =      CY_r_save_record           ;
  record_map["CY_da_save"]        =      CY_da_save_record          ;
  record_map["CY_dr_save"]        =      CY_dr_save_record          ;
  record_map["CY_dra_save"]       =      CY_dra_save_record         ;
  record_map["CY_bdot_save"]      =      CY_bdot_save_record        ;
  record_map["Cl"]                =      Cl_record                  ;
  record_map["ClfadaI"]           =      ClfadaI_record             ;
  record_map["ClfbetadrI"]        =      ClfbetadrI_record          ;
  record_map["ClfabetafI"]        =      ClfabetafI_record          ;
  record_map["ClfadafI"]          =      ClfadafI_record            ;
  record_map["ClfadrfI"]          =      ClfadrfI_record            ;
  record_map["ClfapfI"]           =      ClfapfI_record             ;
  record_map["ClfarfI"]           =      ClfarfI_record             ;
  record_map["Clo_save"]          =      Clo_save_record            ;
  record_map["Cl_beta_save"]      =      Cl_beta_save_record        ;
  record_map["Cl_p_save"]         =      Cl_p_save_record           ;
  record_map["Cl_r_save"]         =      Cl_r_save_record           ;
  record_map["Cl_da_save"]        =      Cl_da_save_record          ;
  record_map["Cl_dr_save"]        =      Cl_dr_save_record          ;
  record_map["Cl_daa_save"]       =      Cl_daa_save_record         ;
  record_map["Cn"]                =      Cn_record                  ;
  record_map["CnfadaI"]           =      CnfadaI_record             ;
  record_map["CnfbetadrI"]        =      CnfbetadrI_record          ;
  record_map["CnfabetafI"]        =      CnfabetafI_record          ;
  record_map["CnfadafI"]          =      CnfadafI_record            ;
  record_map["CnfadrfI"]          =      CnfadrfI_record            ;
  record_map["CnfapfI"]           =      CnfapfI_record             ;
  record_map["CnfarfI"]           =      CnfarfI_record             ;
  record_map["Cno_save"]          =      Cno_save_record            ;
  record_map["Cn_beta_save"]      =      Cn_beta_save_record        ;
  record_map["Cn_p_save"]         =      Cn_p_save_record           ;
  record_map["Cn_r_save"]         =      Cn_r_save_record           ;
  record_map["Cn_da_save"]        =      Cn_da_save_record          ;
  record_map["Cn_dr_save"]        =      Cn_dr_save_record          ;
  record_map["Cn_q_save"]         =      Cn_q_save_record           ;
  record_map["Cn_b3_save"]        =      Cn_b3_save_record          ;


  /******************** Ice Detection ********************/
  record_map["CLclean_wing"]            = CLclean_wing_record            ;
  record_map["CLiced_wing"]             = CLiced_wing_record             ;
  record_map["CLclean_tail"]            = CLclean_tail_record            ;
  record_map["CLiced_tail"]             = CLiced_tail_record             ;
  record_map["Lift_clean_wing"]         = Lift_clean_wing_record         ;
  record_map["Lift_iced_wing"]          = Lift_iced_wing_record          ;
  record_map["Lift_clean_tail"]         = Lift_clean_tail_record         ;
  record_map["Lift_iced_tail"]          = Lift_iced_tail_record          ;
  record_map["Gamma_clean_wing"]        = Gamma_clean_wing_record        ;
  record_map["Gamma_iced_wing"]         = Gamma_iced_wing_record         ;
  record_map["Gamma_clean_tail"]        = Gamma_clean_tail_record        ;
  record_map["Gamma_iced_tail"]         = Gamma_iced_tail_record         ;
  record_map["w_clean_wing"]            = w_clean_wing_record            ;
  record_map["w_iced_wing"]             = w_iced_wing_record             ;
  record_map["w_clean_tail"]            = w_clean_tail_record            ;
  record_map["w_iced_tail"]             = w_iced_tail_record             ;
  record_map["V_total_clean_wing"]      = V_total_clean_wing_record      ;
  record_map["V_total_iced_wing"]       = V_total_iced_wing_record       ;
  record_map["V_total_clean_tail"]      = V_total_clean_tail_record      ;
  record_map["V_total_iced_tail"]       = V_total_iced_tail_record       ;
  record_map["beta_flow_clean_wing"]    = beta_flow_clean_wing_record    ;
  record_map["beta_flow_clean_wing_deg"]= beta_flow_clean_wing_deg_record;
  record_map["beta_flow_iced_wing"]     = beta_flow_iced_wing_record     ;
  record_map["beta_flow_iced_wing_deg"] = beta_flow_iced_wing_deg_record ;
  record_map["beta_flow_clean_tail"]    = beta_flow_clean_tail_record    ;
  record_map["beta_flow_clean_tail_deg"]= beta_flow_clean_tail_deg_record;
  record_map["beta_flow_iced_tail"]     = beta_flow_iced_tail_record     ;
  record_map["beta_flow_iced_tail_deg"] = beta_flow_iced_tail_deg_record ;
  record_map["Dbeta_flow_wing"]         = Dbeta_flow_wing_record         ;
  record_map["Dbeta_flow_wing_deg"]     = Dbeta_flow_wing_deg_record     ;
  record_map["Dbeta_flow_tail"]         = Dbeta_flow_tail_record         ;
  record_map["Dbeta_flow_tail_deg"]     = Dbeta_flow_tail_deg_record     ;
  record_map["pct_beta_flow_wing"]      = pct_beta_flow_wing_record      ;
  record_map["pct_beta_flow_tail"]      = pct_beta_flow_tail_record      ;
}

// end uiuc_map_record4.cpp
