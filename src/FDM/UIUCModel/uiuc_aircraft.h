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

#include <map>
#include <iostream>
#include "uiuc_parsefile.h"

typedef stack :: iterator LIST;

/* Add more keywords here if required*/
enum {init_flag = 1000, geometry_flag, controlSurface_flag, controlsMixer_flag, 
      mass_flag, engine_flag, CD_flag, CL_flag, Cm_flag, CY_flag, Cl_flag, 
      Cn_flag, gear_flag, ice_flag, record_flag};

// init ======= Initial values for equation of motion
enum {Dx_pilot_flag = 2000, Dy_pilot_flag, Dz_pilot_flag, 
      Dx_cg_flag, Dy_cg_flag, Dz_cg_flag, 
      V_north_flag, V_east_flag, V_down_flag, 
      P_body_flag, Q_body_flag, R_body_flag, 
      Phi_flag, Theta_flag, Psi_flag};

// geometry === Aircraft-specific geometric quantities
enum {bw_flag = 3000, cbar_flag, Sw_flag};

// controlSurface = Control surface deflections and properties
enum {de_flag = 4000, da_flag, dr_flag};

// controlsMixer == Controls mixer
enum {nomix_flag = 14000};

//mass ======== Aircraft-specific mass properties
enum {Mass_flag = 5000, I_xx_flag, I_yy_flag, I_zz_flag, I_xz_flag};

// engine ===== Propulsion data
enum {simpleSingle_flag = 6000, c172_flag};

// CD ========= Aerodynamic x-force quantities (longitudinal)
enum {CDo_flag = 7000, CDK_flag, CD_a_flag, CD_de_flag, CDfa_flag, CDfade_flag};

// CL ========= Aerodynamic z-force quantities (longitudinal)
enum {CLo_flag = 8000, CL_a_flag, CL_adot_flag, CL_q_flag, CL_de_flag, CLfa_flag, CLfade_flag};

// Cm ========= Aerodynamic m-moment quantities (longitudinal)
enum {Cmo_flag = 9000, Cm_a_flag, Cm_adot_flag, Cm_q_flag, Cm_de_flag, Cmfade_flag};

// CY ========= Aerodynamic y-force quantities (lateral)
enum {CYo_flag = 10000, CY_beta_flag, CY_p_flag, CY_r_flag, CY_da_flag, CY_dr_flag, 
      CYfada_flag, CYfbetadr_flag};

// Cl ========= Aerodynamic l-moment quantities (lateral)
enum {Clo_flag = 11000, Cl_beta_flag, Cl_betafCL_flag, Cl_p_flag, Cl_r_flag, Cl_rfCL_flag, 
      Cl_da_flag, Cl_dr_flag, Clfada_flag, Clfbetadr_flag};

// Cn ========= Aerodynamic n-moment quantities (lateral)
enum {Cno_flag = 12000, Cn_beta_flag, Cn_betafCL_flag, Cn_p_flag, Cn_pfCL_flag, Cn_r_flag, 
      Cn_rfCL_flag, Cn_da_flag, Cn_dr_flag, Cn_drfCL_flag, Cnfada_flag, Cnfbetadr_flag};

// gear ======= Landing gear model quantities

// ice ======== Ice model quantities
enum {iceTime_flag = 15000, transientTime_flag, eta_final_flag, 
      kCDo_flag, kCDK_flag, kCD_a_flag, kCD_de_flag, 
      kCLo_flag, kCL_a_flag, kCL_adot_flag, kCL_q_flag, kCL_de_flag, 
      kCmo_flag, kCm_a_flag, kCm_adot_flag, kCm_q_flag, kCm_de_flag, 
      kCYo_flag, kCY_beta_flag, kCY_p_flag, kCY_r_flag, kCY_da_flag, kCY_dr_flag, 
      kClo_flag, kCl_beta_flag, kCl_p_flag, kCl_r_flag, kCl_da_flag, kCl_dr_flag, 
      kCno_flag, kCn_beta_flag, kCn_p_flag, kCn_r_flag, kCn_da_flag, kCn_dr_flag};

// record ===== Record desired quantites to file

enum {Dx_pilot_record = 13000, Dy_pilot_record, Dz_pilot_record, 
      Dx_cg_record, Dy_cg_record, Dz_cg_record, 
      V_north_record, V_east_record, V_down_record,
      V_rel_wind_record, Dynamic_pressure_record, 
      Alpha_record, Alpha_dot_record, Beta_record, Beta_dot_record, Gamma_record,
      P_body_record, Q_body_record, R_body_record, 
      Phi_record, Theta_record, Psi_record, Theta_dot_record, 
      density_record, Mass_record, Simtime_record, dt_record, 
      elevator_record, aileron_record, rudder_record, 
      CD_record, CDfaI_record, CDfadeI_record, 
      CL_record, CLfaI_record, CLfadeI_record, 
      Cm_record, CmfadeI_record, 
      CY_record, CYfadaI_record, CYfbetadrI_record, 
      Cl_record, ClfadaI_record, ClfbetadrI_record, 
      Cn_record, CnfadaI_record, CnfbetadrI_record,
      F_X_wind_record, F_Y_wind_record, F_Z_wind_record, 
      F_X_aero_record, F_Y_aero_record, F_Z_aero_record,
      F_X_engine_record, F_Y_engine_record, F_Z_engine_record, 
      F_X_gear_record, F_Y_gear_record, F_Z_gear_record, 
      F_X_record, F_Y_record, F_Z_record, 
      M_l_aero_record, M_m_aero_record, M_n_aero_record, 
      M_l_engine_record, M_m_engine_record, M_n_engine_record, 
      M_l_gear_record, M_m_gear_record, M_n_gear_record, 
      M_l_rp_record, M_m_rp_record, M_n_rp_record};

typedef struct
{
  // ParseFile stuff [] Bipin to add more comments
  ParseFile *airplane;
#define  airplane        aircraft_->airplane
  ParseFile *initParts;
#define  initParts       aircraft_->initParts
  ParseFile *geometryParts;
#define  geometryParts   aircraft_->geometryParts
  ParseFile *massParts;
#define  massParts       aircraft_->massParts
  ParseFile *aeroParts;
#define  aeroParts       aircraft_->aeroParts
  ParseFile *engineParts;
#define  engineParts     aircraft_->engineParts
  ParseFile *gearParts;
#define  gearParts       aircraft_->gearParts
  ParseFile *recordParts;
#define  recordParts     aircraft_->recordParts
  
  /*= Keywords (token1) ===========================================*/
  map <string,int>      Keyword_map;
#define      Keyword_map         aircraft_->Keyword_map       

  double CL;
  double CD;
  double Cm;
  double CY;
  double Cl;
  double Cn;

#define CL  aircraft_->CL
#define CD  aircraft_->CD
#define Cm  aircraft_->Cm
#define CY  aircraft_->CY
#define Cl  aircraft_->Cl
#define Cn  aircraft_->Cn
  
  /*========================================*/
  /* Variables (token2) - 14 groups (000210)*/
  /*========================================*/
  
  /* Variables (token2) ===========================================*/
  /* init ========== Initial values for equations of motion =======*/
  
  map <string,int> init_map;
#define      init_map            aircraft_->init_map          
  
  /* Variables (token2) ===========================================*/
  /* geometry ====== Aircraft-specific geometric quantities =======*/
  
  map <string,int> geometry_map;
#define      geometry_map        aircraft_->geometry_map       
  
  double bw;
  double cbar;
  double Sw;
#define bw   aircraft_->bw
#define cbar aircraft_->cbar
#define Sw   aircraft_->Sw       
  
  /* Variables (token2) ===========================================*/
  /* controlSurface  Control surface deflections and properties ===*/
  
  map <string,int> controlSurface_map;
#define      controlSurface_map  aircraft_->controlSurface_map
  
  double demax;
  double demin;
  double damax;
  double damin;
  double drmax;
  double drmin;
#define demax  aircraft_->demax
#define demin  aircraft_->demin
#define damax  aircraft_->damax
#define damin  aircraft_->damin
#define drmax  aircraft_->drmax
#define drmin  aircraft_->drmin

  double aileron;
  double elevator;
  double rudder;
#define aileron   aircraft_->aileron
#define elevator  aircraft_->elevator
#define rudder    aircraft_->rudder

  
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
  
  
  /* Variables (token2) ===========================================*/
  /* engine ======== Propulsion data ==============================*/
  
  map <string,int> engine_map;
#define      engine_map            aircraft_->engine_map          
  
  double simpleSingleMaxThrust;
#define simpleSingleMaxThrust  aircraft_->simpleSingleMaxThrust
  
  /* Variables (token2) ===========================================*/
  /* CD ============ Aerodynamic x-force quantities (longitudinal) */
  
  map <string,int> CD_map;
#define      CD_map              aircraft_->CD_map            
  
  double CDo;
  double CDK;
  double CD_a;
  double CD_de;
#define CDo      aircraft_->CDo
#define CDK      aircraft_->CDK
#define CD_a     aircraft_->CD_a
#define CD_de    aircraft_->CD_de
  string CDfa;
  int CDfaData;
  double CDfa_aArray[100];
  double CDfa_CDArray[100];
  int CDfa_nAlpha;
  double CDfaI;
#define CDfa               aircraft_->CDfa
#define CDfaData           aircraft_->CDfaData
#define CDfa_aArray        aircraft_->CDfa_aArray
#define CDfa_CDArray       aircraft_->CDfa_CDArray
#define CDfa_nAlpha        aircraft_->CDfa_nAlpha
#define CDfaI              aircraft_->CDfaI
  string CDfade;
  int CDfadeData;
  double CDfade_aArray[100][100];
  double CDfade_deArray[100];
  double CDfade_CDArray[100][100];
  int CDfade_nAlphaArray[100];
  int CDfade_nde;
  double CDfadeI;
#define CDfade             aircraft_->CDfade
#define CDfadeData         aircraft_->CDfadeData
#define CDfade_aArray      aircraft_->CDfade_aArray
#define CDfade_deArray     aircraft_->CDfade_deArray
#define CDfade_CDArray     aircraft_->CDfade_CDArray
#define CDfade_nAlphaArray aircraft_->CDfade_nAlphaArray
#define CDfade_nde         aircraft_->CDfade_nde
#define CDfadeI            aircraft_->CDfadeI
  
  /* Variables (token2) ===========================================*/
  /* CL ============ Aerodynamic z-force quantities (longitudinal) */
  
  map <string,int> CL_map;
#define      CL_map              aircraft_->CL_map            
  
  double CLo;
  double CL_a;
  double CL_adot;
  double CL_q;
  double CL_de;
#define CLo      aircraft_->CLo
#define CL_a     aircraft_->CL_a
#define CL_adot  aircraft_->CL_adot
#define CL_q     aircraft_->CL_q
#define CL_de    aircraft_->CL_de
  string CLfa;
  int CLfaData;
  double CLfa_aArray[100];
  double CLfa_CLArray[100];
  int CLfa_nAlpha;
  double CLfaI;
#define CLfa               aircraft_->CLfa
#define CLfaData           aircraft_->CLfaData
#define CLfa_aArray        aircraft_->CLfa_aArray
#define CLfa_CLArray       aircraft_->CLfa_CLArray
#define CLfa_nAlpha        aircraft_->CLfa_nAlpha
#define CLfaI              aircraft_->CLfaI
  string CLfade;
  int CLfadeData;
  double CLfade_aArray[100][100];
  double CLfade_deArray[100];
  double CLfade_CLArray[100][100];
  int CLfade_nAlphaArray[100];
  int CLfade_nde;
  double CLfadeI;
#define CLfade             aircraft_->CLfade
#define CLfadeData         aircraft_->CLfadeData
#define CLfade_aArray      aircraft_->CLfade_aArray
#define CLfade_deArray     aircraft_->CLfade_deArray
#define CLfade_CLArray     aircraft_->CLfade_CLArray
#define CLfade_nAlphaArray aircraft_->CLfade_nAlphaArray
#define CLfade_nde         aircraft_->CLfade_nde
#define CLfadeI            aircraft_->CLfadeI

  /* Variables (token2) ===========================================*/
  /* Cm ============ Aerodynamic m-moment quantities (longitudinal) */
  
  map <string,int> Cm_map;
#define      Cm_map              aircraft_->Cm_map            
  
  double Cmo;
  double Cm_a;
  double Cm_adot;
  double Cm_q;
  double Cm_de;
#define Cmo      aircraft_->Cmo
#define Cm_a     aircraft_->Cm_a
#define Cm_adot  aircraft_->Cm_adot
#define Cm_q     aircraft_->Cm_q
#define Cm_de    aircraft_->Cm_de
  string Cmfade;
  int CmfadeData;
  double Cmfade_aArray[100][100];
  double Cmfade_deArray[100];
  double Cmfade_CmArray[100][100];
  int Cmfade_nAlphaArray[100];
  int Cmfade_nde;
  double CmfadeI;
#define Cmfade             aircraft_->Cmfade
#define CmfadeData         aircraft_->CmfadeData
#define Cmfade_aArray      aircraft_->Cmfade_aArray
#define Cmfade_deArray     aircraft_->Cmfade_deArray
#define Cmfade_CmArray     aircraft_->Cmfade_CmArray
#define Cmfade_nAlphaArray aircraft_->Cmfade_nAlphaArray
#define Cmfade_nde         aircraft_->Cmfade_nde
#define CmfadeI            aircraft_->CmfadeI
  
  /* Variables (token2) ===========================================*/
  /* CY ============ Aerodynamic y-force quantities (lateral) =====*/
  
  map <string,int> CY_map;
#define      CY_map              aircraft_->CY_map            
  
  double CYo;
  double CY_beta;
  double CY_p;
  double CY_r;
  double CY_da;
  double CY_dr;
#define CYo      aircraft_->CYo
#define CY_beta  aircraft_->CY_beta
#define CY_p     aircraft_->CY_p
#define CY_r     aircraft_->CY_r
#define CY_da    aircraft_->CY_da
#define CY_dr    aircraft_->CY_dr
  string CYfada;
  int CYfadaData;
  double CYfada_aArray[100][100];
  double CYfada_daArray[100];
  double CYfada_CYArray[100][100];
  int CYfada_nAlphaArray[100];
  int CYfada_nda;
  double CYfadaI;
#define CYfada             aircraft_->CYfada
#define CYfadaData         aircraft_->CYfadaData
#define CYfada_aArray      aircraft_->CYfada_aArray
#define CYfada_daArray     aircraft_->CYfada_daArray
#define CYfada_CYArray     aircraft_->CYfada_CYArray
#define CYfada_nAlphaArray aircraft_->CYfada_nAlphaArray
#define CYfada_nda         aircraft_->CYfada_nda
#define CYfadaI            aircraft_->CYfadaI
  string CYfbetadr;
  int CYfbetadrData;
  double CYfbetadr_betaArray[100][100];
  double CYfbetadr_drArray[100];
  double CYfbetadr_CYArray[100][100];
  int CYfbetadr_nBetaArray[100];
  int CYfbetadr_ndr;
  double CYfbetadrI;
#define CYfbetadr             aircraft_->CYfbetadr
#define CYfbetadrData         aircraft_->CYfbetadrData
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
  
  double Clo;
  double Cl_beta;
  double Cl_betafCL;
  double Cl_p;
  double Cl_r;
  double Cl_rfCL;
  double Cl_da;
  double Cl_dr;
#define Clo      aircraft_->Clo
#define Cl_beta  aircraft_->Cl_beta
#define Cl_betafCL aircraft_->Cl_betafCL
#define Cl_p     aircraft_->Cl_p
#define Cl_r     aircraft_->Cl_r
#define Cl_rfCL  aircraft_->Cl_rfCL
#define Cl_da    aircraft_->Cl_da
#define Cl_dr    aircraft_->Cl_dr
  string Clfada;
  int ClfadaData;
  double Clfada_aArray[100][100];
  double Clfada_daArray[100];
  double Clfada_ClArray[100][100];
  int Clfada_nAlphaArray[100];
  int Clfada_nda;
  double ClfadaI;
#define Clfada             aircraft_->Clfada
#define ClfadaData         aircraft_->ClfadaData
#define Clfada_aArray      aircraft_->Clfada_aArray
#define Clfada_daArray     aircraft_->Clfada_daArray
#define Clfada_ClArray     aircraft_->Clfada_ClArray
#define Clfada_nAlphaArray aircraft_->Clfada_nAlphaArray
#define Clfada_nda         aircraft_->Clfada_nda
#define ClfadaI            aircraft_->ClfadaI
  string Clfbetadr;
  int ClfbetadrData;
  double Clfbetadr_betaArray[100][100];
  double Clfbetadr_drArray[100];
  double Clfbetadr_ClArray[100][100];
  int Clfbetadr_nBetaArray[100];
  int Clfbetadr_ndr;
  double ClfbetadrI;
#define Clfbetadr             aircraft_->Clfbetadr
#define ClfbetadrData         aircraft_->ClfbetadrData
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

  double Cno;
  double Cn_beta;
  double Cn_p;
  double Cn_r;
  double Cn_da;
  double Cn_dr;
#define Cno      aircraft_->Cno
#define Cn_beta  aircraft_->Cn_beta
#define Cn_p     aircraft_->Cn_p
#define Cn_r     aircraft_->Cn_r
#define Cn_da    aircraft_->Cn_da
#define Cn_dr    aircraft_->Cn_dr
  string Cnfada;
  int CnfadaData;
  double Cnfada_aArray[100][100];
  double Cnfada_daArray[100];
  double Cnfada_CnArray[100][100];
  int Cnfada_nAlphaArray[100];
  int Cnfada_nda;
  double CnfadaI;
#define Cnfada             aircraft_->Cnfada
#define CnfadaData         aircraft_->CnfadaData
#define Cnfada_aArray      aircraft_->Cnfada_aArray
#define Cnfada_daArray     aircraft_->Cnfada_daArray
#define Cnfada_CnArray     aircraft_->Cnfada_CnArray
#define Cnfada_nAlphaArray aircraft_->Cnfada_nAlphaArray
#define Cnfada_nda         aircraft_->Cnfada_nda
#define CnfadaI            aircraft_->CnfadaI
  string Cnfbetadr;
  int CnfbetadrData;
  double Cnfbetadr_betaArray[100][100];
  double Cnfbetadr_drArray[100];
  double Cnfbetadr_CnArray[100][100];
  int Cnfbetadr_nBetaArray[100];
  int Cnfbetadr_ndr;
  double CnfbetadrI;
#define Cnfbetadr             aircraft_->Cnfbetadr
#define CnfbetadrData         aircraft_->CnfbetadrData
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
  
  /* Variables (token2) ===========================================*/
  /* ice =========== Ice model quantities ======================== */
  
  map <string,int> ice_map;
#define      ice_map              aircraft_->ice_map            

  double iceTime;
  double transientTime;
  double eta_final;
  double eta;
#define iceTime        aircraft_->iceTime
#define transientTime  aircraft_->transientTime
#define eta_final      aircraft_->eta_final
#define eta            aircraft_->eta
  double kCDo;
  double kCDK;
  double kCD_a;
  double kCD_de;
  double CDo_clean;
  double CDK_clean;
  double CD_a_clean;
  double CD_de_clean;
#define kCDo           aircraft_->kCDo
#define kCDK           aircraft_->kCDK
#define kCD_a          aircraft_->kCD_a
#define kCD_de         aircraft_->kCD_de
#define CDo_clean      aircraft_->CDo_clean
#define CDK_clean      aircraft_->CDK_clean
#define CD_a_clean     aircraft_->CD_a_clean
#define CD_de_clean    aircraft_->CD_de_clean
  double kCLo;
  double kCL_a;
  double kCL_adot;
  double kCL_q;
  double kCL_de;
  double CLo_clean;
  double CL_a_clean;
  double CL_adot_clean;
  double CL_q_clean;
  double CL_de_clean;
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
  double kCmo;
  double kCm_a;
  double kCm_adot;
  double kCm_q;
  double kCm_de;
  double Cmo_clean;
  double Cm_a_clean;
  double Cm_adot_clean;
  double Cm_q_clean;
  double Cm_de_clean;
#define kCmo           aircraft_->kCmo
#define kCm_a          aircraft_->kCm_a
#define kCm_adot       aircraft_->kCm_adot
#define kCm_q          aircraft_->kCm_q
#define kCm_de         aircraft_->kCm_de
#define Cmo_clean      aircraft_->Cmo_clean
#define Cm_a_clean     aircraft_->Cm_a_clean
#define Cm_adot_clean  aircraft_->Cm_adot_clean
#define Cm_q_clean     aircraft_->Cm_q_clean
#define Cm_de_clean    aircraft_->Cm_de_clean
  double kCYo;
  double kCY_beta;
  double kCY_p;
  double kCY_r;
  double kCY_da;
  double kCY_dr;
  double CYo_clean;
  double CY_beta_clean;
  double CY_p_clean;
  double CY_r_clean;
  double CY_da_clean;
  double CY_dr_clean;
#define kCYo           aircraft_->kCYo
#define kCY_beta       aircraft_->kCY_beta
#define kCY_p          aircraft_->kCY_p
#define kCY_r          aircraft_->kCY_r
#define kCY_da         aircraft_->kCY_da
#define kCY_dr         aircraft_->kCY_dr
#define CYo_clean      aircraft_->CYo_clean
#define CY_beta_clean  aircraft_->CY_beta_clean
#define CY_p_clean     aircraft_->CY_p_clean
#define CY_r_clean     aircraft_->CY_r_clean
#define CY_da_clean    aircraft_->CY_da_clean
#define CY_dr_clean    aircraft_->CY_dr_clean
  double kClo;
  double kCl_beta;
  double kCl_p;
  double kCl_r;
  double kCl_da;
  double kCl_dr;
  double Clo_clean;
  double Cl_beta_clean;
  double Cl_p_clean;
  double Cl_r_clean;
  double Cl_da_clean;
  double Cl_dr_clean;
#define kClo           aircraft_->kClo
#define kCl_beta       aircraft_->kCl_beta
#define kCl_p          aircraft_->kCl_p
#define kCl_r          aircraft_->kCl_r
#define kCl_da         aircraft_->kCl_da
#define kCl_dr         aircraft_->kCl_dr
#define Clo_clean      aircraft_->Clo_clean
#define Cl_beta_clean  aircraft_->Cl_beta_clean
#define Cl_p_clean     aircraft_->Cl_p_clean
#define Cl_r_clean     aircraft_->Cl_r_clean
#define Cl_da_clean    aircraft_->Cl_da_clean
#define Cl_dr_clean    aircraft_->Cl_dr_clean
  double kCno;
  double kCn_beta;
  double kCn_p;
  double kCn_r;
  double kCn_da;
  double kCn_dr;
  double Cno_clean;
  double Cn_beta_clean;
  double Cn_p_clean;
  double Cn_r_clean;
  double Cn_da_clean;
  double Cn_dr_clean;
#define kCno           aircraft_->kCno
#define kCn_beta       aircraft_->kCn_beta
#define kCn_p          aircraft_->kCn_p
#define kCn_r          aircraft_->kCn_r
#define kCn_da         aircraft_->kCn_da
#define kCn_dr         aircraft_->kCn_dr
#define Cno_clean      aircraft_->Cno_clean
#define Cn_beta_clean  aircraft_->Cn_beta_clean
#define Cn_p_clean     aircraft_->Cn_p_clean
#define Cn_r_clean     aircraft_->Cn_r_clean
#define Cn_da_clean    aircraft_->Cn_da_clean
#define Cn_dr_clean    aircraft_->Cn_dr_clean

  /* Variables (token2) ===========================================*/
  /* record ======== Record desired quantites to file =============*/
  
  map <string,int> record_map;
#define      record_map              aircraft_->record_map

  /***** Forces *******/

  double F_X_wind, F_Y_wind, F_Z_wind;

#define F_X_wind aircraft_->F_X_wind
#define F_Y_wind aircraft_->F_Y_wind
#define F_Z_wind aircraft_->F_Z_wind


  /* Miscellaneous ================================================*/

  int conversion1, conversion2, conversion3;
  double confac1, confac2, confac3;

#define conversion1 aircraft_->conversion1
#define conversion2 aircraft_->conversion2
#define conversion3 aircraft_->conversion3
#define confac1 aircraft_->confac1
#define confac2 aircraft_->confac2
#define confac3 aircraft_->confac3


  ofstream fout;
  
#define fout aircraft_->fout
  
  
} AIRCRAFT;

// usually defined in the first program that includes uiuc_aircraft.h
extern AIRCRAFT *aircraft_;

#endif  // endif _AIRCRAFT_H
