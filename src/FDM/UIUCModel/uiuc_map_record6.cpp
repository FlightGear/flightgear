#include "uiuc_map_record6.h"


void uiuc_map_record6()
{
  /******************** Ice Detection ********************/
  record_map["CL_clean"]                = CL_clean_record                ;
  record_map["CD_clean"]                = CD_clean_record                ;
  record_map["Cm_clean"]                = Cm_clean_record                ;
  record_map["Ch_clean"]                = Ch_clean_record                ;
  record_map["Cl_clean"]                = Cl_clean_record                ;
  record_map["Cn_clean"]                = Cn_clean_record                ;
  record_map["CL_iced"]                 = CL_iced_record                 ;
  record_map["CD_iced"]                 = CD_iced_record                 ;
  record_map["Cm_iced"]                 = Cm_iced_record                 ;
  record_map["Ch_iced"]                 = Ch_iced_record                 ;
  record_map["Cl_iced"]                 = Cl_iced_record                 ;
  record_map["Cn_iced"]                 = Cn_iced_record                 ;
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
  record_map["eta_ice"]                 = eta_ice_record                 ;
  record_map["eta_wing_left"]           = eta_wing_left_record           ;
  record_map["eta_wing_right"]          = eta_wing_right_record          ;
  record_map["eta_tail"]                = eta_tail_record                ;
  record_map["delta_CL"]                = delta_CL_record                ;
  record_map["delta_CD"]                = delta_CD_record                ;
  record_map["delta_Cm"]                = delta_Cm_record                ;
  record_map["delta_Cl"]                = delta_Cl_record                ;
  record_map["delta_Cn"]                = delta_Cn_record                ;
  record_map["boot_cycle_tail"]         = boot_cycle_tail_record         ;
  record_map["boot_cycle_wing_left"]    = boot_cycle_wing_left_record    ;
  record_map["boot_cycle_wing_right"]   = boot_cycle_wing_right_record   ;
  record_map["autoIPS_tail"]            = autoIPS_tail_record            ;
  record_map["autoIPS_wing_left"]       = autoIPS_wing_left_record       ;
  record_map["autoIPS_wing_right"]      = autoIPS_wing_right_record      ;
  record_map["eps_pitch_input"]         = eps_pitch_input_record         ;
  record_map["eps_alpha_max"]           = eps_alpha_max_record           ;
  record_map["eps_pitch_max"]           = eps_pitch_max_record           ;
  record_map["eps_pitch_min"]           = eps_pitch_min_record           ;
  record_map["eps_roll_max"]            = eps_roll_max_record            ;
  record_map["eps_thrust_min"]          = eps_thrust_min_record          ;
  record_map["eps_flap_max"]            = eps_flap_max_record            ;
  record_map["eps_airspeed_max"]        = eps_airspeed_max_record        ;
  record_map["eps_airspeed_min"]        = eps_airspeed_min_record        ;
  record_map["tactilefadefI"]           = tactilefadefI_record           ;
  /******************************autopilot****************************/
  record_map["ap_pah_on"]               = ap_pah_on_record               ;
  record_map["ap_alh_on"]               = ap_alh_on_record               ;
  record_map["ap_rah_on"]               = ap_rah_on_record               ;
  record_map["ap_hh_on"]                = ap_hh_on_record                ;
  record_map["ap_Theta_ref_deg"]        = ap_Theta_ref_deg_record        ;
  record_map["ap_Theta_ref_rad"]        = ap_Theta_ref_rad_record        ;
  record_map["ap_alt_ref_ft"]           = ap_alt_ref_ft_record           ;
  record_map["ap_Phi_ref_deg"]          = ap_Phi_ref_deg_record          ;
  record_map["ap_Phi_ref_rad"]          = ap_Phi_ref_rad_record          ;
  record_map["ap_Psi_ref_deg"]          = ap_Psi_ref_deg_record          ;
  record_map["ap_Psi_ref_rad"]          = ap_Psi_ref_rad_record          ;
  /***********************trigger variables**************************/
  record_map["trigger_on"]              = trigger_on_record              ;
  record_map["trigger_num"]             = trigger_num_record             ;
  record_map["trigger_toggle"]          = trigger_toggle_record          ;
  record_map["trigger_counter"]         = trigger_counter_record         ;
  /****************local to body transformation matrix**************/
  record_map["T_local_to_body_11"]      = T_local_to_body_11_record      ;
  record_map["T_local_to_body_12"]      = T_local_to_body_12_record      ;
  record_map["T_local_to_body_13"]      = T_local_to_body_13_record      ;
  record_map["T_local_to_body_21"]      = T_local_to_body_21_record      ;
  record_map["T_local_to_body_22"]      = T_local_to_body_22_record      ;
  record_map["T_local_to_body_23"]      = T_local_to_body_23_record      ;
  record_map["T_local_to_body_31"]      = T_local_to_body_31_record      ;
  record_map["T_local_to_body_32"]      = T_local_to_body_32_record      ;
  record_map["T_local_to_body_33"]      = T_local_to_body_33_record      ;
}

// end uiuc_map_record6.cpp
