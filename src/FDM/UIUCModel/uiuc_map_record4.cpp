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
  record_map["CL"]                =      CL_record                  ;
  record_map["CLfaI"]             =      CLfaI_record               ;
  record_map["CLfadeI"]           =      CLfadeI_record             ;
  record_map["CLfdfI"]            =      CLfdfI_record              ;
  record_map["CLfadfI"]           =      CLfadfI_record             ;
  record_map["CZ"]                =      CZ_record                  ;
  record_map["Cm"]                =      Cm_record                  ;
  record_map["CmfaI"]             =      CmfaI_record               ;
  record_map["CmfadeI"]           =      CmfadeI_record             ;
  record_map["CmfdfI"]            =      CmfdfI_record              ;
  record_map["CmfadfI"]           =      CmfadfI_record             ;
  record_map["CY"]                =      CY_record                  ;
  record_map["CYfadaI"]           =      CYfadaI_record             ;
  record_map["CYfbetadrI"]        =      CYfbetadrI_record          ;
  record_map["Cl"]                =      Cl_record                  ;
  record_map["ClfadaI"]           =      ClfadaI_record             ;
  record_map["ClfbetadrI"]        =      ClfbetadrI_record          ;
  record_map["Cn"]                =      Cn_record                  ;
  record_map["CnfadaI"]           =      CnfadaI_record             ;
  record_map["CnfbetadrI"]        =      CnfbetadrI_record          ;


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
