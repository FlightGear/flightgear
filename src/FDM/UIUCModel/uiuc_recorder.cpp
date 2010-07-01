/**********************************************************************
 
 FILENAME:     uiuc_recorder.cpp

----------------------------------------------------------------------

 DESCRIPTION:  outputs variables specified in input file to recorder 
               file

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   Liberty, Jesse.  "Sam's Teach Yourself C++ in 21 Days,"
               3rd ed., 1999.

----------------------------------------------------------------------

 HISTORY:      01/31/2000   initial release
               03/02/2000   (JS) added record options for 1D and 2D 
                            interpolated variables
               04/01/2000   (JS) added throttle, longitudinal, lateral, 
                            and rudder inputs to record map
               04/24/2000   (JS) added rest of variables in 
                            ls_generic.h
	       07/06/2001   (RD) changed Flap handle output
	       07/20/2001   (RD) fixed Lat_control and Rudder_pedal
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (Cxfxxf0I)
               11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (CxfxxfI).  Removed zero flap variables.
			    Added flap_pos and flap_cmd_deg.
	       02/13/2002   (RD) Added variables so linear aero model
	                    values can be recorded
               03/03/2003   (RD) Added flap_cmd_record
               03/16/2003   (RD) Added trigger record variables
               07/17/2003   (RD) Added error checking code (default
                            routine) since it was removed from
                            uiuc_menu_record()
               08/20/2003   (RD) Changed spoiler variables to match
                            flap convention.  Added flap_pos_norm

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         http://www.jeffscott.net/
               Robert Deters      <rdeters@uiuc.edu>
	       Michael Selig      <m-selig@uiuc.edu>
----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       n/a

----------------------------------------------------------------------

 OUTPUTS:      -variables recorded in uiuc_recorder.dat

----------------------------------------------------------------------

 CALLED BY:   uiuc_wrapper.cpp 

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>
#include <Main/fg_props.hxx>

#include "uiuc_recorder.h"

using std::endl;		// -dw

void uiuc_recorder( double dt )
{
  stack command_list;
  string linetoken;
  // static int init = 0;
  static int recordStep = 0;
  string record_variables = "# ";

  // int modulus = recordStep % recordRate;

  //static double lat1;
  //static double long1;
  //double D_cg_north1;
  //double D_cg_east1;
  //if (Simtime == 0)
  // {
  //    lat1=Latitude;
  //    long1=Longitude;
  //  }

  if ((recordStep % recordRate) == 0)
    {
      command_list = recordParts->getCommands();
      fout << endl;

      LIST command_line;
      for (command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
        record_variables += recordParts->getToken(*command_line,2) + "  ";

      fout << record_variables << endl; 
      for (command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
        {
          linetoken = recordParts->getToken(*command_line, 2); 

          switch(record_map[linetoken])
            {
              /************************* Time ************************/
            case Simtime_record:
              {
                fout << Simtime << " ";
                break;
              }
            case dt_record:
              {
                fout << dt << " ";
                break;
              }

              /************************* Mass ************************/
            case Weight_record:
              {
                fout << Weight << " ";
                break;
              }
            case Mass_record:
              {
                fout << Mass << " ";
                break;
              }
            case I_xx_record:
              {
                fout << I_xx << " ";
                break;
              }
            case I_yy_record:
              {
                fout << I_yy << " ";
                break;
              }
            case I_zz_record:
              {
                fout << I_zz << " ";
                break;
              }
            case I_xz_record:
              {
                fout << I_xz << " ";
                break;
              }

              /*********************** Geometry **********************/
            case Dx_pilot_record:
              {
                fout << Dx_pilot << " ";
                break;
              }
            case Dy_pilot_record:
              {
                fout << Dy_pilot << " ";
                break;
              }
            case Dz_pilot_record:
              {
                fout << Dz_pilot << " ";
                break;
              }
            case Dx_cg_record:
              {
                fout << Dx_cg << " ";
                break;
              }
            case Dy_cg_record:
              {
                fout << Dy_cg << " ";
                break;
              }
            case Dz_cg_record:
              {
                fout << Dz_cg << " ";
                break;
              }

              /********************** Positions **********************/
            case Lat_geocentric_record:
              {
                fout << Lat_geocentric << " ";
                break;
              }
            case Lon_geocentric_record:
              {
                fout << Lon_geocentric << " ";
                break;
              }
            case Radius_to_vehicle_record:
              {
                fout << Radius_to_vehicle << " ";
                break;
              }
            case Latitude_record:
              {
                fout << Latitude << " ";
                break;
              }
            case Longitude_record:
              {
                fout << Longitude << " ";
                break;
              }
            case Altitude_record:
              {
                fout << Altitude << " ";
                break;
              }
            case Phi_record:
              {
                fout << Phi << " ";
                break;
              }
            case Theta_record:
              {
                fout << Theta << " ";
                break;
              }
            case Psi_record:
              {
                fout << Psi << " ";
                break;
              }
            case Phi_deg_record:
              {
                fout << Phi*RAD_TO_DEG << " ";
                break;
              }
            case Theta_deg_record:
              {
                fout << Theta*RAD_TO_DEG << " ";
                break;
              }
            case Psi_deg_record:
              {
                fout << Psi*RAD_TO_DEG << " ";
                break;
              }

              /******************** Accelerations ********************/
            case V_dot_north_record:
              {
                fout << V_dot_north << " ";
                break;
              }
            case V_dot_east_record:
              {
                fout << V_dot_east << " ";
                break;
              }
            case V_dot_down_record:
              {
                fout << V_dot_down << " ";
                break;
              }
            case U_dot_body_record:
              {
                fout << U_dot_body << " ";
                break;
              }
            case V_dot_body_record:
              {
                fout << V_dot_body << " ";
                break;
              }
            case W_dot_body_record:
              {
                fout << W_dot_body << " ";
                break;
              }
            case A_X_pilot_record:
              {
                fout << A_X_pilot << " ";
                break;
              }
            case A_Y_pilot_record:
              {
                fout << A_Y_pilot << " ";
                break;
              }
            case A_Z_pilot_record:
              {
                fout << A_Z_pilot << " ";
                break;
              }
            case A_X_cg_record:
              {
                fout << A_X_cg << " ";
                break;
              }
            case A_Y_cg_record:
              {
                fout << A_Y_cg << " ";
                break;
              }
            case A_Z_cg_record:
              {
                fout << A_Z_cg << " ";
                break;
              }
            case N_X_pilot_record:
              {
                fout << N_X_pilot << " ";
                break;
              }
            case N_Y_pilot_record:
              {
                fout << N_Y_pilot << " ";
                break;
              }
            case N_Z_pilot_record:
              {
                fout << N_Z_pilot << " ";
                break;
              }
            case N_X_cg_record:
              {
                fout << N_X_cg << " ";
                break;
              }
            case N_Y_cg_record:
              {
                fout << N_Y_cg << " ";
                break;
              }
            case N_Z_cg_record:
              {
                fout << N_Z_cg << " ";
                break;
              }
            case P_dot_body_record:
              {
                fout << P_dot_body << " ";
                break;
              }
            case Q_dot_body_record:
              {
                fout << Q_dot_body << " ";
                break;
              }
            case R_dot_body_record:
              {
                fout << R_dot_body << " ";
                break;
              }

              /********************** Velocities *********************/
            case V_north_record:
              {
                fout << V_north << " ";
                break;
              }
            case V_east_record:
              {
                fout << V_east << " ";
                break;
              }
            case V_down_record:
              {
                fout << V_down << " ";
                break;
              }
	    case V_down_fpm_record:
	      {
		fout << V_down * 60 << " ";
		break;
	      }
            case V_north_rel_ground_record:
              {
                fout << V_north_rel_ground << " ";
                break;
              }
            case V_east_rel_ground_record:
              {
                fout << V_east_rel_ground << " ";
                break;
              }
            case V_down_rel_ground_record:
              {
                fout << V_down_rel_ground << " ";
                break;
              }
            case V_north_airmass_record:
              {
                fout << V_north_airmass << " ";
                break;
              }
            case V_east_airmass_record:
              {
                fout << V_east_airmass << " ";
                break;
              }
            case V_down_airmass_record:
              {
                fout << V_down_airmass << " ";
                break;
              }
            case V_north_rel_airmass_record:
              {
                fout << V_north_rel_airmass << " ";
                break;
              }
            case V_east_rel_airmass_record:
              {
                fout << V_east_rel_airmass << " ";
                break;
              }
            case V_down_rel_airmass_record:
              {
                fout << V_down_rel_airmass << " ";
                break;
              }
            case U_gust_record:
              {
                fout << U_gust << " ";
                break;
              }
            case V_gust_record:
              {
                fout << V_gust << " ";
                break;
              }
            case W_gust_record:
              {
                fout << W_gust << " ";
                break;
              }
            case U_body_record:
              {
                fout << U_body << " ";
                break;
              }
            case V_body_record:
              {
                fout << V_body << " ";
                break;
              }
            case W_body_record:
              {
                fout << W_body << " ";
                break;
              }
            case V_rel_wind_record:
              {
                fout << V_rel_wind << " ";
                break;
              }
            case V_true_kts_record:
              {
                fout << V_true_kts << " ";
                break;
              }
            case V_rel_ground_record:
              {
                fout << V_rel_ground << " ";
                break;
              }
            case V_inertial_record:
              {
                fout << V_inertial << " ";
                break;
              }
            case V_ground_speed_record:
              {
                fout << V_ground_speed << " ";
                break;
              }
            case V_equiv_record:
              {
                fout << V_equiv << " ";
                break;
              }
            case V_equiv_kts_record:
              {
                fout << V_equiv_kts << " ";
                break;
              }
            case V_calibrated_record:
              {
                fout << V_calibrated << " ";
                break;
              }
            case V_calibrated_kts_record:
              {
                fout << V_calibrated_kts << " ";
                break;
              }
            case P_local_record:
              {
                fout << P_local << " ";
                break;
              }
            case Q_local_record:
              {
                fout << Q_local << " ";
                break;
              }
            case R_local_record:
              {
                fout << R_local << " ";
                break;
              }
            case P_body_record:
              {
                fout << P_body << " ";
                break;
              }
            case Q_body_record:
              {
                fout << Q_body << " ";
                break;
              }
            case R_body_record:
              {
                fout << R_body << " ";
                break;
              }
            case P_total_record:
              {
                fout << P_total << " ";
                break;
              }
            case Q_total_record:
              {
                fout << Q_total << " ";
                break;
              }
            case R_total_record:
              {
                fout << R_total << " ";
                break;
              }
            case Phi_dot_record:
              {
                fout << Phi_dot << " ";
                break;
              }
            case Theta_dot_record:
              {
                fout << Theta_dot << " ";
                break;
              }
            case Psi_dot_record:
              {
                fout << Psi_dot << " ";
                break;
              }
            case Latitude_dot_record:
              {
                fout << Latitude_dot << " ";
                break;
              }
            case Longitude_dot_record:
              {
                fout << Longitude_dot << " ";
                break;
              }
            case Radius_dot_record:
              {
                fout << Radius_dot << " ";
                break;
              }

              /************************ Angles ***********************/
            case Alpha_record:
              {
                fout << Std_Alpha << " ";
                break;
              }
            case Alpha_deg_record:
              {
                fout << Std_Alpha * RAD_TO_DEG << " ";
                break;
              }
            case Alpha_dot_record:
              {
                fout << Std_Alpha_dot << " ";
                break;
              }
            case Alpha_dot_deg_record:
              {
                fout << Std_Alpha_dot * RAD_TO_DEG << " ";
                break;
              }
            case Beta_record:
              {
                fout << Std_Beta << " ";
                break;
              }
            case Beta_deg_record:
              {
                fout << Std_Beta * RAD_TO_DEG << " ";
                break;
              }
            case Beta_dot_record:
              {
                fout << Std_Beta_dot << " ";
                break;
              }
            case Beta_dot_deg_record:
              {
                fout << Std_Beta_dot * RAD_TO_DEG << " ";
                break;
              }
            case Gamma_vert_record:
              {
                fout << Gamma_vert_rad << " ";
                break;
              }
            case Gamma_vert_deg_record:
              {
                fout << Gamma_vert_rad * RAD_TO_DEG << " ";
                break;
              }
            case Gamma_horiz_record:
              {
                fout << Gamma_horiz_rad << " ";
                break;
              }
            case Gamma_horiz_deg_record:
              {
                fout << Gamma_horiz_rad * RAD_TO_DEG << " ";
                break;
              }

              /**************** Atmospheric Properties ***************/
            case Density_record:
              {
                fout << Density << " ";
                break;
              }
            case V_sound_record:
              {
                fout << V_sound << " ";
                break;
              }
            case Mach_number_record:
              {
                fout << Mach_number << " ";
                break;
              }
            case Static_pressure_record:
              {
                fout << Static_pressure << " ";
                break;
              }
            case Total_pressure_record:
              {
                fout << Total_pressure << " ";
                break;
              }
            case Impact_pressure_record:
              {
                fout << Impact_pressure << " ";
                break;
              }
            case Dynamic_pressure_record:
              {
                fout << Dynamic_pressure << " ";
                break;
              }
            case Static_temperature_record:
              {
                fout << Static_temperature << " ";
                break;
              }
            case Total_temperature_record:
              {
                fout << Total_temperature << " ";
                break;
              }

              /******************** Earth Properties *****************/
            case Gravity_record:
              {
                fout << Gravity << " ";
                break;
              }
            case Sea_level_radius_record:
              {
                fout << Sea_level_radius << " ";
                break;
              }
            case Earth_position_angle_record:
              {
                fout << Earth_position_angle << " ";
                break;
              }
            case Runway_altitude_record:
              {
                fout << Runway_altitude << " ";
                break;
              }
            case Runway_latitude_record:
              {
                fout << Runway_latitude << " ";
                break;
              }
            case Runway_longitude_record:
              {
                fout << Runway_longitude << " ";
                break;
              }
            case Runway_heading_record:
              {
                fout << Runway_heading << " ";
                break;
              }
            case Radius_to_rwy_record:
              {
                fout << Radius_to_rwy << " ";
                break;
              }
            case D_pilot_north_of_rwy_record:
              {
                fout << D_pilot_north_of_rwy << " ";
                break;
              }
            case D_pilot_east_of_rwy_record:
              {
                fout << D_pilot_east_of_rwy << " ";
                break;
              }
            case D_pilot_above_rwy_record:
              {
                fout << D_pilot_above_rwy << " ";
                break;
              }
            case X_pilot_rwy_record:
              {
                fout << X_pilot_rwy << " ";
                break;
              }
            case Y_pilot_rwy_record:
              {
                fout << Y_pilot_rwy << " ";
                break;
              }
            case H_pilot_rwy_record:
              {
                fout << H_pilot_rwy << " ";
                break;
              }
            case D_cg_north_of_rwy_record:
              {
                fout << D_cg_north_of_rwy << " ";
                break;
              }
            case D_cg_east_of_rwy_record:
              {
                fout << D_cg_east_of_rwy << " ";
                break;
              }
            case D_cg_above_rwy_record:
              {
                fout << D_cg_above_rwy << " ";
                break;
              }
            case X_cg_rwy_record:
              {
                fout << X_cg_rwy << " ";
                break;
              }
            case Y_cg_rwy_record:
              {
                fout << Y_cg_rwy << " ";
                break;
              }
            case H_cg_rwy_record:
              {
                fout << H_cg_rwy << " ";
                break;
              }

              /********************* Engine Inputs *******************/
            case Throttle_3_record:
              {
                fout << Throttle[3] << " ";
                break;
              }
            case Throttle_pct_record:
              {
                fout << Throttle_pct << " ";
                break;
              }

              /************************ Controls ***********************/
            case Long_control_record:
              {
                fout << Long_control << " ";
                break;
              }
            case Long_trim_record:
              {
                fout << Long_trim << " ";
                break;
              }
            case Long_trim_deg_record:
              {
                fout << Long_trim * RAD_TO_DEG << " ";
                break;
              }
            case elevator_record:
              {
                fout << elevator << " ";
                break;
              }
            case elevator_deg_record:
              {
                fout << elevator * RAD_TO_DEG << " ";
                break;
              }
	    case elevator_sas_deg_record:
	      {
		fout << elevator_sas * RAD_TO_DEG << " ";
		break;
	      }
            case Lat_control_record:
              {
                fout << Lat_control << " ";
                break;
              }
            case aileron_record:
              {
                fout << aileron << " ";
                break;
              }
            case aileron_deg_record:
              {
                fout << aileron * RAD_TO_DEG << " ";
                break;
              }
	    case aileron_sas_deg_record:
	      {
		fout << aileron_sas * RAD_TO_DEG << " ";
		break;
	      }
            case Rudder_pedal_record:
              {
                fout << Rudder_pedal << " ";
                break;
              }
            case rudder_record:
              {
                fout << rudder << " ";
                break;
              }
            case rudder_deg_record:
              {
                fout << rudder * RAD_TO_DEG << " ";
                break;
              }
	    case rudder_sas_deg_record:
	      {
		fout << rudder_sas * RAD_TO_DEG << " ";
		break;
	      }
            case Flap_handle_record:
              {
                fout << Flap_handle << " ";
                break;
              }
            case flap_cmd_record:
              {
                fout << flap_cmd << " ";
                break;
              }
            case flap_cmd_deg_record:
              {
                fout << flap_cmd * RAD_TO_DEG << " ";
                break;
              }
            case flap_pos_record:
              {
                fout << flap_pos << " ";
                break;
              }
            case flap_pos_deg_record:
              {
                fout << flap_pos * RAD_TO_DEG << " ";
                break;
              }
            case flap_pos_norm_record:
              {
                fout << flap_pos_norm << " ";
                break;
              }
            case Spoiler_handle_record:
              {
                fout << Spoiler_handle << " ";
                break;
              }
            case spoiler_cmd_record:
              {
                fout << spoiler_cmd << " ";
                break;
              }
            case spoiler_cmd_deg_record:
              {
                fout << spoiler_cmd * RAD_TO_DEG << " ";
                break;
              }
            case spoiler_pos_record:
              {
                fout << spoiler_pos << " ";
                break;
              }
            case spoiler_pos_deg_record:
              {
                fout << spoiler_pos * RAD_TO_DEG << " ";
                break;
              }
            case spoiler_pos_norm_record:
              {
                fout << spoiler_pos_norm << " ";
                break;
              }

              /****************** Gear Inputs ************************/
            case Gear_handle_record:
              {
                fout << Gear_handle << " ";
                break;
              }
            case gear_cmd_norm_record:
              {
                fout << gear_cmd_norm << " ";
                break;
              }
            case gear_pos_norm_record:
              {
                fout << gear_pos_norm << " ";
                break;
              }

              /****************** Aero Coefficients ******************/
            case CD_record:
              {
                fout << CD << " ";
                break;
              }
            case CDfaI_record:
              {
                fout << CDfaI << " ";
                break;
              }
            case CDfCLI_record:
              {
                fout << CDfCLI << " ";
                break;
              }
            case CDfadeI_record:
              {
                fout << CDfadeI << " ";
                break;
              }
            case CDfdfI_record:
              {
                fout << CDfdfI << " ";
                break;
              }
            case CDfadfI_record:
              {
                fout << CDfadfI << " ";
                break;
              }
            case CX_record:
              {
                fout << CX << " ";
                break;
              }
            case CXfabetafI_record:
              {
                fout << CXfabetafI << " ";
                break;
              }
            case CXfadefI_record:
              {
                fout << CXfadefI << " ";
                break;
              }
            case CXfaqfI_record:
              {
                fout << CXfaqfI << " ";
                break;
              }
            case CDo_save_record:
              {
                fout << CDo_save << " ";
                break;
              }
            case CDK_save_record:
              {
                fout << CDK_save << " ";
                break;
              }
            case CLK_save_record:
              {
                fout << CLK_save << " ";
                break;
              }
            case CD_a_save_record:
              {
                fout << CD_a_save << " ";
                break;
              }
            case CD_adot_save_record:
              {
                fout << CD_adot_save << " ";
                break;
              }
            case CD_q_save_record:
              {
                fout << CD_q_save << " ";
                break;
              }
            case CD_ih_save_record:
              {
                fout << CD_ih_save << " ";
                break;
              }
            case CD_de_save_record:
              {
                fout << CD_de_save << " ";
                break;
              }
            case CD_dr_save_record:
              {
                fout << CD_dr_save << " ";
                break;
              }
            case CD_da_save_record:
              {
                fout << CD_da_save << " ";
                break;
              }
            case CD_beta_save_record:
              {
                fout << CD_beta_save << " ";
                break;
              }
            case CD_df_save_record:
              {
                fout << CD_df_save << " ";
                break;
              }
            case CD_ds_save_record:
              {
                fout << CD_ds_save << " ";
                break;
              }
            case CD_dg_save_record:
              {
                fout << CD_dg_save << " ";
                break;
              }
            case CXo_save_record:
              {
                fout << CXo_save << " ";
                break;
              }
            case CXK_save_record:
              {
                fout << CXK_save << " ";
                break;
              }
            case CX_a_save_record:
              {
                fout << CX_a_save << " ";
                break;
              }
            case CX_a2_save_record:
              {
                fout << CX_a2_save << " ";
                break;
              }
            case CX_a3_save_record:
              {
                fout << CX_a3_save << " ";
                break;
              }
            case CX_adot_save_record:
              {
                fout << CX_adot_save << " ";
                break;
              }
            case CX_q_save_record:
              {
                fout << CX_q_save << " ";
                break;
              }
            case CX_de_save_record:
              {
                fout << CX_de_save << " ";
                break;
              }
            case CX_dr_save_record:
              {
                fout << CX_dr_save << " ";
                break;
              }
            case CX_df_save_record:
              {
                fout << CX_df_save << " ";
                break;
              }
            case CX_adf_save_record:
              {
                fout << CX_adf_save << " ";
                break;
              }
            case CL_record:
              {
                fout << CL << " ";
                break;
              }
            case CLfaI_record:
              {
                fout << CLfaI << " ";
                break;
              }
            case CLfadeI_record:
              {
                fout << CLfadeI << " ";
                break;
              }
            case CLfdfI_record:
              {
                fout << CLfdfI << " ";
                break;
              }
            case CLfadfI_record:
              {
                fout << CLfadfI << " ";
                break;
              }
            case CZ_record:
              {
                fout << CZ << " ";
                break;
              }
            case CZfaI_record:
              {
                fout << CZfaI << " ";
                break;
              }
            case CZfabetafI_record:
	      {
                fout << CZfabetafI << " ";
                break;
              }
            case CZfadefI_record:
              {
                fout << CZfadefI << " ";
                break;
              }
            case CZfaqfI_record:
              {
                fout << CZfaqfI << " ";
                break;
              }
            case CLo_save_record:
              {
                fout << CLo_save << " ";
                break;
              }
            case CL_a_save_record:
              {
                fout << CL_a_save << " ";
                break;
              }
            case CL_adot_save_record:
              {
                fout << CL_adot_save << " ";
                break;
              }
            case CL_q_save_record:
              {
                fout << CL_q_save << " ";
                break;
              }
            case CL_ih_save_record:
              {
                fout << CL_ih_save << " ";
                break;
              }
            case CL_de_save_record:
              {
                fout << CL_de_save << " ";
                break;
              }
            case CL_df_save_record:
              {
                fout << CL_df_save << " ";
                break;
              }
            case CL_ds_save_record:
              {
                fout << CL_ds_save << " ";
                break;
              }
            case CL_dg_save_record:
              {
                fout << CL_dg_save << " ";
                break;
              }
            case CZo_save_record:
              {
                fout << CZo_save << " ";
                break;
              }
            case CZ_a_save_record:
              {
                fout << CZ_a_save << " ";
                break;
              }
            case CZ_a2_save_record:
              {
                fout << CZ_a2_save << " ";
                break;
              }
            case CZ_a3_save_record:
              {
                fout << CZ_a3_save << " ";
                break;
              }
            case CZ_adot_save_record:
              {
                fout << CZ_adot_save << " ";
                break;
              }
            case CZ_q_save_record:
              {
                fout << CZ_q_save << " ";
                break;
              }
            case CZ_de_save_record:
              {
                fout << CZ_de_save << " ";
                break;
              }
            case CZ_deb2_save_record:
              {
                fout << CZ_deb2_save << " ";
                break;
              }
            case CZ_df_save_record:
              {
                fout << CZ_df_save << " ";
                break;
              }
            case CZ_adf_save_record:
              {
                fout << CZ_adf_save << " ";
                break;
              }
            case Cm_record:
              {
                fout << Cm << " ";
                break;
              }
            case CmfaI_record:
              {
                fout << CmfaI << " ";
                break;
              }
            case CmfadeI_record:
              {
                fout << CmfadeI << " ";
                break;
              }
            case CmfdfI_record:
              {
                fout << CmfdfI << " ";
                break;
              }
            case CmfadfI_record:
              {
                fout << CmfadfI << " ";
                break;
              }
            case CmfabetafI_record:
              {
                fout << CmfabetafI << " ";
                break;
              }
            case CmfadefI_record:
              {
                fout << CmfadefI << " ";
                break;
              }
            case CmfaqfI_record:
              {
                fout << CmfaqfI << " ";
                break;
              }
            case Cmo_save_record:
              {
                fout << Cmo_save << " ";
                break;
              }
            case Cm_a_save_record:
              {
                fout << Cm_a_save << " ";
                break;
              }
            case Cm_a2_save_record:
              {
                fout << Cm_a2_save << " ";
                break;
              }
            case Cm_adot_save_record:
              {
                fout << Cm_adot_save << " ";
                break;
              }
            case Cm_q_save_record:
              {
                fout << Cm_q_save << " ";
                break;
              }
            case Cm_ih_save_record:
              {
                fout << Cm_ih_save << " ";
                break;
              }
            case Cm_de_save_record:
              {
                fout << Cm_de_save << " ";
                break;
              }
            case Cm_b2_save_record:
              {
                fout << Cm_b2_save << " ";
                break;
              }
            case Cm_r_save_record:
              {
                fout << Cm_r_save << " ";
                break;
              }
            case Cm_df_save_record:
              {
                fout << Cm_df_save << " ";
                break;
              }
            case Cm_ds_save_record:
              {
                fout << Cm_ds_save << " ";
                break;
              }
            case Cm_dg_save_record:
              {
                fout << Cm_dg_save << " ";
                break;
              }
            case CY_record:
              {
                fout << CY  << " ";
                break;
              }
            case CYfadaI_record:
              {
                fout << CYfadaI << " ";
                break;
              }
            case CYfbetadrI_record:
              {
                fout << CYfbetadrI << " ";
                break;
              }
	    case CYfabetafI_record:
	      {
		fout << CYfabetafI << " ";
		break;
	      }
	    case CYfadafI_record:
	      {
		fout << CYfadafI << " ";
		break;
	      }
	    case CYfadrfI_record:
	      {
		fout << CYfadrfI << " ";
		break;
	      }
	    case CYfapfI_record:
	      {
		fout << CYfapfI << " ";
		break;
	      }
	    case CYfarfI_record:
	      {
		fout << CYfarfI << " ";
		break;
	      }
	    case CYo_save_record:
	      {
		fout << CYo_save << " ";
		break;
	      }
	    case CY_beta_save_record:
	      {
		fout << CY_beta_save << " ";
		break;
	      }
	    case CY_p_save_record:
	      {
		fout << CY_p_save << " ";
		break;
	      }
	    case CY_r_save_record:
	      {
		fout << CY_r_save << " ";
		break;
	      }
	    case CY_da_save_record:
	      {
		fout << CY_da_save << " ";
		break;
	      }
	    case CY_dr_save_record:
	      {
		fout << CY_dr_save << " ";
		break;
	      }
	    case CY_dra_save_record:
	      {
		fout << CY_dra_save << " ";
		break;
	      }
	    case CY_bdot_save_record:
	      {
		fout << CY_bdot_save << " ";
		break;
	      }
            case Cl_record:
              {
                fout << Cl << " ";
                break;
              }
            case ClfadaI_record:
              {
                fout << ClfadaI << " ";
                break;
              }
            case ClfbetadrI_record:
              {
                fout << ClfbetadrI << " ";
                break;
              }
	    case ClfabetafI_record:
	      {
		fout << ClfabetafI << " ";
		break;
	      }
	    case ClfadafI_record:
	      {
		fout << ClfadafI << " ";
		break;
	      }
	    case ClfadrfI_record:
	      {
		fout << ClfadrfI << " ";
		break;
	      }
	    case ClfapfI_record:
	      {
		fout << ClfapfI << " ";
		break;
	      }
	    case ClfarfI_record:
	      {
		fout << ClfarfI << " ";
		break;
	      }
	    case Clo_save_record:
	      {
		fout << Clo_save << " ";
		break;
	      }
	    case Cl_beta_save_record:
	      {
		fout << Cl_beta_save << " ";
		break;
	      }
	    case Cl_p_save_record:
	      {
		fout << Cl_p_save << " ";
		break;
	      }
	    case Cl_r_save_record:
	      {
		fout << Cl_r_save << " ";
		break;
	      }
	    case Cl_da_save_record:
	      {
		fout << Cl_da_save << " ";
		break;
	      }
	    case Cl_dr_save_record:
	      {
		fout << Cl_dr_save << " ";
		break;
	      }
	    case Cl_daa_save_record:
	      {
		fout << Cl_daa_save << " ";
		break;
	      }
            case Cn_record:
              {
                fout << Cn << " ";
                break;
              }
            case CnfadaI_record:
              {
                fout << CnfadaI << " ";
                break;
              }
            case CnfbetadrI_record:
              {
                fout << CnfbetadrI << " ";
                break;
              }
	    case CnfabetafI_record:
	      {
		fout << CnfabetafI << " ";
		break;
	      }
	    case CnfadafI_record:
	      {
		fout << CnfadafI << " ";
		break;
	      }
	    case CnfadrfI_record:
	      {
		fout << CnfadrfI << " ";
		break;
	      }
	    case CnfapfI_record:
	      {
		fout << CnfapfI << " ";
		break;
	      }
	    case CnfarfI_record:
	      {
		fout << CnfarfI << " ";
		break;
	      }
	    case Cno_save_record:
	      {
		fout << Cno_save << " ";
		break;
	      }
	    case Cn_beta_save_record:
	      {
		fout << Cn_beta_save << " ";
		break;
	      }
	    case Cn_p_save_record:
	      {
		fout << Cn_p_save << " ";
		break;
	      }
	    case Cn_r_save_record:
	      {
		fout << Cn_r_save << " ";
		break;
	      }
	    case Cn_da_save_record:
	      {
		fout << Cn_da_save << " ";
		break;
	      }
	    case Cn_dr_save_record:
	      {
		fout << Cn_dr_save << " ";
		break;
	      }
	    case Cn_q_save_record:
	      {
		fout << Cn_q_save << " ";
		break;
	      }
	    case Cn_b3_save_record:
	      {
		fout << Cn_b3_save << " ";
		break;
	      }

              /******************** Ice Detection ********************/
	    case CL_clean_record:
	      {
		fout << CL_clean << " ";
		break;
	      }
	    case CL_iced_record:
	      {
		fout << CL_iced << " ";
		break;
	      }
	    case CD_clean_record:
	      {
		fout << CD_clean << " ";
		break;
	      }
	    case CD_iced_record:
	      {
		fout << CD_iced << " ";
		break;
	      }
	    case Cm_clean_record:
	      {
		fout << Cm_clean << " ";
		break;
	      }
	    case Cm_iced_record:
	      {
		fout << Cm_iced << " ";
		break;
	      }
	    case Ch_clean_record:
	      {
		fout << Ch_clean << " ";
		break;
	      }
	    case Ch_iced_record:
	      {
		fout << Ch_iced << " ";
		break;
	      }
	    case Cl_clean_record:
	      {
		fout << Cl_clean << " ";
		break;
	      }
	    case Cl_iced_record:
	      {
		fout << Cl_iced << " ";
		break;
	      }
            case CLclean_wing_record:
              {
                fout << CLclean_wing << " ";
                break;
              }
            case CLiced_wing_record:
              {
                fout << CLiced_wing << " ";
                break;
              }
            case CLclean_tail_record:
              {
                fout << CLclean_tail << " ";
                break;
              }
            case CLiced_tail_record:
              {
                fout << CLiced_tail << " ";
                break;
              }
            case Lift_clean_wing_record:
              {
                fout << Lift_clean_wing << " ";
                break;
              }
            case Lift_iced_wing_record:
              {
                fout << Lift_iced_wing << " ";
                break;
              }
            case Lift_clean_tail_record:
              {
                fout << Lift_clean_tail << " ";
                break;
              }
            case Lift_iced_tail_record:
              {
                fout << Lift_iced_tail << " ";
                break;
              }
            case Gamma_clean_wing_record:
              {
                fout << Gamma_clean_wing << " ";
                break;
              }
            case Gamma_iced_wing_record:
              {
                fout << Gamma_iced_wing << " ";
                break;
              }
            case Gamma_clean_tail_record:
              {
                fout << Gamma_clean_tail << " ";
                break;
              }
            case Gamma_iced_tail_record:
              {
                fout << Gamma_iced_tail << " ";
                break;
              }
            case w_clean_wing_record:
              {
                fout << w_clean_wing << " ";
                break;
              }
            case w_iced_wing_record:
              {
                fout << w_iced_wing << " ";
                break;
              }
            case w_clean_tail_record:
              {
                fout << w_clean_tail << " ";
                break;
              }
            case w_iced_tail_record:
              {
                fout << w_iced_tail << " ";
                break;
              }
            case V_total_clean_wing_record:
              {
                fout << V_total_clean_wing << " ";
                break;
              }
            case V_total_iced_wing_record:
              {
                fout << V_total_iced_wing << " ";
                break;
              }
            case V_total_clean_tail_record:
              {
                fout << V_total_clean_tail << " ";
                break;
              }
            case V_total_iced_tail_record:
              {
                fout << V_total_iced_tail << " ";
                break;
              }
            case beta_flow_clean_wing_record:
              {
                fout << beta_flow_clean_wing << " ";
                break;
              }
            case beta_flow_clean_wing_deg_record:
              {
                fout << beta_flow_clean_wing * RAD_TO_DEG << " ";
                break;
              }
            case beta_flow_iced_wing_record:
              {
                fout << beta_flow_iced_wing << " ";
                break;
              }
            case beta_flow_iced_wing_deg_record:
              {
                fout << beta_flow_iced_wing * RAD_TO_DEG << " ";
                break;
              }
            case beta_flow_clean_tail_record:
              {
                fout << beta_flow_clean_tail << " ";
                break;
              }
            case beta_flow_clean_tail_deg_record:
              {
                fout << beta_flow_clean_tail * RAD_TO_DEG << " ";
                break;
              }
            case beta_flow_iced_tail_record:
              {
                fout << beta_flow_iced_tail << " ";
                break;
              }
            case beta_flow_iced_tail_deg_record:
              {
                fout << beta_flow_iced_tail * RAD_TO_DEG << " ";
                break;
              }
            case Dbeta_flow_wing_record:
              {
                fout << Dbeta_flow_wing << " ";
                break;
              }
            case Dbeta_flow_wing_deg_record:
              {
                fout << Dbeta_flow_wing * RAD_TO_DEG << " ";
                break;
              }
            case Dbeta_flow_tail_record:
              {
                fout << Dbeta_flow_tail << " ";
                break;
              }
            case Dbeta_flow_tail_deg_record:
              {
                fout << Dbeta_flow_tail * RAD_TO_DEG << " ";
                break;
              }
            case pct_beta_flow_wing_record:
              {
                fout << pct_beta_flow_wing << " ";
                break;
              }
            case pct_beta_flow_tail_record:
              {
                fout << pct_beta_flow_tail << " ";
                break;
              }
	    case eta_ice_record:
	      {
	        fout << eta_ice << " ";
		break;
	      }
	    case eta_wing_left_record:
	      {
		fout << eta_wing_left << " ";
		break;
	      }
	    case eta_wing_right_record:
	      {
		fout << eta_wing_right << " ";
		break;
	      }
	    case eta_tail_record:
	      {
		fout << eta_tail << " ";
		break;
	      }
	    case delta_CL_record:
	      {
		fout << delta_CL << " ";
		break;
	      }
	    case delta_CD_record:
	      {
		fout << delta_CD << " ";
		break;
	      }
	    case delta_Cm_record:
	      {
		fout << delta_Cm << " ";
		break;
	      }
	    case delta_Cl_record:
	      {
		fout << delta_Cl << " ";
		break;
	      }
	    case delta_Cn_record:
	      {
		fout << delta_Cn << " ";
		break;
	      }
	    case boot_cycle_tail_record:
	      {
		fout << boot_cycle_tail << " ";
		break;
	      }
	    case boot_cycle_wing_left_record:
	      {
		fout << boot_cycle_wing_left << " ";
		break;
	      }
	    case boot_cycle_wing_right_record:
	      {
		fout << boot_cycle_wing_right << " ";
		break;
	      }
	    case autoIPS_tail_record:
	      {
		fout << autoIPS_tail << " ";
		break;
	      }
	    case autoIPS_wing_left_record:
	      {
		fout << autoIPS_wing_left << " ";
		break;
	      }
	    case autoIPS_wing_right_record:
	      {
		fout << autoIPS_wing_right << " ";
		break;
	      }
	    case eps_pitch_input_record:
	      {
		fout << eps_pitch_input << " ";
		break;
	      }
	    case eps_alpha_max_record:
	      {
		fout << eps_alpha_max << " ";
		break;
	      }
	    case eps_pitch_max_record:
	      {
		fout << eps_pitch_max << " ";
		break;
	      }
	    case eps_pitch_min_record:
	      {
		fout << eps_pitch_min << " ";
		break;
	      }
	    case eps_roll_max_record:
	      {
		fout << eps_roll_max << " ";
		break;
	      }
	    case eps_thrust_min_record:
	      {
		fout << eps_thrust_min << " ";
		break;
	      }
	    case eps_flap_max_record:
	      {
		fout << eps_flap_max << " ";
		break;
	      }
	    case eps_airspeed_max_record:
	      {
		fout << eps_airspeed_max << " ";
		break;
	      }
	    case eps_airspeed_min_record:
	      {
		fout << eps_airspeed_min << " ";
		break;
	      }

	      /****************** Autopilot **************************/
	    case ap_pah_on_record:
	      {
		fout << ap_pah_on << " ";
		break;
	      }
	    case ap_alh_on_record:
	      {
		fout << ap_alh_on << " ";
		break;
	      }
	    case ap_rah_on_record:
	      {
		fout << ap_rah_on << " ";
		break;
	      }
	    case ap_hh_on_record:
	      {
		fout << ap_hh_on << " ";
		break;
	      }
	    case ap_Theta_ref_deg_record:
	      {
		fout << ap_Theta_ref_rad*RAD_TO_DEG << " ";
		break;
	      }
	    case ap_Theta_ref_rad_record:
	      {
		fout << ap_Theta_ref_rad << " ";
		break;
	      }
	    case ap_alt_ref_ft_record:
	      {
		fout << ap_alt_ref_ft << " ";
		break;
	      }
	    case ap_Phi_ref_deg_record:
	      {
		fout << ap_Phi_ref_rad*RAD_TO_DEG << " ";
		break;
	      }
	    case ap_Phi_ref_rad_record:
	      {
		fout << ap_Phi_ref_rad << " ";
		break;
	      }
	    case ap_Psi_ref_deg_record:
	      {
		fout << ap_Psi_ref_rad*RAD_TO_DEG << " ";
		break;
	      }
	    case ap_Psi_ref_rad_record:
	      {
		fout << ap_Psi_ref_rad << " ";
		break;
	      }

              /************************ Forces ***********************/
            case F_X_wind_record:
              {
                fout << F_X_wind << " ";
                break;
              }
            case F_Y_wind_record:
              {
                fout << F_Y_wind << " ";
                break;
              }
            case F_Z_wind_record:
              {
                fout << F_Z_wind << " ";
                break;
              }
            case F_X_aero_record:
              {
                fout << F_X_aero << " ";
                break;
              }
            case F_Y_aero_record:
              {
                fout << F_Y_aero << " ";
                break;
              }
            case F_Z_aero_record:
              {
                fout << F_Z_aero << " ";
                break;
              }
            case F_X_engine_record:
              {
                fout << F_X_engine << " ";
                break;
              }
            case F_Y_engine_record:
              {
                fout << F_Y_engine << " ";
                break;
              }
            case F_Z_engine_record:
              {
                fout << F_Z_engine << " ";
                break;
              }
            case F_X_gear_record:
              {
                fout << F_X_gear << " ";
                break;
              }
            case F_Y_gear_record:
              {
                fout << F_Y_gear << " ";
                break;
              }
            case F_Z_gear_record:
              {
                fout << F_Z_gear << " ";
                break;
              }
            case F_X_record:
              {
                fout << F_X << " ";
                break;
              }
            case F_Y_record:
              {
                fout << F_Y << " ";
                break;
              }
            case F_Z_record:
              {
                fout << F_Z << " ";
                break;
              }
            case F_north_record:
              {
                fout << F_north << " ";
                break;
              }
            case F_east_record:
              {
                fout << F_east << " ";
                break;
              }
            case F_down_record:
              {
                fout << F_down << " ";
                break;
              }

              /*********************** Moments ***********************/
            case M_l_aero_record:
              {
                fout << M_l_aero << " ";
                break;
              }
            case M_m_aero_record:
              {
                fout << M_m_aero << " ";
                break;
              }
            case M_n_aero_record:
              {
                fout << M_n_aero << " ";
                break;
              }
            case M_l_engine_record:
              {
                fout << M_l_engine << " ";
                break;
              }
            case M_m_engine_record:
              {
                fout << M_m_engine << " ";
                break;
              }
            case M_n_engine_record:
              {
                fout << M_n_engine << " ";
                break;
              }
            case M_l_gear_record:
              {
                fout << M_l_gear << " ";
                break;
              }
            case M_m_gear_record:
              {
                fout << M_m_gear << " ";
                break;
              }
            case M_n_gear_record:
              {
                fout << M_n_gear << " ";
                break;
              }
            case M_l_rp_record:
              {
                fout << M_l_rp << " ";
                break;
              }
            case M_m_rp_record:
              {
                fout << M_m_rp << " ";
                break;
              }
            case M_n_rp_record:
              {
                fout << M_n_rp << " ";
                break;
              }
            case M_l_cg_record:
              {
                fout << M_l_cg << " ";
                break;
              }
            case M_m_cg_record:
              {
                fout << M_m_cg << " ";
                break;
              }
            case M_n_cg_record:
              {
                fout << M_n_cg << " ";
                break;
              }

              /********************* flapper *********************/
	    case flapper_freq_record:
	      {
		fout << flapper_freq << " ";
		break;
	      }
	    case flapper_phi_record:
	      {
		fout << flapper_phi << " ";
		break;
	      }
	    case flapper_phi_deg_record:
	      {
		fout << flapper_phi*RAD_TO_DEG << " ";
		break;
	      }
	    case flapper_Lift_record:
	      {
		fout << flapper_Lift << " ";
		break;
	      }
	    case flapper_Thrust_record:
	      {
		fout << flapper_Thrust << " ";
		break;
	      }
	    case flapper_Inertia_record:
	      {
		fout << flapper_Inertia << " ";
		break;
	      }
	    case flapper_Moment_record:
	      {
		fout << flapper_Moment << " ";
		break;
	      }
	      /****************Other Variables*******************/
	    case gyroMomentQ_record:
	      {
		fout << polarInertia * engineOmega * Q_body << " ";
		break;
	      }
	    case gyroMomentR_record:
	      {
		fout << -polarInertia * engineOmega * R_body << " ";
		break;
	      }
	    case eta_q_record:
	      {
		fout << eta_q << " ";
		break;
	      }
	    case rpm_record:
	      {
		fout << (engineOmega * 60 / (2 * LS_PI)) << " ";
		break;
	      }
	    case w_induced_record:
	      {
		fout << w_induced << " ";
		break;
	      }
	    case downwashAngle_deg_record:
	      {
		fout << downwashAngle * RAD_TO_DEG << " ";
		break;
	      }
	    case alphaTail_deg_record:
	      {
		fout << alphaTail * RAD_TO_DEG << " ";
		break;
	      }
	    case gammaWing_record:
	      {
		fout << gammaWing << " ";
		break;
	      }
	    case LD_record:
	      {
		fout << V_ground_speed/V_down_rel_ground  << " ";
		break;
	      }
	    case gload_record:
	      {
		fout << -A_Z_cg/32.174 << " ";
		break;
	      }
            case tactilefadefI_record:
              {
                fout << tactilefadefI << " ";
                break;
              }
	      /****************Trigger Variables*******************/
	    case trigger_on_record:
	      {
		fout << trigger_on << " ";
		break;
	      }
	    case trigger_num_record:
	      {
		fout << trigger_num << " ";
		break;
	      }
	    case trigger_toggle_record:
	      {
		fout << trigger_toggle << " ";
		break;
	      }
	    case trigger_counter_record:
	      {
		fout << trigger_counter << " ";
		break;
	      }
 	      /*********local to body transformation matrix********/
	    case T_local_to_body_11_record:
	      {
		fout << T_local_to_body_11 << " ";
		break;
	      }
	    case T_local_to_body_12_record:
	      {
		fout << T_local_to_body_12 << " ";
		break;
	      }
	    case T_local_to_body_13_record:
	      {
		fout << T_local_to_body_13 << " ";
		break;
	      }
	    case T_local_to_body_21_record:
	      {
		fout << T_local_to_body_21 << " ";
		break;
	      }
	    case T_local_to_body_22_record:
	      {
		fout << T_local_to_body_22 << " ";
		break;
	      }
	    case T_local_to_body_23_record:
	      {
		fout << T_local_to_body_23 << " ";
		break;
	      }
	    case T_local_to_body_31_record:
	      {
		fout << T_local_to_body_31 << " ";
		break;
	      }
	    case T_local_to_body_32_record:
	      {
		fout << T_local_to_body_32 << " ";
		break;
	      }
	    case T_local_to_body_33_record:
	      {
		fout << T_local_to_body_33 << " ";
		break;
	      }
    
             /********* MSS debug and other data *******************/
              /* debug variables for use in probing data            */
              /* comment out old lines, and add new                 */
              /* only remove code that you have written             */
	    case debug1_record:
	      {
		// eta_q term check
		// fout << eta_q_Cm_q_fac << " ";
		// fout << eta_q_Cm_adot_fac << " ";
		// fout << eta_q_Cmfade_fac << " ";
		// fout << eta_q_Cl_dr_fac << " ";
		// fout << eta_q_Cm_de_fac << " ";
		// eta on tail
		// fout << eta_q  << " ";
		// engine RPM
		// fout << engineOmega * 60 / (2 * LS_PI)<< " ";
		// vertical climb rate in fpm
		fout << V_down * 60 << " ";
		// vertical climb rate in fps
		// fout << V_down << " ";
		// w_induced downwash at tail due to wing
		// fout << gammaWing   << " ";
		//fout << outside_control << " ";
		break;
	      }
	    case debug2_record:
	      {
		// Lift to drag ratio 
		// fout <<  V_ground_speed/V_down_rel_ground << " ";
 		// g's through the c.g. of the aircraft
		fout << (-A_Z_cg/32.174) << " ";
		// L/D via forces (used in 201 class for L/D)
		// fout << (F_Z_wind/F_X_wind) << " ";
		// gyroscopic moment (see uiuc_wrapper.cpp)
		// fout << (polarInertia * engineOmega * Q_body) << " ";
		// downwashAngle at tail
		// fout << downwashAngle * 57.29 << " ";
		// w_induced from engine
		// fout << w_induced << " ";
		break;
	      }
	    case debug3_record:
	      {
		// die off function for eta_q
		// fout << (Cos_alpha * Cos_alpha)       << " ";
		// gyroscopic moment (see uiuc_wrapper.cpp)
		// fout << (-polarInertia * engineOmega * R_body) << " ";
		// eta on tail
		// fout << eta_q << " ";
		// flapper cycle percentage
		fout << (sin(flapper_phi - 3 * LS_PI / 2)) << " ";
		break;
	      }
              /********* RD debug and other data *******************/
              /* debug variables for use in probing data            */
              /* comment out old lines, and add new                 */
              /* only remove code that you have written             */
	    case debug4_record:
	      {
		// flapper F_X_aero_flapper
		//fout << F_X_aero_flapper << " ";
		//ap_pah_on
		//fout << ap_pah_on << " ";
		//D_cg_north1 = Radius_to_rwy*(Latitude - lat1);
		//fout << D_cg_north1 << " ";
		break;
	      }
	    case debug5_record:
	      {
		// flapper F_Z_aero_flapper
		//fout << F_Z_aero_flapper << " ";
		// gear_rate
		//D_cg_east1 = Radius_to_rwy*cos(lat1)*(Longitude - long1);
		//fout << D_cg_east1 << " ";
		break;
	      }
	    case debug6_record:
	      {
		//gear_max
		//fout << gear_max << " ";
		//fout << sqrt(D_cg_north1*D_cg_north1+D_cg_east1*D_cg_east1) << " ";
		break;
	      }
	    case debug7_record:
	      {
		//Debug7
		fout << debug7 << " ";
		break;
	      }
	    case debug8_record:
	      {
		//Debug8
		fout << debug8 << " ";
		break;
	      }
	    case debug9_record:
	      {
		//Debug9
		fout << debug9 << " ";
		break;
	      }
	    case debug10_record:
	      {
		//Debug10
		fout << debug10 << " ";
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
        } // end record map
    }
  recordStep++;
}

// end uiuc_recorder.cpp
