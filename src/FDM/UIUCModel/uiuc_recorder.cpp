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

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.

**********************************************************************/

#include "uiuc_recorder.h"

FG_USING_STD(endl);		// -dw

void uiuc_recorder( double dt )
{
  stack command_list;
  string linetoken;
  static int init = 0;
  static int recordStep = 0;
  string record_variables = "# ";

  int modulus = recordStep % recordRate;

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
                fout << Alpha << " ";
                break;
              }
            case Alpha_deg_record:
              {
                fout << Alpha * RAD_TO_DEG << " ";
                break;
              }
            case Alpha_dot_record:
              {
                fout << Alpha_dot << " ";
                break;
              }
            case Alpha_dot_deg_record:
              {
                fout << Alpha_dot * RAD_TO_DEG << " ";
                break;
              }
            case Beta_record:
              {
                fout << Beta << " ";
                break;
              }
            case Beta_deg_record:
              {
                fout << Beta * RAD_TO_DEG << " ";
                break;
              }
            case Beta_dot_record:
              {
                fout << Beta_dot << " ";
                break;
              }
            case Beta_dot_deg_record:
              {
                fout << Beta_dot * RAD_TO_DEG << " ";
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

              /******************** Control Inputs *******************/
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
            case Lat_control_record:
              {
                fout << Long_control << " ";
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
            case Rudder_pedal_record:
              {
                fout << Long_control << " ";
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
            case Flap_handle_record:
              {
                fout << flap << " ";
                break;
              }
            case flap_record:
              {
                fout << flap << " ";
                break;
              }
            case flap_deg_record:
              {
                fout << flap * RAD_TO_DEG << " ";
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

              /******************** Ice Detection ********************/
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
            };
        } // end record map
    }
  recordStep++;
}

// end uiuc_recorder.cpp
