/**********************************************************************

 FILENAME:     uiuc_aircraft.h

----------------------------------------------------------------------

 DESCRIPTION:  creates maps for all keywords and variables expected in 
               aircraft input file, includes all parameters that
               define the aircraft for use in the uiuc aircraft models.

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      01/26/2000   initial release
               02/10/2000   (JS) changed aeroData to aeroParts (etc.)
                            added Twin Otter 2.5 equation variables
                            added Dx_cg (etc.) to init & record maps
                            added controlsMixer to top level map
               02/18/2000   (JS) added variables needed for 1D file 
                            reading of CL and CD as functions of alpha
               02/29/2000   (JS) added variables needed for 2D file 
                            reading of CL, CD, and Cm as functions of 
                            alpha and delta_e; of CY and Cn as function 
                            of alpha and delta_r; and of Cl and Cn as 
                            functions of alpha and delta_a
	       03/02/2000   (JS) added record features for 1D and 2D 
	                    interpolations
               03/29/2000   (JS) added Cmfa interpolation functions 
	                    and Weight; added misc map
               04/01/2000   (JS) added throttle, longitudinal, lateral, 
	                    and rudder inputs to record map
               03/09/2001   (DPM) added support for gear
	       06/18/2001   (RD) added variables needed for aileron,
	                    rudder, and throttle_pct input files.  
			    Added Alpha, Beta, U_body, V_body, and 
                            W_body to init map.  Added variables 
			    needed to ignore elevator, aileron, and 
			    rudder pilot inputs.  Added CZ as a function
			    of alpha, CZfa.  Added twinotter to engine
			    group.
	       07/05/2001   (RD) added pilot_elev_no_check, pilot_ail_no
	                    _check, and pilot_rud_no_check variables.
			    This allows pilot to fly aircraft after the
			    input files have been used.
	       08/27/2001   (RD) Added xxx_init_true and xxx_init for
	                    P_body, Q_body, R_body, Phi, Theta, Psi,
			    U_body, V_body, and W_body to help in
			    starting the A/C at an initial condition.
	       10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (Cxfxxf0).
	       11/12/2001   (RD) Added variables needed for Twin Otter
	                    non-linear model with flaps (Cxfxxf). 
			    Zero flap variables removed.
               01/11/2002   (AP) Added keywords for bootTime
	       02/13/2002   (RD) Added variables so linear aero model
	                    values can be recorded
	       02/18/2002   (RD) Added variables necessary to use the
	                    uiuc_3Dinterp_quick() function.  Takes
			    advantage of data in a "nice" form (data
			    that are in a rectangular matrix).
	       04/21/2002   (MSS) Added new variables for apparent mass effects
                            and options for computing *_2U coefficient
                            scale factors.
               08/29/2002   (MSS) Added simpleSingleModel
	       09/18/2002   (MSS) Added downwash options
               03/03/2003   (RD) Changed flap_cmd_deg to flap_cmd (rad)
               03/16/2003   (RD) Added trigger variables
               08/20/2003   (RD) Removed old_flap_routine.  Changed spoiler
                            variables to match flap convention.  Changed
                            flap_pos_pct to flap_pos_norm
               10/31/2003   (RD) Added variables and keywords for pah and alh
                            autopilots
               11/04/2003   (RD) Added variables and keywords for rah and hh
                            autopilots

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
	       Robert Deters      <rdeters@uiuc.edu>
               David Megginson    <david@megginson.com>
	       Ann Peedikayil	  <peedikay@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       none

----------------------------------------------------------------------

 OUTPUTS:      none

----------------------------------------------------------------------

 CALLED BY:    uiuc_1DdataFileReader.cpp
               uiuc_2DdataFileReader.cpp
               uiuc_aerodeflections.cpp
               uiuc_coefficients.cpp
               uiuc_engine.cpp
               uiuc_initializemaps.cpp
               uiuc_menu.cpp
               uiuc_recorder.cpp

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


#ifndef _AIRCRAFT_H_
#define _AIRCRAFT_H_

#include <simgear/compiler.h>

#include <FDM/LaRCsim/ls_types.h>

#include <map>
#include <fstream>
#include <cmath>

#include "uiuc_parsefile.h"
#include "uiuc_flapdata.h"

typedef stack :: iterator LIST;

/* Add more keywords here if required*/
enum {init_flag = 1000, geometry_flag, controlSurface_flag, controlsMixer_flag, 
      mass_flag, engine_flag, CD_flag, CL_flag, Cm_flag, CY_flag, Cl_flag, 
      Cn_flag, gear_flag, ice_flag, record_flag, misc_flag, fog_flag};

// init ======= Initial values for equation of motion
// added to uiuc_map_init.cpp
enum {Dx_pilot_flag = 2000, 
      Dy_pilot_flag,
      Dz_pilot_flag,
      Dx_cg_flag,
      Dy_cg_flag,
      Dz_cg_flag,
      Altitude_flag,
      V_north_flag,
      V_east_flag,
      V_down_flag, 
      P_body_flag,
      Q_body_flag,
      R_body_flag, 
      Phi_flag,
      Theta_flag,
      Psi_flag,
      Long_trim_flag,
      recordRate_flag,
      recordStartTime_flag, 
      use_V_rel_wind_2U_flag,
      nondim_rate_V_rel_wind_flag, 
      use_abs_U_body_2U_flag,
      dyn_on_speed_flag,
      dyn_on_speed_zero_flag, 
      use_dyn_on_speed_curve1_flag,
      use_Alpha_dot_on_speed_flag, 
      use_gamma_horiz_on_speed_flag, 
      gamma_horiz_on_speed,
      downwashMode_flag,
      downwashCoef_flag,
      Alpha_flag, 
      Beta_flag,
      U_body_flag,
      V_body_flag,
      W_body_flag,
      ignore_unknown_keywords_flag,
      trim_case_2_flag,
      use_uiuc_network_flag,
      icing_demo_flag,
      outside_control_flag};

// geometry === Aircraft-specific geometric quantities
// added to uiuc_map_geometry.cpp
enum {bw_flag = 3000, cbar_flag, Sw_flag, ih_flag, bh_flag, ch_flag, Sh_flag};

// controlSurface = Control surface deflections and properties
// added to uiuc_map_controlSurface.cpp
enum {de_flag = 4000, da_flag, dr_flag, 
      set_Long_trim_flag, set_Long_trim_deg_flag, zero_Long_trim_flag, 
      elevator_step_flag, elevator_singlet_flag, elevator_doublet_flag,
      elevator_input_flag, aileron_input_flag, rudder_input_flag, flap_pos_input_flag, 
      pilot_elev_no_flag, pilot_ail_no_flag, pilot_rud_no_flag, 
      flap_max_flag, flap_rate_flag, use_flaps_flag, 
      spoiler_max_flag, spoiler_rate_flag, use_spoilers_flag, 
      aileron_sas_KP_flag, 
      aileron_sas_max_flag, 
      aileron_stick_gain_flag,
      elevator_sas_KQ_flag, 
      elevator_sas_max_flag, 
      elevator_sas_min_flag, 
      elevator_stick_gain_flag,
      rudder_sas_KR_flag, 
      rudder_sas_max_flag, 
      rudder_stick_gain_flag,
      use_elevator_sas_type1_flag, 
      use_aileron_sas_type1_flag, 
      use_rudder_sas_type1_flag,
      ap_pah_flag, ap_alh_flag, ap_Theta_ref_flag, ap_alt_ref_flag,
      ap_rah_flag, ap_Phi_ref_flag, ap_hh_flag, ap_Psi_ref_flag};

// controlsMixer == Controls mixer
enum {nomix_flag = 5000};

//mass ======== Aircraft-specific mass properties
// added to uiuc_map_mass.cpp
enum {Weight_flag = 6000, 
      Mass_flag, I_xx_flag, I_yy_flag, I_zz_flag, I_xz_flag,
      Mass_appMass_ratio_flag, I_xx_appMass_ratio_flag, 
      I_yy_appMass_ratio_flag, I_zz_appMass_ratio_flag, 
      Mass_appMass_flag,       I_xx_appMass_flag,      
      I_yy_appMass_flag,       I_zz_appMass_flag};

// engine ===== Propulsion data
// added to uiuc_map_engine.cpp
enum {simpleSingle_flag = 7000, simpleSingleModel_flag,
      c172_flag, cherokee_flag, gyroForce_Q_body_flag, gyroForce_R_body_flag, 
      omega_flag, omegaRPM_flag, polarInertia_flag,
      slipstream_effects_flag, propDia_flag,
      eta_q_Cm_q_flag,
      eta_q_Cm_adot_flag,
      eta_q_Cmfade_flag,
      eta_q_Cm_de_flag,
      eta_q_Cl_beta_flag,
      eta_q_Cl_p_flag,
      eta_q_Cl_r_flag,
      eta_q_Cl_dr_flag,
      eta_q_CY_beta_flag,
      eta_q_CY_p_flag,
      eta_q_CY_r_flag,
      eta_q_CY_dr_flag,
      eta_q_Cn_beta_flag,
      eta_q_Cn_p_flag,
      eta_q_Cn_r_flag,
      eta_q_Cn_dr_flag,
      Throttle_pct_input_flag, forcemom_flag, Xp_input_flag, Zp_input_flag, Mp_input_flag};

// CD ========= Aerodynamic x-force quantities (longitudinal)
// added to uiuc_map_CD.cpp
enum {CDo_flag = 8000, CDK_flag, CLK_flag, CD_a_flag, CD_adot_flag, CD_q_flag, CD_ih_flag, 
      CD_de_flag, CD_dr_flag, CD_da_flag, CD_beta_flag,
      CD_df_flag,
      CD_ds_flag,
      CD_dg_flag,
      CDfa_flag, CDfCL_flag, CDfade_flag, CDfdf_flag, CDfadf_flag, 
      CXo_flag, CXK_flag, CX_a_flag, CX_a2_flag, CX_a3_flag, CX_adot_flag, 
      CX_q_flag, CX_de_flag, CX_dr_flag, CX_df_flag, CX_adf_flag, 
      CXfabetaf_flag, CXfadef_flag, CXfaqf_flag};

// CL ========= Aerodynamic z-force quantities (longitudinal)
// added to uiuc_map_CL.cpp
enum {CLo_flag = 9000, CL_a_flag, CL_adot_flag, CL_q_flag, CL_ih_flag, CL_de_flag, 
      CL_df_flag,
      CL_ds_flag,
      CL_dg_flag,
      CLfa_flag, CLfade_flag, CLfdf_flag, CLfadf_flag, 
      CZo_flag, CZ_a_flag, CZ_a2_flag, CZ_a3_flag, CZ_adot_flag, 
      CZ_q_flag, CZ_de_flag, CZ_deb2_flag, CZ_df_flag, CZ_adf_flag, 
      CZfa_flag, CZfabetaf_flag, CZfadef_flag, CZfaqf_flag};

// Cm ========= Aerodynamic m-moment quantities (longitudinal)
// added to uiuc_map_Cm.cpp
enum {Cmo_flag = 10000, Cm_a_flag, Cm_a2_flag, Cm_adot_flag, Cm_q_flag, 
      Cm_ih_flag, Cm_de_flag, Cm_b2_flag, Cm_r_flag,
      Cm_df_flag,
      Cm_ds_flag,
      Cm_dg_flag,
      Cmfa_flag, Cmfade_flag, Cmfdf_flag, Cmfadf_flag, 
      Cmfabetaf_flag, Cmfadef_flag, Cmfaqf_flag};

// CY ========= Aerodynamic y-force quantities (lateral)
// added to uiuc_map_CY.cpp
enum {CYo_flag = 11000, CY_beta_flag, CY_p_flag, CY_r_flag, CY_da_flag, 
      CY_dr_flag, CY_dra_flag, CY_bdot_flag, CYfada_flag, CYfbetadr_flag, 
      CYfabetaf_flag, CYfadaf_flag, CYfadrf_flag, CYfapf_flag, CYfarf_flag};

// Cl ========= Aerodynamic l-moment quantities (lateral)
// added to uiuc_map_Cl.cpp
enum {Clo_flag = 12000, Cl_beta_flag, Cl_p_flag, Cl_r_flag, Cl_da_flag, 
      Cl_dr_flag, Cl_daa_flag, Clfada_flag, Clfbetadr_flag, Clfabetaf_flag,
      Clfadaf_flag, Clfadrf_flag, Clfapf_flag, Clfarf_flag};

// Cn ========= Aerodynamic n-moment quantities (lateral)
// added to uiuc_map_Cn.cpp
enum {Cno_flag = 13000, Cn_beta_flag, Cn_p_flag, Cn_r_flag, Cn_da_flag, 
      Cn_dr_flag, Cn_q_flag, Cn_b3_flag, Cnfada_flag, Cnfbetadr_flag, 
      Cnfabetaf_flag, Cnfadaf_flag, Cnfadrf_flag, Cnfapf_flag, Cnfarf_flag};

// gear ======= Landing gear model quantities
// added to uiuc_map_gear.cpp
enum {Dx_gear_flag = 14000, Dy_gear_flag, Dz_gear_flag, cgear_flag,
      kgear_flag, muGear_flag, strutLength_flag,
      gear_max_flag, gear_rate_flag, use_gear_flag};

// ice ======== Ice model quantities
// added to uiuc_map_ice.cpp
enum {iceTime_flag = 15000, transientTime_flag, eta_ice_final_flag, 
      beta_probe_wing_flag, beta_probe_tail_flag, 
      kCDo_flag, kCDK_flag, kCD_a_flag, kCD_adot_flag, kCD_q_flag, kCD_de_flag, 
      kCXo_flag, kCXK_flag, kCX_a_flag, kCX_a2_flag, kCX_a3_flag, kCX_adot_flag, 
      kCX_q_flag, kCX_de_flag, kCX_dr_flag, kCX_df_flag, kCX_adf_flag, 
      kCLo_flag, kCL_a_flag, kCL_adot_flag, kCL_q_flag, kCL_de_flag, 
      kCZo_flag, kCZ_a_flag, kCZ_a2_flag, kCZ_a3_flag, kCZ_adot_flag, 
      kCZ_q_flag, kCZ_de_flag, kCZ_deb2_flag, kCZ_df_flag, kCZ_adf_flag, 
      kCmo_flag, kCm_a_flag, kCm_a2_flag, kCm_adot_flag, kCm_q_flag, 
      kCm_de_flag, kCm_b2_flag, kCm_r_flag, kCm_df_flag, 
      kCYo_flag, kCY_beta_flag, kCY_p_flag, kCY_r_flag, kCY_da_flag, 
      kCY_dr_flag, kCY_dra_flag, kCY_bdot_flag, 
      kClo_flag, kCl_beta_flag, kCl_p_flag, kCl_r_flag, kCl_da_flag, 
      kCl_dr_flag, kCl_daa_flag, 
      kCno_flag, kCn_beta_flag, kCn_p_flag, kCn_r_flag, kCn_da_flag, 
      kCn_dr_flag, kCn_q_flag, kCn_b3_flag, bootTime_flag,
      
      eta_wing_left_input_flag, eta_wing_right_input_flag, 
      eta_tail_input_flag, nonlin_ice_case_flag, eta_tail_flag,
      eta_wing_left_flag, eta_wing_right_flag,

      demo_eps_alpha_max_flag, demo_eps_pitch_max_flag, 
      demo_eps_pitch_min_flag, demo_eps_roll_max_flag, 
      demo_eps_thrust_min_flag, demo_eps_flap_max_flag, 
      demo_eps_airspeed_max_flag, demo_eps_airspeed_min_flag,
      demo_boot_cycle_tail_flag, demo_boot_cycle_wing_left_flag,
      demo_boot_cycle_wing_right_flag, demo_eps_pitch_input_flag,
      tactilefadef_flag, tactile_pitch_flag, demo_ap_pah_on_flag,
      demo_ap_alh_on_flag, demo_ap_rah_on_flag, demo_ap_hh_on_flag,
      demo_ap_Theta_ref_flag, demo_ap_alt_ref_flag,
      demo_ap_Phi_ref_flag, demo_ap_Psi_ref_flag, 
      demo_tactile_flag, demo_ice_tail_flag, 
      demo_ice_left_flag, demo_ice_right_flag};

// record ===== Record desired quantites to file
enum {Simtime_record = 16000, dt_record, 

      cbar_2U_record, b_2U_record, ch_2U_record,

      // added to uiuc_map_record1.cpp
      Weight_record, Mass_record, I_xx_record, I_yy_record, I_zz_record, I_xz_record, 
      Mass_appMass_ratio_record, I_xx_appMass_ratio_record, 
      I_yy_appMass_ratio_record, I_zz_appMass_ratio_record, 
      Mass_appMass_record,       I_xx_appMass_record,      
      I_yy_appMass_record,       I_zz_appMass_record,

      // added to uiuc_map_record1.cpp
      Dx_pilot_record, Dy_pilot_record, Dz_pilot_record, 
      Dx_cg_record, Dy_cg_record, Dz_cg_record,
      Lat_geocentric_record, Lon_geocentric_record, Radius_to_vehicle_record, 
      Latitude_record, Longitude_record, Altitude_record, 
      Phi_record, Theta_record, Psi_record,
      Phi_deg_record, Theta_deg_record, Psi_deg_record, 

      // added to uiuc_map_record1.cpp
      V_dot_north_record, V_dot_east_record, V_dot_down_record, 
      U_dot_body_record, V_dot_body_record, W_dot_body_record, 
      A_X_pilot_record, A_Y_pilot_record, A_Z_pilot_record, 
      A_X_cg_record, A_Y_cg_record, A_Z_cg_record, 
      N_X_pilot_record, N_Y_pilot_record, N_Z_pilot_record, 
      N_X_cg_record, N_Y_cg_record, N_Z_cg_record, 
      P_dot_body_record, Q_dot_body_record, R_dot_body_record, 

      // added to uiuc_map_record2.cpp
      V_north_record, V_east_record, V_down_record, 
      V_north_rel_ground_record, V_east_rel_ground_record, V_down_rel_ground_record, 
      V_north_airmass_record, V_east_airmass_record, V_down_airmass_record, 
      V_north_rel_airmass_record, V_east_rel_airmass_record, V_down_rel_airmass_record, 
      U_gust_record, V_gust_record, W_gust_record, 
      U_body_record, V_body_record, W_body_record, 
      V_rel_wind_record, V_true_kts_record, V_rel_ground_record, 
      V_inertial_record, V_ground_speed_record, V_equiv_record, 
      V_equiv_kts_record, V_calibrated_record, V_calibrated_kts_record, 
      P_local_record, Q_local_record, R_local_record, 
      P_body_record, Q_body_record, R_body_record, 
      P_total_record, Q_total_record, R_total_record, 
      Phi_dot_record, Theta_dot_record, Psi_dot_record, 
      Latitude_dot_record, Longitude_dot_record, Radius_dot_record, 

      // added to uiuc_map_record2.cpp
      Alpha_record, Alpha_deg_record, Alpha_dot_record, Alpha_dot_deg_record, 
      Beta_record, Beta_deg_record, Beta_dot_record, Beta_dot_deg_record, 
      Gamma_vert_record, Gamma_vert_deg_record, Gamma_horiz_record, Gamma_horiz_deg_record,

      // added to uiuc_map_record3.cpp
      Density_record, V_sound_record, Mach_number_record, 
      Static_pressure_record, Total_pressure_record, Impact_pressure_record, 
      Dynamic_pressure_record, 
      Static_temperature_record, Total_temperature_record, 

      // added to uiuc_map_record3.cpp
      Gravity_record, Sea_level_radius_record, Earth_position_angle_record, 
      Runway_altitude_record, Runway_latitude_record, Runway_longitude_record, 
      Runway_heading_record, Radius_to_rwy_record, 
      D_pilot_north_of_rwy_record, D_pilot_east_of_rwy_record, D_pilot_above_rwy_record, 
      X_pilot_rwy_record, Y_pilot_rwy_record, H_pilot_rwy_record, 
      D_cg_north_of_rwy_record, D_cg_east_of_rwy_record, D_cg_above_rwy_record, 
      X_cg_rwy_record, Y_cg_rwy_record, H_cg_rwy_record, 

      // added to uiuc_map_record3.cpp
      Throttle_3_record, Throttle_pct_record, 

      // added to uiuc_map_record3.cpp
      Long_control_record, Long_trim_record, Long_trim_deg_record, 
      elevator_record, elevator_deg_record, 
      Lat_control_record, aileron_record, aileron_deg_record, 
      Rudder_pedal_record, rudder_record, rudder_deg_record, 
      Flap_handle_record, flap_cmd_record, flap_cmd_deg_record,
      flap_pos_record, flap_pos_deg_record, flap_pos_norm_record,
      Spoiler_handle_record, spoiler_cmd_deg_record, 
      spoiler_pos_deg_record, spoiler_pos_norm_record, spoiler_pos_record,
      spoiler_cmd_record,

      // added to uiuc_map_record4.cpp
      CD_record, CDfaI_record, CDfCLI_record, CDfadeI_record, CDfdfI_record, 
      CDfadfI_record, CX_record, CXfabetafI_record, CXfadefI_record, 
      CXfaqfI_record,
      CDo_save_record, CDK_save_record, CLK_save_record, CD_a_save_record, CD_adot_save_record,
      CD_q_save_record, CD_ih_save_record, 
      CD_de_save_record, CD_dr_save_record, CD_da_save_record, CD_beta_save_record, 
      CD_df_save_record,
      CD_ds_save_record,
      CD_dg_save_record,
      CXo_save_record,
      CXK_save_record, CX_a_save_record, CX_a2_save_record, CX_a3_save_record,
      CX_adot_save_record, CX_q_save_record, CX_de_save_record,
      CX_dr_save_record, CX_df_save_record, CX_adf_save_record,
      CL_record, CLfaI_record, CLfadeI_record, CLfdfI_record, CLfadfI_record, 
      CZ_record, CZfaI_record, CZfabetafI_record, CZfadefI_record, 
      CZfaqfI_record, 
      CLo_save_record, CL_a_save_record, CL_adot_save_record, CL_q_save_record,
      CL_ih_save_record, CL_de_save_record, 
      CL_df_save_record,
      CL_ds_save_record,
      CL_dg_save_record,
      CZo_save_record, CZ_a_save_record,
      CZ_a2_save_record, CZ_a3_save_record, CZ_adot_save_record,
      CZ_q_save_record, CZ_de_save_record, CZ_deb2_save_record,
      CZ_df_save_record, CZ_adf_save_record,
      Cm_record, CmfaI_record, CmfadeI_record, CmfdfI_record, CmfadfI_record, 
      CmfabetafI_record, CmfadefI_record, CmfaqfI_record,
      Cmo_save_record, Cm_a_save_record, Cm_a2_save_record,
      Cm_adot_save_record, Cm_q_save_record, Cm_ih_save_record,
      Cm_de_save_record, Cm_b2_save_record, Cm_r_save_record, 
      Cm_df_save_record,
      Cm_ds_save_record,
      Cm_dg_save_record,
      CY_record, CYfadaI_record, CYfbetadrI_record, CYfabetafI_record, 
      CYfadafI_record, CYfadrfI_record, CYfapfI_record, CYfarfI_record, 
      CYo_save_record, CY_beta_save_record, CY_p_save_record,
      CY_r_save_record, CY_da_save_record, CY_dr_save_record, 
      CY_dra_save_record, CY_bdot_save_record,
      Cl_record, ClfadaI_record, ClfbetadrI_record, ClfabetafI_record, 
      ClfadafI_record, ClfadrfI_record, ClfapfI_record, ClfarfI_record,
      Clo_save_record, Cl_beta_save_record, Cl_p_save_record, Cl_r_save_record,
      Cl_da_save_record, Cl_dr_save_record, Cl_daa_save_record,
      Cn_record, CnfadaI_record, CnfbetadrI_record, CnfabetafI_record, 
      CnfadafI_record, CnfadrfI_record, CnfapfI_record, CnfarfI_record, 
      Cno_save_record, Cn_beta_save_record, Cn_p_save_record, Cn_r_save_record,
      Cn_da_save_record, Cn_dr_save_record, Cn_q_save_record,
      Cn_b3_save_record,
      
      // added to uiuc_map_record5.cpp
      F_X_wind_record, F_Y_wind_record, F_Z_wind_record, 
      F_X_aero_record, F_Y_aero_record, F_Z_aero_record,
      F_X_engine_record, F_Y_engine_record, F_Z_engine_record, 
      F_X_gear_record, F_Y_gear_record, F_Z_gear_record, 
      F_X_record, F_Y_record, F_Z_record, 
      F_north_record, F_east_record, F_down_record, 
      
      // added to uiuc_map_record5.cpp
      M_l_aero_record, M_m_aero_record, M_n_aero_record, 
      M_l_engine_record, M_m_engine_record, M_n_engine_record, 
      M_l_gear_record, M_m_gear_record, M_n_gear_record, 
      M_l_rp_record, M_m_rp_record, M_n_rp_record,
      M_l_cg_record, M_m_cg_record, M_n_cg_record,

      // added to uiuc_map_record5.cpp
      flapper_freq_record, flapper_phi_record,
      flapper_phi_deg_record, flapper_Lift_record, flapper_Thrust_record,
      flapper_Inertia_record, flapper_Moment_record,

      // added to uiuc_map_record5.cpp
      debug1_record, 
      debug2_record, 
      debug3_record,
      V_down_fpm_record,
      eta_q_record,
      rpm_record,
      elevator_sas_deg_record,
      aileron_sas_deg_record,
      rudder_sas_deg_record,
      w_induced_record,
      downwashAngle_deg_record,
      alphaTail_deg_record,
      gammaWing_record,
      LD_record,
      gload_record,
      gyroMomentQ_record,
      gyroMomentR_record,

      // added to uiuc_map_record5.cpp
      Gear_handle_record, gear_cmd_norm_record, gear_pos_norm_record,

      // added to uiuc_map_record5.cpp
      debug4_record, 
      debug5_record, 
      debug6_record,
      debug7_record, 
      debug8_record, 
      debug9_record,
      debug10_record,

      // added to uiuc_map_record6.cpp
      CL_clean_record, CL_iced_record,
      CD_clean_record, CD_iced_record,
      Cm_clean_record, Cm_iced_record,
      Ch_clean_record, Ch_iced_record,
      Cl_clean_record, Cl_iced_record,
      Cn_clean_record, Cn_iced_record,
      CLclean_wing_record, CLiced_wing_record, 
      CLclean_tail_record, CLiced_tail_record, 
      Lift_clean_wing_record, Lift_iced_wing_record, 
      Lift_clean_tail_record, Lift_iced_tail_record, 
      Gamma_clean_wing_record, Gamma_iced_wing_record, 
      Gamma_clean_tail_record, Gamma_iced_tail_record, 
      w_clean_wing_record, w_iced_wing_record, 
      w_clean_tail_record, w_iced_tail_record, 
      V_total_clean_wing_record, V_total_iced_wing_record, 
      V_total_clean_tail_record, V_total_iced_tail_record, 
      beta_flow_clean_wing_record, beta_flow_clean_wing_deg_record, 
      beta_flow_iced_wing_record, beta_flow_iced_wing_deg_record, 
      beta_flow_clean_tail_record, beta_flow_clean_tail_deg_record, 
      beta_flow_iced_tail_record, beta_flow_iced_tail_deg_record, 
      Dbeta_flow_wing_record, Dbeta_flow_wing_deg_record, 
      Dbeta_flow_tail_record, Dbeta_flow_tail_deg_record, 
      pct_beta_flow_wing_record, pct_beta_flow_tail_record, eta_ice_record,
      eta_wing_right_record, eta_wing_left_record, eta_tail_record,
      delta_CL_record, delta_CD_record, delta_Cm_record, delta_Cl_record,
      delta_Cn_record,

      // added to uiuc_map_record6.cpp
      boot_cycle_tail_record, boot_cycle_wing_left_record,
      boot_cycle_wing_right_record, autoIPS_tail_record, 
      autoIPS_wing_left_record, autoIPS_wing_right_record,
      eps_pitch_input_record, eps_alpha_max_record, eps_pitch_max_record, 
      eps_pitch_min_record, eps_roll_max_record, eps_thrust_min_record, 
      eps_flap_max_record, eps_airspeed_max_record, eps_airspeed_min_record,
      tactilefadefI_record,

      // added to uiuc_map_record6.cpp
      ap_pah_on_record, ap_alh_on_record, ap_Theta_ref_deg_record,
      ap_Theta_ref_rad_record, ap_alt_ref_ft_record, trigger_on_record,
      ap_rah_on_record, ap_Phi_ref_rad_record, ap_Phi_ref_deg_record,
      ap_hh_on_record, ap_Psi_ref_rad_record, ap_Psi_ref_deg_record,
      trigger_num_record, trigger_toggle_record, trigger_counter_record,

      // added to uiuc_map_record6.cpp
      T_local_to_body_11_record, T_local_to_body_12_record,
      T_local_to_body_13_record, T_local_to_body_21_record,
      T_local_to_body_22_record, T_local_to_body_23_record,
      T_local_to_body_31_record, T_local_to_body_32_record,
      T_local_to_body_33_record};


// misc ======= Miscellaneous inputs
// added to uiuc_map_misc.cpp
enum {simpleHingeMomentCoef_flag = 17000, flapper_flag,
      flapper_phi_init_flag};

//321654
// fog ======== Fog field quantities
enum {fog_segments_flag = 18000, fog_point_flag}; 

//321654  
struct AIRCRAFT
{
  // ParseFile stuff [] Bipin to add more comments
  ParseFile *airplane;
#define  airplane           aircraft_->airplane
  ParseFile *initParts;
#define  initParts          aircraft_->initParts
  ParseFile *geometryParts;
#define  geometryParts      aircraft_->geometryParts
  ParseFile *massParts;
#define  massParts          aircraft_->massParts
  ParseFile *aeroDragParts;
#define  aeroDragParts      aircraft_->aeroDragParts
  ParseFile *aeroLiftParts;
#define  aeroLiftParts      aircraft_->aeroLiftParts
  ParseFile *aeroPitchParts;
#define  aeroPitchParts     aircraft_->aeroPitchParts
  ParseFile *aeroSideforceParts;
#define  aeroSideforceParts aircraft_->aeroSideforceParts
  ParseFile *aeroRollParts;
#define  aeroRollParts      aircraft_->aeroRollParts
  ParseFile *aeroYawParts;
#define  aeroYawParts       aircraft_->aeroYawParts
  ParseFile *engineParts;
#define  engineParts        aircraft_->engineParts
  ParseFile *gearParts;
#define  gearParts          aircraft_->gearParts
  ParseFile *recordParts;
#define  recordParts        aircraft_->recordParts
  
  /*= Keywords (token1) ===========================================*/
  std::map <string,int>      Keyword_map;
#define      Keyword_map         aircraft_->Keyword_map       

  double CD, CX, CL, CZ, Cm, CY, Cl, Cn;
#define CD  aircraft_->CD
#define CX  aircraft_->CX
#define CL  aircraft_->CL
#define CZ  aircraft_->CZ
#define Cm  aircraft_->Cm
#define CY  aircraft_->CY
#define Cl  aircraft_->Cl
#define Cn  aircraft_->Cn
  double CXclean_wing, CXclean_tail, CXiced_wing, CXiced_tail;
  double CLclean_wing, CLclean_tail, CLiced_wing, CLiced_tail;
  double CZclean_wing, CZclean_tail, CZiced_wing, CZiced_tail;
#define CXclean_wing  aircraft_->CXclean_wing
#define CXclean_tail  aircraft_->CXclean_tail
#define CXiced_wing   aircraft_->CXiced_wing
#define CXiced_tail   aircraft_->CXiced_tail
#define CLclean_wing  aircraft_->CLclean_wing
#define CLclean_tail  aircraft_->CLclean_tail
#define CLiced_wing   aircraft_->CLiced_wing
#define CLiced_tail   aircraft_->CLiced_tail
#define CZclean_wing  aircraft_->CZclean_wing
#define CZclean_tail  aircraft_->CZclean_tail
#define CZiced_wing   aircraft_->CZiced_wing
#define CZiced_tail   aircraft_->CZiced_tail

  /*========================================*/
  /* Variables (token2) - 17 groups (000329)*/
  /*========================================*/

  /* Variables (token2) ===========================================*/
  /* init ========== Initial values for equations of motion =======*/

  std::map <string,int> init_map;
#define      init_map          aircraft_->init_map          

  int recordRate;
#define recordRate             aircraft_->recordRate
  double recordStartTime;
#define recordStartTime        aircraft_->recordStartTime
  bool use_V_rel_wind_2U;
#define use_V_rel_wind_2U      aircraft_->use_V_rel_wind_2U
  bool nondim_rate_V_rel_wind;
#define nondim_rate_V_rel_wind aircraft_->nondim_rate_V_rel_wind
  bool use_abs_U_body_2U;
#define use_abs_U_body_2U      aircraft_->use_abs_U_body_2U
  double dyn_on_speed;
#define dyn_on_speed           aircraft_->dyn_on_speed
  double dyn_on_speed_zero;
#define dyn_on_speed_zero      aircraft_->dyn_on_speed_zero
  bool use_dyn_on_speed_curve1;
#define use_dyn_on_speed_curve1 aircraft_->use_dyn_on_speed_curve1

  bool use_Alpha_dot_on_speed;
#define use_Alpha_dot_on_speed  aircraft_->use_Alpha_dot_on_speed
  double Alpha_dot_on_speed;
#define Alpha_dot_on_speed      aircraft_->Alpha_dot_on_speed

  bool use_gamma_horiz_on_speed;
#define use_gamma_horiz_on_speed aircraft_->use_gamma_horiz_on_speed
  double gamma_horiz_on_speed;
#define gamma_horiz_on_speed     aircraft_->gamma_horiz_on_speed


  bool b_downwashMode;
#define b_downwashMode          aircraft_->b_downwashMode
  int downwashMode;
#define downwashMode            aircraft_->downwashMode
  double downwashCoef;
#define downwashCoef            aircraft_->downwashCoef
  double gammaWing;
#define gammaWing               aircraft_->gammaWing
  double downwash;
#define downwash                aircraft_->downwash
  double downwashAngle;
#define downwashAngle           aircraft_->downwashAngle
  double alphaTail;
#define alphaTail               aircraft_->alphaTail


  bool P_body_init_true;
  double P_body_init;
#define P_body_init_true       aircraft_->P_body_init_true
#define P_body_init            aircraft_->P_body_init
  bool Q_body_init_true;
  double Q_body_init;
#define Q_body_init_true       aircraft_->Q_body_init_true
#define Q_body_init            aircraft_->Q_body_init
  bool R_body_init_true;
  double R_body_init;
#define R_body_init_true       aircraft_->R_body_init_true
#define R_body_init            aircraft_->R_body_init
  bool Phi_init_true;
  double Phi_init;
#define Phi_init_true          aircraft_->Phi_init_true
#define Phi_init               aircraft_->Phi_init
  bool Theta_init_true;
  double Theta_init;
#define Theta_init_true        aircraft_->Theta_init_true
#define Theta_init             aircraft_->Theta_init
  bool Psi_init_true;
  double Psi_init;
#define Psi_init_true          aircraft_->Psi_init_true
#define Psi_init               aircraft_->Psi_init
  bool Alpha_init_true;
  double Alpha_init;
#define Alpha_init_true        aircraft_->Alpha_init_true
#define Alpha_init             aircraft_->Alpha_init
  bool Beta_init_true;
  double Beta_init;
#define Beta_init_true         aircraft_->Beta_init_true
#define Beta_init              aircraft_->Beta_init
  bool U_body_init_true;
  double U_body_init;
#define U_body_init_true       aircraft_->U_body_init_true
#define U_body_init            aircraft_->U_body_init
  bool V_body_init_true;
  double V_body_init;
#define V_body_init_true       aircraft_->V_body_init_true
#define V_body_init            aircraft_->V_body_init
  bool W_body_init_true;
  double W_body_init;
#define W_body_init_true       aircraft_->W_body_init_true
#define W_body_init            aircraft_->W_body_init
  bool trim_case_2;
#define trim_case_2            aircraft_->trim_case_2
  bool use_uiuc_network;
  string server_IP;
  int port_num;
#define use_uiuc_network       aircraft_->use_uiuc_network
#define server_IP              aircraft_->server_IP
#define port_num               aircraft_->port_num
  bool icing_demo;
#define icing_demo             aircraft_->icing_demo
  bool outside_control;
#define outside_control        aircraft_->outside_control

  /* Variables (token2) ===========================================*/
  /* geometry ====== Aircraft-specific geometric quantities =======*/
  
  std::map <string,int> geometry_map;
#define      geometry_map        aircraft_->geometry_map       
  
  double bw, cbar, Sw, ih, bh, chord_h, Sh;
#define bw   aircraft_->bw
#define cbar aircraft_->cbar
#define Sw   aircraft_->Sw       
#define ih   aircraft_->ih
#define bh   aircraft_->bh
#define chord_h   aircraft_->chord_h
#define Sh   aircraft_->Sh

  
  /* Variables (token2) ===========================================*/
  /* controlSurface  Control surface deflections and properties ===*/
  
  std::map <string,int> controlSurface_map;
#define      controlSurface_map  aircraft_->controlSurface_map
  
  double demax, demin;
  double damax, damin;
  double drmax, drmin;
#define demax             aircraft_->demax
#define demin             aircraft_->demin
#define damax             aircraft_->damax
#define damin             aircraft_->damin
#define drmax             aircraft_->drmax
#define drmin             aircraft_->drmin

  double aileron, elevator, rudder;
#define aileron           aircraft_->aileron
#define elevator          aircraft_->elevator
#define rudder            aircraft_->rudder
  //  double flap;
  //#define flap              aircraft_->flap

  bool set_Long_trim, zero_Long_trim;
  double Long_trim_constant;
#define set_Long_trim      aircraft_->set_Long_trim
#define Long_trim_constant aircraft_->Long_trim_constant
#define zero_Long_trim     aircraft_->zero_Long_trim

  bool elevator_step;
  double elevator_step_angle, elevator_step_startTime;
#define elevator_step              aircraft_->elevator_step
#define elevator_step_angle        aircraft_->elevator_step_angle
#define elevator_step_startTime    aircraft_->elevator_step_startTime

  bool elevator_singlet;
  double elevator_singlet_angle, elevator_singlet_startTime;
  double elevator_singlet_duration;
#define elevator_singlet           aircraft_->elevator_singlet
#define elevator_singlet_angle     aircraft_->elevator_singlet_angle
#define elevator_singlet_startTime aircraft_->elevator_singlet_startTime
#define elevator_singlet_duration  aircraft_->elevator_singlet_duration

  bool elevator_doublet;
  double elevator_doublet_angle, elevator_doublet_startTime;
  double elevator_doublet_duration;
#define elevator_doublet           aircraft_->elevator_doublet
#define elevator_doublet_angle     aircraft_->elevator_doublet_angle
#define elevator_doublet_startTime aircraft_->elevator_doublet_startTime
#define elevator_doublet_duration  aircraft_->elevator_doublet_duration

  bool elevator_input;
  string elevator_input_file;
  double elevator_input_timeArray[7500];
  double elevator_input_deArray[7500];
  int elevator_input_ntime;
  double elevator_input_startTime;
#define elevator_input             aircraft_->elevator_input
#define elevator_input_file        aircraft_->elevator_input_file
#define elevator_input_timeArray   aircraft_->elevator_input_timeArray
#define elevator_input_deArray     aircraft_->elevator_input_deArray
#define elevator_input_ntime       aircraft_->elevator_input_ntime
#define elevator_input_startTime   aircraft_->elevator_input_startTime

  bool aileron_input;
  string aileron_input_file;
  double aileron_input_timeArray[1500];
  double aileron_input_daArray[1500];
  int aileron_input_ntime;
  double aileron_input_startTime;
#define aileron_input             aircraft_->aileron_input
#define aileron_input_file        aircraft_->aileron_input_file
#define aileron_input_timeArray   aircraft_->aileron_input_timeArray
#define aileron_input_daArray     aircraft_->aileron_input_daArray
#define aileron_input_ntime       aircraft_->aileron_input_ntime
#define aileron_input_startTime   aircraft_->aileron_input_startTime

  bool rudder_input;
  string rudder_input_file;
  double rudder_input_timeArray[500];
  double rudder_input_drArray[500];
  int rudder_input_ntime;
  double rudder_input_startTime;
#define rudder_input             aircraft_->rudder_input
#define rudder_input_file        aircraft_->rudder_input_file
#define rudder_input_timeArray   aircraft_->rudder_input_timeArray
#define rudder_input_drArray     aircraft_->rudder_input_drArray
#define rudder_input_ntime       aircraft_->rudder_input_ntime
#define rudder_input_startTime   aircraft_->rudder_input_startTime

  bool pilot_elev_no;
#define pilot_elev_no            aircraft_->pilot_elev_no
  bool pilot_elev_no_check;
#define pilot_elev_no_check      aircraft_->pilot_elev_no_check

  bool pilot_ail_no;
#define pilot_ail_no             aircraft_->pilot_ail_no
  bool pilot_ail_no_check;
#define pilot_ail_no_check       aircraft_->pilot_ail_no_check

  bool pilot_rud_no;
#define pilot_rud_no             aircraft_->pilot_rud_no
  bool pilot_rud_no_check;
#define pilot_rud_no_check       aircraft_->pilot_rud_no_check

  double flap_max, flap_rate;
#define flap_max                 aircraft_->flap_max
#define flap_rate                aircraft_->flap_rate
  bool use_flaps;
#define use_flaps                aircraft_->use_flaps

  double spoiler_max, spoiler_rate;
#define spoiler_max                 aircraft_->spoiler_max
#define spoiler_rate                aircraft_->spoiler_rate
  bool use_spoilers;
#define use_spoilers                aircraft_->use_spoilers


  bool flap_pos_input;
  string flap_pos_input_file;
  double flap_pos_input_timeArray[500];
  double flap_pos_input_dfArray[500];
  int flap_pos_input_ntime;
  double flap_pos_input_startTime;
#define flap_pos_input             aircraft_->flap_pos_input
#define flap_pos_input_file        aircraft_->flap_pos_input_file
#define flap_pos_input_timeArray   aircraft_->flap_pos_input_timeArray
#define flap_pos_input_dfArray     aircraft_->flap_pos_input_dfArray
#define flap_pos_input_ntime       aircraft_->flap_pos_input_ntime
#define flap_pos_input_startTime   aircraft_->flap_pos_input_startTime


  // SAS system: pitch, roll and yaw damping  MSS
  double elevator_sas_KQ;
  double elevator_sas_max;
  double elevator_sas_min;
  double elevator_stick_gain;
  double aileron_sas_KP;
  double aileron_sas_max;
  double aileron_stick_gain;
  double rudder_sas_KR;
  double rudder_sas_max;
  double rudder_stick_gain;



#define elevator_sas_KQ             aircraft_->elevator_sas_KQ
#define elevator_sas_max            aircraft_->elevator_sas_max
#define elevator_sas_min            aircraft_->elevator_sas_min
#define elevator_stick_gain         aircraft_->elevator_stick_gain
#define aileron_sas_KP              aircraft_->aileron_sas_KP
#define aileron_sas_max             aircraft_->aileron_sas_max
#define aileron_stick_gain          aircraft_->aileron_stick_gain
#define rudder_sas_KR               aircraft_->rudder_sas_KR
#define rudder_sas_max              aircraft_->rudder_sas_max
#define rudder_stick_gain           aircraft_->rudder_stick_gain

  double elevator_sas;
#define elevator_sas                aircraft_->elevator_sas
  double aileron_sas;
#define aileron_sas                 aircraft_->aileron_sas
  double rudder_sas;
#define rudder_sas                  aircraft_->rudder_sas

  bool use_elevator_sas_type1;
#define use_elevator_sas_type1      aircraft_->use_elevator_sas_type1
  bool use_elevator_sas_max;
#define use_elevator_sas_max        aircraft_->use_elevator_sas_max
  bool use_elevator_sas_min;
#define use_elevator_sas_min        aircraft_->use_elevator_sas_min
  bool use_elevator_stick_gain;
#define use_elevator_stick_gain     aircraft_->use_elevator_stick_gain
  bool use_aileron_sas_type1;
#define use_aileron_sas_type1       aircraft_->use_aileron_sas_type1
  bool use_aileron_sas_max;
#define use_aileron_sas_max         aircraft_->use_aileron_sas_max
  bool use_aileron_stick_gain;
#define use_aileron_stick_gain      aircraft_->use_aileron_stick_gain
  bool use_rudder_sas_type1;
#define use_rudder_sas_type1        aircraft_->use_rudder_sas_type1
  bool use_rudder_sas_max;
#define use_rudder_sas_max          aircraft_->use_rudder_sas_max
  bool use_rudder_stick_gain;
#define use_rudder_stick_gain       aircraft_->use_rudder_stick_gain



  /* Variables (token2) ===========================================*/
  /* controlsMixer = Control mixer ================================*/
  
  std::map <string,int> controlsMixer_map;
#define      controlsMixer_map  aircraft_->controlsMixer_map
  
  double nomix;
#define nomix  aircraft_->nomix

  
  /* Variables (token2) ===========================================*/
  /* mass =========== Aircraft-specific mass properties ===========*/
  
  std::map <string,int> mass_map;
#define      mass_map            aircraft_->mass_map

  double Weight;
#define Weight  aircraft_->Weight

  double Mass_appMass_ratio;
#define Mass_appMass_ratio aircraft_->Mass_appMass_ratio
  double I_xx_appMass_ratio;
#define I_xx_appMass_ratio aircraft_->I_xx_appMass_ratio
  double I_yy_appMass_ratio;
#define I_yy_appMass_ratio aircraft_->I_yy_appMass_ratio
  double I_zz_appMass_ratio;
#define I_zz_appMass_ratio aircraft_->I_zz_appMass_ratio
  double Mass_appMass;
#define Mass_appMass aircraft_->Mass_appMass
  double I_xx_appMass;
#define I_xx_appMass aircraft_->I_xx_appMass
  double I_yy_appMass;
#define I_yy_appMass aircraft_->I_yy_appMass
  double I_zz_appMass;
#define I_zz_appMass aircraft_->I_zz_appMass

  /* Variables (token2) ===========================================*/
  /* engine ======== Propulsion data ==============================*/
  
  std::map <string,int> engine_map;
#define      engine_map            aircraft_->engine_map          
  
  double simpleSingleMaxThrust;
#define simpleSingleMaxThrust  aircraft_->simpleSingleMaxThrust

  bool simpleSingleModel;
#define simpleSingleModel  aircraft_->simpleSingleModel
  double t_v0;
#define t_v0  aircraft_->t_v0
  double dtdv_t0;
#define dtdv_t0  aircraft_->dtdv_t0
  double v_t0;
#define v_t0  aircraft_->v_t0
  double dtdvvt;
#define dtdvvt  aircraft_->dtdvvt

  double tc, induced, eta_q;
#define tc      aircraft_->tc
#define induced aircraft_->induced
#define eta_q   aircraft_->eta_q
  
  bool Throttle_pct_input;
  string Throttle_pct_input_file;
  double Throttle_pct_input_timeArray[1500];
  double Throttle_pct_input_dTArray[1500];
  int Throttle_pct_input_ntime;
  double Throttle_pct_input_startTime;
#define Throttle_pct_input            aircraft_->Throttle_pct_input
#define Throttle_pct_input_file       aircraft_->Throttle_pct_input_file
#define Throttle_pct_input_timeArray  aircraft_->Throttle_pct_input_timeArray
#define Throttle_pct_input_dTArray    aircraft_->Throttle_pct_input_dTArray
#define Throttle_pct_input_ntime      aircraft_->Throttle_pct_input_ntime
#define Throttle_pct_input_startTime  aircraft_->Throttle_pct_input_startTime
  bool gyroForce_Q_body, gyroForce_R_body;
  double minOmega, maxOmega, minOmegaRPM, maxOmegaRPM, engineOmega, polarInertia;
#define gyroForce_Q_body              aircraft_->gyroForce_Q_body
#define gyroForce_R_body              aircraft_->gyroForce_R_body
#define minOmega                      aircraft_->minOmega
#define maxOmega                      aircraft_->maxOmega
#define minOmegaRPM                   aircraft_->minOmegaRPM
#define maxOmegaRPM                   aircraft_->maxOmegaRPM
#define engineOmega                   aircraft_->engineOmega
#define polarInertia                  aircraft_->polarInertia

  // propeller slipstream effects
  bool b_slipstreamEffects;
  double propDia;
  double eta_q_Cm_q, eta_q_Cm_q_fac;
  double eta_q_Cm_adot, eta_q_Cm_adot_fac;
  double eta_q_Cmfade, eta_q_Cmfade_fac;
  double eta_q_Cm_de, eta_q_Cm_de_fac;
  double eta_q_Cl_beta, eta_q_Cl_beta_fac;
  double eta_q_Cl_p, eta_q_Cl_p_fac;
  double eta_q_Cl_r, eta_q_Cl_r_fac;
  double eta_q_Cl_dr, eta_q_Cl_dr_fac;
  double eta_q_CY_beta, eta_q_CY_beta_fac;
  double eta_q_CY_p, eta_q_CY_p_fac;
  double eta_q_CY_r, eta_q_CY_r_fac;
  double eta_q_CY_dr, eta_q_CY_dr_fac;
  double eta_q_Cn_beta, eta_q_Cn_beta_fac;
  double eta_q_Cn_p, eta_q_Cn_p_fac;
  double eta_q_Cn_r, eta_q_Cn_r_fac;
  double eta_q_Cn_dr, eta_q_Cn_dr_fac;

#define b_slipstreamEffects  aircraft_->b_slipstreamEffects
#define propDia              aircraft_->propDia
#define eta_q_Cm_q           aircraft_->eta_q_Cm_q
#define eta_q_Cm_q_fac       aircraft_->eta_q_Cm_q_fac
#define eta_q_Cm_adot        aircraft_->eta_q_Cm_adot
#define eta_q_Cm_adot_fac    aircraft_->eta_q_Cm_adot_fac
#define eta_q_Cmfade         aircraft_->eta_q_Cmfade
#define eta_q_Cmfade_fac     aircraft_->eta_q_Cmfade_fac
#define eta_q_Cm_de          aircraft_->eta_q_Cm_de
#define eta_q_Cm_de_fac      aircraft_->eta_q_Cm_de_fac
#define eta_q_Cl_beta        aircraft_->eta_q_Cl_beta
#define eta_q_Cl_beta_fac    aircraft_->eta_q_Cl_beta_fac
#define eta_q_Cl_p           aircraft_->eta_q_Cl_p
#define eta_q_Cl_p_fac       aircraft_->eta_q_Cl_p_fac
#define eta_q_Cl_r           aircraft_->eta_q_Cl_r
#define eta_q_Cl_r_fac       aircraft_->eta_q_Cl_r_fac
#define eta_q_Cl_dr          aircraft_->eta_q_Cl_dr
#define eta_q_Cl_dr_fac      aircraft_->eta_q_Cl_dr_fac
#define eta_q_CY_beta        aircraft_->eta_q_CY_beta
#define eta_q_CY_beta_fac    aircraft_->eta_q_CY_beta_fac
#define eta_q_CY_p           aircraft_->eta_q_CY_p
#define eta_q_CY_p_fac       aircraft_->eta_q_CY_p_fac
#define eta_q_CY_r           aircraft_->eta_q_CY_r
#define eta_q_CY_r_fac       aircraft_->eta_q_CY_r_fac
#define eta_q_CY_dr          aircraft_->eta_q_CY_dr
#define eta_q_CY_dr_fac      aircraft_->eta_q_CY_dr_fac
#define eta_q_Cn_beta        aircraft_->eta_q_Cn_beta
#define eta_q_Cn_beta_fac    aircraft_->eta_q_Cn_beta_fac
#define eta_q_Cn_p           aircraft_->eta_q_Cn_p
#define eta_q_Cn_p_fac       aircraft_->eta_q_Cn_p_fac
#define eta_q_Cn_r           aircraft_->eta_q_Cn_r
#define eta_q_Cn_r_fac       aircraft_->eta_q_Cn_r_fac
#define eta_q_Cn_dr          aircraft_->eta_q_Cn_dr
#define eta_q_Cn_dr_fac      aircraft_->eta_q_Cn_dr_fac


  bool Xp_input;
  string Xp_input_file;
  double Xp_input_timeArray[5400];
  double Xp_input_XpArray[5400];
  int Xp_input_ntime;
  double Xp_input_startTime;
#define Xp_input            aircraft_->Xp_input
#define Xp_input_file       aircraft_->Xp_input_file
#define Xp_input_timeArray  aircraft_->Xp_input_timeArray
#define Xp_input_XpArray    aircraft_->Xp_input_XpArray
#define Xp_input_ntime      aircraft_->Xp_input_ntime
#define Xp_input_startTime  aircraft_->Xp_input_startTime
  bool Zp_input;
  string Zp_input_file;
  double Zp_input_timeArray[5400];
  double Zp_input_ZpArray[5400];
  int Zp_input_ntime;
  double Zp_input_startTime;
#define Zp_input            aircraft_->Zp_input
#define Zp_input_file       aircraft_->Zp_input_file
#define Zp_input_timeArray  aircraft_->Zp_input_timeArray
#define Zp_input_ZpArray    aircraft_->Zp_input_ZpArray
#define Zp_input_ntime      aircraft_->Zp_input_ntime
#define Zp_input_startTime  aircraft_->Zp_input_startTime
  bool Mp_input;
  string Mp_input_file;
  double Mp_input_timeArray[5400];
  double Mp_input_MpArray[5400];
  int Mp_input_ntime;
  double Mp_input_startTime;
#define Mp_input            aircraft_->Mp_input
#define Mp_input_file       aircraft_->Mp_input_file
#define Mp_input_timeArray  aircraft_->Mp_input_timeArray
#define Mp_input_MpArray    aircraft_->Mp_input_MpArray
#define Mp_input_ntime      aircraft_->Mp_input_ntime
#define Mp_input_startTime  aircraft_->Mp_input_startTime


  /* Variables (token2) ===========================================*/
  /* CD ============ Aerodynamic x-force quantities (longitudinal) */
  
  std::map <string,int> CD_map;
#define      CD_map              aircraft_->CD_map            
  
  double CDo, CDK, CLK, CD_a, CD_adot, CD_q, CD_ih, CD_de, CD_dr, CD_da, CD_beta;
  double CD_df, CD_ds, CD_dg;
#define CDo      aircraft_->CDo
#define CDK      aircraft_->CDK
#define CLK      aircraft_->CLK
#define CD_a     aircraft_->CD_a
#define CD_adot  aircraft_->CD_adot
#define CD_q     aircraft_->CD_q
#define CD_ih    aircraft_->CD_ih
#define CD_de    aircraft_->CD_de
#define CD_dr    aircraft_->CD_dr
#define CD_da    aircraft_->CD_da
#define CD_beta  aircraft_->CD_beta
#define CD_df    aircraft_->CD_df
#define CD_ds    aircraft_->CD_ds
#define CD_dg    aircraft_->CD_dg
  bool b_CLK;
#define b_CLK      aircraft_->b_CLK
  string CDfa;
  double CDfa_aArray[100];
  double CDfa_CDArray[100];
  int CDfa_nAlpha;
  double CDfaI;
#define CDfa               aircraft_->CDfa
#define CDfa_aArray        aircraft_->CDfa_aArray
#define CDfa_CDArray       aircraft_->CDfa_CDArray
#define CDfa_nAlpha        aircraft_->CDfa_nAlpha
#define CDfaI              aircraft_->CDfaI
  string CDfCL;
  double CDfCL_CLArray[100];
  double CDfCL_CDArray[100];
  int CDfCL_nCL;
  double CDfCLI;
#define CDfCL              aircraft_->CDfCL
#define CDfCL_CLArray      aircraft_->CDfCL_CLArray
#define CDfCL_CDArray      aircraft_->CDfCL_CDArray
#define CDfCL_nCL          aircraft_->CDfCL_nCL
#define CDfCLI             aircraft_->CDfCLI
  string CDfade;
  double CDfade_aArray[100][100];
  double CDfade_deArray[100];
  double CDfade_CDArray[100][100];
  int CDfade_nAlphaArray[100];
  int CDfade_nde;
  double CDfadeI;
#define CDfade             aircraft_->CDfade
#define CDfade_aArray      aircraft_->CDfade_aArray
#define CDfade_deArray     aircraft_->CDfade_deArray
#define CDfade_CDArray     aircraft_->CDfade_CDArray
#define CDfade_nAlphaArray aircraft_->CDfade_nAlphaArray
#define CDfade_nde         aircraft_->CDfade_nde
#define CDfadeI            aircraft_->CDfadeI
  string CDfdf;
  double CDfdf_dfArray[100];
  double CDfdf_CDArray[100];
  int CDfdf_ndf;
  double CDfdfI;
#define CDfdf              aircraft_->CDfdf
#define CDfdf_dfArray      aircraft_->CDfdf_dfArray
#define CDfdf_CDArray      aircraft_->CDfdf_CDArray
#define CDfdf_ndf          aircraft_->CDfdf_ndf
#define CDfdfI             aircraft_->CDfdfI
  string CDfadf;
  double CDfadf_aArray[100][100];
  double CDfadf_dfArray[100];
  double CDfadf_CDArray[100][100];
  int CDfadf_nAlphaArray[100];
  int CDfadf_ndf;
  double CDfadfI;
#define CDfadf             aircraft_->CDfadf
#define CDfadf_aArray      aircraft_->CDfadf_aArray
#define CDfadf_dfArray     aircraft_->CDfadf_dfArray
#define CDfadf_CDArray     aircraft_->CDfadf_CDArray
#define CDfadf_nAlphaArray aircraft_->CDfadf_nAlphaArray
#define CDfadf_ndf         aircraft_->CDfadf_ndf
#define CDfadfI            aircraft_->CDfadfI
  double CXo, CXK, CX_a, CX_a2, CX_a3, CX_adot;
  double CX_q, CX_de, CX_dr, CX_df, CX_adf;
#define CXo      aircraft_->CXo
#define CXK      aircraft_->CXK
#define CX_a     aircraft_->CX_a
#define CX_a2    aircraft_->CX_a2
#define CX_a3    aircraft_->CX_a3
#define CX_adot  aircraft_->CX_adot
#define CX_q     aircraft_->CX_q
#define CX_de    aircraft_->CX_de
#define CX_dr    aircraft_->CX_dr
#define CX_df    aircraft_->CX_df
#define CX_adf   aircraft_->CX_adf
  double CXfabetaf_aArray[30][100][100];
  double CXfabetaf_betaArray[30][100];
  double CXfabetaf_CXArray[30][100][100];
  int CXfabetaf_nAlphaArray[30][100];
  int CXfabetaf_nbeta[30];
  double CXfabetaf_fArray[30];
  int CXfabetaf_nf;
  double CXfabetafI;
  int CXfabetaf_nice, CXfabetaf_na_nice, CXfabetaf_nb_nice;
  double CXfabetaf_bArray_nice[100];
  double CXfabetaf_aArray_nice[100];
#define CXfabetaf_aArray        aircraft_->CXfabetaf_aArray
#define CXfabetaf_betaArray     aircraft_->CXfabetaf_betaArray
#define CXfabetaf_CXArray       aircraft_->CXfabetaf_CXArray
#define CXfabetaf_nAlphaArray   aircraft_->CXfabetaf_nAlphaArray
#define CXfabetaf_nbeta         aircraft_->CXfabetaf_nbeta
#define CXfabetaf_fArray        aircraft_->CXfabetaf_fArray
#define CXfabetaf_nf            aircraft_->CXfabetaf_nf
#define CXfabetafI              aircraft_->CXfabetafI
#define CXfabetaf_nice          aircraft_->CXfabetaf_nice
#define CXfabetaf_na_nice       aircraft_->CXfabetaf_na_nice
#define CXfabetaf_nb_nice       aircraft_->CXfabetaf_nb_nice
#define CXfabetaf_bArray_nice   aircraft_->CXfabetaf_bArray_nice
#define CXfabetaf_aArray_nice   aircraft_->CXfabetaf_aArray_nice
  double CXfadef_aArray[30][100][100];
  double CXfadef_deArray[30][100];
  double CXfadef_CXArray[30][100][100];
  int CXfadef_nAlphaArray[30][100];
  int CXfadef_nde[30];
  double CXfadef_fArray[30];
  int CXfadef_nf;
  double CXfadefI;
  int CXfadef_nice, CXfadef_na_nice, CXfadef_nde_nice;
  double CXfadef_deArray_nice[100];
  double CXfadef_aArray_nice[100];
#define CXfadef_aArray        aircraft_->CXfadef_aArray
#define CXfadef_deArray       aircraft_->CXfadef_deArray
#define CXfadef_CXArray       aircraft_->CXfadef_CXArray
#define CXfadef_nAlphaArray   aircraft_->CXfadef_nAlphaArray
#define CXfadef_nde           aircraft_->CXfadef_nde
#define CXfadef_fArray        aircraft_->CXfadef_fArray
#define CXfadef_nf            aircraft_->CXfadef_nf
#define CXfadefI              aircraft_->CXfadefI
#define CXfadef_nice          aircraft_->CXfadef_nice
#define CXfadef_na_nice       aircraft_->CXfadef_na_nice
#define CXfadef_nde_nice      aircraft_->CXfadef_nde_nice
#define CXfadef_deArray_nice  aircraft_->CXfadef_deArray_nice
#define CXfadef_aArray_nice   aircraft_->CXfadef_aArray_nice
  double CXfaqf_aArray[30][100][100];
  double CXfaqf_qArray[30][100];
  double CXfaqf_CXArray[30][100][100];
  int CXfaqf_nAlphaArray[30][100];
  int CXfaqf_nq[30];
  double CXfaqf_fArray[30];
  int CXfaqf_nf;
  double CXfaqfI;
  int CXfaqf_nice, CXfaqf_na_nice, CXfaqf_nq_nice;
  double CXfaqf_qArray_nice[100];
  double CXfaqf_aArray_nice[100];
#define CXfaqf_aArray        aircraft_->CXfaqf_aArray
#define CXfaqf_qArray        aircraft_->CXfaqf_qArray
#define CXfaqf_CXArray       aircraft_->CXfaqf_CXArray
#define CXfaqf_nAlphaArray   aircraft_->CXfaqf_nAlphaArray
#define CXfaqf_nq            aircraft_->CXfaqf_nq
#define CXfaqf_fArray        aircraft_->CXfaqf_fArray
#define CXfaqf_nf            aircraft_->CXfaqf_nf
#define CXfaqfI              aircraft_->CXfaqfI
#define CXfaqf_nice          aircraft_->CXfaqf_nice
#define CXfaqf_na_nice       aircraft_->CXfaqf_na_nice
#define CXfaqf_nq_nice       aircraft_->CXfaqf_nq_nice
#define CXfaqf_qArray_nice   aircraft_->CXfaqf_qArray_nice
#define CXfaqf_aArray_nice   aircraft_->CXfaqf_aArray_nice
  double CDo_save, CDK_save, CLK_save, CD_a_save, CD_adot_save, CD_q_save, CD_ih_save;
  double CD_de_save, CD_dr_save, CD_da_save, CD_beta_save;
  double CD_df_save, CD_ds_save, CD_dg_save;
  double CXo_save, CXK_save, CX_a_save, CX_a2_save, CX_a3_save;
  double CX_adot_save, CX_q_save, CX_de_save;
  double CX_dr_save, CX_df_save, CX_adf_save;
#define CDo_save             aircraft_->CDo_save  
#define CDK_save             aircraft_->CDK_save  
#define CLK_save             aircraft_->CLK_save  
#define CD_a_save            aircraft_->CD_a_save  
#define CD_adot_save         aircraft_->CD_adot_save  
#define CD_q_save            aircraft_->CD_q_save  
#define CD_ih_save           aircraft_->CD_ih_save  
#define CD_de_save           aircraft_->CD_de_save  
#define CD_dr_save           aircraft_->CD_dr_save  
#define CD_da_save           aircraft_->CD_da_save  
#define CD_beta_save         aircraft_->CD_beta_save  
#define CD_df_save           aircraft_->CD_df_save  
#define CD_ds_save           aircraft_->CD_ds_save  
#define CD_dg_save           aircraft_->CD_dg_save  
#define CXo_save             aircraft_->CXo_save  
#define CXK_save             aircraft_->CXK_save  
#define CX_a_save            aircraft_->CX_a_save  
#define CX_a2_save           aircraft_->CX_a2_save  
#define CX_a3_save           aircraft_->CX_a3_save  
#define CX_adot_save         aircraft_->CX_adot_save  
#define CX_q_save            aircraft_->CX_q_save  
#define CX_de_save           aircraft_->CX_de_save
#define CX_dr_save           aircraft_->CX_dr_save  
#define CX_df_save           aircraft_->CX_df_save  
#define CX_adf_save          aircraft_->CX_adf_save  


  /* Variables (token2) ===========================================*/
  /* CL ============ Aerodynamic z-force quantities (longitudinal) */
  
  std::map <string,int> CL_map;
#define      CL_map              aircraft_->CL_map            
  
  double CLo, CL_a, CL_adot, CL_q, CL_ih, CL_de;
  double CL_df, CL_ds, CL_dg;
#define CLo      aircraft_->CLo
#define CL_a     aircraft_->CL_a
#define CL_adot  aircraft_->CL_adot
#define CL_q     aircraft_->CL_q
#define CL_ih    aircraft_->CL_ih
#define CL_de    aircraft_->CL_de
#define CL_df    aircraft_->CL_df
#define CL_ds    aircraft_->CL_ds
#define CL_dg    aircraft_->CL_dg
  string CLfa;
  double CLfa_aArray[100];
  double CLfa_CLArray[100];
  int CLfa_nAlpha;
  double CLfaI;
#define CLfa               aircraft_->CLfa
#define CLfa_aArray        aircraft_->CLfa_aArray
#define CLfa_CLArray       aircraft_->CLfa_CLArray
#define CLfa_nAlpha        aircraft_->CLfa_nAlpha
#define CLfaI              aircraft_->CLfaI
  string CLfade;
  double CLfade_aArray[100][100];
  double CLfade_deArray[100];
  double CLfade_CLArray[100][100];
  int CLfade_nAlphaArray[100];
  int CLfade_nde;
  double CLfadeI;
#define CLfade             aircraft_->CLfade
#define CLfade_aArray      aircraft_->CLfade_aArray
#define CLfade_deArray     aircraft_->CLfade_deArray
#define CLfade_CLArray     aircraft_->CLfade_CLArray
#define CLfade_nAlphaArray aircraft_->CLfade_nAlphaArray
#define CLfade_nde         aircraft_->CLfade_nde
#define CLfadeI            aircraft_->CLfadeI
  string CLfdf;
  double CLfdf_dfArray[100];
  double CLfdf_CLArray[100];
  int CLfdf_ndf;
  double CLfdfI;
#define CLfdf              aircraft_->CLfdf
#define CLfdf_dfArray      aircraft_->CLfdf_dfArray
#define CLfdf_CLArray      aircraft_->CLfdf_CLArray
#define CLfdf_ndf          aircraft_->CLfdf_ndf
#define CLfdfI             aircraft_->CLfdfI
  string CLfadf;
  double CLfadf_aArray[100][100];
  double CLfadf_dfArray[100];
  double CLfadf_CLArray[100][100];
  int CLfadf_nAlphaArray[100];
  int CLfadf_ndf;
  double CLfadfI;
#define CLfadf             aircraft_->CLfadf
#define CLfadf_aArray      aircraft_->CLfadf_aArray
#define CLfadf_dfArray     aircraft_->CLfadf_dfArray
#define CLfadf_CLArray     aircraft_->CLfadf_CLArray
#define CLfadf_nAlphaArray aircraft_->CLfadf_nAlphaArray
#define CLfadf_ndf         aircraft_->CLfadf_ndf
#define CLfadfI            aircraft_->CLfadfI
  double CZo, CZ_a, CZ_a2, CZ_a3, CZ_adot;
  double CZ_q, CZ_de, CZ_deb2, CZ_df, CZ_adf;
#define CZo      aircraft_->CZo
#define CZ_a     aircraft_->CZ_a
#define CZ_a2    aircraft_->CZ_a2
#define CZ_a3    aircraft_->CZ_a3
#define CZ_adot  aircraft_->CZ_adot
#define CZ_q     aircraft_->CZ_q
#define CZ_de    aircraft_->CZ_de
#define CZ_deb2  aircraft_->CZ_deb2
#define CZ_df    aircraft_->CZ_df
#define CZ_adf   aircraft_->CZ_adf
  string CZfa;
  double CZfa_aArray[100];
  double CZfa_CZArray[100];
  int CZfa_nAlpha;
  double CZfaI;
#define CZfa               aircraft_->CZfa
#define CZfa_aArray        aircraft_->CZfa_aArray
#define CZfa_CZArray       aircraft_->CZfa_CZArray
#define CZfa_nAlpha        aircraft_->CZfa_nAlpha
#define CZfaI              aircraft_->CZfaI
  double CZfabetaf_aArray[30][100][100];
  double CZfabetaf_betaArray[30][100];
  double CZfabetaf_CZArray[30][100][100];
  int CZfabetaf_nAlphaArray[30][100];
  int CZfabetaf_nbeta[30];
  double CZfabetaf_fArray[30];
  int CZfabetaf_nf;
  double CZfabetafI;
  int CZfabetaf_nice, CZfabetaf_na_nice, CZfabetaf_nb_nice;
  double CZfabetaf_bArray_nice[100];
  double CZfabetaf_aArray_nice[100];
#define CZfabetaf_aArray        aircraft_->CZfabetaf_aArray
#define CZfabetaf_betaArray     aircraft_->CZfabetaf_betaArray
#define CZfabetaf_CZArray       aircraft_->CZfabetaf_CZArray
#define CZfabetaf_nAlphaArray   aircraft_->CZfabetaf_nAlphaArray
#define CZfabetaf_nbeta         aircraft_->CZfabetaf_nbeta
#define CZfabetaf_fArray        aircraft_->CZfabetaf_fArray
#define CZfabetaf_nf            aircraft_->CZfabetaf_nf
#define CZfabetafI              aircraft_->CZfabetafI
#define CZfabetaf_nice          aircraft_->CZfabetaf_nice
#define CZfabetaf_na_nice       aircraft_->CZfabetaf_na_nice
#define CZfabetaf_nb_nice       aircraft_->CZfabetaf_nb_nice
#define CZfabetaf_bArray_nice   aircraft_->CZfabetaf_bArray_nice
#define CZfabetaf_aArray_nice   aircraft_->CZfabetaf_aArray_nice
  double CZfadef_aArray[30][100][100];
  double CZfadef_deArray[30][100];
  double CZfadef_CZArray[30][100][100];
  int CZfadef_nAlphaArray[30][100];
  int CZfadef_nde[30];
  double CZfadef_fArray[30];
  int CZfadef_nf;
  double CZfadefI;
  int CZfadef_nice, CZfadef_na_nice, CZfadef_nde_nice;
  double CZfadef_deArray_nice[100];
  double CZfadef_aArray_nice[100];
#define CZfadef_aArray         aircraft_->CZfadef_aArray
#define CZfadef_deArray        aircraft_->CZfadef_deArray
#define CZfadef_CZArray        aircraft_->CZfadef_CZArray
#define CZfadef_nAlphaArray    aircraft_->CZfadef_nAlphaArray
#define CZfadef_nde            aircraft_->CZfadef_nde
#define CZfadef_fArray         aircraft_->CZfadef_fArray
#define CZfadef_nf             aircraft_->CZfadef_nf
#define CZfadefI               aircraft_->CZfadefI
#define CZfadef_nice           aircraft_->CZfadef_nice
#define CZfadef_na_nice        aircraft_->CZfadef_na_nice
#define CZfadef_nde_nice       aircraft_->CZfadef_nde_nice
#define CZfadef_deArray_nice   aircraft_->CZfadef_deArray_nice
#define CZfadef_aArray_nice    aircraft_->CZfadef_aArray_nice
  double CZfaqf_aArray[30][100][100];
  double CZfaqf_qArray[30][100];
  double CZfaqf_CZArray[30][100][100];
  int CZfaqf_nAlphaArray[30][100];
  int CZfaqf_nq[30];
  double CZfaqf_fArray[30];
  int CZfaqf_nf;
  double CZfaqfI;
  int CZfaqf_nice, CZfaqf_na_nice, CZfaqf_nq_nice;
  double CZfaqf_qArray_nice[100];
  double CZfaqf_aArray_nice[100];
#define CZfaqf_aArray         aircraft_->CZfaqf_aArray
#define CZfaqf_qArray         aircraft_->CZfaqf_qArray
#define CZfaqf_CZArray        aircraft_->CZfaqf_CZArray
#define CZfaqf_nAlphaArray    aircraft_->CZfaqf_nAlphaArray
#define CZfaqf_nq             aircraft_->CZfaqf_nq
#define CZfaqf_fArray         aircraft_->CZfaqf_fArray
#define CZfaqf_nf             aircraft_->CZfaqf_nf
#define CZfaqfI               aircraft_->CZfaqfI
#define CZfaqf_nice           aircraft_->CZfaqf_nice
#define CZfaqf_na_nice        aircraft_->CZfaqf_na_nice
#define CZfaqf_nq_nice        aircraft_->CZfaqf_nq_nice
#define CZfaqf_qArray_nice    aircraft_->CZfaqf_qArray_nice
#define CZfaqf_aArray_nice    aircraft_->CZfaqf_aArray_nice
  double CLo_save, CL_a_save, CL_adot_save; 
  double CL_q_save, CL_ih_save, CL_de_save;
  double CL_df_save, CL_ds_save, CL_dg_save;
  double CZo_save, CZ_a_save, CZ_a2_save;
  double CZ_a3_save, CZ_adot_save, CZ_q_save;
  double CZ_de_save, CZ_deb2_save, CZ_df_save;
  double CZ_adf_save;
#define CLo_save              aircraft_->CLo_save
#define CL_a_save             aircraft_->CL_a_save
#define CL_adot_save          aircraft_->CL_adot_save
#define CL_q_save             aircraft_->CL_q_save
#define CL_ih_save            aircraft_->CL_ih_save
#define CL_de_save            aircraft_->CL_de_save
#define CL_df_save            aircraft_->CL_df_save
#define CL_ds_save            aircraft_->CL_ds_save
#define CL_dg_save            aircraft_->CL_dg_save
#define CZo_save              aircraft_->CZo_save
#define CZ_a_save             aircraft_->CZ_a_save
#define CZ_a2_save            aircraft_->CZ_a2_save
#define CZ_a3_save            aircraft_->CZ_a3_save
#define CZ_adot_save          aircraft_->CZ_adot_save
#define CZ_q_save             aircraft_->CZ_q_save
#define CZ_de_save            aircraft_->CZ_de_save
#define CZ_deb2_save          aircraft_->CZ_deb2_save
#define CZ_df_save            aircraft_->CZ_df_save
#define CZ_adf_save           aircraft_->CZ_adf_save


  /* Variables (token2) ===========================================*/
  /* Cm ============ Aerodynamic m-moment quantities (longitudinal) */
  
  std::map <string,int> Cm_map;
#define      Cm_map              aircraft_->Cm_map            
  
  double Cmo, Cm_a, Cm_a2, Cm_adot, Cm_q;
  double Cm_ih, Cm_de, Cm_b2, Cm_r;
  double Cm_df, Cm_ds, Cm_dg;
#define Cmo      aircraft_->Cmo
#define Cm_a     aircraft_->Cm_a
#define Cm_a2    aircraft_->Cm_a2
#define Cm_adot  aircraft_->Cm_adot
#define Cm_q     aircraft_->Cm_q
#define Cm_ih    aircraft_->Cm_ih
#define Cm_de    aircraft_->Cm_de
#define Cm_b2    aircraft_->Cm_b2
#define Cm_r     aircraft_->Cm_r
#define Cm_df    aircraft_->Cm_df
#define Cm_ds    aircraft_->Cm_ds
#define Cm_dg    aircraft_->Cm_dg
  string Cmfa;
  double Cmfa_aArray[100];
  double Cmfa_CmArray[100];
  int Cmfa_nAlpha;
  double CmfaI;
#define Cmfa               aircraft_->Cmfa
#define Cmfa_aArray        aircraft_->Cmfa_aArray
#define Cmfa_CmArray       aircraft_->Cmfa_CmArray
#define Cmfa_nAlpha        aircraft_->Cmfa_nAlpha
#define CmfaI              aircraft_->CmfaI
  string Cmfade;
  double Cmfade_aArray[100][100];
  double Cmfade_deArray[100];
  double Cmfade_CmArray[100][100];
  int Cmfade_nAlphaArray[100];
  int Cmfade_nde;
  double CmfadeI;
#define Cmfade             aircraft_->Cmfade
#define Cmfade_aArray      aircraft_->Cmfade_aArray
#define Cmfade_deArray     aircraft_->Cmfade_deArray
#define Cmfade_CmArray     aircraft_->Cmfade_CmArray
#define Cmfade_nAlphaArray aircraft_->Cmfade_nAlphaArray
#define Cmfade_nde         aircraft_->Cmfade_nde
#define CmfadeI            aircraft_->CmfadeI
  
  // induced flow in slipstream impinging on tail
  double w_induced;
#define w_induced          aircraft_->w_induced
  
  
  string Cmfdf;
  double Cmfdf_dfArray[100];
  double Cmfdf_CmArray[100];
  int Cmfdf_ndf;
  double CmfdfI;
#define Cmfdf              aircraft_->Cmfdf
#define Cmfdf_dfArray      aircraft_->Cmfdf_dfArray
#define Cmfdf_CmArray      aircraft_->Cmfdf_CmArray
#define Cmfdf_ndf          aircraft_->Cmfdf_ndf
#define CmfdfI             aircraft_->CmfdfI
  string Cmfadf;
  double Cmfadf_aArray[100][100];
  double Cmfadf_dfArray[100];
  double Cmfadf_CmArray[100][100];
  int Cmfadf_nAlphaArray[100];
  int Cmfadf_ndf;
  double CmfadfI;
#define Cmfadf             aircraft_->Cmfadf
#define Cmfadf_aArray      aircraft_->Cmfadf_aArray
#define Cmfadf_dfArray     aircraft_->Cmfadf_dfArray
#define Cmfadf_CmArray     aircraft_->Cmfadf_CmArray
#define Cmfadf_nAlphaArray aircraft_->Cmfadf_nAlphaArray
#define Cmfadf_ndf         aircraft_->Cmfadf_ndf
#define CmfadfI            aircraft_->CmfadfI
  double Cmfabetaf_aArray[30][100][100];
  double Cmfabetaf_betaArray[30][100];
  double Cmfabetaf_CmArray[30][100][100];
  int Cmfabetaf_nAlphaArray[30][100];
  int Cmfabetaf_nbeta[30];
  double Cmfabetaf_fArray[30];
  int Cmfabetaf_nf;
  double CmfabetafI;
  int Cmfabetaf_nice, Cmfabetaf_na_nice, Cmfabetaf_nb_nice;
  double Cmfabetaf_bArray_nice[100];
  double Cmfabetaf_aArray_nice[100];
#define Cmfabetaf_aArray        aircraft_->Cmfabetaf_aArray
#define Cmfabetaf_betaArray     aircraft_->Cmfabetaf_betaArray
#define Cmfabetaf_CmArray       aircraft_->Cmfabetaf_CmArray
#define Cmfabetaf_nAlphaArray   aircraft_->Cmfabetaf_nAlphaArray
#define Cmfabetaf_nbeta         aircraft_->Cmfabetaf_nbeta
#define Cmfabetaf_fArray        aircraft_->Cmfabetaf_fArray
#define Cmfabetaf_nf            aircraft_->Cmfabetaf_nf
#define CmfabetafI              aircraft_->CmfabetafI
#define Cmfabetaf_nice          aircraft_->Cmfabetaf_nice
#define Cmfabetaf_na_nice       aircraft_->Cmfabetaf_na_nice
#define Cmfabetaf_nb_nice       aircraft_->Cmfabetaf_nb_nice
#define Cmfabetaf_bArray_nice   aircraft_->Cmfabetaf_bArray_nice
#define Cmfabetaf_aArray_nice   aircraft_->Cmfabetaf_aArray_nice
  double Cmfadef_aArray[30][100][100];
  double Cmfadef_deArray[30][100];
  double Cmfadef_CmArray[30][100][100];
  int Cmfadef_nAlphaArray[30][100];
  int Cmfadef_nde[30];
  double Cmfadef_fArray[30];
  int Cmfadef_nf;
  double CmfadefI;
  int Cmfadef_nice, Cmfadef_na_nice, Cmfadef_nde_nice;
  double Cmfadef_deArray_nice[100];
  double Cmfadef_aArray_nice[100];
#define Cmfadef_aArray        aircraft_->Cmfadef_aArray
#define Cmfadef_deArray       aircraft_->Cmfadef_deArray
#define Cmfadef_CmArray       aircraft_->Cmfadef_CmArray
#define Cmfadef_nAlphaArray   aircraft_->Cmfadef_nAlphaArray
#define Cmfadef_nde           aircraft_->Cmfadef_nde
#define Cmfadef_fArray        aircraft_->Cmfadef_fArray
#define Cmfadef_nf            aircraft_->Cmfadef_nf
#define CmfadefI              aircraft_->CmfadefI
#define Cmfadef_nice          aircraft_->Cmfadef_nice
#define Cmfadef_na_nice       aircraft_->Cmfadef_na_nice
#define Cmfadef_nde_nice      aircraft_->Cmfadef_nde_nice
#define Cmfadef_deArray_nice  aircraft_->Cmfadef_deArray_nice
#define Cmfadef_aArray_nice   aircraft_->Cmfadef_aArray_nice
  double Cmfaqf_aArray[30][100][100];
  double Cmfaqf_qArray[30][100];
  double Cmfaqf_CmArray[30][100][100];
  int Cmfaqf_nAlphaArray[30][100];
  int Cmfaqf_nq[30];
  double Cmfaqf_fArray[30];
  int Cmfaqf_nf;
  double CmfaqfI;
  int Cmfaqf_nice, Cmfaqf_na_nice, Cmfaqf_nq_nice;
  double Cmfaqf_qArray_nice[100];
  double Cmfaqf_aArray_nice[100];
#define Cmfaqf_aArray        aircraft_->Cmfaqf_aArray
#define Cmfaqf_qArray        aircraft_->Cmfaqf_qArray
#define Cmfaqf_CmArray       aircraft_->Cmfaqf_CmArray
#define Cmfaqf_nAlphaArray   aircraft_->Cmfaqf_nAlphaArray
#define Cmfaqf_nq            aircraft_->Cmfaqf_nq
#define Cmfaqf_fArray        aircraft_->Cmfaqf_fArray
#define Cmfaqf_nf            aircraft_->Cmfaqf_nf
#define CmfaqfI              aircraft_->CmfaqfI
#define Cmfaqf_nice          aircraft_->Cmfaqf_nice
#define Cmfaqf_na_nice       aircraft_->Cmfaqf_na_nice
#define Cmfaqf_nq_nice       aircraft_->Cmfaqf_nq_nice
#define Cmfaqf_qArray_nice   aircraft_->Cmfaqf_qArray_nice
#define Cmfaqf_aArray_nice   aircraft_->Cmfaqf_aArray_nice
  double Cmo_save, Cm_a_save, Cm_a2_save, Cm_adot_save, Cm_q_save, Cm_ih_save;
  double Cm_de_save, Cm_b2_save, Cm_r_save;
  double Cm_df_save, Cm_ds_save, Cm_dg_save;
#define Cmo_save             aircraft_->Cmo_save
#define Cm_a_save            aircraft_->Cm_a_save
#define Cm_a2_save           aircraft_->Cm_a2_save
#define Cm_adot_save         aircraft_->Cm_adot_save
#define Cm_q_save            aircraft_->Cm_q_save
#define Cm_ih_save           aircraft_->Cm_ih_save 
#define Cm_de_save           aircraft_->Cm_de_save
#define Cm_b2_save           aircraft_->Cm_b2_save
#define Cm_r_save            aircraft_->Cm_r_save
#define Cm_df_save           aircraft_->Cm_df_save
#define Cm_ds_save           aircraft_->Cm_ds_save
#define Cm_dg_save           aircraft_->Cm_dg_save
  

  /* Variables (token2) ===========================================*/
  /* CY ============ Aerodynamic y-force quantities (lateral) =====*/
  
  std::map <string,int> CY_map;
#define      CY_map              aircraft_->CY_map            
  
  double CYo, CY_beta, CY_p, CY_r, CY_da, CY_dr, CY_dra, CY_bdot;
#define CYo      aircraft_->CYo
#define CY_beta  aircraft_->CY_beta
#define CY_p     aircraft_->CY_p
#define CY_r     aircraft_->CY_r
#define CY_da    aircraft_->CY_da
#define CY_dr    aircraft_->CY_dr
#define CY_dra   aircraft_->CY_dra
#define CY_bdot  aircraft_->CY_bdot
  string CYfada;
  double CYfada_aArray[100][100];
  double CYfada_daArray[100];
  double CYfada_CYArray[100][100];
  int CYfada_nAlphaArray[100];
  int CYfada_nda;
  double CYfadaI;
#define CYfada             aircraft_->CYfada
#define CYfada_aArray      aircraft_->CYfada_aArray
#define CYfada_daArray     aircraft_->CYfada_daArray
#define CYfada_CYArray     aircraft_->CYfada_CYArray
#define CYfada_nAlphaArray aircraft_->CYfada_nAlphaArray
#define CYfada_nda         aircraft_->CYfada_nda
#define CYfadaI            aircraft_->CYfadaI
  string CYfbetadr;
  double CYfbetadr_betaArray[100][100];
  double CYfbetadr_drArray[100];
  double CYfbetadr_CYArray[100][100];
  int CYfbetadr_nBetaArray[100];
  int CYfbetadr_ndr;
  double CYfbetadrI;
#define CYfbetadr             aircraft_->CYfbetadr
#define CYfbetadr_betaArray   aircraft_->CYfbetadr_betaArray
#define CYfbetadr_drArray     aircraft_->CYfbetadr_drArray
#define CYfbetadr_CYArray     aircraft_->CYfbetadr_CYArray
#define CYfbetadr_nBetaArray  aircraft_->CYfbetadr_nBetaArray
#define CYfbetadr_ndr         aircraft_->CYfbetadr_ndr
#define CYfbetadrI            aircraft_->CYfbetadrI
  double CYfabetaf_aArray[30][100][100];
  double CYfabetaf_betaArray[30][100];
  double CYfabetaf_CYArray[30][100][100];
  int CYfabetaf_nAlphaArray[30][100];
  int CYfabetaf_nbeta[30];
  double CYfabetaf_fArray[30];
  int CYfabetaf_nf;
  double CYfabetafI;
  int CYfabetaf_nice, CYfabetaf_na_nice, CYfabetaf_nb_nice;
  double CYfabetaf_bArray_nice[100];
  double CYfabetaf_aArray_nice[100];
#define CYfabetaf_aArray        aircraft_->CYfabetaf_aArray
#define CYfabetaf_betaArray     aircraft_->CYfabetaf_betaArray
#define CYfabetaf_CYArray       aircraft_->CYfabetaf_CYArray
#define CYfabetaf_nAlphaArray   aircraft_->CYfabetaf_nAlphaArray
#define CYfabetaf_nbeta         aircraft_->CYfabetaf_nbeta
#define CYfabetaf_fArray        aircraft_->CYfabetaf_fArray
#define CYfabetaf_nf            aircraft_->CYfabetaf_nf
#define CYfabetafI              aircraft_->CYfabetafI
#define CYfabetaf_nice          aircraft_->CYfabetaf_nice
#define CYfabetaf_na_nice       aircraft_->CYfabetaf_na_nice
#define CYfabetaf_nb_nice       aircraft_->CYfabetaf_nb_nice
#define CYfabetaf_bArray_nice   aircraft_->CYfabetaf_bArray_nice
#define CYfabetaf_aArray_nice   aircraft_->CYfabetaf_aArray_nice
  double CYfadaf_aArray[30][100][100];
  double CYfadaf_daArray[30][100];
  double CYfadaf_CYArray[30][100][100];
  int CYfadaf_nAlphaArray[30][100];
  int CYfadaf_nda[30];
  double CYfadaf_fArray[30];
  int CYfadaf_nf;
  double CYfadafI;
  int CYfadaf_nice, CYfadaf_na_nice, CYfadaf_nda_nice;
  double CYfadaf_daArray_nice[100];
  double CYfadaf_aArray_nice[100];
#define CYfadaf_aArray        aircraft_->CYfadaf_aArray
#define CYfadaf_daArray       aircraft_->CYfadaf_daArray
#define CYfadaf_CYArray       aircraft_->CYfadaf_CYArray
#define CYfadaf_nAlphaArray   aircraft_->CYfadaf_nAlphaArray
#define CYfadaf_nda           aircraft_->CYfadaf_nda
#define CYfadaf_fArray        aircraft_->CYfadaf_fArray
#define CYfadaf_nf            aircraft_->CYfadaf_nf
#define CYfadafI              aircraft_->CYfadafI
#define CYfadaf_nice          aircraft_->CYfadaf_nice
#define CYfadaf_na_nice       aircraft_->CYfadaf_na_nice
#define CYfadaf_nda_nice      aircraft_->CYfadaf_nda_nice
#define CYfadaf_daArray_nice  aircraft_->CYfadaf_daArray_nice
#define CYfadaf_aArray_nice   aircraft_->CYfadaf_aArray_nice
  double CYfadrf_aArray[30][100][100];
  double CYfadrf_drArray[30][100];
  double CYfadrf_CYArray[30][100][100];
  int CYfadrf_nAlphaArray[30][100];
  int CYfadrf_ndr[30];
  double CYfadrf_fArray[30];
  int CYfadrf_nf;
  double CYfadrfI;
  int CYfadrf_nice, CYfadrf_na_nice, CYfadrf_ndr_nice;
  double CYfadrf_drArray_nice[100];
  double CYfadrf_aArray_nice[100];
#define CYfadrf_aArray        aircraft_->CYfadrf_aArray
#define CYfadrf_drArray       aircraft_->CYfadrf_drArray
#define CYfadrf_CYArray       aircraft_->CYfadrf_CYArray
#define CYfadrf_nAlphaArray   aircraft_->CYfadrf_nAlphaArray
#define CYfadrf_ndr           aircraft_->CYfadrf_ndr
#define CYfadrf_fArray        aircraft_->CYfadrf_fArray
#define CYfadrf_nf            aircraft_->CYfadrf_nf
#define CYfadrfI              aircraft_->CYfadrfI
#define CYfadrf_nice          aircraft_->CYfadrf_nice
#define CYfadrf_na_nice       aircraft_->CYfadrf_na_nice
#define CYfadrf_ndr_nice      aircraft_->CYfadrf_ndr_nice
#define CYfadrf_drArray_nice  aircraft_->CYfadrf_drArray_nice
#define CYfadrf_aArray_nice   aircraft_->CYfadrf_aArray_nice
  double CYfapf_aArray[30][100][100];
  double CYfapf_pArray[30][100];
  double CYfapf_CYArray[30][100][100];
  int CYfapf_nAlphaArray[30][100];
  int CYfapf_np[30];
  double CYfapf_fArray[30];
  int CYfapf_nf;
  double CYfapfI;
  int CYfapf_nice, CYfapf_na_nice, CYfapf_np_nice;
  double CYfapf_pArray_nice[100];
  double CYfapf_aArray_nice[100];
#define CYfapf_aArray        aircraft_->CYfapf_aArray
#define CYfapf_pArray        aircraft_->CYfapf_pArray
#define CYfapf_CYArray       aircraft_->CYfapf_CYArray
#define CYfapf_nAlphaArray   aircraft_->CYfapf_nAlphaArray
#define CYfapf_np            aircraft_->CYfapf_np
#define CYfapf_fArray        aircraft_->CYfapf_fArray
#define CYfapf_nf            aircraft_->CYfapf_nf
#define CYfapfI              aircraft_->CYfapfI
#define CYfapf_nice          aircraft_->CYfapf_nice
#define CYfapf_na_nice       aircraft_->CYfapf_na_nice
#define CYfapf_np_nice       aircraft_->CYfapf_np_nice
#define CYfapf_pArray_nice   aircraft_->CYfapf_pArray_nice
#define CYfapf_aArray_nice   aircraft_->CYfapf_aArray_nice
  double CYfarf_aArray[30][100][100];
  double CYfarf_rArray[30][100];
  double CYfarf_CYArray[30][100][100];
  int CYfarf_nAlphaArray[30][100];
  int CYfarf_nr[30];
  double CYfarf_fArray[30];
  int CYfarf_nf;
  double CYfarfI;
  int CYfarf_nice, CYfarf_na_nice, CYfarf_nr_nice;
  double CYfarf_rArray_nice[100];
  double CYfarf_aArray_nice[100];
#define CYfarf_aArray        aircraft_->CYfarf_aArray
#define CYfarf_rArray        aircraft_->CYfarf_rArray
#define CYfarf_CYArray       aircraft_->CYfarf_CYArray
#define CYfarf_nAlphaArray   aircraft_->CYfarf_nAlphaArray
#define CYfarf_nr            aircraft_->CYfarf_nr
#define CYfarf_fArray        aircraft_->CYfarf_fArray
#define CYfarf_nf            aircraft_->CYfarf_nf
#define CYfarfI              aircraft_->CYfarfI
#define CYfarf_nice          aircraft_->CYfarf_nice
#define CYfarf_na_nice       aircraft_->CYfarf_na_nice
#define CYfarf_nr_nice       aircraft_->CYfarf_nr_nice
#define CYfarf_rArray_nice   aircraft_->CYfarf_rArray_nice
#define CYfarf_aArray_nice   aircraft_->CYfarf_aArray_nice
  double CYo_save, CY_beta_save, CY_p_save, CY_r_save, CY_da_save, CY_dr_save;
  double CY_dra_save, CY_bdot_save;
#define CYo_save             aircraft_->CYo_save
#define CY_beta_save         aircraft_->CY_beta_save
#define CY_p_save            aircraft_->CY_p_save
#define CY_r_save            aircraft_->CY_r_save
#define CY_da_save           aircraft_->CY_da_save
#define CY_dr_save           aircraft_->CY_dr_save
#define CY_dra_save          aircraft_->CY_dra_save
#define CY_bdot_save         aircraft_->CY_bdot_save


  /* Variables (token2) ===========================================*/
  /* Cl ============ Aerodynamic l-moment quantities (lateral) ====*/
  
  std::map <string,int> Cl_map;
#define      Cl_map              aircraft_->Cl_map            
  
  double Clo, Cl_beta, Cl_p, Cl_r, Cl_da, Cl_dr, Cl_daa;
#define Clo      aircraft_->Clo
#define Cl_beta  aircraft_->Cl_beta
#define Cl_p     aircraft_->Cl_p
#define Cl_r     aircraft_->Cl_r
#define Cl_da    aircraft_->Cl_da
#define Cl_dr    aircraft_->Cl_dr
#define Cl_daa   aircraft_->Cl_daa
  string Clfada;
  double Clfada_aArray[100][100];
  double Clfada_daArray[100];
  double Clfada_ClArray[100][100];
  int Clfada_nAlphaArray[100];
  int Clfada_nda;
  double ClfadaI;
#define Clfada             aircraft_->Clfada
#define Clfada_aArray      aircraft_->Clfada_aArray
#define Clfada_daArray     aircraft_->Clfada_daArray
#define Clfada_ClArray     aircraft_->Clfada_ClArray
#define Clfada_nAlphaArray aircraft_->Clfada_nAlphaArray
#define Clfada_nda         aircraft_->Clfada_nda
#define ClfadaI            aircraft_->ClfadaI
  string Clfbetadr;
  double Clfbetadr_betaArray[100][100];
  double Clfbetadr_drArray[100];
  double Clfbetadr_ClArray[100][100];
  int Clfbetadr_nBetaArray[100];
  int Clfbetadr_ndr;
  double ClfbetadrI;
#define Clfbetadr             aircraft_->Clfbetadr
#define Clfbetadr_betaArray   aircraft_->Clfbetadr_betaArray
#define Clfbetadr_drArray     aircraft_->Clfbetadr_drArray
#define Clfbetadr_ClArray     aircraft_->Clfbetadr_ClArray
#define Clfbetadr_nBetaArray  aircraft_->Clfbetadr_nBetaArray
#define Clfbetadr_ndr         aircraft_->Clfbetadr_ndr
#define ClfbetadrI            aircraft_->ClfbetadrI
  double Clfabetaf_aArray[30][100][100];
  double Clfabetaf_betaArray[30][100];
  double Clfabetaf_ClArray[30][100][100];
  int Clfabetaf_nAlphaArray[30][100];
  int Clfabetaf_nbeta[30];
  double Clfabetaf_fArray[30];
  int Clfabetaf_nf;
  double ClfabetafI;
  int Clfabetaf_nice, Clfabetaf_na_nice, Clfabetaf_nb_nice;
  double Clfabetaf_bArray_nice[100];
  double Clfabetaf_aArray_nice[100];
#define Clfabetaf_aArray        aircraft_->Clfabetaf_aArray
#define Clfabetaf_betaArray     aircraft_->Clfabetaf_betaArray
#define Clfabetaf_ClArray       aircraft_->Clfabetaf_ClArray
#define Clfabetaf_nAlphaArray   aircraft_->Clfabetaf_nAlphaArray
#define Clfabetaf_nbeta         aircraft_->Clfabetaf_nbeta
#define Clfabetaf_fArray        aircraft_->Clfabetaf_fArray
#define Clfabetaf_nf            aircraft_->Clfabetaf_nf
#define ClfabetafI              aircraft_->ClfabetafI
#define Clfabetaf_nice          aircraft_->Clfabetaf_nice
#define Clfabetaf_na_nice       aircraft_->Clfabetaf_na_nice
#define Clfabetaf_nb_nice       aircraft_->Clfabetaf_nb_nice
#define Clfabetaf_bArray_nice   aircraft_->Clfabetaf_bArray_nice
#define Clfabetaf_aArray_nice   aircraft_->Clfabetaf_aArray_nice
  double Clfadaf_aArray[30][100][100];
  double Clfadaf_daArray[30][100];
  double Clfadaf_ClArray[30][100][100];
  int Clfadaf_nAlphaArray[30][100];
  int Clfadaf_nda[30];
  double Clfadaf_fArray[30];
  int Clfadaf_nf;
  double ClfadafI;
  int Clfadaf_nice, Clfadaf_na_nice, Clfadaf_nda_nice;
  double Clfadaf_daArray_nice[100];
  double Clfadaf_aArray_nice[100];
#define Clfadaf_aArray        aircraft_->Clfadaf_aArray
#define Clfadaf_daArray       aircraft_->Clfadaf_daArray
#define Clfadaf_ClArray       aircraft_->Clfadaf_ClArray
#define Clfadaf_nAlphaArray   aircraft_->Clfadaf_nAlphaArray
#define Clfadaf_nda           aircraft_->Clfadaf_nda
#define Clfadaf_fArray        aircraft_->Clfadaf_fArray
#define Clfadaf_nf            aircraft_->Clfadaf_nf
#define ClfadafI              aircraft_->ClfadafI
#define Clfadaf_nice          aircraft_->Clfadaf_nice
#define Clfadaf_na_nice       aircraft_->Clfadaf_na_nice
#define Clfadaf_nda_nice      aircraft_->Clfadaf_nda_nice
#define Clfadaf_daArray_nice  aircraft_->Clfadaf_daArray_nice
#define Clfadaf_aArray_nice   aircraft_->Clfadaf_aArray_nice
  double Clfadrf_aArray[30][100][100];
  double Clfadrf_drArray[30][100];
  double Clfadrf_ClArray[30][100][100];
  int Clfadrf_nAlphaArray[30][100];
  int Clfadrf_ndr[30];
  double Clfadrf_fArray[30];
  int Clfadrf_nf;
  double ClfadrfI;
  int Clfadrf_nice, Clfadrf_na_nice, Clfadrf_ndr_nice;
  double Clfadrf_drArray_nice[100];
  double Clfadrf_aArray_nice[100];
#define Clfadrf_aArray        aircraft_->Clfadrf_aArray
#define Clfadrf_drArray       aircraft_->Clfadrf_drArray
#define Clfadrf_ClArray       aircraft_->Clfadrf_ClArray
#define Clfadrf_nAlphaArray   aircraft_->Clfadrf_nAlphaArray
#define Clfadrf_ndr           aircraft_->Clfadrf_ndr
#define Clfadrf_fArray        aircraft_->Clfadrf_fArray
#define Clfadrf_nf            aircraft_->Clfadrf_nf
#define ClfadrfI              aircraft_->ClfadrfI
#define Clfadrf_nice          aircraft_->Clfadrf_nice
#define Clfadrf_na_nice       aircraft_->Clfadrf_na_nice
#define Clfadrf_ndr_nice      aircraft_->Clfadrf_ndr_nice
#define Clfadrf_drArray_nice  aircraft_->Clfadrf_drArray_nice
#define Clfadrf_aArray_nice   aircraft_->Clfadrf_aArray_nice
  double Clfapf_aArray[30][100][100];
  double Clfapf_pArray[30][100];
  double Clfapf_ClArray[30][100][100];
  int Clfapf_nAlphaArray[30][100];
  int Clfapf_np[30];
  double Clfapf_fArray[30];
  int Clfapf_nf;
  double ClfapfI;
  int Clfapf_nice, Clfapf_na_nice, Clfapf_np_nice;
  double Clfapf_pArray_nice[100];
  double Clfapf_aArray_nice[100];
#define Clfapf_aArray        aircraft_->Clfapf_aArray
#define Clfapf_pArray        aircraft_->Clfapf_pArray
#define Clfapf_ClArray       aircraft_->Clfapf_ClArray
#define Clfapf_nAlphaArray   aircraft_->Clfapf_nAlphaArray
#define Clfapf_np            aircraft_->Clfapf_np
#define Clfapf_fArray        aircraft_->Clfapf_fArray
#define Clfapf_nf            aircraft_->Clfapf_nf
#define ClfapfI              aircraft_->ClfapfI
#define Clfapf_nice          aircraft_->Clfapf_nice
#define Clfapf_na_nice       aircraft_->Clfapf_na_nice
#define Clfapf_np_nice       aircraft_->Clfapf_np_nice
#define Clfapf_pArray_nice   aircraft_->Clfapf_pArray_nice
#define Clfapf_aArray_nice   aircraft_->Clfapf_aArray_nice
  double Clfarf_aArray[30][100][100];
  double Clfarf_rArray[30][100];
  double Clfarf_ClArray[30][100][100];
  int Clfarf_nAlphaArray[30][100];
  int Clfarf_nr[30];
  double Clfarf_fArray[30];
  int Clfarf_nf;
  double ClfarfI;
  int Clfarf_nice, Clfarf_na_nice, Clfarf_nr_nice;
  double Clfarf_rArray_nice[100];
  double Clfarf_aArray_nice[100];
#define Clfarf_aArray        aircraft_->Clfarf_aArray
#define Clfarf_rArray        aircraft_->Clfarf_rArray
#define Clfarf_ClArray       aircraft_->Clfarf_ClArray
#define Clfarf_nAlphaArray   aircraft_->Clfarf_nAlphaArray
#define Clfarf_nr            aircraft_->Clfarf_nr
#define Clfarf_fArray        aircraft_->Clfarf_fArray
#define Clfarf_nf            aircraft_->Clfarf_nf
#define ClfarfI              aircraft_->ClfarfI
#define Clfarf_nice          aircraft_->Clfarf_nice
#define Clfarf_na_nice       aircraft_->Clfarf_na_nice
#define Clfarf_nr_nice       aircraft_->Clfarf_nr_nice
#define Clfarf_rArray_nice   aircraft_->Clfarf_rArray_nice
#define Clfarf_aArray_nice   aircraft_->Clfarf_aArray_nice
  double Clo_save, Cl_beta_save, Cl_p_save, Cl_r_save, Cl_da_save; 
  double Cl_dr_save, Cl_daa_save;
#define Clo_save             aircraft_->Clo_save
#define Cl_beta_save         aircraft_->Cl_beta_save
#define Cl_p_save            aircraft_->Cl_p_save
#define Cl_r_save            aircraft_->Cl_r_save
#define Cl_da_save           aircraft_->Cl_da_save
#define Cl_dr_save           aircraft_->Cl_dr_save
#define Cl_daa_save          aircraft_->Cl_daa_save


  /* Variables (token2) ===========================================*/
  /* Cn ============ Aerodynamic n-moment quantities (lateral) ====*/
  
  std::map <string,int> Cn_map;
#define      Cn_map              aircraft_->Cn_map

  double Cno, Cn_beta, Cn_p, Cn_r, Cn_da, Cn_dr, Cn_q, Cn_b3;
#define Cno      aircraft_->Cno
#define Cn_beta  aircraft_->Cn_beta
#define Cn_p     aircraft_->Cn_p
#define Cn_r     aircraft_->Cn_r
#define Cn_da    aircraft_->Cn_da
#define Cn_dr    aircraft_->Cn_dr
#define Cn_q     aircraft_->Cn_q
#define Cn_b3    aircraft_->Cn_b3
  string Cnfada;
  double Cnfada_aArray[100][100];
  double Cnfada_daArray[100];
  double Cnfada_CnArray[100][100];
  int Cnfada_nAlphaArray[100];
  int Cnfada_nda;
  double CnfadaI;
#define Cnfada             aircraft_->Cnfada
#define Cnfada_aArray      aircraft_->Cnfada_aArray
#define Cnfada_daArray     aircraft_->Cnfada_daArray
#define Cnfada_CnArray     aircraft_->Cnfada_CnArray
#define Cnfada_nAlphaArray aircraft_->Cnfada_nAlphaArray
#define Cnfada_nda         aircraft_->Cnfada_nda
#define CnfadaI            aircraft_->CnfadaI
  string Cnfbetadr;
  double Cnfbetadr_betaArray[100][100];
  double Cnfbetadr_drArray[100];
  double Cnfbetadr_CnArray[100][100];
  int Cnfbetadr_nBetaArray[100];
  int Cnfbetadr_ndr;
  double CnfbetadrI;
#define Cnfbetadr             aircraft_->Cnfbetadr
#define Cnfbetadr_betaArray   aircraft_->Cnfbetadr_betaArray
#define Cnfbetadr_drArray     aircraft_->Cnfbetadr_drArray
#define Cnfbetadr_CnArray     aircraft_->Cnfbetadr_CnArray
#define Cnfbetadr_nBetaArray  aircraft_->Cnfbetadr_nBetaArray
#define Cnfbetadr_ndr         aircraft_->Cnfbetadr_ndr
#define CnfbetadrI            aircraft_->CnfbetadrI
  double Cnfabetaf_aArray[30][100][100];
  double Cnfabetaf_betaArray[30][100];
  double Cnfabetaf_CnArray[30][100][100];
  int Cnfabetaf_nAlphaArray[30][100];
  int Cnfabetaf_nbeta[30];
  double Cnfabetaf_fArray[30];
  int Cnfabetaf_nf;
  double CnfabetafI;
  int Cnfabetaf_nice, Cnfabetaf_na_nice, Cnfabetaf_nb_nice;
  double Cnfabetaf_bArray_nice[100];
  double Cnfabetaf_aArray_nice[100];
#define Cnfabetaf_aArray        aircraft_->Cnfabetaf_aArray
#define Cnfabetaf_betaArray     aircraft_->Cnfabetaf_betaArray
#define Cnfabetaf_CnArray       aircraft_->Cnfabetaf_CnArray
#define Cnfabetaf_nAlphaArray   aircraft_->Cnfabetaf_nAlphaArray
#define Cnfabetaf_nbeta         aircraft_->Cnfabetaf_nbeta
#define Cnfabetaf_fArray        aircraft_->Cnfabetaf_fArray
#define Cnfabetaf_nf            aircraft_->Cnfabetaf_nf
#define CnfabetafI              aircraft_->CnfabetafI
#define Cnfabetaf_nice          aircraft_->Cnfabetaf_nice
#define Cnfabetaf_na_nice       aircraft_->Cnfabetaf_na_nice
#define Cnfabetaf_nb_nice       aircraft_->Cnfabetaf_nb_nice
#define Cnfabetaf_bArray_nice   aircraft_->Cnfabetaf_bArray_nice
#define Cnfabetaf_aArray_nice   aircraft_->Cnfabetaf_aArray_nice
  double Cnfadaf_aArray[30][100][100];
  double Cnfadaf_daArray[30][100];
  double Cnfadaf_CnArray[30][100][100];
  int Cnfadaf_nAlphaArray[30][100];
  int Cnfadaf_nda[30];
  double Cnfadaf_fArray[30];
  int Cnfadaf_nf;
  double CnfadafI;
  int Cnfadaf_nice, Cnfadaf_na_nice, Cnfadaf_nda_nice;
  double Cnfadaf_daArray_nice[100];
  double Cnfadaf_aArray_nice[100];
#define Cnfadaf_aArray        aircraft_->Cnfadaf_aArray
#define Cnfadaf_daArray       aircraft_->Cnfadaf_daArray
#define Cnfadaf_CnArray       aircraft_->Cnfadaf_CnArray
#define Cnfadaf_nAlphaArray   aircraft_->Cnfadaf_nAlphaArray
#define Cnfadaf_nda           aircraft_->Cnfadaf_nda
#define Cnfadaf_fArray        aircraft_->Cnfadaf_fArray
#define Cnfadaf_nf            aircraft_->Cnfadaf_nf
#define CnfadafI              aircraft_->CnfadafI
#define Cnfadaf_nice          aircraft_->Cnfadaf_nice
#define Cnfadaf_na_nice       aircraft_->Cnfadaf_na_nice
#define Cnfadaf_nda_nice      aircraft_->Cnfadaf_nda_nice
#define Cnfadaf_daArray_nice  aircraft_->Cnfadaf_daArray_nice
#define Cnfadaf_aArray_nice   aircraft_->Cnfadaf_aArray_nice
  double Cnfadrf_aArray[30][100][100];
  double Cnfadrf_drArray[30][100];
  double Cnfadrf_CnArray[30][100][100];
  int Cnfadrf_nAlphaArray[30][100];
  int Cnfadrf_ndr[30];
  double Cnfadrf_fArray[30];
  int Cnfadrf_nf;
  double CnfadrfI;
  int Cnfadrf_nice, Cnfadrf_na_nice, Cnfadrf_ndr_nice;
  double Cnfadrf_drArray_nice[100];
  double Cnfadrf_aArray_nice[100];
#define Cnfadrf_aArray        aircraft_->Cnfadrf_aArray
#define Cnfadrf_drArray       aircraft_->Cnfadrf_drArray
#define Cnfadrf_CnArray       aircraft_->Cnfadrf_CnArray
#define Cnfadrf_nAlphaArray   aircraft_->Cnfadrf_nAlphaArray
#define Cnfadrf_ndr           aircraft_->Cnfadrf_ndr
#define Cnfadrf_fArray        aircraft_->Cnfadrf_fArray
#define Cnfadrf_nf            aircraft_->Cnfadrf_nf
#define CnfadrfI              aircraft_->CnfadrfI
#define Cnfadrf_nice          aircraft_->Cnfadrf_nice
#define Cnfadrf_na_nice       aircraft_->Cnfadrf_na_nice
#define Cnfadrf_ndr_nice      aircraft_->Cnfadrf_ndr_nice
#define Cnfadrf_drArray_nice  aircraft_->Cnfadrf_drArray_nice
#define Cnfadrf_aArray_nice   aircraft_->Cnfadrf_aArray_nice
  double Cnfapf_aArray[30][100][100];
  double Cnfapf_pArray[30][100];
  double Cnfapf_CnArray[30][100][100];
  int Cnfapf_nAlphaArray[30][100];
  int Cnfapf_np[30];
  double Cnfapf_fArray[30];
  int Cnfapf_nf;
  double CnfapfI;
  int Cnfapf_nice, Cnfapf_na_nice, Cnfapf_np_nice;
  double Cnfapf_pArray_nice[100];
  double Cnfapf_aArray_nice[100];
#define Cnfapf_aArray        aircraft_->Cnfapf_aArray
#define Cnfapf_pArray        aircraft_->Cnfapf_pArray
#define Cnfapf_CnArray       aircraft_->Cnfapf_CnArray
#define Cnfapf_nAlphaArray   aircraft_->Cnfapf_nAlphaArray
#define Cnfapf_np            aircraft_->Cnfapf_np
#define Cnfapf_fArray        aircraft_->Cnfapf_fArray
#define Cnfapf_nf            aircraft_->Cnfapf_nf
#define CnfapfI              aircraft_->CnfapfI
#define Cnfapf_nice          aircraft_->Cnfapf_nice
#define Cnfapf_na_nice       aircraft_->Cnfapf_na_nice
#define Cnfapf_np_nice       aircraft_->Cnfapf_np_nice
#define Cnfapf_pArray_nice   aircraft_->Cnfapf_pArray_nice
#define Cnfapf_aArray_nice   aircraft_->Cnfapf_aArray_nice
  double Cnfarf_aArray[30][100][100];
  double Cnfarf_rArray[30][100];
  double Cnfarf_CnArray[30][100][100];
  int Cnfarf_nAlphaArray[30][100];
  int Cnfarf_nr[30];
  double Cnfarf_fArray[30];
  int Cnfarf_nf;
  double CnfarfI;
  int Cnfarf_nice, Cnfarf_na_nice, Cnfarf_nr_nice;
  double Cnfarf_rArray_nice[100];
  double Cnfarf_aArray_nice[100];
#define Cnfarf_aArray        aircraft_->Cnfarf_aArray
#define Cnfarf_rArray        aircraft_->Cnfarf_rArray
#define Cnfarf_CnArray       aircraft_->Cnfarf_CnArray
#define Cnfarf_nAlphaArray   aircraft_->Cnfarf_nAlphaArray
#define Cnfarf_nr            aircraft_->Cnfarf_nr
#define Cnfarf_fArray        aircraft_->Cnfarf_fArray
#define Cnfarf_nf            aircraft_->Cnfarf_nf
#define CnfarfI              aircraft_->CnfarfI
#define Cnfarf_nice          aircraft_->Cnfarf_nice
#define Cnfarf_na_nice       aircraft_->Cnfarf_na_nice
#define Cnfarf_nr_nice       aircraft_->Cnfarf_nr_nice
#define Cnfarf_rArray_nice   aircraft_->Cnfarf_rArray_nice
#define Cnfarf_aArray_nice   aircraft_->Cnfarf_aArray_nice
  double Cno_save, Cn_beta_save, Cn_p_save, Cn_r_save;
  double Cn_da_save, Cn_dr_save, Cn_q_save, Cn_b3_save;
#define Cno_save             aircraft_->Cno_save
#define Cn_beta_save         aircraft_->Cn_beta_save
#define Cn_p_save            aircraft_->Cn_p_save
#define Cn_r_save            aircraft_->Cn_r_save
#define Cn_da_save           aircraft_->Cn_da_save
#define Cn_dr_save           aircraft_->Cn_dr_save
#define Cn_q_save            aircraft_->Cn_q_save
#define Cn_b3_save           aircraft_->Cn_b3_save


  /* Variables (token2) ===========================================*/
  /* gear ========== Landing gear model quantities ================*/
  
  std::map <string,int> gear_map;
  
#define      gear_map              aircraft_->gear_map
#define MAX_GEAR 16
  bool gear_model[MAX_GEAR];
  SCALAR D_gear_v[MAX_GEAR][3];
  SCALAR cgear[MAX_GEAR];
  SCALAR kgear[MAX_GEAR];
  SCALAR muGear[MAX_GEAR];
  SCALAR strutLength[MAX_GEAR];
#define D_gear_v aircraft_->D_gear_v
#define gear_model aircraft_->gear_model
#define cgear aircraft_->cgear
#define kgear aircraft_->kgear
#define muGear aircraft_->muGear
#define strutLength aircraft_->strutLength
  double gear_max, gear_rate;
#define gear_max                 aircraft_->gear_max
#define gear_rate                aircraft_->gear_rate
  bool use_gear;
#define use_gear                 aircraft_->use_gear
  

  /* Variables (token2) ===========================================*/
  /* ice =========== Ice model quantities ======================== */
  
  std::map <string,int> ice_map;
#define      ice_map              aircraft_->ice_map            

  bool ice_model, ice_on, beta_model;
  double iceTime;
  double transientTime;
  double eta_ice_final;
  double eta_ice;
  double x_probe_wing;
  double x_probe_tail;
#define ice_model      aircraft_->ice_model
#define ice_on         aircraft_->ice_on
#define beta_model     aircraft_->beta_model
#define iceTime        aircraft_->iceTime
#define transientTime  aircraft_->transientTime
#define eta_ice_final  aircraft_->eta_ice_final
#define eta_ice        aircraft_->eta_ice
#define x_probe_wing   aircraft_->x_probe_wing
#define x_probe_tail   aircraft_->x_probe_tail
  double kCDo, kCDK, kCD_a, kCD_adot, kCD_q, kCD_de;
  double CDo_clean, CDK_clean, CD_a_clean, CD_adot_clean, CD_q_clean, CD_de_clean;
#define kCDo           aircraft_->kCDo
#define kCDK           aircraft_->kCDK
#define kCD_a          aircraft_->kCD_a
#define kCD_adot       aircraft_->kCD_adot
#define kCD_q          aircraft_->kCD_q
#define kCD_de         aircraft_->kCD_de
#define CDo_clean      aircraft_->CDo_clean
#define CDK_clean      aircraft_->CDK_clean
#define CD_a_clean     aircraft_->CD_a_clean
#define CD_adot_clean  aircraft_->CD_adot_clean
#define CD_q_clean     aircraft_->CD_q_clean
#define CD_de_clean    aircraft_->CD_de_clean
  double kCXo, kCXK, kCX_a, kCX_a2, kCX_a3, kCX_adot;
  double kCX_q, kCX_de, kCX_dr, kCX_df, kCX_adf;
  double CXo_clean, CXK_clean, CX_a_clean, CX_a2_clean, CX_a3_clean, CX_adot_clean;
  double CX_q_clean, CX_de_clean, CX_dr_clean, CX_df_clean, CX_adf_clean;
#define kCXo           aircraft_->kCXo
#define kCXK           aircraft_->kCXK
#define kCX_a          aircraft_->kCX_a
#define kCX_a2         aircraft_->kCX_a2
#define kCX_a3         aircraft_->kCX_a3
#define kCX_adot       aircraft_->kCX_adot
#define kCX_q          aircraft_->kCX_q
#define kCX_de         aircraft_->kCX_de
#define kCX_dr         aircraft_->kCX_dr
#define kCX_df         aircraft_->kCX_df
#define kCX_adf        aircraft_->kCX_adf
#define CXo_clean      aircraft_->CXo_clean
#define CXK_clean      aircraft_->CXK_clean
#define CX_a_clean     aircraft_->CX_a_clean
#define CX_a2_clean    aircraft_->CX_a2_clean
#define CX_a3_clean    aircraft_->CX_a3_clean
#define CX_adot_clean  aircraft_->CX_adot_clean
#define CX_q_clean     aircraft_->CX_q_clean
#define CX_de_clean    aircraft_->CX_de_clean
#define CX_dr_clean    aircraft_->CX_dr_clean
#define CX_df_clean    aircraft_->CX_df_clean
#define CX_adf_clean   aircraft_->CX_adf_clean
  double kCLo, kCL_a, kCL_adot, kCL_q, kCL_de;
  double CLo_clean, CL_a_clean, CL_adot_clean, CL_q_clean, CL_de_clean;
#define kCLo           aircraft_->kCLo
#define kCL_a          aircraft_->kCL_a
#define kCL_adot       aircraft_->kCL_adot
#define kCL_q          aircraft_->kCL_q
#define kCL_de         aircraft_->kCL_de
#define CLo_clean      aircraft_->CLo_clean
#define CL_a_clean     aircraft_->CL_a_clean
#define CL_adot_clean  aircraft_->CL_adot_clean
#define CL_q_clean     aircraft_->CL_q_clean
#define CL_de_clean    aircraft_->CL_de_clean
  double kCZo, kCZ_a, kCZ_a2, kCZ_a3, kCZ_adot, kCZ_q, kCZ_de, kCZ_deb2, kCZ_df, kCZ_adf;
  double CZo_clean, CZ_a_clean, CZ_a2_clean, CZ_a3_clean, CZ_adot_clean;
  double CZ_q_clean, CZ_de_clean, CZ_deb2_clean, CZ_df_clean, CZ_adf_clean;
#define kCZo           aircraft_->kCZo
#define kCZ_a          aircraft_->kCZ_a
#define kCZ_a2         aircraft_->kCZ_a2
#define kCZ_a3         aircraft_->kCZ_a3
#define kCZ_adot       aircraft_->kCZ_adot
#define kCZ_q          aircraft_->kCZ_q
#define kCZ_de         aircraft_->kCZ_de
#define kCZ_deb2       aircraft_->kCZ_deb2
#define kCZ_df         aircraft_->kCZ_df
#define kCZ_adf        aircraft_->kCZ_adf
#define CZo_clean      aircraft_->CZo_clean
#define CZ_a_clean     aircraft_->CZ_a_clean
#define CZ_a2_clean    aircraft_->CZ_a2_clean
#define CZ_a3_clean    aircraft_->CZ_a3_clean
#define CZ_adot_clean  aircraft_->CZ_adot_clean
#define CZ_q_clean     aircraft_->CZ_q_clean
#define CZ_de_clean    aircraft_->CZ_de_clean
#define CZ_deb2_clean  aircraft_->CZ_deb2_clean
#define CZ_df_clean    aircraft_->CZ_df_clean
#define CZ_adf_clean   aircraft_->CZ_adf_clean
  double kCmo, kCm_a, kCm_a2, kCm_adot, kCm_q, kCm_de, kCm_b2, kCm_r, kCm_df;
  double Cmo_clean, Cm_a_clean, Cm_a2_clean, Cm_adot_clean, Cm_q_clean;
  double Cm_de_clean, Cm_b2_clean, Cm_r_clean, Cm_df_clean;
#define kCmo           aircraft_->kCmo
#define kCm_a          aircraft_->kCm_a
#define kCm_a2         aircraft_->kCm_a2
#define kCm_adot       aircraft_->kCm_adot
#define kCm_q          aircraft_->kCm_q
#define kCm_de         aircraft_->kCm_de
#define kCm_b2         aircraft_->kCm_b2
#define kCm_r          aircraft_->kCm_r
#define kCm_df         aircraft_->kCm_df
#define Cmo_clean      aircraft_->Cmo_clean
#define Cm_a_clean     aircraft_->Cm_a_clean
#define Cm_a2_clean    aircraft_->Cm_a2_clean
#define Cm_adot_clean  aircraft_->Cm_adot_clean
#define Cm_q_clean     aircraft_->Cm_q_clean
#define Cm_de_clean    aircraft_->Cm_de_clean
#define Cm_b2_clean    aircraft_->Cm_b2_clean
#define Cm_r_clean     aircraft_->Cm_r_clean
#define Cm_df_clean    aircraft_->Cm_df_clean
  double kCYo, kCY_beta, kCY_p, kCY_r, kCY_da, kCY_dr, kCY_dra, kCY_bdot;
  double CYo_clean, CY_beta_clean, CY_p_clean, CY_r_clean, CY_da_clean;
  double CY_dr_clean, CY_dra_clean, CY_bdot_clean;
#define kCYo           aircraft_->kCYo
#define kCY_beta       aircraft_->kCY_beta
#define kCY_p          aircraft_->kCY_p
#define kCY_r          aircraft_->kCY_r
#define kCY_da         aircraft_->kCY_da
#define kCY_dr         aircraft_->kCY_dr
#define kCY_dra        aircraft_->kCY_dra
#define kCY_bdot       aircraft_->kCY_bdot
#define CYo_clean      aircraft_->CYo_clean
#define CY_beta_clean  aircraft_->CY_beta_clean
#define CY_p_clean     aircraft_->CY_p_clean
#define CY_r_clean     aircraft_->CY_r_clean
#define CY_da_clean    aircraft_->CY_da_clean
#define CY_dr_clean    aircraft_->CY_dr_clean
#define CY_dra_clean   aircraft_->CY_dra_clean
#define CY_bdot_clean  aircraft_->CY_bdot_clean
  double kClo, kCl_beta, kCl_p, kCl_r, kCl_da, kCl_dr, kCl_daa;
  double Clo_clean, Cl_beta_clean, Cl_p_clean, Cl_r_clean, Cl_da_clean;
  double Cl_dr_clean, Cl_daa_clean;
#define kClo           aircraft_->kClo
#define kCl_beta       aircraft_->kCl_beta
#define kCl_p          aircraft_->kCl_p
#define kCl_r          aircraft_->kCl_r
#define kCl_da         aircraft_->kCl_da
#define kCl_dr         aircraft_->kCl_dr
#define kCl_daa        aircraft_->kCl_daa
#define Clo_clean      aircraft_->Clo_clean
#define Cl_beta_clean  aircraft_->Cl_beta_clean
#define Cl_p_clean     aircraft_->Cl_p_clean
#define Cl_r_clean     aircraft_->Cl_r_clean
#define Cl_da_clean    aircraft_->Cl_da_clean
#define Cl_dr_clean    aircraft_->Cl_dr_clean
#define Cl_daa_clean   aircraft_->Cl_daa_clean
  double kCno, kCn_beta, kCn_p, kCn_r, kCn_da, kCn_dr, kCn_q, kCn_b3;
  double Cno_clean, Cn_beta_clean, Cn_p_clean, Cn_r_clean, Cn_da_clean;
  double Cn_dr_clean, Cn_q_clean, Cn_b3_clean;
#define kCno           aircraft_->kCno
#define kCn_beta       aircraft_->kCn_beta
#define kCn_p          aircraft_->kCn_p
#define kCn_r          aircraft_->kCn_r
#define kCn_da         aircraft_->kCn_da
#define kCn_dr         aircraft_->kCn_dr
#define kCn_q          aircraft_->kCn_q
#define kCn_b3         aircraft_->kCn_b3
#define Cno_clean      aircraft_->Cno_clean
#define Cn_beta_clean  aircraft_->Cn_beta_clean
#define Cn_p_clean     aircraft_->Cn_p_clean
#define Cn_r_clean     aircraft_->Cn_r_clean
#define Cn_da_clean    aircraft_->Cn_da_clean
#define Cn_dr_clean    aircraft_->Cn_dr_clean
#define Cn_q_clean     aircraft_->Cn_q_clean
#define Cn_b3_clean    aircraft_->Cn_b3_clean
  double bootTime[20];
  int    bootindex;
  bool   bootTrue[20];
#define bootTime       aircraft_->bootTime  
#define bootindex      aircraft_->bootindex 
#define bootTrue       aircraft_->bootTrue
  bool eta_from_file;
#define eta_from_file             aircraft_->eta_from_file
  bool eta_wing_left_input;
  string eta_wing_left_input_file;
  double eta_wing_left_input_timeArray[100];
  double eta_wing_left_input_daArray[100];
  int eta_wing_left_input_ntime;
  double eta_wing_left_input_startTime;
#define eta_wing_left_input           aircraft_->eta_wing_left_input
#define eta_wing_left_input_file      aircraft_->eta_wing_left_input_file
#define eta_wing_left_input_timeArray aircraft_->eta_wing_left_input_timeArray
#define eta_wing_left_input_daArray   aircraft_->eta_wing_left_input_daArray
#define eta_wing_left_input_ntime     aircraft_->eta_wing_left_input_ntime
#define eta_wing_left_input_startTime aircraft_->eta_wing_left_input_startTime
  bool eta_wing_right_input;
  string eta_wing_right_input_file;
  double eta_wing_right_input_timeArray[100];
  double eta_wing_right_input_daArray[100];
  int eta_wing_right_input_ntime;
  double eta_wing_right_input_startTime;
#define eta_wing_right_input           aircraft_->eta_wing_right_input
#define eta_wing_right_input_file      aircraft_->eta_wing_right_input_file
#define eta_wing_right_input_timeArray aircraft_->eta_wing_right_input_timeArray
#define eta_wing_right_input_daArray   aircraft_->eta_wing_right_input_daArray
#define eta_wing_right_input_ntime     aircraft_->eta_wing_right_input_ntime
#define eta_wing_right_input_startTime aircraft_->eta_wing_right_input_startTime
  bool eta_tail_input;
  string eta_tail_input_file;
  double eta_tail_input_timeArray[100];
  double eta_tail_input_daArray[100];
  int eta_tail_input_ntime;
  double eta_tail_input_startTime;
#define eta_tail_input           aircraft_->eta_tail_input
#define eta_tail_input_file      aircraft_->eta_tail_input_file
#define eta_tail_input_timeArray aircraft_->eta_tail_input_timeArray
#define eta_tail_input_daArray   aircraft_->eta_tail_input_daArray
#define eta_tail_input_ntime     aircraft_->eta_tail_input_ntime
#define eta_tail_input_startTime aircraft_->eta_tail_input_startTime
  bool demo_eps_alpha_max;
  string demo_eps_alpha_max_file;
  double demo_eps_alpha_max_timeArray[100];
  double demo_eps_alpha_max_daArray[100];
  int demo_eps_alpha_max_ntime;
  double demo_eps_alpha_max_startTime;
#define demo_eps_alpha_max           aircraft_->demo_eps_alpha_max
#define demo_eps_alpha_max_file      aircraft_->demo_eps_alpha_max_file
#define demo_eps_alpha_max_timeArray aircraft_->demo_eps_alpha_max_timeArray
#define demo_eps_alpha_max_daArray   aircraft_->demo_eps_alpha_max_daArray
#define demo_eps_alpha_max_ntime     aircraft_->demo_eps_alpha_max_ntime
#define demo_eps_alpha_max_startTime aircraft_->demo_eps_alpha_max_startTime
  bool demo_eps_pitch_max;
  string demo_eps_pitch_max_file;
  double demo_eps_pitch_max_timeArray[100];
  double demo_eps_pitch_max_daArray[100];
  int demo_eps_pitch_max_ntime;
  double demo_eps_pitch_max_startTime;
#define demo_eps_pitch_max           aircraft_->demo_eps_pitch_max
#define demo_eps_pitch_max_file      aircraft_->demo_eps_pitch_max_file
#define demo_eps_pitch_max_timeArray aircraft_->demo_eps_pitch_max_timeArray
#define demo_eps_pitch_max_daArray   aircraft_->demo_eps_pitch_max_daArray
#define demo_eps_pitch_max_ntime     aircraft_->demo_eps_pitch_max_ntime
#define demo_eps_pitch_max_startTime aircraft_->demo_eps_pitch_max_startTime
  bool demo_eps_pitch_min;
  string demo_eps_pitch_min_file;
  double demo_eps_pitch_min_timeArray[100];
  double demo_eps_pitch_min_daArray[100];
  int demo_eps_pitch_min_ntime;
  double demo_eps_pitch_min_startTime;
#define demo_eps_pitch_min           aircraft_->demo_eps_pitch_min
#define demo_eps_pitch_min_file      aircraft_->demo_eps_pitch_min_file
#define demo_eps_pitch_min_timeArray aircraft_->demo_eps_pitch_min_timeArray
#define demo_eps_pitch_min_daArray   aircraft_->demo_eps_pitch_min_daArray
#define demo_eps_pitch_min_ntime     aircraft_->demo_eps_pitch_min_ntime
#define demo_eps_pitch_min_startTime aircraft_->demo_eps_pitch_min_startTime
  bool demo_eps_roll_max;
  string demo_eps_roll_max_file;
  double demo_eps_roll_max_timeArray[10];
  double demo_eps_roll_max_daArray[10];
  int demo_eps_roll_max_ntime;
  double demo_eps_roll_max_startTime;
#define demo_eps_roll_max           aircraft_->demo_eps_roll_max
#define demo_eps_roll_max_file      aircraft_->demo_eps_roll_max_file
#define demo_eps_roll_max_timeArray aircraft_->demo_eps_roll_max_timeArray
#define demo_eps_roll_max_daArray   aircraft_->demo_eps_roll_max_daArray
#define demo_eps_roll_max_ntime     aircraft_->demo_eps_roll_max_ntime
#define demo_eps_roll_max_startTime aircraft_->demo_eps_roll_max_startTime
  bool demo_eps_thrust_min;
  string demo_eps_thrust_min_file;
  double demo_eps_thrust_min_timeArray[100];
  double demo_eps_thrust_min_daArray[100];
  int demo_eps_thrust_min_ntime;
  double demo_eps_thrust_min_startTime;
#define demo_eps_thrust_min           aircraft_->demo_eps_thrust_min
#define demo_eps_thrust_min_file      aircraft_->demo_eps_thrust_min_file
#define demo_eps_thrust_min_timeArray aircraft_->demo_eps_thrust_min_timeArray
#define demo_eps_thrust_min_daArray   aircraft_->demo_eps_thrust_min_daArray
#define demo_eps_thrust_min_ntime     aircraft_->demo_eps_thrust_min_ntime
#define demo_eps_thrust_min_startTime aircraft_->demo_eps_thrust_min_startTime
  bool demo_eps_airspeed_max;
  string demo_eps_airspeed_max_file;
  double demo_eps_airspeed_max_timeArray[10];
  double demo_eps_airspeed_max_daArray[10];
  int demo_eps_airspeed_max_ntime;
  double demo_eps_airspeed_max_startTime;
#define demo_eps_airspeed_max           aircraft_->demo_eps_airspeed_max
#define demo_eps_airspeed_max_file      aircraft_->demo_eps_airspeed_max_file
#define demo_eps_airspeed_max_timeArray aircraft_->demo_eps_airspeed_max_timeArray
#define demo_eps_airspeed_max_daArray   aircraft_->demo_eps_airspeed_max_daArray
#define demo_eps_airspeed_max_ntime     aircraft_->demo_eps_airspeed_max_ntime
#define demo_eps_airspeed_max_startTime aircraft_->demo_eps_airspeed_max_startTime
  bool demo_eps_airspeed_min;
  string demo_eps_airspeed_min_file;
  double demo_eps_airspeed_min_timeArray[100];
  double demo_eps_airspeed_min_daArray[100];
  int demo_eps_airspeed_min_ntime;
  double demo_eps_airspeed_min_startTime;
#define demo_eps_airspeed_min           aircraft_->demo_eps_airspeed_min
#define demo_eps_airspeed_min_file      aircraft_->demo_eps_airspeed_min_file
#define demo_eps_airspeed_min_timeArray aircraft_->demo_eps_airspeed_min_timeArray
#define demo_eps_airspeed_min_daArray   aircraft_->demo_eps_airspeed_min_daArray
#define demo_eps_airspeed_min_ntime     aircraft_->demo_eps_airspeed_min_ntime
#define demo_eps_airspeed_min_startTime aircraft_->demo_eps_airspeed_min_startTime
  bool demo_eps_flap_max;
  string demo_eps_flap_max_file;
  double demo_eps_flap_max_timeArray[10];
  double demo_eps_flap_max_daArray[10];
  int demo_eps_flap_max_ntime;
  double demo_eps_flap_max_startTime;
#define demo_eps_flap_max           aircraft_->demo_eps_flap_max
#define demo_eps_flap_max_file      aircraft_->demo_eps_flap_max_file
#define demo_eps_flap_max_timeArray aircraft_->demo_eps_flap_max_timeArray
#define demo_eps_flap_max_daArray   aircraft_->demo_eps_flap_max_daArray
#define demo_eps_flap_max_ntime     aircraft_->demo_eps_flap_max_ntime
#define demo_eps_flap_max_startTime aircraft_->demo_eps_flap_max_startTime
  bool demo_boot_cycle_tail;
  string demo_boot_cycle_tail_file;
  double demo_boot_cycle_tail_timeArray[100];
  int demo_boot_cycle_tail_daArray[100];
  int demo_boot_cycle_tail_ntime;
  double demo_boot_cycle_tail_startTime;
#define demo_boot_cycle_tail           aircraft_->demo_boot_cycle_tail
#define demo_boot_cycle_tail_file      aircraft_->demo_boot_cycle_tail_file
#define demo_boot_cycle_tail_timeArray aircraft_->demo_boot_cycle_tail_timeArray
#define demo_boot_cycle_tail_daArray   aircraft_->demo_boot_cycle_tail_daArray
#define demo_boot_cycle_tail_ntime     aircraft_->demo_boot_cycle_tail_ntime
#define demo_boot_cycle_tail_startTime aircraft_->demo_boot_cycle_tail_startTime
  bool demo_boot_cycle_wing_left;
  string demo_boot_cycle_wing_left_file;
  double demo_boot_cycle_wing_left_timeArray[100];
  int demo_boot_cycle_wing_left_daArray[100];
  int demo_boot_cycle_wing_left_ntime;
  double demo_boot_cycle_wing_left_startTime;
#define demo_boot_cycle_wing_left           aircraft_->demo_boot_cycle_wing_left
#define demo_boot_cycle_wing_left_file      aircraft_->demo_boot_cycle_wing_left_file
#define demo_boot_cycle_wing_left_timeArray aircraft_->demo_boot_cycle_wing_left_timeArray
#define demo_boot_cycle_wing_left_daArray   aircraft_->demo_boot_cycle_wing_left_daArray
#define demo_boot_cycle_wing_left_ntime     aircraft_->demo_boot_cycle_wing_left_ntime
#define demo_boot_cycle_wing_left_startTime aircraft_->demo_boot_cycle_wing_left_startTime
  bool demo_boot_cycle_wing_right;
  string demo_boot_cycle_wing_right_file;
  double demo_boot_cycle_wing_right_timeArray[100];
  int demo_boot_cycle_wing_right_daArray[100];
  int demo_boot_cycle_wing_right_ntime;
  double demo_boot_cycle_wing_right_startTime;
#define demo_boot_cycle_wing_right           aircraft_->demo_boot_cycle_wing_right
#define demo_boot_cycle_wing_right_file      aircraft_->demo_boot_cycle_wing_right_file
#define demo_boot_cycle_wing_right_timeArray aircraft_->demo_boot_cycle_wing_right_timeArray
#define demo_boot_cycle_wing_right_daArray   aircraft_->demo_boot_cycle_wing_right_daArray
#define demo_boot_cycle_wing_right_ntime     aircraft_->demo_boot_cycle_wing_right_ntime
#define demo_boot_cycle_wing_right_startTime aircraft_->demo_boot_cycle_wing_right_startTime
  bool demo_eps_pitch_input;
  string demo_eps_pitch_input_file;
  double demo_eps_pitch_input_timeArray[100];
  int demo_eps_pitch_input_daArray[100];
  int demo_eps_pitch_input_ntime;
  double demo_eps_pitch_input_startTime;
#define demo_eps_pitch_input           aircraft_->demo_eps_pitch_input
#define demo_eps_pitch_input_file      aircraft_->demo_eps_pitch_input_file
#define demo_eps_pitch_input_timeArray aircraft_->demo_eps_pitch_input_timeArray
#define demo_eps_pitch_input_daArray   aircraft_->demo_eps_pitch_input_daArray
#define demo_eps_pitch_input_ntime     aircraft_->demo_eps_pitch_input_ntime
#define demo_eps_pitch_input_startTime aircraft_->demo_eps_pitch_input_startTime
  bool tactilefadef;
  double tactilefadef_aArray[30][100][100];
  double tactilefadef_deArray[30][100];
  double tactilefadef_tactileArray[30][100][100];
  int tactilefadef_nAlphaArray[30][100];
  int tactilefadef_nde[30];
  double tactilefadef_fArray[30];
  int tactilefadef_nf;
  double tactilefadefI;
  int tactilefadef_nice, tactilefadef_na_nice, tactilefadef_nde_nice;
  double tactilefadef_deArray_nice[100];
  double tactilefadef_aArray_nice[100];
#define tactilefadef               aircraft_->tactilefadef
#define tactilefadef_aArray        aircraft_->tactilefadef_aArray
#define tactilefadef_deArray       aircraft_->tactilefadef_deArray
#define tactilefadef_tactileArray  aircraft_->tactilefadef_tactileArray
#define tactilefadef_nAlphaArray   aircraft_->tactilefadef_nAlphaArray
#define tactilefadef_nde           aircraft_->tactilefadef_nde
#define tactilefadef_fArray        aircraft_->tactilefadef_fArray
#define tactilefadef_nf            aircraft_->tactilefadef_nf
#define tactilefadefI              aircraft_->tactilefadefI
#define tactilefadef_nice          aircraft_->tactilefadef_nice
#define tactilefadef_na_nice       aircraft_->tactilefadef_na_nice
#define tactilefadef_nde_nice      aircraft_->tactilefadef_nde_nice
#define tactilefadef_deArray_nice  aircraft_->tactilefadef_deArray_nice
#define tactilefadef_aArray_nice   aircraft_->tactilefadef_aArray_nice
  int tactile_pitch;
#define tactile_pitch              aircraft_->tactile_pitch
  bool demo_ap_pah_on;
  string demo_ap_pah_on_file;
  double demo_ap_pah_on_timeArray[10];
  int demo_ap_pah_on_daArray[10];
  int demo_ap_pah_on_ntime;
  double demo_ap_pah_on_startTime;
#define demo_ap_pah_on           aircraft_->demo_ap_pah_on
#define demo_ap_pah_on_file      aircraft_->demo_ap_pah_on_file
#define demo_ap_pah_on_timeArray aircraft_->demo_ap_pah_on_timeArray
#define demo_ap_pah_on_daArray   aircraft_->demo_ap_pah_on_daArray
#define demo_ap_pah_on_ntime     aircraft_->demo_ap_pah_on_ntime
#define demo_ap_pah_on_startTime aircraft_->demo_ap_pah_on_startTime
  bool demo_ap_alh_on;
  string demo_ap_alh_on_file;
  double demo_ap_alh_on_timeArray[10];
  int demo_ap_alh_on_daArray[10];
  int demo_ap_alh_on_ntime;
  double demo_ap_alh_on_startTime;
#define demo_ap_alh_on           aircraft_->demo_ap_alh_on
#define demo_ap_alh_on_file      aircraft_->demo_ap_alh_on_file
#define demo_ap_alh_on_timeArray aircraft_->demo_ap_alh_on_timeArray
#define demo_ap_alh_on_daArray   aircraft_->demo_ap_alh_on_daArray
#define demo_ap_alh_on_ntime     aircraft_->demo_ap_alh_on_ntime
#define demo_ap_alh_on_startTime aircraft_->demo_ap_alh_on_startTime
  bool demo_ap_rah_on;
  string demo_ap_rah_on_file;
  double demo_ap_rah_on_timeArray[10];
  int demo_ap_rah_on_daArray[10];
  int demo_ap_rah_on_ntime;
  double demo_ap_rah_on_startTime;
#define demo_ap_rah_on           aircraft_->demo_ap_rah_on
#define demo_ap_rah_on_file      aircraft_->demo_ap_rah_on_file
#define demo_ap_rah_on_timeArray aircraft_->demo_ap_rah_on_timeArray
#define demo_ap_rah_on_daArray   aircraft_->demo_ap_rah_on_daArray
#define demo_ap_rah_on_ntime     aircraft_->demo_ap_rah_on_ntime
#define demo_ap_rah_on_startTime aircraft_->demo_ap_rah_on_startTime
  bool demo_ap_hh_on;
  string demo_ap_hh_on_file;
  double demo_ap_hh_on_timeArray[10];
  int demo_ap_hh_on_daArray[10];
  int demo_ap_hh_on_ntime;
  double demo_ap_hh_on_startTime;
#define demo_ap_hh_on           aircraft_->demo_ap_hh_on
#define demo_ap_hh_on_file      aircraft_->demo_ap_hh_on_file
#define demo_ap_hh_on_timeArray aircraft_->demo_ap_hh_on_timeArray
#define demo_ap_hh_on_daArray   aircraft_->demo_ap_hh_on_daArray
#define demo_ap_hh_on_ntime     aircraft_->demo_ap_hh_on_ntime
#define demo_ap_hh_on_startTime aircraft_->demo_ap_hh_on_startTime
  bool demo_ap_Theta_ref;
  string demo_ap_Theta_ref_file;
  double demo_ap_Theta_ref_timeArray[10];
  double demo_ap_Theta_ref_daArray[10];
  int demo_ap_Theta_ref_ntime;
  double demo_ap_Theta_ref_startTime;
#define demo_ap_Theta_ref           aircraft_->demo_ap_Theta_ref
#define demo_ap_Theta_ref_file      aircraft_->demo_ap_Theta_ref_file
#define demo_ap_Theta_ref_timeArray aircraft_->demo_ap_Theta_ref_timeArray
#define demo_ap_Theta_ref_daArray   aircraft_->demo_ap_Theta_ref_daArray
#define demo_ap_Theta_ref_ntime     aircraft_->demo_ap_Theta_ref_ntime
#define demo_ap_Theta_ref_startTime aircraft_->demo_ap_Theta_ref_startTime
  bool demo_ap_alt_ref;
  string demo_ap_alt_ref_file;
  double demo_ap_alt_ref_timeArray[10];
  double demo_ap_alt_ref_daArray[10];
  int demo_ap_alt_ref_ntime;
  double demo_ap_alt_ref_startTime;
#define demo_ap_alt_ref           aircraft_->demo_ap_alt_ref
#define demo_ap_alt_ref_file      aircraft_->demo_ap_alt_ref_file
#define demo_ap_alt_ref_timeArray aircraft_->demo_ap_alt_ref_timeArray
#define demo_ap_alt_ref_daArray   aircraft_->demo_ap_alt_ref_daArray
#define demo_ap_alt_ref_ntime     aircraft_->demo_ap_alt_ref_ntime
#define demo_ap_alt_ref_startTime aircraft_->demo_ap_alt_ref_startTime
  bool demo_ap_Phi_ref;
  string demo_ap_Phi_ref_file;
  double demo_ap_Phi_ref_timeArray[10];
  double demo_ap_Phi_ref_daArray[10];
  int demo_ap_Phi_ref_ntime;
  double demo_ap_Phi_ref_startTime;
#define demo_ap_Phi_ref           aircraft_->demo_ap_Phi_ref
#define demo_ap_Phi_ref_file      aircraft_->demo_ap_Phi_ref_file
#define demo_ap_Phi_ref_timeArray aircraft_->demo_ap_Phi_ref_timeArray
#define demo_ap_Phi_ref_daArray   aircraft_->demo_ap_Phi_ref_daArray
#define demo_ap_Phi_ref_ntime     aircraft_->demo_ap_Phi_ref_ntime
#define demo_ap_Phi_ref_startTime aircraft_->demo_ap_Phi_ref_startTime
  bool demo_ap_Psi_ref;
  string demo_ap_Psi_ref_file;
  double demo_ap_Psi_ref_timeArray[10];
  double demo_ap_Psi_ref_daArray[10];
  int demo_ap_Psi_ref_ntime;
  double demo_ap_Psi_ref_startTime;
#define demo_ap_Psi_ref           aircraft_->demo_ap_Psi_ref
#define demo_ap_Psi_ref_file      aircraft_->demo_ap_Psi_ref_file
#define demo_ap_Psi_ref_timeArray aircraft_->demo_ap_Psi_ref_timeArray
#define demo_ap_Psi_ref_daArray   aircraft_->demo_ap_Psi_ref_daArray
#define demo_ap_Psi_ref_ntime     aircraft_->demo_ap_Psi_ref_ntime
#define demo_ap_Psi_ref_startTime aircraft_->demo_ap_Psi_ref_startTime
  bool demo_tactile;
  string demo_tactile_file;
  double demo_tactile_timeArray[1500];
  double demo_tactile_daArray[1500];
  int demo_tactile_ntime;
  double demo_tactile_startTime;
#define demo_tactile           aircraft_->demo_tactile
#define demo_tactile_file      aircraft_->demo_tactile_file
#define demo_tactile_timeArray aircraft_->demo_tactile_timeArray
#define demo_tactile_daArray   aircraft_->demo_tactile_daArray
#define demo_tactile_ntime     aircraft_->demo_tactile_ntime
#define demo_tactile_startTime aircraft_->demo_tactile_startTime
  bool demo_ice_tail;
  string demo_ice_tail_file;
  double demo_ice_tail_timeArray[10];
  int demo_ice_tail_daArray[10];
  int demo_ice_tail_ntime;
  double demo_ice_tail_startTime;
#define demo_ice_tail           aircraft_->demo_ice_tail
#define demo_ice_tail_file      aircraft_->demo_ice_tail_file
#define demo_ice_tail_timeArray aircraft_->demo_ice_tail_timeArray
#define demo_ice_tail_daArray   aircraft_->demo_ice_tail_daArray
#define demo_ice_tail_ntime     aircraft_->demo_ice_tail_ntime
#define demo_ice_tail_startTime aircraft_->demo_ice_tail_startTime
  bool demo_ice_left;
  string demo_ice_left_file;
  double demo_ice_left_timeArray[10];
  int demo_ice_left_daArray[10];
  int demo_ice_left_ntime;
  double demo_ice_left_startTime;
#define demo_ice_left           aircraft_->demo_ice_left
#define demo_ice_left_file      aircraft_->demo_ice_left_file
#define demo_ice_left_timeArray aircraft_->demo_ice_left_timeArray
#define demo_ice_left_daArray   aircraft_->demo_ice_left_daArray
#define demo_ice_left_ntime     aircraft_->demo_ice_left_ntime
#define demo_ice_left_startTime aircraft_->demo_ice_left_startTime
  bool demo_ice_right;
  string demo_ice_right_file;
  double demo_ice_right_timeArray[10];
  int demo_ice_right_daArray[10];
  int demo_ice_right_ntime;
  double demo_ice_right_startTime;
#define demo_ice_right           aircraft_->demo_ice_right
#define demo_ice_right_file      aircraft_->demo_ice_right_file
#define demo_ice_right_timeArray aircraft_->demo_ice_right_timeArray
#define demo_ice_right_daArray   aircraft_->demo_ice_right_daArray
#define demo_ice_right_ntime     aircraft_->demo_ice_right_ntime
#define demo_ice_right_startTime aircraft_->demo_ice_right_startTime

  //321654
  /* Variables (token2) ===========================================*/
  /* fog =========== Fog field quantities ======================== */

  std::map <string,int> fog_map;
#define fog_map 	aircraft_->fog_map

  bool fog_field;
  int fog_segments;
  int fog_point_index;
  double *fog_time;
  int *fog_value;
  double fog_next_time;
  int fog_current_segment;
 
  int Fog;
  
#define fog_field	aircraft_->fog_field
#define fog_segments	aircraft_->fog_segments
#define fog_point_index	aircraft_->fog_point_index
#define fog_time	aircraft_->fog_time
#define fog_value	aircraft_->fog_value
#define fog_next_time	aircraft_->fog_next_time
#define fog_current_segment	aircraft_->fog_current_segment

#define Fog             aircraft_->Fog

 

  /* Variables (token2) ===========================================*/
  /* record ======== Record desired quantites to file =============*/
  
  std::map <string,int> record_map;
#define      record_map              aircraft_->record_map

  /***** Angles ******/
  double Alpha_deg, Alpha_dot_deg, Beta_deg, Beta_dot_deg;
#define Alpha_deg       aircraft_->Alpha_deg
#define Alpha_dot_deg   aircraft_->Alpha_dot_deg
#define Beta_deg        aircraft_->Beta_deg
#define Beta_dot_deg    aircraft_->Beta_dot_deg
  double Gamma_vert_deg, Gamma_horiz_deg;
#define Gamma_vert_deg  aircraft_->Gamma_vert_deg
#define Gamma_horiz_deg aircraft_->Gamma_horiz_deg

  /** Control Inputs **/
  double Long_trim_deg, elevator_tab, elevator_deg, aileron_deg, rudder_deg;
#define Long_trim_deg   aircraft_->Long_trim_deg
#define elevator_tab    aircraft_->elevator_tab
#define elevator_deg    aircraft_->elevator_deg
#define aileron_deg     aircraft_->aileron_deg
#define rudder_deg      aircraft_->rudder_deg
  //  double flap_pos_deg;
  //#define flap_pos_deg        aircraft_->flap_pos_deg

  /***** Forces ******/
  double F_X_wind, F_Y_wind, F_Z_wind;
#define F_X_wind aircraft_->F_X_wind
#define F_Y_wind aircraft_->F_Y_wind
#define F_Z_wind aircraft_->F_Z_wind
  double Lift_clean_wing, Lift_iced_wing;
  double Lift_clean_tail, Lift_iced_tail;
#define Lift_clean_wing      aircraft_->Lift_clean_wing
#define Lift_iced_wing       aircraft_->Lift_iced_wing
#define Lift_clean_tail      aircraft_->Lift_clean_tail
#define Lift_iced_tail       aircraft_->Lift_iced_tail
  double Gamma_clean_wing, Gamma_iced_wing;
  double Gamma_clean_tail, Gamma_iced_tail;
#define Gamma_clean_wing     aircraft_->Gamma_clean_wing
#define Gamma_iced_wing      aircraft_->Gamma_iced_wing
#define Gamma_clean_tail     aircraft_->Gamma_clean_tail
#define Gamma_iced_tail      aircraft_->Gamma_iced_tail
  double w_clean_wing, w_iced_wing;
  double w_clean_tail, w_iced_tail;
#define w_clean_wing         aircraft_->w_clean_wing
#define w_iced_wing          aircraft_->w_iced_wing
#define w_clean_tail         aircraft_->w_clean_tail
#define w_iced_tail          aircraft_->w_iced_tail
  double V_total_clean_wing, V_total_iced_wing;
  double V_total_clean_tail, V_total_iced_tail;
#define V_total_clean_wing   aircraft_->V_total_clean_wing
#define V_total_iced_wing    aircraft_->V_total_iced_wing
#define V_total_clean_tail   aircraft_->V_total_clean_tail
#define V_total_iced_tail    aircraft_->V_total_iced_tail
  double beta_flow_clean_wing, beta_flow_clean_wing_deg;
  double beta_flow_iced_wing, beta_flow_iced_wing_deg;
  double beta_flow_clean_tail, beta_flow_clean_tail_deg;
  double beta_flow_iced_tail, beta_flow_iced_tail_deg;
#define beta_flow_clean_wing     aircraft_->beta_flow_clean_wing
#define beta_flow_clean_wing_deg aircraft_->beta_flow_clean_wing_deg
#define beta_flow_iced_wing      aircraft_->beta_flow_iced_wing
#define beta_flow_iced_wing_deg  aircraft_->beta_flow_iced_wing_deg
#define beta_flow_clean_tail     aircraft_->beta_flow_clean_tail
#define beta_flow_clean_tail_deg aircraft_->beta_flow_clean_tail_deg
#define beta_flow_iced_tail      aircraft_->beta_flow_iced_tail
#define beta_flow_iced_tail_deg  aircraft_->beta_flow_iced_tail_deg
  double Dbeta_flow_wing, Dbeta_flow_wing_deg;
  double Dbeta_flow_tail, Dbeta_flow_tail_deg;
#define Dbeta_flow_wing      aircraft_->Dbeta_flow_wing
#define Dbeta_flow_wing_deg  aircraft_->Dbeta_flow_wing_deg
#define Dbeta_flow_tail      aircraft_->Dbeta_flow_tail
#define Dbeta_flow_tail_deg  aircraft_->Dbeta_flow_tail_deg
  double pct_beta_flow_wing, pct_beta_flow_tail;
#define pct_beta_flow_wing   aircraft_->pct_beta_flow_wing
#define pct_beta_flow_tail   aircraft_->pct_beta_flow_tail


  /* Variables (token2) ===========================================*/
  /* misc ========== Miscellaneous input commands =================*/

  std::map <string,int> misc_map;
#define      misc_map        aircraft_->misc_map       

  double simpleHingeMomentCoef;
#define simpleHingeMomentCoef    aircraft_->simpleHingeMomentCoef
  //string dfTimefdf;
  //double dfTimefdf_dfArray[100];
  //double dfTimefdf_TimeArray[100];
  //int dfTimefdf_ndf;
  //#define dfTimefdf              aircraft_->dfTimefdf
  //#define dfTimefdf_dfArray      aircraft_->dfTimefdf_dfArray
  //#define dfTimefdf_TimeArray    aircraft_->dfTimefdf_TimeArray
  //#define dfTimefdf_ndf          aircraft_->dfTimefdf_ndf

  FlapData *flapper_data;
#define flapper_data           aircraft_->flapper_data
  bool flapper_model;
#define flapper_model          aircraft_->flapper_model
  double flapper_phi_init;
#define flapper_phi_init       aircraft_->flapper_phi_init
  double flapper_freq, flapper_cycle_pct, flapper_phi;
  double flapper_Lift, flapper_Thrust, flapper_Inertia;
  double flapper_Moment;
#define flapper_freq           aircraft_->flapper_freq
#define flapper_cycle_pct      aircraft_->flapper_cycle_pct
#define flapper_phi            aircraft_->flapper_phi
#define flapper_Lift           aircraft_->flapper_Lift
#define flapper_Thrust         aircraft_->flapper_Thrust
#define flapper_Inertia        aircraft_->flapper_Inertia
#define flapper_Moment         aircraft_->flapper_Moment
  double F_X_aero_flapper, F_Z_aero_flapper;
#define F_X_aero_flapper       aircraft_->F_X_aero_flapper
#define F_Z_aero_flapper       aircraft_->F_Z_aero_flapper

  /* Other variables (not tokens) =================================*/

  double convert_x, convert_y, convert_z;
#define convert_x aircraft_->convert_x
#define convert_y aircraft_->convert_y
#define convert_z aircraft_->convert_z

  double cbar_2U, b_2U, ch_2U;
#define cbar_2U   aircraft_->cbar_2U
#define b_2U      aircraft_->b_2U
#define ch_2U     aircraft_->ch_2U

  int ndf;
  double dfArray[100];
  double TimeArray[100];
#define ndf       aircraft_->ndf
#define dfArray   aircraft_->dfArray
#define TimeArray aircraft_->TimeArray

  double flap_percent, flap_increment_per_timestep, flap_cmd, flap_pos, flap_pos_norm;
#define flap_percent                  aircraft_->flap_percent
#define flap_increment_per_timestep   aircraft_->flap_increment_per_timestep
#define flap_cmd                      aircraft_->flap_cmd
#define flap_pos                      aircraft_->flap_pos
#define flap_pos_norm                 aircraft_->flap_pos_norm

  double Spoiler_handle, spoiler_increment_per_timestep, spoiler_cmd;
  double spoiler_pos_norm, spoiler_pos;
#define Spoiler_handle                 aircraft_->Spoiler_handle
#define spoiler_increment_per_timestep aircraft_->spoiler_increment_per_timestep
#define spoiler_cmd                    aircraft_->spoiler_cmd
#define spoiler_pos_norm               aircraft_->spoiler_pos_norm
#define spoiler_pos                    aircraft_->spoiler_pos

  double Gear_handle, gear_increment_per_timestep, gear_cmd_norm, gear_pos_norm;
#define Gear_handle                   aircraft_->Gear_handle
#define gear_increment_per_timestep   aircraft_->gear_increment_per_timestep
#define gear_cmd_norm                 aircraft_->gear_cmd_norm
#define gear_pos_norm                 aircraft_->gear_pos_norm

  double delta_CL, delta_CD, delta_Cm, delta_Ch, delta_Cl, delta_Cn;
#define delta_CL         aircraft_->delta_CL
#define delta_CD         aircraft_->delta_CD
#define delta_Cm         aircraft_->delta_Cm
#define delta_Ch         aircraft_->delta_Ch
#define delta_Cl         aircraft_->delta_Cl
#define delta_Cn         aircraft_->delta_Cn

  int nonlin_ice_case;
  double eta_wing_left, eta_wing_right, eta_tail;
  double OATemperature_F;
#define nonlin_ice_case  aircraft_->nonlin_ice_case
#define eta_wing_left    aircraft_->eta_wing_left
#define eta_wing_right   aircraft_->eta_wing_right
#define eta_tail         aircraft_->eta_tail
#define OATemperature_F  aircraft_->OATemperature_F
  int boot_cycle_tail, boot_cycle_wing_left, boot_cycle_wing_right;
  int autoIPS_tail, autoIPS_wing_left, autoIPS_wing_right;
  int eps_pitch_input;
  double eps_alpha_max, eps_pitch_max, eps_pitch_min;
  double eps_roll_max, eps_thrust_min, eps_flap_max;
  double eps_airspeed_max, eps_airspeed_min;
#define boot_cycle_tail        aircraft_->boot_cycle_tail 
#define boot_cycle_wing_left   aircraft_->boot_cycle_wing_left
#define boot_cycle_wing_right  aircraft_->boot_cycle_wing_right
#define autoIPS_tail           aircraft_->autoIPS_tail
#define autoIPS_wing_left      aircraft_->autoIPS_wing_left
#define autoIPS_wing_right     aircraft_->autoIPS_wing_right
#define eps_pitch_input        aircraft_->eps_pitch_input
#define eps_alpha_max          aircraft_->eps_alpha_max
#define eps_pitch_max          aircraft_->eps_pitch_max
#define eps_pitch_min          aircraft_->eps_pitch_min
#define eps_roll_max           aircraft_->eps_roll_max
#define eps_thrust_min         aircraft_->eps_thrust_min
#define eps_flap_max           aircraft_->eps_flap_max
#define eps_airspeed_max       aircraft_->eps_airspeed_max
#define eps_airspeed_min       aircraft_->eps_airspeed_min

  double Ch;
#define Ch   aircraft_->Ch;

  double CL_clean, CL_iced;
  double CY_clean, CY_iced;
  double CD_clean, CD_iced;
  double Cm_clean, Cm_iced;
  double Cl_clean, Cl_iced;
  double Cn_clean, Cn_iced;
  double Ch_clean, Ch_iced;
#define CL_clean         aircraft_->CL_clean
#define CY_clean         aircraft_->CY_clean
#define CD_clean         aircraft_->CD_clean
#define Cm_clean         aircraft_->Cm_clean
#define Cl_clean         aircraft_->Cl_clean
#define Cn_clean         aircraft_->Cn_clean
#define Ch_clean         aircraft_->Ch_clean
#define CL_iced          aircraft_->CL_iced
#define CY_iced          aircraft_->CY_iced
#define CD_iced          aircraft_->CD_iced
#define Cm_iced          aircraft_->Cm_iced
#define Cl_iced          aircraft_->Cl_iced
#define Cn_iced          aircraft_->Cn_iced
#define Ch_iced          aircraft_->Ch_iced

  std::ofstream fout;
  
#define fout aircraft_->fout
  
  bool ignore_unknown_keywords;
#define ignore_unknown_keywords           aircraft_->ignore_unknown_keywords
  
  int ap_pah_on;
#define ap_pah_on              aircraft_->ap_pah_on
  double ap_pah_start_time;
#define ap_pah_start_time      aircraft_->ap_pah_start_time
  double ap_Theta_ref_rad;
#define ap_Theta_ref_rad       aircraft_->ap_Theta_ref_rad

  int ap_alh_on;
#define ap_alh_on              aircraft_->ap_alh_on
  double ap_alh_start_time;
#define ap_alh_start_time      aircraft_->ap_alh_start_time
  double ap_alt_ref_ft;
#define ap_alt_ref_ft          aircraft_->ap_alt_ref_ft

  int ap_rah_on;
#define ap_rah_on              aircraft_->ap_rah_on
  double ap_rah_start_time;
#define ap_rah_start_time      aircraft_->ap_rah_start_time
  double ap_Phi_ref_rad;
#define ap_Phi_ref_rad         aircraft_->ap_Phi_ref_rad

  int ap_hh_on;
#define ap_hh_on              aircraft_->ap_hh_on
  double ap_hh_start_time;
#define ap_hh_start_time      aircraft_->ap_hh_start_time
  double ap_Psi_ref_rad;
#define ap_Psi_ref_rad         aircraft_->ap_Psi_ref_rad

  int pitch_trim_up, pitch_trim_down;
#define pitch_trim_up          aircraft_->pitch_trim_up
#define pitch_trim_down        aircraft_->pitch_trim_down

  bool pilot_throttle_no;
#define pilot_throttle_no      aircraft_->pilot_throttle_no

  int ice_tail, ice_left, ice_right;
#define ice_tail               aircraft_->ice_tail
#define ice_left               aircraft_->ice_left
#define ice_right              aircraft_->ice_right

  // Variables for the trigger command
  int trigger_on, trigger_last_time_step, trigger_num;
  int trigger_toggle, trigger_counter;
#define trigger_on             aircraft_->trigger_on
#define trigger_last_time_step aircraft_->trigger_last_time_step
#define trigger_num            aircraft_->trigger_num
#define trigger_toggle         aircraft_->trigger_toggle
#define trigger_counter        aircraft_->trigger_counter

  // temp debug values
  double debug7, debug8, debug9, debug10;
#define debug7                 aircraft_->debug7
#define debug8                 aircraft_->debug8
#define debug9                 aircraft_->debug9
#define debug10                aircraft_->debug10
};

extern AIRCRAFT *aircraft_;    // usually defined in the first program that includes uiuc_aircraft.h

#endif  // endif _AIRCRAFT_H
