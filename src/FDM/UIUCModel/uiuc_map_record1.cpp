/********************************************************************** 
 
 FILENAME:     uiuc_map_record1.cpp

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the record maps for time, mass, geometry, 
               positions, and accelerations

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

#include "uiuc_map_record1.h"


void uiuc_map_record1()
{
  /************************* Time ************************/
  record_map["Simtime"]           =      Simtime_record             ;
  record_map["dt"]                =      dt_record                  ;


  /************************* Mass ************************/
  record_map["Weight"]            =      Weight_record              ;
  record_map["Mass"]              =      Mass_record                ;
  record_map["I_xx"]              =      I_xx_record                ;
  record_map["I_yy"]              =      I_yy_record                ;
  record_map["I_zz"]              =      I_zz_record                ;
  record_map["I_xz"]              =      I_xz_record                ;


  /*********************** Geometry **********************/
  // pilot reference locations
  record_map["Dx_pilot"]          =      Dx_pilot_record            ;
  record_map["Dy_pilot"]          =      Dy_pilot_record            ;
  record_map["Dz_pilot"]          =      Dz_pilot_record            ;

  // cg reference locations
  record_map["Dx_cg"]             =      Dx_cg_record               ;
  record_map["Dy_cg"]             =      Dy_cg_record               ;
  record_map["Dz_cg"]             =      Dz_cg_record               ;


  /********************** Positions **********************/
  // geocentric positions
  record_map["Lat_geocentric"]       =   Lat_geocentric_record       ;
  record_map["Lon_geocentric"]       =   Lon_geocentric_record       ;
  record_map["Radius_to_vehicle"]    =   Radius_to_vehicle_record    ;

  // geodetic positions
  record_map["Latitude"]           =     Latitude_record            ;
  record_map["Longitude"]          =     Longitude_record           ;
  record_map["Altitude"]           =     Altitude_record            ;

  // Euler angles
  record_map["Phi"]               =      Phi_record                 ;
  record_map["Theta"]             =      Theta_record               ;
  record_map["Psi"]               =      Psi_record                 ;
  record_map["Phi_deg"]           =      Phi_deg_record             ;
  record_map["Theta_deg"]         =      Theta_deg_record           ;
  record_map["Psi_deg"]           =      Psi_deg_record             ;


  /******************** Accelerations ********************/
  // accelerations in local axes
  record_map["V_dot_north"]       =      V_dot_north_record         ;
  record_map["V_dot_east"]        =      V_dot_east_record          ;
  record_map["V_dot_down"]        =      V_dot_down_record          ;

  // accelerations in body axes
  record_map["U_dot_body"]        =      U_dot_body_record          ;
  record_map["V_dot_body"]        =      V_dot_body_record          ;
  record_map["W_dot_body"]        =      W_dot_body_record          ;

  // acceleration of pilot
  record_map["A_X_pilot"]         =      A_X_pilot_record           ;
  record_map["A_Y_pilot"]         =      A_Y_pilot_record           ;
  record_map["A_Z_pilot"]         =      A_Z_pilot_record           ;

  // acceleration of cg
  record_map["A_X_cg"]            =      A_X_cg_record              ;
  record_map["A_Y_cg"]            =      A_Y_cg_record              ;
  record_map["A_Z_cg"]            =      A_Z_cg_record              ;

  // acceleration of pilot
  record_map["N_X_pilot"]         =      N_X_pilot_record           ;
  record_map["N_Y_pilot"]         =      N_Y_pilot_record           ;
  record_map["N_Z_pilot"]         =      N_Z_pilot_record           ;

  // acceleration of cg
  record_map["N_X_cg"]            =      N_X_cg_record              ;
  record_map["N_Y_cg"]            =      N_Y_cg_record              ;
  record_map["N_Z_cg"]            =      N_Z_cg_record              ;

  // moment acceleration rates
  record_map["P_dot_body"]        =      P_dot_body_record          ;
  record_map["Q_dot_body"]        =      Q_dot_body_record          ;
  record_map["R_dot_body"]        =      R_dot_body_record          ;
}

// end uiuc_map_record1.cpp
