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

 CALLED BY:   uiuc_wrapper.cpp 

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

#include "uiuc_menu.h"

bool check_float(string  &token)
{
        float value;
        istrstream stream(token.c_str()); 
        return (stream >> value);
}


void uiuc_menu (string aircraft_name)
{
  stack command_list;
  double token_value;
  int token_value1, token_value2, token_value3;
 
  string linetoken1;
  string linetoken2;
  string linetoken3;
  string linetoken4;
  string linetoken5;
  string linetoken6;
  
  /* Read the file and get the list of commands */
  airplane = new ParseFile(aircraft_name); /* struct that includes all lines of the input file */
  command_list = airplane -> getCommands();
  /* structs to include all parts included in the input file for specific keyword groups */
  initParts     = new ParseFile();
  geometryParts = new ParseFile();
  massParts     = new ParseFile();
  engineParts   = new ParseFile();
  aeroParts     = new ParseFile();
  gearParts     = new ParseFile();
  recordParts   = new ParseFile();

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

      token4 >> token_value1;
      token5 >> token_value2;
      token6 >> token_value3;

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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case CDfa_flag:
                {
                  CDfa = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  /* call 1D File Reader with file name (CDfa) and conversion 
                     factors; function returns array of alphas (aArray) and 
                     corresponding CD values (CDArray) and max number of 
                     terms in arrays (nAlpha) */
                  CDfaData = uiuc_1DdataFileReader(CDfa,
                                                   confac2,   /* x */
                                                   confac1,   /* y */
                                                   CDfa_aArray,
                                                   CDfa_CDArray,
                                                   CDfa_nAlpha);
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case CDfade_flag:
                {
                  CDfade = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  conversion3 = token_value3;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  confac3 = uiuc_convert(conversion3);
                  /* call 2D File Reader with file name (CDfade) and 
                     conversion factors; function returns array of 
                     elevator deflections (deArray) and corresponding 
                     alpha (aArray) and delta CD (CDArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nde) */
                  CDfadeData = uiuc_2DdataFileReader(CDfade,
                                                     confac2,   /* x */
                                                     confac3,   /* y */
                                                     confac1,   /* z */
                                                     CDfade_aArray,
                                                     CDfade_deArray,
                                                     CDfade_CDArray,
                                                     CDfade_nAlphaArray,
                                                     CDfade_nde);
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case CLfa_flag:
                {
                  CLfa = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  /* call 1D File Reader with file name (CLfa) and conversion 
                     factors; function returns array of alphas (aArray) and 
                     corresponding CL values (CLArray) and max number of 
                     terms in arrays (nAlpha) */
                  CLfaData = uiuc_1DdataFileReader(CLfa,
                                                   confac2,   /* x */
                                                   confac1,   /* y */
                                                   CLfa_aArray,
                                                   CLfa_CLArray,
                                                   CLfa_nAlpha);
                  aeroParts -> storeCommands (*command_line);
		  break;
                }
              case CLfade_flag:
                {
                  CLfade = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  conversion3 = token_value3;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  confac3 = uiuc_convert(conversion3);
                  /* call 2D File Reader with file name (CLfade) and 
                     conversion factors; function returns array of 
                     elevator deflections (deArray) and corresponding 
                     alpha (aArray) and delta CL (CLArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nde) */
                  CLfadeData = uiuc_2DdataFileReader(CLfade,
                                                     confac2,   /* x */
                                                     confac3,   /* y */
                                                     confac1,   /* z */
                                                     CLfade_aArray,
                                                     CLfade_deArray,
                                                     CLfade_CLArray,
                                                     CLfade_nAlphaArray,
                                                     CLfade_nde);
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case Cmfade_flag:
                {
                  Cmfade = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  conversion3 = token_value3;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  confac3 = uiuc_convert(conversion3);
                  /* call 2D File Reader with file name (Cmfade) and 
                     conversion factors; function returns array of 
                     elevator deflections (deArray) and corresponding 
                     alpha (aArray) and delta Cm (CmArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nde) */
                  CmfadeData = uiuc_2DdataFileReader(Cmfade,
                                                     confac2,   /* x */
                                                     confac3,   /* y */
                                                     confac1,   /* z */
                                                     Cmfade_aArray,
                                                     Cmfade_deArray,
                                                     Cmfade_CmArray,
                                                     Cmfade_nAlphaArray,
                                                     Cmfade_nde);
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case CYfada_flag:
                {
                  CYfada = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  conversion3 = token_value3;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  confac3 = uiuc_convert(conversion3);
                  /* call 2D File Reader with file name (CYfada) and 
                     conversion factors; function returns array of 
                     aileron deflections (daArray) and corresponding 
                     alpha (aArray) and delta CY (CYArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nda) */
                  CYfadaData = uiuc_2DdataFileReader(CYfada,
                                                     confac2,   /* x */
                                                     confac3,   /* y */
                                                     confac1,   /* z */
                                                     CYfada_aArray,
                                                     CYfada_daArray,
                                                     CYfada_CYArray,
                                                     CYfada_nAlphaArray,
                                                     CYfada_nda);
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case CYfbetadr_flag:
                {
                  CYfbetadr = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  conversion3 = token_value3;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  confac3 = uiuc_convert(conversion3);
                  /* call 2D File Reader with file name (CYfbetadr) and 
                     conversion factors; function returns array of 
                     rudder deflections (drArray) and corresponding 
                     beta (betaArray) and delta CY (CYArray) values and 
                     max number of terms in beta arrays (nBetaArray) 
                     and deflection array (ndr) */
                  CYfbetadrData = uiuc_2DdataFileReader(CYfbetadr,
                                                        confac2,   /* x */
                                                        confac3,   /* y */
                                                        confac1,   /* z */
                                                        CYfbetadr_betaArray,
                                                        CYfbetadr_drArray,
                                                        CYfbetadr_CYArray,
                                                        CYfbetadr_nBetaArray,
                                                        CYfbetadr_ndr);
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case Clfada_flag:
                {
                  Clfada = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  conversion3 = token_value3;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  confac3 = uiuc_convert(conversion3);
                  /* call 2D File Reader with file name (Clfada) and 
                     conversion factors; function returns array of 
                     aileron deflections (daArray) and corresponding 
                     alpha (aArray) and delta Cl (ClArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nda) */
                  ClfadaData = uiuc_2DdataFileReader(Clfada,
                                                     confac2,   /* x */
                                                     confac3,   /* y */
                                                     confac1,   /* z */
                                                     Clfada_aArray,
                                                     Clfada_daArray,
                                                     Clfada_ClArray,
                                                     Clfada_nAlphaArray,
                                                     Clfada_nda);
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case Clfbetadr_flag:
                {
                  Clfbetadr = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  conversion3 = token_value3;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  confac3 = uiuc_convert(conversion3);
                  /* call 2D File Reader with file name (Clfbetadr) and 
                     conversion factors; function returns array of 
                     rudder deflections (drArray) and corresponding 
                     beta (betaArray) and delta Cl (ClArray) values and 
                     max number of terms in beta arrays (nBetaArray) 
                     and deflection array (ndr) */
                  ClfbetadrData = uiuc_2DdataFileReader(Clfbetadr,
                                                        confac2,   /* x */
                                                        confac3,   /* y */
                                                        confac1,   /* z */
                                                        Clfbetadr_betaArray,
                                                        Clfbetadr_drArray,
                                                        Clfbetadr_ClArray,
                                                        Clfbetadr_nBetaArray,
                                                        Clfbetadr_ndr);
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
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
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case Cnfada_flag:
                {
                  Cnfada = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  conversion3 = token_value3;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  confac3 = uiuc_convert(conversion3);
                  /* call 2D File Reader with file name (Cnfada) and 
                     conversion factors; function returns array of 
                     aileron deflections (daArray) and corresponding 
                     alpha (aArray) and delta Cn (CnArray) values and 
                     max number of terms in alpha arrays (nAlphaArray) 
                     and deflection array (nda) */
                  CnfadaData = uiuc_2DdataFileReader(Cnfada,
                                                     confac2,   /* x */
                                                     confac3,   /* y */
                                                     confac1,   /* z */
                                                     Cnfada_aArray,
                                                     Cnfada_daArray,
                                                     Cnfada_CnArray,
                                                     Cnfada_nAlphaArray,
                                                     Cnfada_nda);
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case Cnfbetadr_flag:
                {
                  Cnfbetadr = linetoken3;
                  conversion1 = token_value1;
                  conversion2 = token_value2;
                  conversion3 = token_value3;
                  confac1 = uiuc_convert(conversion1);
                  confac2 = uiuc_convert(conversion2);
                  confac3 = uiuc_convert(conversion3);
                  /* call 2D File Reader with file name (Cnfbetadr) and 
                     conversion factors; function returns array of 
                     rudder deflections (drArray) and corresponding 
                     beta (betaArray) and delta Cn (CnArray) values and 
                     max number of terms in beta arrays (nBetaArray) 
                     and deflection array (ndr) */
                  CnfbetadrData = uiuc_2DdataFileReader(Cnfbetadr,
                                                        confac2,   /* x */
                                                        confac3,   /* y */
                                                        confac1,   /* z */
                                                        Cnfbetadr_betaArray,
                                                        Cnfbetadr_drArray,
                                                        Cnfbetadr_CnArray,
                                                        Cnfbetadr_nBetaArray,
                                                        Cnfbetadr_ndr);
                  aeroParts -> storeCommands (*command_line);
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

                  iceTime = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }

              case transientTime_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  transientTime = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }

              case eta_final_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  eta_final = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }

              case kCDo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCDo = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCDK_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCDK = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCD_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCD_a = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCD_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCD_de = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }

              case kCLo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCLo = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCL_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  kCL_a = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCL_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCL_adot = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCL_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCL_q = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCL_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCL_de = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }

              case kCmo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCmo = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCm_a_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_a = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCm_adot_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_adot = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCm_q_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_q = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCm_de_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCm_de = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }

              case kCYo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCYo = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCY_beta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_beta = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCY_p_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_p = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCY_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_r = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCY_da_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_da = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCY_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCY_dr = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }

              case kClo_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kClo = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCl_beta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_beta = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCl_p_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_p = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCl_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_r = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCl_da_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_da = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCl_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCl_dr = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }

              case kCno_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCno = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCn_beta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_beta = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCn_p_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_p = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCn_r_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_r = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCn_da_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_da = token_value;
                  aeroParts -> storeCommands (*command_line);
                  break;
                }
              case kCn_dr_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);

                  kCn_dr = token_value;
                  aeroParts -> storeCommands (*command_line);
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
              case V_rel_wind_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case Dynamic_pressure_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case Alpha_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case Alpha_dot_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case Beta_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case Beta_dot_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case Gamma_record:
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
              case Theta_dot_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case density_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case Mass_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
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
              case elevator_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case aileron_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case rudder_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
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
              case Cm_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case CmfadeI_record:
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
