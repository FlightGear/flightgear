/**********************************************************************
                                                                       
 FILENAME:     uiuc_menu_Cn.cpp

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

#include "uiuc_menu_Cn.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void parse_Cn( const string& linetoken2, const string& linetoken3,
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

    static bool Cnfabetaf_first = true;
    static bool Cnfadaf_first = true;
    static bool Cnfadrf_first = true;
    static bool Cnfapf_first = true;
    static bool Cnfarf_first = true;

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
	  Cnfada = aircraft_directory + linetoken3;
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
	  Cnfbetadr = aircraft_directory + linetoken3;
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
      case Cnfabetaf_flag:
	{
	  int Cnfabetaf_index, i;
	  string Cnfabetaf_file;
	  double flap_value;
	  Cnfabetaf_file = aircraft_directory + linetoken3;
	  token4 >> Cnfabetaf_index;
	  if (Cnfabetaf_index < 0 || Cnfabetaf_index >= 100)
	    uiuc_warnings_errors(1, *command_line);
	  if (Cnfabetaf_index > Cnfabetaf_nf)
	    Cnfabetaf_nf = Cnfabetaf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Cnfabetaf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Cnfabetaf_fArray[Cnfabetaf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Cnfabetaf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Cnfabetaf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Cnfabetaf_aArray, Cnfabetaf_index);
	  d_1_to_2(datafile_yArray, Cnfabetaf_betaArray, Cnfabetaf_index);
	  d_2_to_3(datafile_zArray, Cnfabetaf_CnArray, Cnfabetaf_index);
	  i_1_to_2(datafile_nxArray, Cnfabetaf_nAlphaArray, Cnfabetaf_index);
	  Cnfabetaf_nbeta[Cnfabetaf_index] = datafile_ny;
	  if (Cnfabetaf_first==true)
	    {
	      if (Cnfabetaf_nice == 1)
		{
		  Cnfabetaf_na_nice = datafile_nxArray[1];
		  Cnfabetaf_nb_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Cnfabetaf_bArray_nice);
		  for (i=1; i<=Cnfabetaf_na_nice; i++)
		    Cnfabetaf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroYawParts -> storeCommands (*command_line);
	      Cnfabetaf_first=false;
	    }
	  break;
	}
      case Cnfadaf_flag:
	{
	  int Cnfadaf_index, i;
	  string Cnfadaf_file;
	  double flap_value;
	  Cnfadaf_file = aircraft_directory + linetoken3;
	  token4 >> Cnfadaf_index;
	  if (Cnfadaf_index < 0 || Cnfadaf_index >= 100)
	    uiuc_warnings_errors(1, *command_line);
	  if (Cnfadaf_index > Cnfadaf_nf)
	    Cnfadaf_nf = Cnfadaf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Cnfadaf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Cnfadaf_fArray[Cnfadaf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Cnfadaf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Cnfadaf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Cnfadaf_aArray, Cnfadaf_index);
	  d_1_to_2(datafile_yArray, Cnfadaf_daArray, Cnfadaf_index);
	  d_2_to_3(datafile_zArray, Cnfadaf_CnArray, Cnfadaf_index);
	  i_1_to_2(datafile_nxArray, Cnfadaf_nAlphaArray, Cnfadaf_index);
	  Cnfadaf_nda[Cnfadaf_index] = datafile_ny;
	  if (Cnfadaf_first==true)
	    {
	      if (Cnfadaf_nice == 1)
		{
		  Cnfadaf_na_nice = datafile_nxArray[1];
		  Cnfadaf_nda_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Cnfadaf_daArray_nice);
		  for (i=1; i<=Cnfadaf_na_nice; i++)
		    Cnfadaf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroYawParts -> storeCommands (*command_line);
	      Cnfadaf_first=false;
	    }
	  break;
	}
      case Cnfadrf_flag:
	{
	  int Cnfadrf_index, i;
	  string Cnfadrf_file;
	  double flap_value;
	  Cnfadrf_file = aircraft_directory + linetoken3;
	  token4 >> Cnfadrf_index;
	  if (Cnfadrf_index < 0 || Cnfadrf_index >= 100)
	    uiuc_warnings_errors(1, *command_line);
	  if (Cnfadrf_index > Cnfadrf_nf)
	    Cnfadrf_nf = Cnfadrf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Cnfadrf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Cnfadrf_fArray[Cnfadrf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Cnfadrf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Cnfadrf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Cnfadrf_aArray, Cnfadrf_index);
	  d_1_to_2(datafile_yArray, Cnfadrf_drArray, Cnfadrf_index);
	  d_2_to_3(datafile_zArray, Cnfadrf_CnArray, Cnfadrf_index);
	  i_1_to_2(datafile_nxArray, Cnfadrf_nAlphaArray, Cnfadrf_index);
	  Cnfadrf_ndr[Cnfadrf_index] = datafile_ny;
	  if (Cnfadrf_first==true)
	    {
	      if (Cnfadrf_nice == 1)
		{
		  Cnfadrf_na_nice = datafile_nxArray[1];
		  Cnfadrf_ndr_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Cnfadrf_drArray_nice);
		  for (i=1; i<=Cnfadrf_na_nice; i++)
		    Cnfadrf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroYawParts -> storeCommands (*command_line);
	      Cnfadrf_first=false;
	    }
	  break;
	}
      case Cnfapf_flag:
	{
	  int Cnfapf_index, i;
	  string Cnfapf_file;
	  double flap_value;
	  Cnfapf_file = aircraft_directory + linetoken3;
	  token4 >> Cnfapf_index;
	  if (Cnfapf_index < 0 || Cnfapf_index >= 100)
	    uiuc_warnings_errors(1, *command_line);
	  if (Cnfapf_index > Cnfapf_nf)
	    Cnfapf_nf = Cnfapf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Cnfapf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Cnfapf_fArray[Cnfapf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Cnfapf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Cnfapf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Cnfapf_aArray, Cnfapf_index);
	  d_1_to_2(datafile_yArray, Cnfapf_pArray, Cnfapf_index);
	  d_2_to_3(datafile_zArray, Cnfapf_CnArray, Cnfapf_index);
	  i_1_to_2(datafile_nxArray, Cnfapf_nAlphaArray, Cnfapf_index);
	  Cnfapf_np[Cnfapf_index] = datafile_ny;
	  if (Cnfapf_first==true)
	    {
	      if (Cnfapf_nice == 1)
		{
		  Cnfapf_na_nice = datafile_nxArray[1];
		  Cnfapf_np_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Cnfapf_pArray_nice);
		  for (i=1; i<=Cnfapf_na_nice; i++)
		    Cnfapf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroYawParts -> storeCommands (*command_line);
	      Cnfapf_first=false;
	    }
	  break;
	}
      case Cnfarf_flag:
	{
	  int Cnfarf_index, i;
	  string Cnfarf_file;
	  double flap_value;
	  Cnfarf_file = aircraft_directory + linetoken3;
	  token4 >> Cnfarf_index;
	  if (Cnfarf_index < 0 || Cnfarf_index >= 100)
	    uiuc_warnings_errors(1, *command_line);
	  if (Cnfarf_index > Cnfarf_nf)
	    Cnfarf_nf = Cnfarf_index;
	  token5 >> flap_value;
	  token6 >> token_value_convert1;
	  token7 >> token_value_convert2;
	  token8 >> token_value_convert3;
	  token9 >> token_value_convert4;
	  token10 >> Cnfarf_nice;
	  convert_z = uiuc_convert(token_value_convert1);
	  convert_x = uiuc_convert(token_value_convert2);
	  convert_y = uiuc_convert(token_value_convert3);
	  convert_f = uiuc_convert(token_value_convert4);
	  Cnfarf_fArray[Cnfarf_index] = flap_value * convert_f;
	  /* call 2D File Reader with file name (Cnfarf_file) and 
	     conversion factors; function returns array of 
	     elevator deflections (deArray) and corresponding 
	     alpha (aArray) and delta CZ (CZArray) values and 
	     max number of terms in alpha arrays (nAlphaArray) 
	     and delfection array (nde) */
	  uiuc_2DdataFileReader(Cnfarf_file,
				datafile_xArray,
				datafile_yArray,
				datafile_zArray,
				datafile_nxArray,
				datafile_ny);
	  d_2_to_3(datafile_xArray, Cnfarf_aArray, Cnfarf_index);
	  d_1_to_2(datafile_yArray, Cnfarf_rArray, Cnfarf_index);
	  d_2_to_3(datafile_zArray, Cnfarf_CnArray, Cnfarf_index);
	  i_1_to_2(datafile_nxArray, Cnfarf_nAlphaArray, Cnfarf_index);
	  Cnfarf_nr[Cnfarf_index] = datafile_ny;
	  if (Cnfarf_first==true)
	    {
	      if (Cnfarf_nice == 1)
		{
		  Cnfarf_na_nice = datafile_nxArray[1];
		  Cnfarf_nr_nice = datafile_ny;
		  d_1_to_1(datafile_yArray, Cnfarf_rArray_nice);
		  for (i=1; i<=Cnfarf_na_nice; i++)
		    Cnfarf_aArray_nice[i] = datafile_xArray[1][i];
		}
	      aeroYawParts -> storeCommands (*command_line);
	      Cnfarf_first=false;
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
