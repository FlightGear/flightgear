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
               03/09/2001   (DPM) added support for gear.
	       06/18/2001   (RD) Added Alpha, Beta, U_body,
	                    V_body, and W_body to init map.  Added 
			    aileron_input, rudder_input, pilot_elev_no,
			    pilot_ail_no, and pilot_rud_no to
			    controlSurface map.  Added Throttle_pct_input
			    to engine map.  Added CZfa to CL map.
	       07/05/2001   (RD) Changed pilot_elev_no = true to pilot_
	                    elev_no_check=false.  This is to allow pilot
			    to fly aircraft after input files have been
			    used.
 	       08/27/2001   (RD) Added xxx_init_true and xxx_init for
	                    P_body, Q_body, R_body, Phi, Theta, Psi,
			    U_body, V_body, and W_body to help in
			    starting the A/C at an initial condition.
               10/25/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model at zero flaps
			    (Cxfxxf0)
               11/12/2001   (RD) Added new variables needed for the non-
	                    linear Twin Otter model with flaps
			    (Cxfxxf).  Removed zero flap variables.
			    Added minmaxfind() which is needed for non-
			    linear variables
	       02/13/2002   (RD) Added variables so linear aero model
	                    values can be recorded
	       02/18/2002   (RD) Added variables necessary to use the
	                    uiuc_3Dinterp_quick() function.  Takes
			    advantage of data in a "nice" form (data
			    that are in a rectangular matrix).
	       03/13/2002   (RD) Added aircraft_directory so full path
	                    is no longer needed in the aircraft.dat file
	       04/02/2002   (RD) Removed minmaxfind() since it was no
	                    longer needed.  Added d_2_to_3(),
			    d_1_to_2(), and i_1_to_2() so uiuc_menu()
			    will compile with certain compilers.

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
	       Robert Deters      <rdeters@uiuc.edu>
               Michael Selig      <m-selig@uiuc.edu>
               David Megginson    <david@megginson.com>

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
#include <string.h>
#include STL_IOSTREAM

#include "uiuc_menu.h"

#if !defined (SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cerr);
SG_USING_STD(cout);
SG_USING_STD(endl);

#  ifndef _MSC_VER
SG_USING_STD(exit);
#  endif
#endif

bool check_float(string  &token)
{
  float value;
  istrstream stream(token.c_str()); 
  return (stream >> value);
}

//void minmaxfind( bool tarray[30], int &tmin, int &tmax)
//{
//  int n=0;
//  bool first=true;
//
//  while (n<=30)
//    {
//      if (tarray[n])
//	{
//	  if (first)
//	    {
//	      tmin=n;
//	      first=false;
//	    }
//	  tmax=n;
//	}
//      n++;
//    }
//}

void d_2_to_3( double array2D[100][100], double array3D[][100][100], int index3D)
{
  for (register int i=0; i<=99; i++)
    {
      for (register int j=1; j<=99; j++)
	{
	  array3D[index3D][i][j]=array2D[i][j];
	}
    }
}

void d_1_to_2( double array1D[100], double array2D[][100], int index2D)
{
  for (register int i=0; i<=99; i++)
    {
      array2D[index2D][i]=array1D[i];
    }
}

void d_1_to_1( double array1[100], double array2[100] )
{
   memcpy( array2, array1, sizeof(array2) );
}

void i_1_to_2( int array1D[100], int array2D[][100], int index2D)
{
  for (register int i=0; i<=99; i++)
    {
      array2D[index2D][i]=array1D[i];
    }
}

void uiuc_menu( string aircraft_name )
{
  string aircraft_directory;
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
  string linetoken7;
  string linetoken8;
  string linetoken9;

  double datafile_xArray[100][100], datafile_yArray[100];
  double datafile_zArray[100][100];
  int datafile_nxArray[100], datafile_ny;

  bool CXfabetaf_first = true;
  bool CXfadef_first = true;
  bool CXfaqf_first = true;
  bool CZfabetaf_first = true;
  bool CZfadef_first = true;
  bool CZfaqf_first = true;
  bool Cmfabetaf_first = true;
  bool Cmfadef_first = true;
  bool Cmfaqf_first = true;
  bool CYfabetaf_first = true;
  bool CYfadaf_first = true;
  bool CYfadrf_first = true;
  bool CYfapf_first = true;
  bool CYfarf_first = true;
  bool Clfabetaf_first = true;
  bool Clfadaf_first = true;
  bool Clfadrf_first = true;
  bool Clfapf_first = true;
  bool Clfarf_first = true;
  bool Cnfabetaf_first = true;
  bool Cnfadaf_first = true;
  bool Cnfadrf_first = true;
  bool Cnfapf_first = true;
  bool Cnfarf_first = true;

  /* the following default setting should eventually be moved to a default or uiuc_init routine */

  recordRate = 1;       /* record every time step, default */
  recordStartTime = 0;  /* record from beginning of simulation */

/* set speed at which dynamic pressure terms will be accounted for,
   since if velocity is too small, coefficients will go to infinity */
  dyn_on_speed      = 33;    /* 20 kts, default */
  dyn_on_speed_zero = 0.5 * dyn_on_speed;    /* only used of use_dyn_on_speed_curve1 is used */



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
  
  //construct aircraft-directory
  aircraft_directory = aircraft_name;
  int index_aircraft_dat = aircraft_directory.find("aircraft.dat");
  aircraft_directory.erase(index_aircraft_dat,12);

  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      cout << *command_line << endl;

      linetoken1 = airplane -> getToken (*command_line, 1); 
      linetoken2 = airplane -> getToken (*command_line, 2); 
      linetoken3 = airplane -> getToken (*command_line, 3); 
      linetoken4 = airplane -> getToken (*command_line, 4); 
      linetoken5 = airplane -> getToken (*command_line, 5); 
      linetoken6 = airplane -> getToken (*command_line, 6); 
      linetoken7 = airplane -> getToken (*command_line, 7);
      linetoken8 = airplane -> getToken (*command_line, 8);
      linetoken9 = airplane -> getToken (*command_line, 9);
      
      istrstream token3(linetoken3.c_str());
      istrstream token4(linetoken4.c_str());
      istrstream token5(linetoken5.c_str());
      istrstream token6(linetoken6.c_str());
      istrstream token7(linetoken7.c_str());
      istrstream token8(linetoken8.c_str());
      istrstream token9(linetoken9.c_str());

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

                  P_body_init_true = true;
                  P_body_init = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Q_body_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
		  Q_body_init_true = true;
                  Q_body_init = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case R_body_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
		  R_body_init_true = true;
                  R_body_init = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Phi_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
		  Phi_init_true = true;
                  Phi_init = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Theta_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
		  Theta_init_true = true;
                  Theta_init = token_value;
                  initParts -> storeCommands (*command_line);
                  break;
                }
              case Psi_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
		  Psi_init_true = true;
                  Psi_init = token_value;
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
              case use_V_rel_wind_2U_flag:
                {
                  use_V_rel_wind_2U = true;
                  break;
                }
              case nondim_rate_V_rel_wind_flag:
                {
                  nondim_rate_V_rel_wind = true;
                  break;
                }
              case use_abs_U_body_2U_flag:
                {
                  use_abs_U_body_2U = true;
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
              case dyn_on_speed_zero_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  dyn_on_speed_zero = token_value;
                  break;
                }
              case use_dyn_on_speed_curve1_flag:
                {
                  use_dyn_on_speed_curve1 = true;
                  break;
                }
	      case Alpha_flag:
		{
		  if (check_float(linetoken3))
		    token3 >> token_value;
		  else
		    uiuc_warnings_errors(1, *command_line);

		  Alpha_init_true = true;
		  Alpha_init = token_value * DEG_TO_RAD;
		  break;
		}
	      case Beta_flag:
		{
		  if (check_float(linetoken3))
		    token3 >> token_value;
		  else
		    uiuc_warnings_errors(1, *command_line);

		  Beta_init_true = true;
		  Beta_init = token_value * DEG_TO_RAD;
		  break;
		}
	      case U_body_flag:
		{
		  if (check_float(linetoken3))
		    token3 >> token_value;
		  else
		    uiuc_warnings_errors(1, *command_line);

		  U_body_init_true = true;
		  U_body_init = token_value;
		  break;
		}
	      case V_body_flag:
		{
		  if (check_float(linetoken3))
		    token3 >> token_value;
		  else
		    uiuc_warnings_errors(1, *command_line);

		  V_body_init_true = true;
		  V_body_init = token_value;
		  break;
		}
	      case W_body_flag:
		{
		  if (check_float(linetoken3))
		    token3 >> token_value;
		  else
		    uiuc_warnings_errors(1, *command_line);

		  W_body_init_true = true;
		  W_body_init = token_value;
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
                  elevator_input_file = aircraft_directory + linetoken3;
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
	      case aileron_input_flag:
                {
                  aileron_input = true;
                  aileron_input_file = aircraft_directory + linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  convert_y = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  uiuc_1DdataFileReader(aileron_input_file,
                                        aileron_input_timeArray,
                                        aileron_input_daArray,
                                        aileron_input_ntime);
                  token6 >> token_value;
                  aileron_input_startTime = token_value;
                  break;
                }
	      case rudder_input_flag:
                {
                  rudder_input = true;
                  rudder_input_file = aircraft_directory + linetoken3;
                  token4 >> token_value_convert1;
                  token5 >> token_value_convert2;
                  convert_y = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  uiuc_1DdataFileReader(rudder_input_file,
                                        rudder_input_timeArray,
                                        rudder_input_drArray,
                                        rudder_input_ntime);
                  token6 >> token_value;
                  rudder_input_startTime = token_value;
                  break;
                }
	      case pilot_elev_no_flag:
		{
		  pilot_elev_no_check = true;
		  break;
		}
	      case pilot_ail_no_flag:
		{
		  pilot_ail_no_check = true;
		  break;
		}
	      case pilot_rud_no_flag:
		{
		  pilot_rud_no_check = true;
		  break;
		}
	      case flap_max_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  flap_max = token_value;
                  break;
                }
	      case flap_rate_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  flap_rate = token_value;
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
              case Mass_appMass_ratio_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Mass_appMass_ratio = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case I_xx_appMass_ratio_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  I_xx_appMass_ratio = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case I_yy_appMass_ratio_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  I_yy_appMass_ratio = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case I_zz_appMass_ratio_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  I_zz_appMass_ratio = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case Mass_appMass_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  Mass_appMass = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case I_xx_appMass_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  I_xx_appMass = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case I_yy_appMass_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  I_yy_appMass = token_value;
                  massParts -> storeCommands (*command_line);
                  break;
                }
              case I_zz_appMass_flag:
                {
                  if (check_float(linetoken3))
                    token3 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
                  
                  I_zz_appMass = token_value;
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
              case Throttle_pct_input_flag:
                {
                  Throttle_pct_input = true;
                  Throttle_pct_input_file = aircraft_directory + linetoken3;
		  token4 >> token_value_convert1;
		  token5 >> token_value_convert2;
		  convert_y = uiuc_convert(token_value_convert1);
		  convert_x = uiuc_convert(token_value_convert2);
                  uiuc_1DdataFileReader(Throttle_pct_input_file,
                                        Throttle_pct_input_timeArray,
                                        Throttle_pct_input_dTArray,
                                        Throttle_pct_input_ntime);
                  token6 >> token_value;
                  Throttle_pct_input_startTime = token_value;
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
		  CXfabetaf_fArray[CXfabetaf_index] = flap_value;
		  token6 >> token_value_convert1;
		  token7 >> token_value_convert2;
		  token8 >> token_value_convert3;
		  token9 >> CXfabetaf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CXfabetaf_bArray_nice, datafile_yArray);
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
		  CXfadef_fArray[CXfadef_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> CXfadef_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CXfadef_deArray_nice, datafile_yArray);
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
		  CXfaqf_fArray[CXfaqf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> CXfaqf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CXfaqf_qArray_nice, datafile_yArray);
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
		  CZfabetaf_fArray[CZfabetaf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> CZfabetaf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CZfabetaf_bArray_nice, datafile_yArray);
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
		  CZfadef_fArray[CZfadef_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> CZfadef_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CZfadef_deArray_nice, datafile_yArray);
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
		  CZfaqf_fArray[CZfaqf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> CZfaqf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CZfaqf_qArray_nice, datafile_yArray);
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
		  Cmfabetaf_fArray[Cmfabetaf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Cmfabetaf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Cmfabetaf_bArray_nice, datafile_yArray);
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
		  Cmfadef_fArray[Cmfadef_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Cmfadef_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Cmfadef_deArray_nice, datafile_yArray);
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
		  Cmfaqf_fArray[Cmfaqf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Cmfaqf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Cmfaqf_qArray_nice, datafile_yArray);
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
		  CYfabetaf_fArray[CYfabetaf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> CYfabetaf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CYfabetaf_bArray_nice, datafile_yArray);
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
		  CYfadaf_fArray[CYfadaf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> CYfadaf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CYfadaf_daArray_nice, datafile_yArray);
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
		  CYfadrf_fArray[CYfadrf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> CYfadrf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CYfadrf_drArray_nice, datafile_yArray);
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
		  CYfapf_fArray[CYfapf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> CYfapf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CYfapf_pArray_nice, datafile_yArray);
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
		  CYfarf_fArray[CYfarf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> CYfarf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(CYfarf_rArray_nice, datafile_yArray);
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
		  Clfabetaf_fArray[Clfabetaf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Clfabetaf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Clfabetaf_bArray_nice, datafile_yArray);
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
		  Clfadaf_fArray[Clfadaf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Clfadaf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Clfadaf_daArray_nice, datafile_yArray);
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
		  Clfadrf_fArray[Clfadrf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Clfadrf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Clfadrf_drArray_nice, datafile_yArray);
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
		  Clfapf_fArray[Clfapf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Clfapf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Clfapf_pArray_nice, datafile_yArray);
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
		  Clfarf_fArray[Clfarf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Clfarf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Clfarf_rArray_nice, datafile_yArray);
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
		  Cnfabetaf_fArray[Cnfabetaf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Cnfabetaf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Cnfabetaf_bArray_nice, datafile_yArray);
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
		  Cnfadaf_fArray[Cnfadaf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Cnfadaf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Cnfadaf_daArray_nice, datafile_yArray);
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
		  Cnfadrf_fArray[Cnfadrf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Cnfadrf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Cnfadrf_drArray_nice, datafile_yArray);
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
		  Cnfapf_fArray[Cnfapf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Cnfapf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Cnfapf_pArray_nice, datafile_yArray);
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
		  Cnfarf_fArray[Cnfarf_index] = flap_value;
                  token6 >> token_value_convert1;
                  token7 >> token_value_convert2;
                  token8 >> token_value_convert3;
		  token9 >> Cnfarf_nice;
                  convert_z = uiuc_convert(token_value_convert1);
                  convert_x = uiuc_convert(token_value_convert2);
                  convert_y = uiuc_convert(token_value_convert3);
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
			  d_1_to_1(Cnfarf_rArray_nice, datafile_yArray);
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
                  uiuc_warnings_errors(2, *command_line);
                  break;
                }
              };
            break;
          } // end Cn map
          
        
        case gear_flag:
          {
	    int index;
	    token3 >> index;
	    if (index < 0 || index >= 16)
	      uiuc_warnings_errors(1, *command_line);
            switch(gear_map[linetoken2])
              {
	      case Dx_gear_flag:
		{
                  if (check_float(linetoken3))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
		  D_gear_v[index][0] = token_value;
		  gear_model[index] = true;
		  break;
		}
	      case Dy_gear_flag:
		{
                  if (check_float(linetoken3))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
		  D_gear_v[index][1] = token_value;
		  gear_model[index] = true;
		  break;
		}
	      case Dz_gear_flag:
		{
                  if (check_float(linetoken3))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
		  D_gear_v[index][2] = token_value;
		  gear_model[index] = true;
		  break;
		}
	      case cgear_flag:
		{
                  if (check_float(linetoken3))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
		  cgear[index] = token_value;
		  gear_model[index] = true;
		  break;
		}
              case kgear_flag:
                {
                  if (check_float(linetoken3))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
		  kgear[index] = token_value;
		  gear_model[index] = true;
                  break;
                }
	      case muGear_flag:
		{
                  if (check_float(linetoken3))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
		  muGear[index] = token_value;
		  gear_model[index] = true;
		  break;
		}
	      case strutLength_flag:
		{
                  if (check_float(linetoken3))
                    token4 >> token_value;
                  else
                    uiuc_warnings_errors(1, *command_line);
		  strutLength[index] = token_value;
		  gear_model[index] = true;
		  break;
		}
              default:
                {
                  uiuc_warnings_errors(2, *command_line);
		  break;
                }
              };
	    break;
          } // end gear map
      

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
         

	case fog_flag:
          {
	    switch(fog_map[linetoken2])
              {
              case fog_segments_flag:
                {
                  if (check_float(linetoken3))
		    token3 >> token_value_convert1;
		  else
		    uiuc_warnings_errors(1, *command_line);

	          if (token_value_convert1 < 1 || token_value_convert1 > 100)
		    uiuc_warnings_errors(1, *command_line);
		  
		  fog_field = true;
		  fog_point_index = 0;
		  delete[] fog_time;
		  delete[] fog_value;
		  fog_segments = token_value_convert1;
		  fog_time = new double[fog_segments+1];
		  fog_time[0] = 0.0;
		  fog_value = new int[fog_segments+1];
		  fog_value[0] = 0;
		  
		  break;
		}
	      case fog_point_flag:
		{
		  if (check_float(linetoken3))
		    token3 >> token_value;
	          else
		    uiuc_warnings_errors(1, *command_line);

		  if (token_value < 0.1)
		    uiuc_warnings_errors(1, *command_line);

		  if (check_float(linetoken4))
		    token4 >> token_value_convert1;
		  else
		    uiuc_warnings_errors(1, *command_line);

		  if (token_value_convert1 < -1000 || token_value_convert1 > 1000)
		    uiuc_warnings_errors(1, *command_line);

		  if (fog_point_index == fog_segments || fog_point_index == -1)
		    uiuc_warnings_errors(1, *command_line);

		  fog_point_index++;
		  fog_time[fog_point_index] = token_value;
		  fog_value[fog_point_index] = token_value_convert1;

		  break;
		}
	      default:
		{
		  uiuc_warnings_errors(2, *command_line);
		  break;
		}
	      };
	    break;
	  } // end fog map	  
	  



	  

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
              case flap_goal_record:
                {
                  recordParts -> storeCommands (*command_line);
                  break;
                }
              case flap_pos_record:
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
	      case CXfabetafI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CXfadefI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CXfaqfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CDo_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CDK_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CD_a_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CD_adot_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CD_q_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CD_ih_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CD_de_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CXo_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CXK_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CX_a_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CX_a2_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CX_a3_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CX_adot_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CX_q_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CX_de_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CX_dr_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CX_df_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CX_adf_save_record:
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
	      case CZfaI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZfabetafI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZfadefI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZfaqfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CLo_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CL_a_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CL_adot_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CL_q_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CL_ih_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CL_de_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZo_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZ_a_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZ_a2_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZ_a3_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZ_adot_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZ_q_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZ_de_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZ_deb2_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZ_df_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CZ_adf_save_record:
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
	      case CmfabetafI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CmfadefI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CmfaqfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cmo_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cm_a_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cm_a2_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cm_adot_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cm_q_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cm_ih_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cm_de_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cm_b2_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cm_r_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cm_df_save_record:
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
	      case CYfabetafI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CYfadafI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CYfadrfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CYfapfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CYfarfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CYo_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CY_beta_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CY_p_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CY_r_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CY_da_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CY_dr_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CY_dra_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CY_bdot_save_record:
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
	      case ClfabetafI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case ClfadafI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case ClfadrfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case ClfapfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case ClfarfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Clo_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cl_beta_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cl_p_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cl_r_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cl_da_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cl_dr_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cl_daa_save_record:
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
	      case CnfabetafI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CnfadafI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CnfadrfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CnfapfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case CnfarfI_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cno_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cn_beta_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cn_p_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cn_r_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cn_da_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cn_dr_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cn_q_save_record:
		{
		  recordParts -> storeCommands (*command_line);
		  break;
		}
	      case Cn_b3_save_record:
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
