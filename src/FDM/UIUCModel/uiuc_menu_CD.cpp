/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_CD.cpp

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

#include "uiuc_menu_CD.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_CD( const string& linetoken2, const string& linetoken3,
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

    static bool CXfabetaf_first = true;
    static bool CXfadef_first = true;
    static bool CXfaqf_first = true;

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
      case CLK_flag:
	{
	  b_CLK = true;
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  CLK = token_value;
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
      case CD_dr_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  CD_dr = token_value;
	  aeroDragParts -> storeCommands (*command_line);
	  break;
	}
      case CD_da_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  CD_da = token_value;
	  aeroDragParts -> storeCommands (*command_line);
	  break;
	}
      case CD_beta_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  CD_beta = token_value;
	  aeroDragParts -> storeCommands (*command_line);
	  break;
	}
      case CD_df_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  CD_df = token_value;
	  aeroDragParts -> storeCommands (*command_line);
	  break;
	}
      case CD_ds_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  CD_ds = token_value;
	  aeroDragParts -> storeCommands (*command_line);
	  break;
	}
      case CD_dg_flag:
	{
	  if (check_float(linetoken3))
	    token3 >> token_value;
	  else
	    uiuc_warnings_errors(1, *command_line);
	  
	  CD_dg = token_value;
	  aeroDragParts -> storeCommands (*command_line);
	  break;
	}
      case CDfa_flag:
	{
	  CDfa = aircraft_directory + linetoken3;
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
	  CDfCL = aircraft_directory + linetoken3;
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
	  CDfade = aircraft_directory + linetoken3;
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
	  CDfdf = aircraft_directory + linetoken3;
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
	  CDfadf = aircraft_directory + linetoken3;
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
      case CXfabetaf_flag:
	{
	  int CXfabetaf_index, i;
	  string CXfabetaf_file;
	  double flap_value;
	  CXfabetaf_file = aircraft_directory + linetoken3;
	  token4 >> CXfabetaf_index;
	  if (CXfabetaf_index < 1 || CXfabetaf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CXfabetaf_index > CXfabetaf_nf)
	    CXfabetaf_nf = CXfabetaf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CXfabetaf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CXfabetaf_fArray[CXfabetaf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CXfabetaf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(CXfabetaf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CXfabetaf_aArray, CXfabetaf_index);
	  d_1_to_2(datafile_yArray, CXfabetaf_betaArray, CXfabetaf_index);
	  d_2_to_3(datafile_zArray, CXfabetaf_CXArray, CXfabetaf_index);
	  i_1_to_2(datafile_nxArray, CXfabetaf_nAlphaArray, CXfabetaf_index);
	  CXfabetaf_nbeta[CXfabetaf_index] = datafile_ny;
	  if (CXfabetaf_first==true)
	    {
	      if (CXfabetaf_nice == 1)
		{
		  CXfabetaf_na_nice = datafile_nxArray[1];
		  CXfabetaf_nb_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CXfabetaf_bArray_nice);
		  for (i=1; i<=CXfabetaf_na_nice; i++)
		    CXfabetaf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroDragParts -> storeCommands (*command_line);
	      CXfabetaf_first=false;
	    }
	  break;
	}
      case CXfadef_flag:
	{
	  int CXfadef_index, i;
	  string CXfadef_file;
	  double flap_value;
	  CXfadef_file = aircraft_directory + linetoken3;
	  token4 >> CXfadef_index;
	  if (CXfadef_index < 0 || CXfadef_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CXfadef_index > CXfadef_nf)
	    CXfadef_nf = CXfadef_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CXfadef_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CXfadef_fArray[CXfadef_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CXfadef_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(CXfadef_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CXfadef_aArray, CXfadef_index);
	  d_1_to_2(datafile_yArray, CXfadef_deArray, CXfadef_index);
	  d_2_to_3(datafile_zArray, CXfadef_CXArray, CXfadef_index);
	  i_1_to_2(datafile_nxArray, CXfadef_nAlphaArray, CXfadef_index);
	  CXfadef_nde[CXfadef_index] = datafile_ny;
	  if (CXfadef_first==true)
	    {
	      if (CXfadef_nice == 1)
		{
		  CXfadef_na_nice = datafile_nxArray[1];
		  CXfadef_nde_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CXfadef_deArray_nice);
		  for (i=1; i<=CXfadef_na_nice; i++)
		    CXfadef_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroDragParts -> storeCommands (*command_line);
	      CXfadef_first=false;
	    }
	  break;
	}
      case CXfaqf_flag:
	{
	  int CXfaqf_index, i;
	  string CXfaqf_file;
	  double flap_value;
	  CXfaqf_file = aircraft_directory + linetoken3;
	  token4 >> CXfaqf_index;
	  if (CXfaqf_index < 0 || CXfaqf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CXfaqf_index > CXfaqf_nf)
	    CXfaqf_nf = CXfaqf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CXfaqf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CXfaqf_fArray[CXfaqf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CXfaqf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(CXfaqf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CXfaqf_aArray, CXfaqf_index);
	  d_1_to_2(datafile_yArray, CXfaqf_qArray, CXfaqf_index);
	  d_2_to_3(datafile_zArray, CXfaqf_CXArray, CXfaqf_index);
	  i_1_to_2(datafile_nxArray, CXfaqf_nAlphaArray, CXfaqf_index);
	  CXfaqf_nq[CXfaqf_index] = datafile_ny;
	  if (CXfaqf_first==true)
	    {
	      if (CXfaqf_nice == 1)
		{
		  CXfaqf_na_nice = datafile_nxArray[1];
		  CXfaqf_nq_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CXfaqf_qArray_nice);
		  for (i=1; i<=CXfaqf_na_nice; i++)
		    CXfaqf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroDragParts -> storeCommands (*command_line);
	      CXfaqf_first=false;
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
