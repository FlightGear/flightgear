/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu.cpp

----------------------------------------------------------------------

 DESCRIPTION:  reads input file for specified aircraft and creates 
               approporiate data storage space

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   based on "menu reader" format of Michael Selig

----------------------------------------------------------------------

 HISTORY:      01/29/2000   initial release
               02/18/2000   (JS) added 1D data file capability for 
                            CL(a) and CD(a) -- calls 
                            uiuc_1DdataFileReader
               02/22/2000   (JS) added ice map functions
               02/29/2000   (JS) added 2D data file capability for 
                            CL(a,de), CD(a,de), Cm(a,de), CY(a,da), 
                            CY(beta,dr), Cl(a,da), Cl(beta,dr), 
                            Cn(a,da), Cn(beta,dr) -- calls 
                            uiuc_2DdataFileReader
               02/02/2000   (JS) added record options for 1D and 
                            2D interpolated variables
               03/28/2000   (JS) streamlined conversion factors for 
                            file readers -- since they are global 
                            variables, it is not necessary to put 
                            them in the function calls
               03/29/2000   (JS) added Cmfa and Weight flags;
                            added misc map; added Dx_cg (etc) to init 
                            map
               04/01/2000   (JS) added throttle, longitudinal, lateral, 
                            and rudder inputs to record map
               04/05/2000   (JS) added Altitude to init and record
                            maps; added zero_Long_trim to 
                            controlSurface map

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
               Michael Selig      <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       n/a

----------------------------------------------------------------------

 OUTPUTS:      n/a

----------------------------------------------------------------------

 CALLED BY:    uiuc_wrapper.cpp 

----------------------------------------------------------------------

 CALLS TO:     aircraft.dat
               tabulated data files (if needed)

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

#include <simgear/compiler.h>

#if defined( __MWERKS__ )
// -dw- optimizer chokes (big-time) trying to optimize humongous
// loop/switch statements
#pragma optimization_level 0
#endif

#include <stdlib.h>
#include <iostream>

#include "uiuc_menu.h"

FG_USING_STD(cerr);
FG_USING_STD(cout);
FG_USING_STD(endl);

#ifndef _MSC_VER
FG_USING_STD(exit);
#endif

bool check_float(string  &token)
{
  float value;
  istrstream stream(token.c_str()); 
  return (stream >> value);
}


void uiuc_menu( string aircraft_name )
{
  stack command_list;
  double token_value;
  int token_value_recordRate;
  int token_value_convert1, token_value_convert2, token_value_convert3;
 
  string linetoken1;
  string linetoken2;
  string linetoken3;
  string linetoken4;
  string linetoken5;
  string linetoken6;


  /* the following default setting should eventually be moved to a default or uiuc_init routine */

  recordRate = 1;       /* record every time step, default */
  recordStartTime = 0;  /* record from beginning of simulation */

/* set speed at which dynamic pressure terms will be accounted for,
   since if velocity is too small, coefficients will go to infinity */
  dyn_on_speed = 33;    /* 20 kts, default */



  /* Read the file and get the list of commands */
  airplane = new ParseFile(aircraft_name); /* struct that includes all lines of the input file */
  command_list = airplane -> getCommands();
  /* structs to include all parts included in the input file for specific keyword groups */
  initParts          = new ParseFile();
  geometryParts      = new ParseFile();
  massParts          = new ParseFile();
  engineParts        = new ParseFile();
  aeroDragParts      = new ParseFile();
  aeroLiftParts      = new ParseFile();
  aeroPitchParts     = new ParseFile();
  aeroSideforceParts = new ParseFile();
  aeroRollParts      = new ParseFile();
  aeroYawParts       = new ParseFile();
  gearParts          = new ParseFile();
  recordParts        = new ParseFile();

  if (command_list.begin() == command_list.end())
  {
    cerr << "UIUC ERROR: File " << aircraft_name <<" does not exist" << endl;
    exit(-1);
  }
  
  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      cout << *command_line << endl;

      linetoken1 = airplane -> getToken (*command_line, 1); 
      linetoken2 = airplane -> getToken (*command_line, 2); 
      linetoken3 = airplane -> getToken (*command_line, 3); 
      linetoken4 = airplane -> getToken (*command_line, 4); 
      linetoken5 = airplane -> getToken (*command_line, 5); 
      linetoken6 = airplane -> getToken (*command_line, 6); 
      
      istrstream token3(linetoken3.c_str());
      istrstream token4(linetoken4.c_str());
      istrstream token5(linetoken5.c_str());
      istrstream token6(linetoken6.c_str());

      switch (Keyword_map[linetoken1])
        {
        case init_flag:
          {
            switch(init_map[linetoken2])
              {
              case Dx_pilot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Dx_pilot = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Dy_pilot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Dy_pilot = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Dz_pilot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Dz_pilot = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Dx_cg_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Dx_cg = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Dy_cg_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Dy_cg = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Dz_cg_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Dz_cg = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Altitude_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Altitude = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case V_north_flag:
                {
                  if (check_float(linetoken3)) 
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  V_north = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case V_east_flag:
                {
                  initParts -> storeCommands (*command_line);
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  V_east = token_value;
                  break;
                }
              case V_down_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  V_down = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case P_body_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  P_body = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Q_body_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Q_body = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case R_body_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  R_body = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Phi_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Phi = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Theta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Theta = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Psi_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Psi = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Long_trim_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Long_trim = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case recordRate_flag:
                {
                  //can't use check_float since variable is integer
                  token3 >> token_value_recordRate;
                  recordRate = 120 / token_value_recordRate;
                  break;
                }
              case recordStartTime_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  recordStartTime = token_value;
                  break;
                }
              case nondim_rate_V_rel_wind_flag:
                {
                  nondim_rate_V_rel_wind = true;
                  break;
                }
              case dyn_on_speed_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  dyn_on_speed = token_value;
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end init map
          
          
        case geometry_flag:
          {
            switch(geometry_map[linetoken2])
              {
              case bw_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  bw = token_value;
                  geometryParts -> storeCommands (*command_line);
                  break;
                }
              case cbar_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  cbar = token_value;
                  geometryParts -> storeCommands (*command_line);
                  break;
                }
              case Sw_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Sw = token_value;
                  geometryParts -> storeCommands (*command_line);
                  break;
                }
              case ih_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  ih = token_value;
                  geometryParts -> storeCommands (*command_line);
                  break;
                }
              case bh_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  bh = token_value;
                  geometryParts -> storeCommands (*command_line);
                  break;
                }
              case ch_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  ch = token_value;
                  geometryParts -> storeCommands (*command_line);
                  break;
                }
              case Sh_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Sh = token_value;
                  geometryParts -> storeCommands (*command_line);
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end geometry map


        case controlSurface_flag:
          {
            switch(controlSurface_map[linetoken2])
              {
              case de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  demax = token_value;
                  
                  if (check_float(linetoken4))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  demin = token_value;
                  break;
                }
              case da_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  damax = token_value;
                  
                  if (check_float(linetoken4))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  damin = token_value;
                  break;
                }
              case dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  drmax = token_value;
                  
                  if (check_float(linetoken4))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  drmin = token_value;
                  break;
                }
              case set_Long_trim_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  set_Long_trim = true;
                  elevator_tab = token_value;
                  break;
                }
              case set_Long_trim_deg_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  set_Long_trim = true;
                  elevator_tab = token_value * DEG_TO_RAD;
                  break;
                }
              case zero_Long_trim_flag:
                {
                  zero_Long_trim = true;
                  break;
                }
              case elevator_step_flag:
                {
                  // set step input flag
                  elevator_step = true;

                  // read in step angle in degrees and convert
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  elevator_step_angle = token_value * DEG_TO_RAD;

                  // read in step start time
                  if (check_float(linetoken4))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  elevator_step_startTime = token_value;
                  break;
                }
              case elevator_singlet_flag:
                {
                  // set singlet input flag
                  elevator_singlet = true;

                  // read in singlet angle in degrees and convert
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  elevator_singlet_angle = token_value * DEG_TO_RAD;

                  // read in singlet start time
                  if (check_float(linetoken4))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  elevator_singlet_startTime = token_value;

                  // read in singlet duration
                  if (check_float(linetoken5))
                    token5 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  elevator_singlet_duration = token_value;
                  break;
                }
              case elevator_doublet_flag:
                {
                  // set doublet input flag
                  elevator_doublet = true;

                  // read in doublet angle in degrees and convert
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  elevator_doublet_angle = token_value * DEG_TO_RAD;

                  // read in doublet start time
                  if (check_float(linetoken4))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  elevator_doublet_startTime = token_value;

                  // read in doublet duration
                  if (check_float(linetoken5))
                    token5 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  elevator_doublet_duration = token_value;
                  break;
                }
              case elevator_input_flag:
                {
                  elevator_input = true;
                  elevator_input_file = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  convert_y = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  uiuc_1DdataFileReader(elevator_input_file,
                                        elevator_input_timeArray,
                                        elevator_input_deArray,
                                        elevator_input_ntime);
                  token6 >> token_value;
                  elevator_input_startTime = token_value;
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end controlSurface map


        case mass_flag:
          {
            switch(mass_map[linetoken2])
              {
              case Weight_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Weight = token_value;
                  Mass = Weight * INVG;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case Mass_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Mass = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case I_xx_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  I_xx = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case I_yy_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  I_yy = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case I_zz_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  I_zz = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case I_xz_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  I_xz = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end mass map
          
          
        case engine_flag:
          {
            switch(engine_map[linetoken2])
              {
              case simpleSingle_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  simpleSingleMaxThrust = token_value; 
                  engineParts -> storeCommands (*command_line);
                  break;
                }
              case c172_flag:
                {
                  engineParts -> storeCommands (*command_line);
                  break;
                }
              case cherokee_flag:
                {
                  engineParts -> storeCommands (*command_line);
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end engine map
          
          
        case CD_flag:
          {
            switch(CD_map[linetoken2])
              {
              case CDo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CDo = token_value;
                  CDo_clean = CDo;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CDK_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CDK = token_value;
                  CDK_clean = CDK;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CD_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CD_a = token_value;
                  CD_a_clean = CD_a;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CD_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CD_adot = token_value;
                  CD_adot_clean = CD_adot;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CD_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CD_q = token_value;
                  CD_q_clean = CD_q;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CD_ih_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CD_ih = token_value;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CD_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CD_de = token_value;
                  CD_de_clean = CD_de;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CDfa_flag:
                {
                  CDfa = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  convert_y = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  /* call 1D File Reader with file name (CDfa) and conversion 
                     factors; function returns array of alphas (aArray) and 
                     corresponding CD values (CDArray) and max number of 
                     terms in arrays (nAlpha) */
                  uiuc_1DdataFileReader(CDfa,
                                        CDfa_aArray,
                                        CDfa_CDArray,
                                        CDfa_nAlpha);
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CDfCL_flag:
                {
                  CDfCL = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  convert_y = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  /* call 1D File Reader with file name (CDfCL) and conversion 
                     factors; function returns array of CLs (CLArray) and 
                     corresponding CD values (CDArray) and max number of 
                     terms in arrays (nCL) */
                  uiuc_1DdataFileReader(CDfCL,
                                        CDfCL_CLArray,
                                        CDfCL_CDArray,
                                        CDfCL_nCL);
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CDfade_flag:
                {
                  CDfade = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (CDfade) and 
                     conversion factors; function returns array of 
                     elevator deflections (deArray) and corresponding 
                     alpha (aArray) and delta CD (CDArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nde) */
                  uiuc_2DdataFileReader(CDfade,
                                        CDfade_aArray,
                                        CDfade_deArray,
                                        CDfade_CDArray,
                                        CDfade_nAlphaArray,
                                        CDfade_nde);
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CDfdf_flag:
                {
                  CDfdf = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  convert_y = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  /* call 1D File Reader with file name (CDfdf) and conversion 
                     factors; function returns array of dfs (dfArray) and 
                     corresponding CD values (CDArray) and max number of 
                     terms in arrays (ndf) */
                  uiuc_1DdataFileReader(CDfdf,
                                        CDfdf_dfArray,
                                        CDfdf_CDArray,
                                        CDfdf_ndf);
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CDfadf_flag:
                {
                  CDfadf = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (CDfadf) and 
                     conversion factors; function returns array of 
                     flap deflections (dfArray) and corresponding 
                     alpha (aArray) and delta CD (CDArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (ndf) */
                  uiuc_2DdataFileReader(CDfadf,
                                        CDfadf_aArray,
                                        CDfadf_dfArray,
                                        CDfadf_CDArray,
                                        CDfadf_nAlphaArray,
                                        CDfadf_ndf);
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CXo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CXo = token_value;
                  CXo_clean = CXo;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CXK_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CXK = token_value;
                  CXK_clean = CXK;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CX_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CX_a = token_value;
                  CX_a_clean = CX_a;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CX_a2_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CX_a2 = token_value;
                  CX_a2_clean = CX_a2;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CX_a3_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CX_a3 = token_value;
                  CX_a3_clean = CX_a3;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CX_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CX_adot = token_value;
                  CX_adot_clean = CX_adot;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CX_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CX_q = token_value;
                  CX_q_clean = CX_q;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CX_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CX_de = token_value;
                  CX_de_clean = CX_de;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CX_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CX_dr = token_value;
                  CX_dr_clean = CX_dr;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CX_df_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CX_df = token_value;
                  CX_df_clean = CX_df;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              case CX_adf_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CX_adf = token_value;
                  CX_adf_clean = CX_adf;
                  aeroDragParts -> storeCommands (*command_line);
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end CD map

          
        case CL_flag:
          {
            switch(CL_map[linetoken2])
              {
              case CLo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CLo = token_value;
                  CLo_clean = CLo;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CL_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CL_a = token_value;
                  CL_a_clean = CL_a;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CL_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CL_adot = token_value;
                  CL_adot_clean = CL_adot;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CL_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CL_q = token_value;
                  CL_q_clean = CL_q;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CL_ih_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CL_ih = token_value;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CL_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CL_de = token_value;
                  CL_de_clean = CL_de;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CLfa_flag:
                {
                  CLfa = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  convert_y = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  /* call 1D File Reader with file name (CLfa) and conversion 
                     factors; function returns array of alphas (aArray) and 
                     corresponding CL values (CLArray) and max number of 
                     terms in arrays (nAlpha) */
                  uiuc_1DdataFileReader(CLfa,
                                        CLfa_aArray,
                                        CLfa_CLArray,
                                        CLfa_nAlpha);
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CLfade_flag:
                {
                  CLfade = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (CLfade) and 
                     conversion factors; function returns array of 
                     elevator deflections (deArray) and corresponding 
                     alpha (aArray) and delta CL (CLArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nde) */
                  uiuc_2DdataFileReader(CLfade,
                                        CLfade_aArray,
                                        CLfade_deArray,
                                        CLfade_CLArray,
                                        CLfade_nAlphaArray,
                                        CLfade_nde);
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CLfdf_flag:
                {
                  CLfdf = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  convert_y = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  /* call 1D File Reader with file name (CLfdf) and conversion 
                     factors; function returns array of dfs (dfArray) and 
                     corresponding CL values (CLArray) and max number of 
                     terms in arrays (ndf) */
                  uiuc_1DdataFileReader(CLfdf,
                                        CLfdf_dfArray,
                                        CLfdf_CLArray,
                                        CLfdf_ndf);
                  aeroLiftParts -> storeCommands (*command_line);

                  // additional variables to streamline flap routine in aerodeflections
                  ndf = CLfdf_ndf;
                  int temp_counter = 1;
                  while (temp_counter <= ndf)
                    {
                      dfArray[temp_counter] = CLfdf_dfArray[temp_counter];
                      TimeArray[temp_counter] = dfTimefdf_TimeArray[temp_counter];
                      temp_counter++;
                    }
                  break;
                }
              case CLfadf_flag:
                {
                  CLfadf = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (CLfadf) and 
                     conversion factors; function returns array of 
                     flap deflections (dfArray) and corresponding 
                     alpha (aArray) and delta CL (CLArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (ndf) */
                  uiuc_2DdataFileReader(CLfadf,
                                        CLfadf_aArray,
                                        CLfadf_dfArray,
                                        CLfadf_CLArray,
                                        CLfadf_nAlphaArray,
                                        CLfadf_ndf);
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CZo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CZo = token_value;
                  CZo_clean = CZo;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CZ_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CZ_a = token_value;
                  CZ_a_clean = CZ_a;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CZ_a2_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CZ_a2 = token_value;
                  CZ_a2_clean = CZ_a2;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CZ_a3_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  CZ_a3 = token_value;
                  CZ_a3_clean = CZ_a3;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CZ_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CZ_adot = token_value;
                  CZ_adot_clean = CZ_adot;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CZ_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CZ_q = token_value;
                  CZ_q_clean = CZ_q;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CZ_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CZ_de = token_value;
                  CZ_de_clean = CZ_de;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CZ_deb2_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CZ_deb2 = token_value;
                  CZ_deb2_clean = CZ_deb2;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CZ_df_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CZ_df = token_value;
                  CZ_df_clean = CZ_df;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              case CZ_adf_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CZ_adf = token_value;
                  CZ_adf_clean = CZ_adf;
                  aeroLiftParts -> storeCommands (*command_line);
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end CL map


        case Cm_flag:
          {
            switch(Cm_map[linetoken2])
              {
              case Cmo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cmo = token_value;
                  Cmo_clean = Cmo;
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cm_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cm_a = token_value;
                  Cm_a_clean = Cm_a;
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cm_a2_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cm_a2 = token_value;
                  Cm_a2_clean = Cm_a2;
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cm_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cm_adot = token_value;
                  Cm_adot_clean = Cm_adot;
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cm_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cm_q = token_value;
                  Cm_q_clean = Cm_q;
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cm_ih_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cm_ih = token_value;
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cm_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cm_de = token_value;
                  Cm_de_clean = Cm_de;
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cm_b2_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cm_b2 = token_value;
                  Cm_b2_clean = Cm_b2;
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cm_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cm_r = token_value;
                  Cm_r_clean = Cm_r;
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cm_df_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cm_df = token_value;
                  Cm_df_clean = Cm_df;
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cmfa_flag:
                {
                  Cmfa = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  convert_y = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  /* call 1D File Reader with file name (Cmfa) and conversion 
                     factors; function returns array of alphas (aArray) and 
                     corresponding Cm values (CmArray) and max number of 
                     terms in arrays (nAlpha) */
                  uiuc_1DdataFileReader(Cmfa,
                                        Cmfa_aArray,
                                        Cmfa_CmArray,
                                        Cmfa_nAlpha);
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cmfade_flag:
                {
                  Cmfade = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (Cmfade) and 
                     conversion factors; function returns array of 
                     elevator deflections (deArray) and corresponding 
                     alpha (aArray) and delta Cm (CmArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nde) */
                  uiuc_2DdataFileReader(Cmfade,
                                        Cmfade_aArray,
                                        Cmfade_deArray,
                                        Cmfade_CmArray,
                                        Cmfade_nAlphaArray,
                                        Cmfade_nde);
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cmfdf_flag:
                {
                  Cmfdf = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  convert_y = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  /* call 1D File Reader with file name (Cmfdf) and conversion 
                     factors; function returns array of dfs (dfArray) and 
                     corresponding Cm values (CmArray) and max number of 
                     terms in arrays (ndf) */
                  uiuc_1DdataFileReader(Cmfdf,
                                        Cmfdf_dfArray,
                                        Cmfdf_CmArray,
                                        Cmfdf_ndf);
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              case Cmfadf_flag:
                {
                  Cmfadf = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (Cmfadf) and 
                     conversion factors; function returns array of 
                     flap deflections (dfArray) and corresponding 
                     alpha (aArray) and delta Cm (CmArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (ndf) */
                  uiuc_2DdataFileReader(Cmfadf,
                                        Cmfadf_aArray,
                                        Cmfadf_dfArray,
                                        Cmfadf_CmArray,
                                        Cmfadf_nAlphaArray,
                                        Cmfadf_ndf);
                  aeroPitchParts -> storeCommands (*command_line);
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end Cm map


        case CY_flag:
          {
            switch(CY_map[linetoken2])
              {
              case CYo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CYo = token_value;
                  CYo_clean = CYo;
                  aeroSideforceParts -> storeCommands (*command_line);
                  break;
                }
              case CY_beta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CY_beta = token_value;
                  CY_beta_clean = CY_beta;
                  aeroSideforceParts -> storeCommands (*command_line);
                  break;
                }
              case CY_p_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CY_p = token_value;
                  CY_p_clean = CY_p;
                  aeroSideforceParts -> storeCommands (*command_line);
                  break;
                }
              case CY_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CY_r = token_value;
                  CY_r_clean = CY_r;
                  aeroSideforceParts -> storeCommands (*command_line);
                  break;
                }
              case CY_da_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  CY_da = token_value;
                  CY_da_clean = CY_da;
                  aeroSideforceParts -> storeCommands (*command_line);
                  break;
                }
              case CY_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(2, *command_line);

                  CY_dr = token_value;
                  CY_dr_clean = CY_dr;
                  aeroSideforceParts -> storeCommands (*command_line);
                  break;
                }
              case CY_dra_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(2, *command_line);

                  CY_dra = token_value;
                  CY_dra_clean = CY_dra;
                  aeroSideforceParts -> storeCommands (*command_line);
                  break;
                }
              case CY_bdot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(2, *command_line);

                  CY_bdot = token_value;
                  CY_bdot_clean = CY_bdot;
                  aeroSideforceParts -> storeCommands (*command_line);
                  break;
                }
              case CYfada_flag:
                {
                  CYfada = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (CYfada) and 
                     conversion factors; function returns array of 
                     aileron deflections (daArray) and corresponding 
                     alpha (aArray) and delta CY (CYArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nda) */
                  uiuc_2DdataFileReader(CYfada,
                                        CYfada_aArray,
                                        CYfada_daArray,
                                        CYfada_CYArray,
                                        CYfada_nAlphaArray,
                                        CYfada_nda);
                  aeroSideforceParts -> storeCommands (*command_line);
                  break;
                }
              case CYfbetadr_flag:
                {
                  CYfbetadr = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (CYfbetadr) and 
                     conversion factors; function returns array of 
                     rudder deflections (drArray) and corresponding 
                     beta (betaArray) and delta CY (CYArray) values and 
                     max number of terms in beta arrays (nBetaArray) 
                     and deflection array (ndr) */
                  uiuc_2DdataFileReader(CYfbetadr,
                                        CYfbetadr_betaArray,
                                        CYfbetadr_drArray,
                                        CYfbetadr_CYArray,
                                        CYfbetadr_nBetaArray,
                                        CYfbetadr_ndr);
                  aeroSideforceParts -> storeCommands (*command_line);
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end CY map


        case Cl_flag:
          {
            switch(Cl_map[linetoken2])
              {
              case Clo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Clo = token_value;
                  Clo_clean = Clo;
                  aeroRollParts -> storeCommands (*command_line);
                  break;
                }
              case Cl_beta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cl_beta = token_value;
                  Cl_beta_clean = Cl_beta;
                  aeroRollParts -> storeCommands (*command_line);
                  break;
                }
              case Cl_p_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cl_p = token_value;
                  Cl_p_clean = Cl_p;
                  aeroRollParts -> storeCommands (*command_line);
                  break;
                }
              case Cl_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cl_r = token_value;
                  Cl_r_clean = Cl_r;
                  aeroRollParts -> storeCommands (*command_line);
                  break;
                }
              case Cl_da_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cl_da = token_value;
                  Cl_da_clean = Cl_da;
                  aeroRollParts -> storeCommands (*command_line);
                  break;
                }
              case Cl_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cl_dr = token_value;
                  Cl_dr_clean = Cl_dr;
                  aeroRollParts -> storeCommands (*command_line);
                  break;
                }
              case Cl_daa_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cl_daa = token_value;
                  Cl_daa_clean = Cl_daa;
                  aeroRollParts -> storeCommands (*command_line);
                  break;
                }
              case Clfada_flag:
                {
                  Clfada = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (Clfada) and 
                     conversion factors; function returns array of 
                     aileron deflections (daArray) and corresponding 
                     alpha (aArray) and delta Cl (ClArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nda) */
                  uiuc_2DdataFileReader(Clfada,
                                        Clfada_aArray,
                                        Clfada_daArray,
                                        Clfada_ClArray,
                                        Clfada_nAlphaArray,
                                        Clfada_nda);
                  aeroRollParts -> storeCommands (*command_line);
                  break;
                }
              case Clfbetadr_flag:
                {
                  Clfbetadr = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (Clfbetadr) and 
                     conversion factors; function returns array of 
                     rudder deflections (drArray) and corresponding 
                     beta (betaArray) and delta Cl (ClArray) values and 
                     max number of terms in beta arrays (nBetaArray) 
                     and deflection array (ndr) */
                  uiuc_2DdataFileReader(Clfbetadr,
                                        Clfbetadr_betaArray,
                                        Clfbetadr_drArray,
                                        Clfbetadr_ClArray,
                                        Clfbetadr_nBetaArray,
                                        Clfbetadr_ndr);
                  aeroRollParts -> storeCommands (*command_line);
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
          break;
                }
              };
            break;
          } // end Cl map


        case Cn_flag:
          {
            switch(Cn_map[linetoken2])
              {
              case Cno_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cno = token_value;
                  Cno_clean = Cno;
                  aeroYawParts -> storeCommands (*command_line);
                  break;
                }
              case Cn_beta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cn_beta = token_value;
                  Cn_beta_clean = Cn_beta;
                  aeroYawParts -> storeCommands (*command_line);
                  break;
                }
              case Cn_p_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cn_p = token_value;
                  Cn_p_clean = Cn_p;
                  aeroYawParts -> storeCommands (*command_line);
                  break;
                }
              case Cn_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cn_r = token_value;
                  Cn_r_clean = Cn_r;
                  aeroYawParts -> storeCommands (*command_line);
                  break;
                }
              case Cn_da_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cn_da = token_value;
                  Cn_da_clean = Cn_da;
                  aeroYawParts -> storeCommands (*command_line);
                  break;
                }
              case Cn_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cn_dr = token_value;
                  Cn_dr_clean = Cn_dr;
                  aeroYawParts -> storeCommands (*command_line);
                  break;
                }
              case Cn_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cn_q = token_value;
                  Cn_q_clean = Cn_q;
                  aeroYawParts -> storeCommands (*command_line);
                  break;
                }
              case Cn_b3_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  Cn_b3 = token_value;
                  Cn_b3_clean = Cn_b3;
                  aeroYawParts -> storeCommands (*command_line);
                  break;
                }
              case Cnfada_flag:
                {
                  Cnfada = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (Cnfada) and 
                     conversion factors; function returns array of 
                     aileron deflections (daArray) and corresponding 
                     alpha (aArray) and delta Cn (CnArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nda) */
                  uiuc_2DdataFileReader(Cnfada,
                                        Cnfada_aArray,
                                        Cnfada_daArray,
                                        Cnfada_CnArray,
                                        Cnfada_nAlphaArray,
                                        Cnfada_nda);
                  aeroYawParts -> storeCommands (*command_line);
                  break;
                }
              case Cnfbetadr_flag:
                {
                  Cnfbetadr = linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  token6 >> token_value_convert3;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
                  /* call 2D File Reader with file name (Cnfbetadr) and 
                     conversion factors; function returns array of 
                     rudder deflections (drArray) and corresponding 
                     beta (betaArray) and delta Cn (CnArray) values and 
                     max number of terms in beta arrays (nBetaArray) 
                     and deflection array (ndr) */
                  uiuc_2DdataFileReader(Cnfbetadr,
                                        Cnfbetadr_betaArray,
                                        Cnfbetadr_drArray,
                                        Cnfbetadr_CnArray,
                                        Cnfbetadr_nBetaArray,
                                        Cnfbetadr_ndr);
                  aeroYawParts -> storeCommands (*command_line);
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end Cn map
          
        
     /*   
        
        case gear_flag:
          {
            switch(gear_map[linetoken2])
              {
              case kgear:
                {
                  // none yet
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
          break;
                }
              };
          } // end gear map
      
*/


        case ice_flag:
          {
            switch(ice_map[linetoken2])
              {
              case iceTime_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;

                  else
                    uiuc_warnings_errors(1, *command_line);

                  ice_model = true;
                  iceTime = token_value;
                  break;
                }
              case transientTime_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  transientTime = token_value;
                  break;
                }
              case eta_ice_final_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  eta_ice_final = token_value;
                  break;
                }
              case beta_probe_wing_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  beta_model = true;
                  x_probe_wing = token_value;
                  break;
                }
              case beta_probe_tail_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  beta_model = true;
                  x_probe_tail = token_value;
                  break;
                }
              case kCDo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCDo = token_value;
                  break;
                }
              case kCDK_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCDK = token_value;
                  break;
                }
              case kCD_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCD_a = token_value;
                  break;
                }
              case kCD_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCD_adot = token_value;
                  break;
                }
              case kCD_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCD_q = token_value;
                  break;
                }
              case kCD_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCD_de = token_value;
                  break;
                }
              case kCXo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCXo = token_value;
                  break;
                }
              case kCXK_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCXK = token_value;
                  break;
                }
              case kCX_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCX_a = token_value;
                  break;
                }
              case kCX_a2_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCX_a2 = token_value;
                  break;
                }
              case kCX_a3_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCX_a3 = token_value;
                  break;
                }
              case kCX_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCX_adot = token_value;
                  break;
                }
              case kCX_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCX_q = token_value;
                  break;
                }
              case kCX_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCX_de = token_value;
                  break;
                }
              case kCX_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCX_dr = token_value;
                  break;
                }
              case kCX_df_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCX_df = token_value;
                  break;
                }
              case kCX_adf_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCX_adf = token_value;
                  break;
                }
              case kCLo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCLo = token_value;
                  break;
                }
              case kCL_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCL_a = token_value;
                  break;
                }
              case kCL_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCL_adot = token_value;
                  break;
                }
              case kCL_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCL_q = token_value;
                  break;
                }
              case kCL_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCL_de = token_value;
                  break;
                }
              case kCZo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCZo = token_value;
                  break;
                }
              case kCZ_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCZ_a = token_value;
                  break;
                }
              case kCZ_a2_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCZ_a2 = token_value;
                  break;
                }
              case kCZ_a3_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCZ_a3 = token_value;
                  break;
                }
              case kCZ_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCZ_adot = token_value;
                  break;
                }
              case kCZ_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCZ_q = token_value;
                  break;
                }
              case kCZ_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCZ_de = token_value;
                  break;
                }
              case kCZ_deb2_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCZ_deb2 = token_value;
                  break;
                }
              case kCZ_df_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCZ_df = token_value;
                  break;
                }
              case kCZ_adf_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCZ_adf = token_value;
                  break;
                }
              case kCmo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCmo = token_value;
                  break;
                }
              case kCm_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_a = token_value;
                  break;
                }
              case kCm_a2_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_a2 = token_value;
                  break;
                }
              case kCm_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_adot = token_value;
                  break;
                }
              case kCm_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_q = token_value;
                  break;
                }
              case kCm_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_de = token_value;
                  break;
                }
              case kCm_b2_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_b2 = token_value;
                  break;
                }
              case kCm_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_r = token_value;
                  break;
                }
              case kCm_df_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_df = token_value;
                  break;
                }
              case kCYo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCYo = token_value;
                  break;
                }
              case kCY_beta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_beta = token_value;
                  break;
                }
              case kCY_p_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_p = token_value;
                  break;
                }
              case kCY_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_r = token_value;
                  break;
                }
              case kCY_da_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_da = token_value;
                  break;
                }
              case kCY_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_dr = token_value;
                  break;
                }
              case kCY_dra_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_dra = token_value;
                  break;
                }
              case kCY_bdot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_bdot = token_value;
                  break;
                }
              case kClo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kClo = token_value;
                  break;
                }
              case kCl_beta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_beta = token_value;
                  break;
                }
              case kCl_p_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_p = token_value;
                  break;
                }
              case kCl_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_r = token_value;
                  break;
                }
              case kCl_da_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_da = token_value;
                  break;
                }
              case kCl_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_dr = token_value;
                  break;
                }
              case kCl_daa_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_daa = token_value;
                  break;
                }
              case kCno_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCno = token_value;
                  break;
                }
              case kCn_beta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_beta = token_value;
                  break;
                }
              case kCn_p_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_p = token_value;
                  break;
                }
              case kCn_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_r = token_value;
                  break;
                }
              case kCn_da_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_da = token_value;
                  break;
                }
              case kCn_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_dr = token_value;
                  break;
                }
              case kCn_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_q = token_value;
                  break;
                }
              case kCn_b3_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_b3 = token_value;
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end ice map
          

        case record_flag:
          {
            static int fout_flag=0;
            if (fout_flag==0)
            {
              fout_flag=-1;
              fout.open("uiuc_record.dat");
            }
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
              case flap_deg_record:
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

                /******************** Ice Detection ********************/
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
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end record map               


        case misc_flag:
          {
            switch(misc_map[linetoken2])
              {
              case simpleHingeMomentCoef_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  simpleHingeMomentCoef = token_value;
                  break;
                }
              case dfTimefdf_flag:
                {
                  dfTimefdf = linetoken3;
                  /* call 1D File Reader with file name (dfTimefdf);
                     function returns array of dfs (dfArray) and 
                     corresponding time values (TimeArray) and max 
                     number of terms in arrays (ndf) */
                  uiuc_1DdataFileReader(dfTimefdf,
                                        dfTimefdf_dfArray,
                                        dfTimefdf_TimeArray,
                                        dfTimefdf_ndf);
                  break;
                }
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end misc map


        default:
          {
            if (linetoken1=="*")
                return;
            uiuc_warnings_errors(2, *command_line);
            break;
          }
        };
    } // end keyword map
  
  delete airplane;
}

// end menu.cpp
