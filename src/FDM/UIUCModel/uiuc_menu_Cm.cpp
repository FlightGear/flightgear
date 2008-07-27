/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_Cm.cpp

----------------------------------------------------------------------

 DESCRIPTION:  reads input data for specified aircraft and creates 
               approporiate data storage space

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   based on "menu reader" format of Michael Selig

----------------------------------------------------------------------

 HISTORY:      04/04/2003   initial release
               06/30/2003   (RD) replaced istrstream with istringstream
                            to get rid of the annoying warning about
                            using the strstream header

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include <simgear/compiler.h>


#include <cstdlib>
#include <string>
#include <iostream>

#include "uiuc_menu_Cm.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_Cm( const string& linetoken2, const string& linetoken3,
	       const string& linetoken4, const string& linetoken5, 
	       const string& linetoken6, const string& linetoken7,
	       const string& linetoken8, const string& linetoken9,
	       const string& linetoken10, const string& aircraft_directory,
	       LIST command_line ) {
    double token_value;
    int token_value_convert1, token_value_convert2, token_value_convert3;
    int token_value_convert4;
    double datafile_xArray[100][100], datafile_yArray[100];
    double datafile_zArray[100][100];
    double convert_f;
    int datafile_nxArray[100], datafile_ny;
    istringstream token3(linetoken3.c_str());
    istringstream token4(linetoken4.c_str());
    istringstream token5(linetoken5.c_str());
    istringstream token6(linetoken6.c_str());
    istringstream token7(linetoken7.c_str());
    istringstream token8(linetoken8.c_str());
    istringstream token9(linetoken9.c_str());
    istringstream token10(linetoken10.c_str());

    static bool Cmfabetaf_first = true;
    static bool Cmfadef_first = true;
    static bool Cmfaqf_first = true;

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
      case Cm_ds_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Cm_ds = token_value;
	  aeroPitchParts -> storeCommands (*command_line);
	  break;
	}
      case Cm_dg_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  Cm_dg = token_value;
	  aeroPitchParts -> storeCommands (*command_line);
	  break;
	}
      case Cmfa_flag:
	{
	  Cmfa = aircraft_directory + linetoken3;
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
	  Cmfade = aircraft_directory + linetoken3;
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
	  Cmfdf = aircraft_directory + linetoken3;
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
	  Cmfadf = aircraft_directory + linetoken3;
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
      case Cmfabetaf_flag:
	{
	  int Cmfabetaf_index, i;
	  string Cmfabetaf_file;
	  double flap_value;
	  Cmfabetaf_file = aircraft_directory + linetoken3;
	  token4 >> Cmfabetaf_index;
	  if (Cmfabetaf_index < 0 || Cmfabetaf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (Cmfabetaf_index > Cmfabetaf_nf)
	    Cmfabetaf_nf = Cmfabetaf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Cmfabetaf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Cmfabetaf_fArray[Cmfabetaf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Cmfabetaf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Cmfabetaf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Cmfabetaf_aArray, Cmfabetaf_index);
	  d_1_to_2(datafile_yArray, Cmfabetaf_betaArray, Cmfabetaf_index);
	  d_2_to_3(datafile_zArray, Cmfabetaf_CmArray, Cmfabetaf_index);
	  i_1_to_2(datafile_nxArray, Cmfabetaf_nAlphaArray, Cmfabetaf_index);
	  Cmfabetaf_nbeta[Cmfabetaf_index] = datafile_ny;
	  if (Cmfabetaf_first==true)
	    {
	      if (Cmfabetaf_nice == 1)
		{
		  Cmfabetaf_na_nice = datafile_nxArray[1];
		  Cmfabetaf_nb_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Cmfabetaf_bArray_nice);
		  for (i=1; i<=Cmfabetaf_na_nice; i++)
		    Cmfabetaf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroPitchParts -> storeCommands (*command_line);
	      Cmfabetaf_first=false;
	    }
	  break;
	}
      case Cmfadef_flag:
	{
	  int Cmfadef_index, i;
	  string Cmfadef_file;
	  double flap_value;
	  Cmfadef_file = aircraft_directory + linetoken3;
	  token4 >> Cmfadef_index;
	  if (Cmfadef_index < 0 || Cmfadef_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (Cmfadef_index > Cmfadef_nf)
	    Cmfadef_nf = Cmfadef_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Cmfadef_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Cmfadef_fArray[Cmfadef_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Cmfadef_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Cmfadef_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Cmfadef_aArray, Cmfadef_index);
	  d_1_to_2(datafile_yArray, Cmfadef_deArray, Cmfadef_index);
	  d_2_to_3(datafile_zArray, Cmfadef_CmArray, Cmfadef_index);
	  i_1_to_2(datafile_nxArray, Cmfadef_nAlphaArray, Cmfadef_index);
	  Cmfadef_nde[Cmfadef_index] = datafile_ny;
	  if (Cmfadef_first==true)
	    {
	      if (Cmfadef_nice == 1)
		{
		  Cmfadef_na_nice = datafile_nxArray[1];
		  Cmfadef_nde_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Cmfadef_deArray_nice);
		  for (i=1; i<=Cmfadef_na_nice; i++)
		    Cmfadef_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroPitchParts -> storeCommands (*command_line);
	      Cmfadef_first=false;
	    }
	  break;
	}
      case Cmfaqf_flag:
	{
	  int Cmfaqf_index, i;
	  string Cmfaqf_file;
	  double flap_value;
	  Cmfaqf_file = aircraft_directory + linetoken3;
	  token4 >> Cmfaqf_index;
	  if (Cmfaqf_index < 0 || Cmfaqf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (Cmfaqf_index > Cmfaqf_nf)
	    Cmfaqf_nf = Cmfaqf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Cmfaqf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Cmfaqf_fArray[Cmfaqf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Cmfaqf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Cmfaqf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Cmfaqf_aArray, Cmfaqf_index);
	  d_1_to_2(datafile_yArray, Cmfaqf_qArray, Cmfaqf_index);
	  d_2_to_3(datafile_zArray, Cmfaqf_CmArray, Cmfaqf_index);
	  i_1_to_2(datafile_nxArray, Cmfaqf_nAlphaArray, Cmfaqf_index);
	  Cmfaqf_nq[Cmfaqf_index] = datafile_ny;
	  if (Cmfaqf_first==true)
	    {
	      if (Cmfaqf_nice == 1)
		{
		  Cmfaqf_na_nice = datafile_nxArray[1];
		  Cmfaqf_nq_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Cmfaqf_qArray_nice);
		  for (i=1; i<=Cmfaqf_na_nice; i++)
		    Cmfaqf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroPitchParts -> storeCommands (*command_line);
	      Cmfaqf_first=false;
	    }
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
