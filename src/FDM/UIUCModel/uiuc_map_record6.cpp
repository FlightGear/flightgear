#include "uiuc_map_record6.h"


void uiuc_map_record6()
{
  /******************** Ice Detection ********************/
  record_map["CL_clean"]                = CL_clean_record                ;
  record_map["CD_clean"]                = CD_clean_record                ;
  record_map["Cm_clean"]                = Cm_clean_record                ;
  record_map["Ch_clean"]                = Ch_clean_record                ;
  record_map["Cl_clean"]                = Cl_clean_record                ;
  record_map["CL_iced"]                 = CL_iced_record                 ;
  record_map["CD_iced"]                 = CD_iced_record                 ;
  record_map["Cm_iced"]                 = Cm_iced_record                 ;
  record_map["Ch_iced"]                 = Ch_iced_record                 ;
  record_map["Cl_iced"]                 = Cl_iced_record                 ;
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

// end uiuc_map_record6.cpp
