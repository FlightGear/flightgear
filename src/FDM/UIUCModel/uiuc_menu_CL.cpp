/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_CL.cpp

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

#include "uiuc_menu_CL.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_CL( const string& linetoken2, const string& linetoken3,
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

    static bool CZfabetaf_first = true;
    static bool CZfadef_first = true;
    static bool CZfaqf_first = true;

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
      case CL_df_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  CL_df = token_value;
	  aeroLiftParts -> storeCommands (*command_line);
	  break;
	}
      case CL_ds_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  CL_ds = token_value;
	  aeroLiftParts -> storeCommands (*command_line);
	  break;
	}
      case CL_dg_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  CL_dg = token_value;
	  aeroLiftParts -> storeCommands (*command_line);
	  break;
	}
      case CLfa_flag:
	{
	  CLfa = aircraft_directory + linetoken3;
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
	  CLfade = aircraft_directory + linetoken3;
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
	  CLfdf = aircraft_directory + linetoken3;
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
	  //ndf = CLfdf_ndf;
	  //int temp_counter = 1;
	  //while (temp_counter <= ndf)
	  //  {
	  //    dfArray[temp_counter] = CLfdf_dfArray[temp_counter];
	  //    TimeArray[temp_counter] = dfTimefdf_TimeArray[temp_counter];
	  //    temp_counter++;
	  //  }
	  break;
	}
      case CLfadf_flag:
	{
	  CLfadf = aircraft_directory + linetoken3;
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
      case CZfa_flag:
	{
	  CZfa = aircraft_directory + linetoken3;
	  token4 >> token_value_convert1;
	  token5 >> token_value_convert2;
	  convert_y = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  /* call 1D File Reader with file name (CZfa) and conversion 
	     factors; function returns array of alphas (aArray) and 
	     corresponding CZ values (CZArray) and max number of 
	     terms in arrays (nAlpha) */
	  uiuc_1DdataFileReader(CZfa,
				CZfa_aArray,
				CZfa_CZArray,
				CZfa_nAlpha);
	  aeroLiftParts -> storeCommands (*command_line);
	  break;
	}
      case CZfabetaf_flag:
	{
	  int CZfabetaf_index, i;
	  string CZfabetaf_file;
	  double flap_value;
	  CZfabetaf_file = aircraft_directory + linetoken3;
	  token4 >> CZfabetaf_index;
	  if (CZfabetaf_index < 0 || CZfabetaf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CZfabetaf_index > CZfabetaf_nf)
	    CZfabetaf_nf = CZfabetaf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CZfabetaf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CZfabetaf_fArray[CZfabetaf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CZfabetaf_file) and 
	     conversion factors; function returns array of 
	     beta (betaArray) and corresponding 
	     alpha (aArray) and CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and beta array (nbeta) */
	  uiuc_2DdataFileReader(CZfabetaf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CZfabetaf_aArray, CZfabetaf_index);
	  d_1_to_2(datafile_yArray, CZfabetaf_betaArray, CZfabetaf_index);
	  d_2_to_3(datafile_zArray, CZfabetaf_CZArray, CZfabetaf_index);
	  i_1_to_2(datafile_nxArray, CZfabetaf_nAlphaArray, CZfabetaf_index);
	  CZfabetaf_nbeta[CZfabetaf_index] = datafile_ny;
	  if (CZfabetaf_first==true)
	    {
	      if (CZfabetaf_nice == 1)
		{
		  CZfabetaf_na_nice = datafile_nxArray[1];
		  CZfabetaf_nb_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CZfabetaf_bArray_nice);
		  for (i=1; i<=CZfabetaf_na_nice; i++)
		    CZfabetaf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroLiftParts -> storeCommands (*command_line);
	      CZfabetaf_first=false;
	    }
	  break;
	}
      case CZfadef_flag:
	{
	  int CZfadef_index, i;
	  string CZfadef_file;
	  double flap_value;
	  CZfadef_file = aircraft_directory + linetoken3;
	  token4 >> CZfadef_index;
	  if (CZfadef_index < 0 || CZfadef_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CZfadef_index > CZfadef_nf)
	    CZfadef_nf = CZfadef_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CZfadef_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CZfadef_fArray[CZfadef_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CZfadef_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(CZfadef_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CZfadef_aArray, CZfadef_index);
	  d_1_to_2(datafile_yArray, CZfadef_deArray, CZfadef_index);
	  d_2_to_3(datafile_zArray, CZfadef_CZArray, CZfadef_index);
	  i_1_to_2(datafile_nxArray, CZfadef_nAlphaArray, CZfadef_index);
	  CZfadef_nde[CZfadef_index] = datafile_ny;
	  if (CZfadef_first==true)
	    {
	      if (CZfadef_nice == 1)
		{
		  CZfadef_na_nice = datafile_nxArray[1];
		  CZfadef_nde_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CZfadef_deArray_nice);
		  for (i=1; i<=CZfadef_na_nice; i++)
		    CZfadef_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroLiftParts -> storeCommands (*command_line);
	      CZfadef_first=false;
	    }
	  break;
	}
      case CZfaqf_flag:
	{
	  int CZfaqf_index, i;
	  string CZfaqf_file;
	  double flap_value;
	  CZfaqf_file = aircraft_directory + linetoken3;
	  token4 >> CZfaqf_index;
	  if (CZfaqf_index < 0 || CZfaqf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CZfaqf_index > CZfaqf_nf)
	    CZfaqf_nf = CZfaqf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CZfaqf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CZfaqf_fArray[CZfaqf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CZfaqf_file) and 
	     conversion factors; function returns array of 
	     pitch rate (qArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and pitch rate array (nq) */
	  uiuc_2DdataFileReader(CZfaqf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CZfaqf_aArray, CZfaqf_index);
	  d_1_to_2(datafile_yArray, CZfaqf_qArray, CZfaqf_index);
	  d_2_to_3(datafile_zArray, CZfaqf_CZArray, CZfaqf_index);
	  i_1_to_2(datafile_nxArray, CZfaqf_nAlphaArray, CZfaqf_index);
	  CZfaqf_nq[CZfaqf_index] = datafile_ny;
	  if (CZfaqf_first==true)
	    {
	      if (CZfaqf_nice == 1)
		{
		  CZfaqf_na_nice = datafile_nxArray[1];
		  CZfaqf_nq_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CZfaqf_qArray_nice);
		  for (i=1; i<=CZfaqf_na_nice; i++)
		    CZfaqf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroLiftParts -> storeCommands (*command_line);
	      CZfaqf_first=false;
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
