/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_record.cpp

----------------------------------------------------------------------

 DESCRIPTION:  reads input data for specified aircraft and creates 
               approporiate data storage space

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   based on "menu reader" format of Michael Selig

----------------------------------------------------------------------

 HISTORY:      04/04/2003   initial release

----------------------------------------------------------------------

 AUTHOR(S):    Robert Deters      <rdeters@uiuc.edu>
               Michael Selig      <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       n/a

----------------------------------------------------------------------

 OUTPUTS:      n/a

----------------------------------------------------------------------

 CALLED BY:    uiuc_menu()

----------------------------------------------------------------------

 CALLS TO:     check_float() if needed
	       d_2_to_3() if needed
	       d_1_to_2() if needed
	       i_1_to_2() if needed
	       d_1_to_1() if needed

 ----------------------------------------------------------------------

 COPYRIGHT:    (C) 2003 by Michael Selig

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

#include <simgear/compiler.h>

#if defined( __MWERKS__ )
// -dw- optimizer chokes (big-time) trying to optimize humongous
// loop/switch statements
#pragma optimization_level 0
#endif

#include <cstdlib>
#include <string>
#include STL_IOSTREAM

#include "uiuc_menu_record.h"

SG_USING_STD(cerr);
SG_USING_STD(cout);
SG_USING_STD(endl);

#ifndef _MSC_VER
SG_USING_STD(exit);
#endif

void parse_record( const string& linetoken2, const string& linetoken3,
		   const string& linetoken4, const string& linetoken5, 
		   const string& linetoken6, const string& linetoken7, 
		   const string& linetoken8, const string& linetoken9,
		   const string& linetoken10, const string& aircraft_directory,
		   LIST command_line ) {

  istrstream token3(linetoken3.c_str());
  istrstream token4(linetoken4.c_str());
  istrstream token5(linetoken5.c_str());
  istrstream token6(linetoken6.c_str());
  istrstream token7(linetoken7.c_str());
  istrstream token8(linetoken8.c_str());
  istrstream token9(linetoken9.c_str());
  istrstream token10(linetoken10.c_str());

  switch(record_map[linetoken2])
    {
      /************************* Time ************************/
    case Simtime_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case dt_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /************************* Mass ************************/
    case Weight_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Mass_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case I_xx_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case I_yy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case I_zz_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case I_xz_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /*********************** Geometry **********************/
    case Dx_pilot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Dy_pilot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Dz_pilot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Dx_cg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Dy_cg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Dz_cg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /********************** Positions **********************/
    case Lat_geocentric_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Lon_geocentric_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Radius_to_vehicle_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Latitude_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Longitude_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Altitude_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Phi_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Theta_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Psi_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /******************** Accelerations ********************/
    case V_dot_north_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_dot_east_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_dot_down_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case U_dot_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_dot_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case W_dot_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case A_X_pilot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case A_Y_pilot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case A_Z_pilot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case A_X_cg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case A_Y_cg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case A_Z_cg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case N_X_pilot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case N_Y_pilot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case N_Z_pilot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case N_X_cg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case N_Y_cg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case N_Z_cg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case P_dot_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Q_dot_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case R_dot_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /********************** Velocities *********************/
    case V_north_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_east_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_down_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_north_rel_ground_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_east_rel_ground_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_down_rel_ground_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_north_airmass_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_east_airmass_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_down_airmass_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_north_rel_airmass_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_east_rel_airmass_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_down_rel_airmass_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case U_gust_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_gust_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case W_gust_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case U_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case W_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_rel_wind_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_true_kts_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_rel_ground_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_inertial_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_ground_speed_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_equiv_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_equiv_kts_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_calibrated_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_calibrated_kts_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case P_local_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Q_local_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case R_local_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case P_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Q_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case R_body_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case P_total_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Q_total_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case R_total_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Phi_dot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Theta_dot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Psi_dot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Latitude_dot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Longitude_dot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Radius_dot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /************************ Angles ***********************/
    case Alpha_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Alpha_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Alpha_dot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Alpha_dot_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Beta_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Beta_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Beta_dot_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Beta_dot_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Gamma_vert_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Gamma_vert_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Gamma_horiz_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Gamma_horiz_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /**************** Atmospheric Properties ***************/
    case Density_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_sound_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Mach_number_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Static_pressure_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Total_pressure_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Impact_pressure_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Dynamic_pressure_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Static_temperature_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Total_temperature_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /******************** Earth Properties *****************/
    case Gravity_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Sea_level_radius_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Earth_position_angle_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Runway_altitude_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Runway_latitude_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Runway_longitude_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Runway_heading_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Radius_to_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case D_pilot_north_of_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case D_pilot_east_of_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case D_pilot_above_rwy_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
    case X_pilot_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Y_pilot_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case H_pilot_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case D_cg_north_of_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case D_cg_east_of_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case D_cg_above_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case X_cg_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Y_cg_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case H_cg_rwy_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /********************* Engine Inputs *******************/
    case Throttle_pct_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Throttle_3_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /******************** Control Inputs *******************/
    case Long_control_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Long_trim_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Long_trim_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case elevator_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case elevator_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Lat_control_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case aileron_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case aileron_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Rudder_pedal_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case rudder_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case rudder_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Flap_handle_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flap_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flap_cmd_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flap_cmd_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flap_pos_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flap_pos_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Spoiler_handle_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case spoiler_cmd_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case spoiler_pos_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case spoiler_pos_norm_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case spoiler_pos_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Gear_handle_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case gear_cmd_norm_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case gear_pos_norm_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /****************** Aero Coefficients ******************/
    case CD_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CDfaI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CDfadeI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CDfdfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CDfadfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CX_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CXfabetafI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CXfadefI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CXfaqfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CDo_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CDK_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CLK_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_a_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_adot_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_q_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_ih_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_de_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_dr_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_da_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_beta_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_df_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_ds_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_dg_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CXo_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CXK_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CX_a_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CX_a2_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CX_a3_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CX_adot_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CX_q_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CX_de_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CX_dr_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CX_df_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CX_adf_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CL_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CLfaI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CLfadeI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CLfdfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CLfadfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZ_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZfaI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZfabetafI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZfadefI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZfaqfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CLo_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CL_a_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CL_adot_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CL_q_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CL_ih_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CL_de_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CL_df_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CL_ds_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CL_dg_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZo_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZ_a_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZ_a2_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZ_a3_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZ_adot_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZ_q_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZ_de_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZ_deb2_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZ_df_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CZ_adf_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CmfaI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CmfadeI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CmfdfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CmfadfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CmfabetafI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CmfadefI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CmfaqfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cmo_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_a_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_a2_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_adot_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_q_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_ih_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_de_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_b2_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_r_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_df_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_ds_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_dg_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CY_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CYfadaI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CYfbetadrI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CYfabetafI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CYfadafI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CYfadrfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CYfapfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CYfarfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CYo_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CY_beta_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CY_p_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CY_r_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CY_da_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CY_dr_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CY_dra_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CY_bdot_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cl_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case ClfadaI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case ClfbetadrI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case ClfabetafI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case ClfadafI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case ClfadrfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case ClfapfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case ClfarfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Clo_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cl_beta_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cl_p_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cl_r_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cl_da_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cl_dr_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cl_daa_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cn_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CnfadaI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CnfbetadrI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CnfabetafI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CnfadafI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CnfadrfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CnfapfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CnfarfI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cno_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cn_beta_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cn_p_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cn_r_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cn_da_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cn_dr_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cn_q_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cn_b3_save_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /******************** Ice Detection ********************/
    case CL_clean_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CL_iced_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_clean_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CD_iced_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_clean_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cm_iced_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Ch_clean_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Ch_iced_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cl_clean_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Cl_iced_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CLclean_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CLiced_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CLclean_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case CLiced_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Lift_clean_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Lift_iced_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Lift_clean_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Lift_iced_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Gamma_clean_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Gamma_iced_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Gamma_clean_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Gamma_iced_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case w_clean_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case w_iced_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case w_clean_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case w_iced_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_total_clean_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_total_iced_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_total_clean_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_total_iced_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case beta_flow_clean_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case beta_flow_clean_wing_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case beta_flow_iced_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case beta_flow_iced_wing_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case beta_flow_clean_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case beta_flow_clean_tail_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case beta_flow_iced_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case beta_flow_iced_tail_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Dbeta_flow_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Dbeta_flow_wing_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Dbeta_flow_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case Dbeta_flow_tail_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case pct_beta_flow_wing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case pct_beta_flow_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eta_ice_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eta_wing_left_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eta_wing_right_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eta_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case delta_CL_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case delta_CD_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case delta_Cm_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case delta_Cl_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case boot_cycle_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case boot_cycle_wing_left_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case boot_cycle_wing_right_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case autoIPS_tail_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case autoIPS_wing_left_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case autoIPS_wing_right_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eps_pitch_input_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eps_alpha_max_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eps_pitch_max_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eps_pitch_min_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eps_roll_max_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eps_thrust_min_record:
       {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eps_flap_max_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eps_airspeed_max_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eps_airspeed_min_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }

      /*********************Auto Pilot************************/
    case ap_Theta_ref_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case ap_pah_on_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
	
      /************************ Forces ***********************/
    case F_X_wind_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_Y_wind_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_Z_wind_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_X_aero_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_Y_aero_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_Z_aero_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_X_engine_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_Y_engine_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_Z_engine_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_X_gear_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_Y_gear_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_Z_gear_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_X_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_Y_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_Z_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_north_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_east_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case F_down_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      
      /*********************** Moments ***********************/
    case M_l_aero_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_m_aero_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_n_aero_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_l_engine_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_m_engine_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_n_engine_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_l_gear_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_m_gear_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_n_gear_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_l_rp_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_m_rp_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case M_n_rp_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      /****************** Flapper Data ***********************/
    case flapper_freq_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flapper_phi_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flapper_phi_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flapper_Lift_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flapper_Thrust_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flapper_Inertia_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case flapper_Moment_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
      /****************** debug keywords ***********************/
    case debug1_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case debug2_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case debug3_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case debug4_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case debug5_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case debug6_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case tactilefadefI_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case V_down_fpm_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case eta_q_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case rpm_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case elevator_sas_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case aileron_sas_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case rudder_sas_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case w_induced_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case downwashAngle_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case alphaTail_deg_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case gammaWing_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case LD_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case gload_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case gyroMomentQ_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case gyroMomentR_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case trigger_on_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case trigger_num_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case trigger_toggle_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    case trigger_counter_record:
      {
	recordParts -> storeCommands (*command_line);
	break;
      }
    default:
      {
	if (ignore_unknown_keywords) {
	  // do nothing
	} else {
	  // print error message
	  uiuc_warnings_errors(2, *command_line);
	}
	break;
      }
    };
}
