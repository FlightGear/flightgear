/********************************************************************** 
 
 FILENAME:     uiuc_map_record3.cpp

---------------------------------------------------------------------- 

 DESCRIPTION:  initializes the record maps for atmospheric properties, 
               Earth properties, engine inputs, and control inputs

----------------------------------------------------------------------
 
 STATUS:       alpha version

----------------------------------------------------------------------
 
 REFERENCES:   
 
----------------------------------------------------------------------
 
 HISTORY:      06/03/2000   file creation
               11/12/2001   (RD) Added flap_cmd_deg and flap_pos
               03/03/2003   (RD) Added flap_cmd
               08/20/2003   (RD) Changed spoiler variables to match
                            flap convention.  Added flap_pos_norm

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include "uiuc_map_record3.h"


void uiuc_map_record3()
{
  /**************** Atmospheric Properties ***************/
  record_map["Density"]           =      Density_record             ;
  record_map["V_sound"]           =      V_sound_record             ;
  record_map["Mach_number"]       =      Mach_number_record         ;

  // pressures
  record_map["Static_pressure"]   =      Static_pressure_record     ;
  record_map["Total_pressure"]    =      Total_pressure_record      ;
  record_map["Impact_pressure"]   =      Impact_pressure_record     ;
  record_map["Dynamic_pressure"]  =      Dynamic_pressure_record    ;

  // temperatures
  record_map["Static_temperature"]=      Static_temperature_record  ;
  record_map["Total_temperature"] =      Total_temperature_record   ;


  /******************** Earth Properties *****************/
  record_map["Gravity"]             =    Gravity_record             ;
  record_map["Sea_level_radius"]    =    Sea_level_radius_record    ;
  record_map["Earth_position_angle"]=    Earth_position_angle_record;

  // runway positions
  record_map["Runway_altitude"]   =      Runway_altitude_record     ;
  record_map["Runway_latitude"]   =      Runway_latitude_record     ;
  record_map["Runway_longitude"]  =      Runway_longitude_record    ;
  record_map["Runway_heading"]    =      Runway_heading_record      ;
  record_map["Radius_to_rwy"]     =      Radius_to_rwy_record       ;

  // pilot relative to runway in local axis
  record_map["D_pilot_north_of_rwy"]=    D_pilot_north_of_rwy_record;
  record_map["D_pilot_east_of_rwy"] =    D_pilot_east_of_rwy_record ;
  record_map["D_pilot_above_rwy"]   =    D_pilot_above_rwy_record   ;

  // pilot relative to runway in runway axis
  record_map["X_pilot_rwy"]       =      X_pilot_rwy_record         ;
  record_map["Y_pilot_rwy"]       =      Y_pilot_rwy_record         ;
  record_map["H_pilot_rwy"]       =      H_pilot_rwy_record         ;

  // cg relative to runway in local axis
  record_map["D_cg_north_of_rwy"] =      D_cg_north_of_rwy_record   ;
  record_map["D_cg_east_of_rwy"]  =      D_cg_east_of_rwy_record    ;
  record_map["D_cg_above_rwy"]    =      D_cg_above_rwy_record      ;

  // cg relative to runway in runway axis
  record_map["X_cg_rwy"]          =      X_cg_rwy_record            ;
  record_map["Y_cg_rwy"]          =      Y_cg_rwy_record            ;
  record_map["H_cg_rwy"]          =      H_cg_rwy_record            ;


  /********************* Engine Inputs *******************/
  record_map["Throttle_3"]        =      Throttle_3_record          ;
  record_map["Throttle_pct"]      =      Throttle_pct_record        ;


  /******************** Control Inputs *******************/
  record_map["Long_control"]      =      Long_control_record        ;
  record_map["Long_trim"]         =      Long_trim_record           ;
  record_map["Long_trim_deg"]     =      Long_trim_deg_record       ;
  record_map["elevator"]          =      elevator_record            ;
  record_map["elevator_deg"]      =      elevator_deg_record        ;
  record_map["Lat_control"]       =      Lat_control_record         ;
  record_map["aileron"]           =      aileron_record             ;
  record_map["aileron_deg"]       =      aileron_deg_record         ;
  record_map["Rudder_pedal"]      =      Rudder_pedal_record        ;
  record_map["rudder"]            =      rudder_record              ;
  record_map["rudder_deg"]        =      rudder_deg_record          ;
  record_map["Flap_handle"]       =      Flap_handle_record         ;
  record_map["flap_cmd"]          =      flap_cmd_record            ;
  record_map["flap_cmd_deg"]      =      flap_cmd_deg_record        ;
  record_map["flap_pos"]          =      flap_pos_record            ;
  record_map["flap_pos_deg"]      =      flap_pos_deg_record        ;
  record_map["flap_pos_norm"]     =      flap_pos_norm_record       ;
  record_map["Spoiler_handle"]    =      Spoiler_handle_record      ;
  record_map["spoiler_cmd"]       =      spoiler_cmd_record         ;
  record_map["spoiler_cmd_deg"]   =      spoiler_cmd_deg_record     ;
  record_map["spoiler_pos"]       =      spoiler_pos_record         ;
  record_map["spoiler_pos_deg"]   =      spoiler_pos_deg_record     ;
  record_map["spoiler_pos_norm"]  =      spoiler_pos_norm_record    ;

}

// end uiuc_map_record3.cpp
