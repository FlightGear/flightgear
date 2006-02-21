/********************************************************************** 
 
 FILENAME:     uiuc_map_record5.cpp

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the record maps for forces and moments

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include "uiuc_map_record5.h"


void uiuc_map_record5()
{
  /************************ Forces ***********************/
  // aero forces in local axis
  record_map["F_X_wind"]          =      F_X_wind_record            ;
  record_map["F_Y_wind"]          =      F_Y_wind_record            ;
  record_map["F_Z_wind"]          =      F_Z_wind_record            ;
  
  // aero forces in body axis
  record_map["F_X_aero"]          =      F_X_aero_record            ;
  record_map["F_Y_aero"]          =      F_Y_aero_record            ;
  record_map["F_Z_aero"]          =      F_Z_aero_record            ;

  // engine forces
  record_map["F_X_engine"]        =      F_X_engine_record          ;
  record_map["F_Y_engine"]        =      F_Y_engine_record          ;
  record_map["F_Z_engine"]        =      F_Z_engine_record          ;

  // gear forces
  record_map["F_X_gear"]          =      F_X_gear_record            ;
  record_map["F_Y_gear"]          =      F_Y_gear_record            ;
  record_map["F_Z_gear"]          =      F_Z_gear_record            ;

  // total forces in body axis
  record_map["F_X"]               =      F_X_record                 ;
  record_map["F_Y"]               =      F_Y_record                 ;
  record_map["F_Z"]               =      F_Z_record                 ;

  // total forces in local axis
  record_map["F_north"]           =      F_north_record             ;
  record_map["F_east"]            =      F_east_record              ;
  record_map["F_down"]            =      F_down_record              ;


  /*********************** Moments ***********************/
  // aero moments
  record_map["M_l_aero"]          =      M_l_aero_record            ;
  record_map["M_m_aero"]          =      M_m_aero_record            ;
  record_map["M_n_aero"]          =      M_n_aero_record            ;

  // engine moments
  record_map["M_l_engine"]        =      M_l_engine_record          ;
  record_map["M_m_engine"]        =      M_m_engine_record          ;
  record_map["M_n_engine"]        =      M_n_engine_record          ;

  // gear moments
  record_map["M_l_gear"]          =      M_l_gear_record            ;
  record_map["M_m_gear"]          =      M_m_gear_record            ;
  record_map["M_n_gear"]          =      M_n_gear_record            ;

  // total moments about reference point
  record_map["M_l_rp"]            =      M_l_rp_record              ;
  record_map["M_m_rp"]            =      M_m_rp_record              ;
  record_map["M_n_rp"]            =      M_n_rp_record              ;

  // total moments about cg
  record_map["M_l_cg"]            =      M_l_cg_record              ;
  record_map["M_m_cg"]            =      M_m_cg_record              ;
  record_map["M_n_cg"]            =      M_n_cg_record              ;
  
  /***********************Flapper Data********************/
  record_map["flapper_freq"]       =      flapper_freq_record        ;
  record_map["flapper_phi"]        =      flapper_phi_record         ;
  record_map["flapper_phi_deg"]    =      flapper_phi_deg_record     ;
  record_map["flapper_Lift"]       =      flapper_Lift_record        ;
  record_map["flapper_Thrust"]     =      flapper_Thrust_record      ;
  record_map["flapper_Inertia"]    =      flapper_Inertia_record     ;
  record_map["flapper_Moment"]     =      flapper_Moment_record      ;
  
  
  /******************** MSS debug **********************************/
  record_map["debug1"]             =       debug1_record              ;
  record_map["debug2"]             =       debug2_record              ;
  record_map["debug3"]             =       debug3_record              ;
  /******************** RD debug ***********************************/
  record_map["debug4"]             =       debug4_record              ;
  record_map["debug5"]             =       debug5_record              ;
  record_map["debug6"]             =       debug6_record              ;

  record_map["debug7"]             =       debug7_record              ;
  record_map["debug8"]             =       debug8_record              ;
  record_map["debug9"]             =       debug9_record              ;
  record_map["debug10"]            =       debug10_record             ;

  /******************** Misc data **********************************/
  record_map["V_down_fpm"]         =       V_down_fpm_record          ;
  record_map["eta_q"]              =       eta_q_record               ;
  record_map["rpm"]                =       rpm_record                 ;
  record_map["elevator_sas_deg"]   =       elevator_sas_deg_record    ;
  record_map["aileron_sas_deg"]    =       aileron_sas_deg_record     ;
  record_map["rudder_sas_deg"]     =       rudder_sas_deg_record      ;
  record_map["w_induced"]          =       w_induced_record           ;
  record_map["downwashAngle_deg"]  =       downwashAngle_deg_record   ;
  record_map["alphaTail_deg"]      =       alphaTail_deg_record       ;
  record_map["gammaWing"]          =       gammaWing_record           ;
  record_map["LD"]                 =       LD_record                  ;
  record_map["gload"]              =       gload_record               ;
  record_map["gyroMomentQ"]        =       gyroMomentQ_record         ;
  record_map["gyroMomentR"]        =       gyroMomentR_record         ;

  /******************** Gear ************************************/
  record_map["Gear_handle"]       =      Gear_handle_record         ;
  record_map["gear_cmd_norm"]     =      gear_cmd_norm_record       ;
  record_map["gear_pos_norm"]     =      gear_pos_norm_record       ;
  
}

// end uiuc_map_record5.cpp
