/********************************************************************** 
 
 FILENAME:     uiuc_map_record2.cpp

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the record maps for velocities and angles

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

#include "uiuc_map_record2.h"


void uiuc_map_record2()
{
  /********************** Velocities *********************/
  // local velocities
  record_map["V_north"]           =      V_north_record             ;
  record_map["V_east"]            =      V_east_record              ;
  record_map["V_down"]            =      V_down_record              ;

  // local velocities wrt ground
  record_map["V_north_rel_ground"]  =    V_north_rel_ground_record  ;
  record_map["V_east_rel_ground"]   =    V_east_rel_ground_record   ;
  record_map["V_down_rel_ground"]   =    V_down_rel_ground_record   ;

  // steady airmass velocities
  record_map["V_north_airmass"]   =      V_north_airmass_record     ;
  record_map["V_east_airmass"]    =      V_east_airmass_record      ;
  record_map["V_down_airmass"]    =      V_down_airmass_record      ;

  // local velocities wrt steady airmass
  record_map["V_north_rel_airmass"]  =   V_north_rel_airmass_record  ;
  record_map["V_east_rel_airmass"]   =   V_east_rel_airmass_record   ;
  record_map["V_down_rel_airmass"]   =   V_down_rel_airmass_record   ;

  // local linear turbulence velocities
  record_map["U_gust"]            =      U_gust_record              ;
  record_map["V_gust"]            =      V_gust_record              ;
  record_map["W_gust"]            =      W_gust_record              ;

  // wind velocities in body axis
  record_map["U_body"]            =      U_body_record              ;
  record_map["V_body"]            =      V_body_record              ;
  record_map["W_body"]            =      W_body_record              ;

  // other velocities
  record_map["V_rel_wind"]        =      V_rel_wind_record          ;
  record_map["V_true_kts"]        =      V_true_kts_record          ;
  record_map["V_rel_ground"]      =      V_rel_ground_record        ;
  record_map["V_inertial"]        =      V_inertial_record          ;
  record_map["V_ground_speed"]    =      V_ground_speed_record      ;
  record_map["V_equiv"]           =      V_equiv_record             ;
  record_map["V_equiv_kts"]       =      V_equiv_kts_record         ;
  record_map["V_calibrated"]      =      V_calibrated_record        ;
  record_map["V_calibrated_kts"]  =      V_calibrated_kts_record    ;

  // angular rates in local axis
  record_map["P_local"]           =      P_local_record             ;
  record_map["Q_local"]           =      Q_local_record             ;
  record_map["R_local"]           =      R_local_record             ;

  // angular rates in body axis
  record_map["P_body"]            =      P_body_record              ;
  record_map["Q_body"]            =      Q_body_record              ;
  record_map["R_body"]            =      R_body_record              ;

  // difference between local and body angular rates
  record_map["P_total"]           =      P_total_record             ;
  record_map["Q_total"]           =      Q_total_record             ;
  record_map["R_total"]           =      R_total_record             ;

  // Euler rates
  record_map["Phi_dot"]           =      Phi_dot_record             ;
  record_map["Theta_dot"]         =      Theta_dot_record           ;
  record_map["Psi_dot"]           =      Psi_dot_record             ;

  // Geocentric rates
  record_map["Latitude_dot"]      =      Latitude_dot_record        ;
  record_map["Longitude_dot"]     =      Longitude_dot_record       ;
  record_map["Radius_dot"]        =      Radius_dot_record          ;


  /************************ Angles ***********************/
  record_map["Alpha"]             =      Alpha_record               ;
  record_map["Alpha_deg"]         =      Alpha_deg_record           ;
  record_map["Alpha_dot"]         =      Alpha_dot_record           ;
  record_map["Alpha_dot_deg"]     =      Alpha_dot_deg_record       ;
  record_map["Beta"]              =      Beta_record                ;
  record_map["Beta_deg"]          =      Beta_deg_record            ;
  record_map["Beta_dot"]          =      Beta_dot_record            ;
  record_map["Beta_dot_deg"]      =      Beta_dot_deg_record        ;
  record_map["Gamma_vert"]        =      Gamma_vert_record          ;
  record_map["Gamma_vert_deg"]    =      Gamma_vert_deg_record      ;
  record_map["Gamma_horiz"]       =      Gamma_horiz_record         ;
  record_map["Gamma_horiz_deg"]   =      Gamma_horiz_deg_record     ;
}

// end uiuc_map_record2.cpp
