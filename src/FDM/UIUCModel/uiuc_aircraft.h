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
----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
	       Robert Deters      <rdeters@uiuc.edu>
               David Megginson    <david@megginson.com>

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.

**********************************************************************/


#ifndef _AIRCRAFT_H_
#define _AIRCRAFT_H_

#include <simgear/compiler.h>

#include <FDM/LaRCsim/ls_types.h>

#include <map>
#include STL_IOSTREAM
#include <math.h>

#include "uiuc_parsefile.h"

SG_USING_STD(map);
#if !defined (SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(iostream);
SG_USING_STD(ofstream);
#endif


typedef stack :: iterator LIST;

/* Add more keywords here if required*/
enum {init_flag = 1000, geometry_flag, controlSurface_flag, controlsMixer_flag, 
      mass_flag, engine_flag, CD_flag, CL_flag, Cm_flag, CY_flag, Cl_flag, 
      Cn_flag, gear_flag, ice_flag, record_flag, misc_flag, fog_flag};

// init ======= Initial values for equation of motion
enum {Dx_pilot_flag = 2000, Dy_pilot_flag, Dz_pilot_flag, 
      Dx_cg_flag, Dy_cg_flag, Dz_cg_flag, Altitude_flag,
      V_north_flag, V_east_flag, V_down_flag, 
      P_body_flag, Q_body_flag, R_body_flag, 
      Phi_flag, Theta_flag, Psi_flag,
      Long_trim_flag, recordRate_flag, recordStartTime_flag, 
      nondim_rate_V_rel_wind_flag, dyn_on_speed_flag, Alpha_flag, 
      Beta_flag, U_body_flag, V_body_flag, W_body_flag};

// geometry === Aircraft-specific geometric quantities
enum {bw_flag = 3000, cbar_flag, Sw_flag, ih_flag, bh_flag, ch_flag, Sh_flag};

// controlSurface = Control surface deflections and properties
enum {de_flag = 4000, da_flag, dr_flag, 
      set_Long_trim_flag, set_Long_trim_deg_flag, zero_Long_trim_flag, 
      elevator_step_flag, elevator_singlet_flag, elevator_doublet_flag, elevator_input_flag, aileron_input_flag, rudder_input_flag, pilot_elev_no_flag, pilot_ail_no_flag, pilot_rud_no_flag};

// controlsMixer == Controls mixer
enum {nomix_flag = 5000};

//mass ======== Aircraft-specific mass properties
enum {Weight_flag = 6000, Mass_flag, I_xx_flag, I_yy_flag, I_zz_flag, I_xz_flag};

// engine ===== Propulsion data
enum {simpleSingle_flag = 7000, c172_flag, cherokee_flag, Throttle_pct_input_flag};

// CD ========= Aerodynamic x-force quantities (longitudinal)
enum {CDo_flag = 8000, CDK_flag, CD_a_flag, CD_adot_flag, CD_q_flag, CD_ih_flag, CD_de_flag, 
      CDfa_flag, CDfCL_flag, CDfade_flag, CDfdf_flag, CDfadf_flag, 
      CXo_flag, CXK_flag, CX_a_flag, CX_a2_flag, CX_a3_flag, CX_adot_flag, 
      CX_q_flag, CX_de_flag, CX_dr_flag, CX_df_flag, CX_adf_flag};

// CL ========= Aerodynamic z-force quantities (longitudinal)
enum {CLo_flag = 9000, CL_a_flag, CL_adot_flag, CL_q_flag, CL_ih_flag, CL_de_flag, 
      CLfa_flag, CLfade_flag, CLfdf_flag, CLfadf_flag, 
      CZo_flag, CZ_a_flag, CZ_a2_flag, CZ_a3_flag, CZ_adot_flag, 
      CZ_q_flag, CZ_de_flag, CZ_deb2_flag, CZ_df_flag, CZ_adf_flag, CZfa_flag};

// Cm ========= Aerodynamic m-moment quantities (longitudinal)
enum {Cmo_flag = 10000, Cm_a_flag, Cm_a2_flag, Cm_adot_flag, Cm_q_flag, 
      Cm_ih_flag, Cm_de_flag, Cm_b2_flag, Cm_r_flag, Cm_df_flag, 
      Cmfa_flag, Cmfade_flag, Cmfdf_flag, Cmfadf_flag};

// CY ========= Aerodynamic y-force quantities (lateral)
enum {CYo_flag = 11000, CY_beta_flag, CY_p_flag, CY_r_flag, CY_da_flag, 
      CY_dr_flag, CY_dra_flag, CY_bdot_flag, CYfada_flag, CYfbetadr_flag};

// Cl ========= Aerodynamic l-moment quantities (lateral)
enum {Clo_flag = 12000, Cl_beta_flag, Cl_p_flag, Cl_r_flag, Cl_da_flag, 
      Cl_dr_flag, Cl_daa_flag, Clfada_flag, Clfbetadr_flag};

// Cn ========= Aerodynamic n-moment quantities (lateral)
enum {Cno_flag = 13000, Cn_beta_flag, Cn_p_flag, Cn_r_flag, Cn_da_flag, 
      Cn_dr_flag, Cn_q_flag, Cn_b3_flag, Cnfada_flag, Cnfbetadr_flag};

// gear ======= Landing gear model quantities
enum {Dx_gear_flag = 14000, Dy_gear_flag, Dz_gear_flag, cgear_flag,
      kgear_flag, muGear_flag, strutLength_flag};

// ice ======== Ice model quantities
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
      kCn_dr_flag, kCn_q_flag, kCn_b3_flag};

// record ===== Record desired quantites to file
enum {Simtime_record = 16000, dt_record, 

      Weight_record, Mass_record, I_xx_record, I_yy_record, I_zz_record, I_xz_record, 

      Dx_pilot_record, Dy_pilot_record, Dz_pilot_record, 
      Dx_cg_record, Dy_cg_record, Dz_cg_record,
      Lat_geocentric_record, Lon_geocentric_record, Radius_to_vehicle_record, 
      Latitude_record, Longitude_record, Altitude_record, 
      Phi_record, Theta_record, Psi_record, 

      V_dot_north_record, V_dot_east_record, V_dot_down_record, 
      U_dot_body_record, V_dot_body_record, W_dot_body_record, 
      A_X_pilot_record, A_Y_pilot_record, A_Z_pilot_record, 
      A_X_cg_record, A_Y_cg_record, A_Z_cg_record, 
      N_X_pilot_record, N_Y_pilot_record, N_Z_pilot_record, 
      N_X_cg_record, N_Y_cg_record, N_Z_cg_record, 
      P_dot_body_record, Q_dot_body_record, R_dot_body_record, 

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

      Alpha_record, Alpha_deg_record, Alpha_dot_record, Alpha_dot_deg_record, 
      Beta_record, Beta_deg_record, Beta_dot_record, Beta_dot_deg_record, 
      Gamma_vert_record, Gamma_vert_deg_record, Gamma_horiz_record, Gamma_horiz_deg_record,

      Density_record, V_sound_record, Mach_number_record, 
      Static_pressure_record, Total_pressure_record, Impact_pressure_record, 
      Dynamic_pressure_record, 
      Static_temperature_record, Total_temperature_record, 

      Gravity_record, Sea_level_radius_record, Earth_position_angle_record, 
      Runway_altitude_record, Runway_latitude_record, Runway_longitude_record, 
      Runway_heading_record, Radius_to_rwy_record, 
      D_pilot_north_of_rwy_record, D_pilot_east_of_rwy_record, D_pilot_above_rwy_record, 
      X_pilot_rwy_record, Y_pilot_rwy_record, H_pilot_rwy_record, 
      D_cg_north_of_rwy_record, D_cg_east_of_rwy_record, D_cg_above_rwy_record, 
      X_cg_rwy_record, Y_cg_rwy_record, H_cg_rwy_record, 

      Throttle_3_record, Throttle_pct_record, 

      Long_control_record, Long_trim_record, Long_trim_deg_record, 
      elevator_record, elevator_deg_record, 
      Lat_control_record, aileron_record, aileron_deg_record, 
      Rudder_pedal_record, rudder_record, rudder_deg_record, 
      Flap_handle_record, flap_record, flap_deg_record, 

      CD_record, CDfaI_record, CDfCLI_record, CDfadeI_record, CDfdfI_record, CDfadfI_record, CX_record,
      CL_record, CLfaI_record, CLfadeI_record, CLfdfI_record, CLfadfI_record, CZ_record,
      Cm_record, CmfaI_record, CmfadeI_record, CmfdfI_record, CmfadfI_record,
      CY_record, CYfadaI_record, CYfbetadrI_record, 
      Cl_record, ClfadaI_record, ClfbetadrI_record, 
      Cn_record, CnfadaI_record, CnfbetadrI_record,

      F_X_wind_record, F_Y_wind_record, F_Z_wind_record, 
      F_X_aero_record, F_Y_aero_record, F_Z_aero_record,
      F_X_engine_record, F_Y_engine_record, F_Z_engine_record, 
      F_X_gear_record, F_Y_gear_record, F_Z_gear_record, 
      F_X_record, F_Y_record, F_Z_record, 
      F_north_record, F_east_record, F_down_record, 

      M_l_aero_record, M_m_aero_record, M_n_aero_record, 
      M_l_engine_record, M_m_engine_record, M_n_engine_record, 
      M_l_gear_record, M_m_gear_record, M_n_gear_record, 
      M_l_rp_record, M_m_rp_record, M_n_rp_record,

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
      pct_beta_flow_wing_record, pct_beta_flow_tail_record};

// misc ======= Miscellaneous inputs
enum {simpleHingeMomentCoef_flag = 17000, dfTimefdf_flag};

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
  map <string,int>      Keyword_map;
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

  map <string,int> init_map;
#define      init_map            aircraft_->init_map          

  int recordRate;
#define recordRate             aircraft_->recordRate
  double recordStartTime;
#define recordStartTime        aircraft_->recordStartTime
  bool nondim_rate_V_rel_wind;
#define nondim_rate_V_rel_wind aircraft_->nondim_rate_V_rel_wind
  double dyn_on_speed;
#define dyn_on_speed           aircraft_->dyn_on_speed
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


  /* Variables (token2) ===========================================*/
  /* geometry ====== Aircraft-specific geometric quantities =======*/
  
  map <string,int> geometry_map;
#define      geometry_map        aircraft_->geometry_map       
  
  double bw, cbar, Sw, ih, bh, ch, Sh;
#define bw   aircraft_->bw
#define cbar aircraft_->cbar
#define Sw   aircraft_->Sw       
#define ih   aircraft_->ih
#define bh   aircraft_->bh
#define ch   aircraft_->ch
#define Sh   aircraft_->Sh

  
  /* Variables (token2) ===========================================*/
  /* controlSurface  Control surface deflections and properties ===*/
  
  map <string,int> controlSurface_map;
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
  double flap;
#define flap              aircraft_->flap

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
  double elevator_input_timeArray[1000];
  double elevator_input_deArray[1000];
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
  double aileron_input_timeArray[1000];
  double aileron_input_daArray[1000];
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
  double rudder_input_timeArray[1000];
  double rudder_input_drArray[1000];
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

  
  /* Variables (token2) ===========================================*/
  /* controlsMixer = Control mixer ================================*/
  
  map <string,int> controlsMixer_map;
#define      controlsMixer_map  aircraft_->controlsMixer_map
  
  double nomix;
#define nomix  aircraft_->nomix

  
  /* Variables (token2) ===========================================*/
  /* mass =========== Aircraft-specific mass properties ===========*/
  
  map <string,int> mass_map;
#define      mass_map            aircraft_->mass_map

  double Weight;
#define Weight  aircraft_->Weight


  /* Variables (token2) ===========================================*/
  /* engine ======== Propulsion data ==============================*/
  
  map <string,int> engine_map;
#define      engine_map            aircraft_->engine_map          
  
  double simpleSingleMaxThrust;
#define simpleSingleMaxThrust  aircraft_->simpleSingleMaxThrust
  
  bool Throttle_pct_input;
  string Throttle_pct_input_file;
  double Throttle_pct_input_timeArray[1000];
  double Throttle_pct_input_dTArray[1000];
  int Throttle_pct_input_ntime;
  double Throttle_pct_input_startTime;
#define Throttle_pct_input            aircraft_->Throttle_pct_input
#define Throttle_pct_input_file       aircraft_->Throttle_pct_input_file
#define Throttle_pct_input_timeArray  aircraft_->Throttle_pct_input_timeArray
#define Throttle_pct_input_dTArray    aircraft_->Throttle_pct_input_dTArray
#define Throttle_pct_input_ntime      aircraft_->Throttle_pct_input_ntime
#define Throttle_pct_input_startTime  aircraft_->Throttle_pct_input_startTime


  /* Variables (token2) ===========================================*/
  /* CD ============ Aerodynamic x-force quantities (longitudinal) */
  
  map <string,int> CD_map;
#define      CD_map              aircraft_->CD_map            
  
  double CDo, CDK, CD_a, CD_adot, CD_q, CD_ih, CD_de;
#define CDo      aircraft_->CDo
#define CDK      aircraft_->CDK
#define CD_a     aircraft_->CD_a
#define CD_adot  aircraft_->CD_adot
#define CD_q     aircraft_->CD_q
#define CD_ih    aircraft_->CD_ih
#define CD_de    aircraft_->CD_de
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
  

  /* Variables (token2) ===========================================*/
  /* CL ============ Aerodynamic z-force quantities (longitudinal) */
  
  map <string,int> CL_map;
#define      CL_map              aircraft_->CL_map            
  
  double CLo, CL_a, CL_adot, CL_q, CL_ih, CL_de;
#define CLo      aircraft_->CLo
#define CL_a     aircraft_->CL_a
#define CL_adot  aircraft_->CL_adot
#define CL_q     aircraft_->CL_q
#define CL_ih    aircraft_->CL_ih
#define CL_de    aircraft_->CL_de
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


  /* Variables (token2) ===========================================*/
  /* Cm ============ Aerodynamic m-moment quantities (longitudinal) */
  
  map <string,int> Cm_map;
#define      Cm_map              aircraft_->Cm_map            
  
  double Cmo, Cm_a, Cm_a2, Cm_adot, Cm_q;
  double Cm_ih, Cm_de, Cm_b2, Cm_r, Cm_df;
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
  

  /* Variables (token2) ===========================================*/
  /* CY ============ Aerodynamic y-force quantities (lateral) =====*/
  
  map <string,int> CY_map;
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


  /* Variables (token2) ===========================================*/
  /* Cl ============ Aerodynamic l-moment quantities (lateral) ====*/
  
  map <string,int> Cl_map;
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
  

  /* Variables (token2) ===========================================*/
  /* Cn ============ Aerodynamic n-moment quantities (lateral) ====*/
  
  map <string,int> Cn_map;
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
  

  /* Variables (token2) ===========================================*/
  /* gear ========== Landing gear model quantities ================*/
  
  map <string,int> gear_map;
  
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
  

  /* Variables (token2) ===========================================*/
  /* ice =========== Ice model quantities ======================== */
  
  map <string,int> ice_map;
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


  //321654
  /* Variables (token2) ===========================================*/
  /* fog =========== Fog field quantities ======================== */

  map <string,int> fog_map;
#define fog_map 	aircraft_->fog_map

  bool fog_field;
  int fog_segments;
  int fog_point_index;
  double *fog_time;
  int *fog_value;
  double fog_next_time;
  int fog_current_segment;
 
  int Fog;
  
  AIRCRAFT()
  {
    fog_field = false;
    fog_segments = 0;
    fog_point_index = -1;
    fog_time = NULL;
    fog_value = NULL;
    fog_next_time = 0.0;
    fog_current_segment = 0;
    Fog = 0;
  };

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
  
  map <string,int> record_map;
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
  double flap_deg;
#define flap_deg        aircraft_->flap_deg

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

  map <string,int> misc_map;
#define      misc_map        aircraft_->misc_map       

  double simpleHingeMomentCoef;
#define simpleHingeMomentCoef    aircraft_->simpleHingeMomentCoef
  string dfTimefdf;
  double dfTimefdf_dfArray[100];
  double dfTimefdf_TimeArray[100];
  int dfTimefdf_ndf;
#define dfTimefdf              aircraft_->dfTimefdf
#define dfTimefdf_dfArray      aircraft_->dfTimefdf_dfArray
#define dfTimefdf_TimeArray    aircraft_->dfTimefdf_TimeArray
#define dfTimefdf_ndf          aircraft_->dfTimefdf_ndf


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


  ofstream fout;
  
#define fout aircraft_->fout
  
  
};

extern AIRCRAFT *aircraft_;    // usually defined in the first program that includes uiuc_aircraft.h

#endif  // endif _AIRCRAFT_H
