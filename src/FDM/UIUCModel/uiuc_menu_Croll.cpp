/**********************************************************************

 FILENAME:     uiuc_menu_Croll.cpp

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

#include "uiuc_menu_Croll.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_Cl( const string& linetoken2, const string& linetoken3,
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

    static bool Clfabetaf_first = true;
    static bool Clfadaf_first = true;
    static bool Clfadrf_first = true;
    static bool Clfapf_first = true;
    static bool Clfarf_first = true;

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
	  Clfada = aircraft_directory + linetoken3;
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
	  Clfbetadr = aircraft_directory + linetoken3;
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
      case Clfabetaf_flag:
	{
	  int Clfabetaf_index, i;
	  string Clfabetaf_file;
	  double flap_value;
	  Clfabetaf_file = aircraft_directory + linetoken3;
	  token4 >> Clfabetaf_index;
	  if (Clfabetaf_index < 0 || Clfabetaf_index >= 100)
	    uiuc_warnings_errors(1, *command_line);
	  if (Clfabetaf_index > Clfabetaf_nf)
	    Clfabetaf_nf = Clfabetaf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Clfabetaf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Clfabetaf_fArray[Clfabetaf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Clfabetaf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Clfabetaf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Clfabetaf_aArray, Clfabetaf_index);
	  d_1_to_2(datafile_yArray, Clfabetaf_betaArray, Clfabetaf_index);
	  d_2_to_3(datafile_zArray, Clfabetaf_ClArray, Clfabetaf_index);
	  i_1_to_2(datafile_nxArray, Clfabetaf_nAlphaArray, Clfabetaf_index);
	  Clfabetaf_nbeta[Clfabetaf_index] = datafile_ny;
	  if (Clfabetaf_first==true)
	    {
	      if (Clfabetaf_nice == 1)
		{
		  Clfabetaf_na_nice = datafile_nxArray[1];
		  Clfabetaf_nb_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Clfabetaf_bArray_nice);
		  for (i=1; i<=Clfabetaf_na_nice; i++)
		    Clfabetaf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroRollParts -> storeCommands (*command_line);
	      Clfabetaf_first=false;
	    }
	  break;
	}
      case Clfadaf_flag:
	{
	  int Clfadaf_index, i;
	  string Clfadaf_file;
	  double flap_value;
	  Clfadaf_file = aircraft_directory + linetoken3;
	  token4 >> Clfadaf_index;
	  if (Clfadaf_index < 0 || Clfadaf_index >= 100)
	    uiuc_warnings_errors(1, *command_line);
	  if (Clfadaf_index > Clfadaf_nf)
	    Clfadaf_nf = Clfadaf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Clfadaf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Clfadaf_fArray[Clfadaf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Clfadaf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Clfadaf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Clfadaf_aArray, Clfadaf_index);
	  d_1_to_2(datafile_yArray, Clfadaf_daArray, Clfadaf_index);
	  d_2_to_3(datafile_zArray, Clfadaf_ClArray, Clfadaf_index);
	  i_1_to_2(datafile_nxArray, Clfadaf_nAlphaArray, Clfadaf_index);
	  Clfadaf_nda[Clfadaf_index] = datafile_ny;
	  if (Clfadaf_first==true)
	    {
	      if (Clfadaf_nice == 1)
		{
		  Clfadaf_na_nice = datafile_nxArray[1];
		  Clfadaf_nda_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Clfadaf_daArray_nice);
		  for (i=1; i<=Clfadaf_na_nice; i++)
		    Clfadaf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroRollParts -> storeCommands (*command_line);
	      Clfadaf_first=false;
	    }
	  break;
	}
      case Clfadrf_flag:
	{
	  int Clfadrf_index, i;
	  string Clfadrf_file;
	  double flap_value;
	  Clfadrf_file = aircraft_directory + linetoken3;
	  token4 >> Clfadrf_index;
	  if (Clfadrf_index < 0 || Clfadrf_index >= 100)
	    uiuc_warnings_errors(1, *command_line);
	  if (Clfadrf_index > Clfadrf_nf)
	    Clfadrf_nf = Clfadrf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Clfadrf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Clfadrf_fArray[Clfadrf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Clfadrf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Clfadrf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Clfadrf_aArray, Clfadrf_index);
	  d_1_to_2(datafile_yArray, Clfadrf_drArray, Clfadrf_index);
	  d_2_to_3(datafile_zArray, Clfadrf_ClArray, Clfadrf_index);
	  i_1_to_2(datafile_nxArray, Clfadrf_nAlphaArray, Clfadrf_index);
	  Clfadrf_ndr[Clfadrf_index] = datafile_ny;
	  if (Clfadrf_first==true)
	    {
	      if (Clfadrf_nice == 1)
		{
		  Clfadrf_na_nice = datafile_nxArray[1];
		  Clfadrf_ndr_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Clfadrf_drArray_nice);
		  for (i=1; i<=Clfadrf_na_nice; i++)
		    Clfadrf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroRollParts -> storeCommands (*command_line);
	      Clfadrf_first=false;
	    }
	  break;
	}
      case Clfapf_flag:
	{
	  int Clfapf_index, i;
	  string Clfapf_file;
	  double flap_value;
	  Clfapf_file = aircraft_directory + linetoken3;
	  token4 >> Clfapf_index;
	  if (Clfapf_index < 0 || Clfapf_index >= 100)
	    uiuc_warnings_errors(1, *command_line);
	  if (Clfapf_index > Clfapf_nf)
	    Clfapf_nf = Clfapf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Clfapf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Clfapf_fArray[Clfapf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Clfapf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Clfapf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Clfapf_aArray, Clfapf_index);
	  d_1_to_2(datafile_yArray, Clfapf_pArray, Clfapf_index);
	  d_2_to_3(datafile_zArray, Clfapf_ClArray, Clfapf_index);
	  i_1_to_2(datafile_nxArray, Clfapf_nAlphaArray, Clfapf_index);
	  Clfapf_np[Clfapf_index] = datafile_ny;
	  if (Clfapf_first==true)
	    {
	      if (Clfapf_nice == 1)
		{
		  Clfapf_na_nice = datafile_nxArray[1];
		  Clfapf_np_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Clfapf_pArray_nice);
		  for (i=1; i<=Clfapf_na_nice; i++)
		    Clfapf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroRollParts -> storeCommands (*command_line);
	      Clfapf_first=false;
	    }
	  break;
	}
      case Clfarf_flag:
	{
	  int Clfarf_index, i;
	  string Clfarf_file;
	  double flap_value;
	  Clfarf_file = aircraft_directory + linetoken3;
	  token4 >> Clfarf_index;
	  if (Clfarf_index < 0 || Clfarf_index >= 100)
	    uiuc_warnings_errors(1, *command_line);
	  if (Clfarf_index > Clfarf_nf)
	    Clfarf_nf = Clfarf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Clfarf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Clfarf_fArray[Clfarf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Clfarf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Clfarf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Clfarf_aArray, Clfarf_index);
	  d_1_to_2(datafile_yArray, Clfarf_rArray, Clfarf_index);
	  d_2_to_3(datafile_zArray, Clfarf_ClArray, Clfarf_index);
	  i_1_to_2(datafile_nxArray, Clfarf_nAlphaArray, Clfarf_index);
	  Clfarf_nr[Clfarf_index] = datafile_ny;
	  if (Clfarf_first==true)
	    {
	      if (Clfarf_nice == 1)
		{
		  Clfarf_na_nice = datafile_nxArray[1];
		  Clfarf_nr_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Clfarf_rArray_nice);
		  for (i=1; i<=Clfarf_na_nice; i++)
		    Clfarf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroRollParts -> storeCommands (*command_line);
	      Clfarf_first=false;
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
