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

void uiuc_recorder(double dt )
{
  
  stack command_list;
  string linetoken;
  static int init = 0;
  string record_variables = "# ";
  LIST command_line;

  command_list = recordParts->getCommands();
  fout << endl;

  for ( command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
          record_variables += recordParts->getToken(*command_line,2) + "  ";

  fout << record_variables << endl; 
  for ( command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      
      linetoken = recordParts->getToken(*command_line, 2); 

      switch(record_map[linetoken])
        {
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
        case V_rel_wind_record:
          {
            fout << V_rel_wind << " ";
            break;
          }
        case Dynamic_pressure_record:
          {
            fout << Dynamic_pressure << " ";
            break;
          }
        case Alpha_record:
          {
            fout << Alpha << " ";
            break;
          }
        case Alpha_dot_record:
          {
            fout << Alpha_dot << " ";
            break;
          }
        case Beta_record:
          {
            fout << Beta << " ";
            break;
          }
        case Beta_dot_record:
          {
            fout << Beta_dot << " ";
            break;
          }
        case Gamma_record:
          {
            // fout << Gamma << " ";
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
        case Theta_dot_record:
          {
            fout << Theta_dot << " ";
            break;
          }
        case density_record:
          {
            fout << Density << " ";
            break;
          }
        case Mass_record:
          {
            fout << Mass << " ";
            break;
          }
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
        case elevator_record:
          {
            fout << elevator << " ";
            break;
          }
        case aileron_record:
          {
            fout << aileron << " ";
            break;
          }
        case rudder_record:
          {
            fout << rudder << " ";
            break;
          }
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
        case CDfadeI_record:
          {
            fout << CDfadeI << " ";
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
        case Cm_record:
          {
            fout << Cm << " ";
            break;
          }
        case CmfadeI_record:
          {
            fout << CmfadeI << " ";
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

// end uiuc_recorder.cpp
