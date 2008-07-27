/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_CY.cpp

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

#include "uiuc_menu_CY.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_CY( const string& linetoken2, const string& linetoken3,
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

    static bool CYfabetaf_first = true;
    static bool CYfadaf_first = true;
    static bool CYfadrf_first = true;
    static bool CYfapf_first = true;
    static bool CYfarf_first = true;

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
	  CYfada = aircraft_directory + linetoken3;
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
	  CYfbetadr = aircraft_directory + linetoken3;
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
      case CYfabetaf_flag:
	{
	  int CYfabetaf_index, i;
	  string CYfabetaf_file;
	  double flap_value;
	  CYfabetaf_file = aircraft_directory + linetoken3;
	  token4 >> CYfabetaf_index;
	  if (CYfabetaf_index < 0 || CYfabetaf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CYfabetaf_index > CYfabetaf_nf)
	    CYfabetaf_nf = CYfabetaf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CYfabetaf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CYfabetaf_fArray[CYfabetaf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CYfabetaf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(CYfabetaf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CYfabetaf_aArray, CYfabetaf_index);
	  d_1_to_2(datafile_yArray, CYfabetaf_betaArray, CYfabetaf_index);
	  d_2_to_3(datafile_zArray, CYfabetaf_CYArray, CYfabetaf_index);
	  i_1_to_2(datafile_nxArray, CYfabetaf_nAlphaArray, CYfabetaf_index);
	  CYfabetaf_nbeta[CYfabetaf_index] = datafile_ny;
	  if (CYfabetaf_first==true)
	    {
	      if (CYfabetaf_nice == 1)
		{
		  CYfabetaf_na_nice = datafile_nxArray[1];
		  CYfabetaf_nb_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CYfabetaf_bArray_nice);
		  for (i=1; i<=CYfabetaf_na_nice; i++)
		    CYfabetaf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroSideforceParts -> storeCommands (*command_line);
	      CYfabetaf_first=false;
	    }
	  break;
	}
      case CYfadaf_flag:
	{
	  int CYfadaf_index, i;
	  string CYfadaf_file;
	  double flap_value;
	  CYfadaf_file = aircraft_directory + linetoken3;
	  token4 >> CYfadaf_index;
	  if (CYfadaf_index < 0 || CYfadaf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CYfadaf_index > CYfadaf_nf)
	    CYfadaf_nf = CYfadaf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CYfadaf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CYfadaf_fArray[CYfadaf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CYfadaf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(CYfadaf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CYfadaf_aArray, CYfadaf_index);
	  d_1_to_2(datafile_yArray, CYfadaf_daArray, CYfadaf_index);
	  d_2_to_3(datafile_zArray, CYfadaf_CYArray, CYfadaf_index);
	  i_1_to_2(datafile_nxArray, CYfadaf_nAlphaArray, CYfadaf_index);
	  CYfadaf_nda[CYfadaf_index] = datafile_ny;
	  if (CYfadaf_first==true)
	    {
	      if (CYfadaf_nice == 1)
		{
		  CYfadaf_na_nice = datafile_nxArray[1];
		  CYfadaf_nda_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CYfadaf_daArray_nice);
		  for (i=1; i<=CYfadaf_na_nice; i++)
		    CYfadaf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroSideforceParts -> storeCommands (*command_line);
	      CYfadaf_first=false;
	    }
	  break;
	}
      case CYfadrf_flag:
	{
	  int CYfadrf_index, i;
	  string CYfadrf_file;
	  double flap_value;
	  CYfadrf_file = aircraft_directory + linetoken3;
	  token4 >> CYfadrf_index;
	  if (CYfadrf_index < 0 || CYfadrf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CYfadrf_index > CYfadrf_nf)
	    CYfadrf_nf = CYfadrf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CYfadrf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CYfadrf_fArray[CYfadrf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CYfadrf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(CYfadrf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CYfadrf_aArray, CYfadrf_index);
	  d_1_to_2(datafile_yArray, CYfadrf_drArray, CYfadrf_index);
	  d_2_to_3(datafile_zArray, CYfadrf_CYArray, CYfadrf_index);
	  i_1_to_2(datafile_nxArray, CYfadrf_nAlphaArray, CYfadrf_index);
	  CYfadrf_ndr[CYfadrf_index] = datafile_ny;
	  if (CYfadrf_first==true)
	    {
	      if (CYfadrf_nice == 1)
		{
		  CYfadrf_na_nice = datafile_nxArray[1];
		  CYfadrf_ndr_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CYfadrf_drArray_nice);
		  for (i=1; i<=CYfadrf_na_nice; i++)
		    CYfadrf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroSideforceParts -> storeCommands (*command_line);
	      CYfadrf_first=false;
	    }
	  break;
	}
      case CYfapf_flag:
	{
	  int CYfapf_index, i;
	  string CYfapf_file;
	  double flap_value;
	  CYfapf_file = aircraft_directory + linetoken3;
	  token4 >> CYfapf_index;
	  if (CYfapf_index < 0 || CYfapf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CYfapf_index > CYfapf_nf)
	    CYfapf_nf = CYfapf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CYfapf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CYfapf_fArray[CYfapf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CYfapf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(CYfapf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CYfapf_aArray, CYfapf_index);
	  d_1_to_2(datafile_yArray, CYfapf_pArray, CYfapf_index);
	  d_2_to_3(datafile_zArray, CYfapf_CYArray, CYfapf_index);
	  i_1_to_2(datafile_nxArray, CYfapf_nAlphaArray, CYfapf_index);
	  CYfapf_np[CYfapf_index] = datafile_ny;
	  if (CYfapf_first==true)
	    {
	      if (CYfapf_nice == 1)
		{
		  CYfapf_na_nice = datafile_nxArray[1];
		  CYfapf_np_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CYfapf_pArray_nice);
		  for (i=1; i<=CYfapf_na_nice; i++)
		    CYfapf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroSideforceParts -> storeCommands (*command_line);
	      CYfapf_first=false;
	    }
	  break;
	}
      case CYfarf_flag:
	{
	  int CYfarf_index, i;
	  string CYfarf_file;
	  double flap_value;
	  CYfarf_file = aircraft_directory + linetoken3;
	  token4 >> CYfarf_index;
	  if (CYfarf_index < 0 || CYfarf_index >= 30)
	    uiuc_warnings_errors(1, *command_line);
	  if (CYfarf_index > CYfarf_nf)
	    CYfarf_nf = CYfarf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> CYfarf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  CYfarf_fArray[CYfarf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (CYfarf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(CYfarf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, CYfarf_aArray, CYfarf_index);
	  d_1_to_2(datafile_yArray, CYfarf_rArray, CYfarf_index);
	  d_2_to_3(datafile_zArray, CYfarf_CYArray, CYfarf_index);
	  i_1_to_2(datafile_nxArray, CYfarf_nAlphaArray, CYfarf_index);
	  CYfarf_nr[CYfarf_index] = datafile_ny;
	  if (CYfarf_first==true)
	    {
	      if (CYfarf_nice == 1)
		{
		  CYfarf_na_nice = datafile_nxArray[1];
		  CYfarf_nr_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, CYfarf_rArray_nice);
		  for (i=1; i<=CYfarf_na_nice; i++)
		    CYfarf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroSideforceParts -> storeCommands (*command_line);
	      CYfarf_first=false;
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
