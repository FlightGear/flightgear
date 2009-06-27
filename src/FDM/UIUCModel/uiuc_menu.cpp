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
	       01/11/2002   (AP) Added keywords for bootTime
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
	       08/29/2002   (RD) Separated each primary keyword into its
	                    own function to speed up compile time
               08/29/2002   (RD w/ help from CO) Made changes to shorten
                            compile time.  [] RD to add more comments here.
               08/29/2003   (MSS) Adding new keywords for new engine model
                            and slipstream effects on tail.
               03/02/2003   (RD) Changed Cxfxxf areas so that there is a
                            conversion factor for flap angle
               03/03/2003   (RD) Added flap_cmd_record
               03/16/2003   (RD) Added trigger record flags
               04/02/2003   (RD) Tokens are now equal to 0 when no value
                            is given in the input file
               04/04/2003   (RD) To speed up compile time on this file,
                            each first level token now goes to its
                            own file uiuc_menu_*.cpp

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         http://www.jeffscott.net/
	       Robert Deters      <rdeters@uiuc.edu>
               Michael Selig      <m-selig@uiuc.edu>
               David Megginson    <david@megginson.com>
	       Ann Peedikayil     <peedikay@uiuc.edu>
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
               uiuc_menu_init()
               uiuc_menu_geometry()
               uiuc_menu_mass()
               uiuc_menu_controlSurface()
               uiuc_menu_CD()
               uiuc_menu_CL()
               uiuc_menu_Cm()
               uiuc_menu_CY()
               uiuc_menu_Cl()
               uiuc_menu_Cn()
               uiuc_menu_ice()
               uiuc_menu_engine()
               uiuc_menu_fog()
               uiuc_menu_gear()
               uiuc_menu_record()
               uiuc_menu_misc()

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include <simgear/compiler.h>


#include <cstdlib>
#include <string>
#include <iostream>

#include "uiuc_menu.h"

using std::cerr;
using std::cout;
using std::endl;

#ifndef _MSC_VER
using std::exit;
#endif

void uiuc_menu( string aircraft_name )
{
  string aircraft_directory;
  stack command_list;
  
  string linetoken1;
  string linetoken2;
  string linetoken3;
  string linetoken4;
  string linetoken5;
  string linetoken6;
  string linetoken7;
  string linetoken8;
  string linetoken9;
  string linetoken10;
  
  /* the following default setting should eventually be moved to a default or uiuc_init routine */

  recordRate = 1;       /* record every time step, default */
  recordStartTime = 0;  /* record from beginning of simulation */

/* set speed at which dynamic pressure terms will be accounted for,
   since if velocity is too small, coefficients will go to infinity */
  dyn_on_speed      = 33;    /* 20 kts (33 ft/sec), default */
  dyn_on_speed_zero = 0.5 * dyn_on_speed;    /* only used if use_dyn_on_speed_curve1 is used */
  bootindex = 0;  // the index for the bootTime

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
      if (linetoken2=="")
	linetoken2="0";
      linetoken3 = airplane -> getToken (*command_line, 3); 
      if (linetoken3=="")
	linetoken3="0";
      linetoken4 = airplane -> getToken (*command_line, 4); 
      if (linetoken4=="")
	linetoken4="0";
      linetoken5 = airplane -> getToken (*command_line, 5); 
      if (linetoken5=="")
	linetoken5="0";
      linetoken6 = airplane -> getToken (*command_line, 6); 
      if (linetoken6=="")
	linetoken6="0";
      linetoken7 = airplane -> getToken (*command_line, 7);
      if (linetoken7=="")
	linetoken7="0";
      linetoken8 = airplane -> getToken (*command_line, 8);
      if (linetoken8=="")
	linetoken8="0";
      linetoken9 = airplane -> getToken (*command_line, 9);
      if (linetoken9=="")
	linetoken9="0";
      linetoken10 = airplane -> getToken (*command_line, 10);
      if (linetoken10=="")
	linetoken10="0";
 
      //cout << linetoken1 << endl;
      //cout << linetoken2 << endl;
      //cout << linetoken3 << endl;
      //cout << linetoken4 << endl;
      //cout << linetoken5 << endl;
      //cout << linetoken6 << endl;
      //cout << linetoken7 << endl;
      //cout << linetoken8 << endl;
      //cout << linetoken9 << endl;
      //cout << linetoken10 << endl;
     
      //istrstream token3(linetoken3.c_str());
      //istrstream token4(linetoken4.c_str());
      //istrstream token5(linetoken5.c_str());
      //istrstream token6(linetoken6.c_str());
      //istrstream token7(linetoken7.c_str());
      //istrstream token8(linetoken8.c_str());
      //istrstream token9(linetoken9.c_str());
      //istrstream token10(linetoken10.c_str());

      switch (Keyword_map[linetoken1])
        {
        case init_flag:
          {
	    parse_init( linetoken2, linetoken3, linetoken4, 
			linetoken5, linetoken6, linetoken7,
			linetoken8, linetoken9, linetoken10,
			aircraft_directory, command_line );
            break;
          } // end init map
          
      
        case geometry_flag:
          {
	    parse_geometry( linetoken2, linetoken3, linetoken4,
			    linetoken5, linetoken6, linetoken7,
			    linetoken8, linetoken9, linetoken10,
			    aircraft_directory, command_line );
            break;
          } // end geometry map


        case controlSurface_flag:
          {
            parse_controlSurface( linetoken2, linetoken3, linetoken4,
                                  linetoken5, linetoken6, linetoken7,
				  linetoken8, linetoken9, linetoken10,
				  aircraft_directory, command_line );
            break;
          } // end controlSurface map


        case mass_flag:
          {
	    parse_mass( linetoken2, linetoken3, linetoken4,
			linetoken5, linetoken6, linetoken7,
			linetoken8, linetoken9, linetoken10,
			aircraft_directory, command_line );
            break;
          } // end mass map
          
          
        case engine_flag:
          {
            parse_engine( linetoken2, linetoken3, linetoken4,
			  linetoken5, linetoken6, linetoken7,
			  linetoken8, linetoken9, linetoken10,
			  aircraft_directory, command_line );
            break;
          } // end engine map
          
          
        case CD_flag:
          {
	    parse_CD( linetoken2, linetoken3, linetoken4,
		      linetoken5, linetoken6, linetoken7,
		      linetoken8, linetoken9, linetoken10,
		      aircraft_directory, command_line );
            break;
          } // end CD map

          
        case CL_flag:
          {
	    parse_CL( linetoken2, linetoken3, linetoken4,
		      linetoken5, linetoken6, linetoken7,
		      linetoken8, linetoken9, linetoken10,
		      aircraft_directory, command_line );
            break;
          } // end CL map


        case Cm_flag:
          {
	    parse_Cm( linetoken2, linetoken3, linetoken4,
		      linetoken5, linetoken6, linetoken7,
		      linetoken8, linetoken9, linetoken10,
		      aircraft_directory, command_line );
            break;
          } // end Cm map


        case CY_flag:
          {
	    parse_CY( linetoken2, linetoken3, linetoken4,
		      linetoken5, linetoken6, linetoken7,
		      linetoken8, linetoken9, linetoken10,
		      aircraft_directory, command_line );
            break;
          } // end CY map


        case Cl_flag:
          {
	    parse_Cl( linetoken2, linetoken3, linetoken4,
		      linetoken5, linetoken6, linetoken7,
		      linetoken8, linetoken9, linetoken10,
		      aircraft_directory, command_line );
            break;
          } // end Cl map


        case Cn_flag:
          {
	    parse_Cn( linetoken2, linetoken3, linetoken4,
		      linetoken5, linetoken6, linetoken7,
		      linetoken8, linetoken9, linetoken10,
		      aircraft_directory, command_line );
            break;
          } // end Cn map
          
        
        case gear_flag:
          {
	    parse_gear( linetoken2, linetoken3, linetoken4,
			linetoken5, linetoken6, linetoken7,
			linetoken8, linetoken9, linetoken10,
			aircraft_directory, command_line );
	    break;
          } // end gear map
      

        case ice_flag:
          {
	    parse_ice( linetoken2, linetoken3, linetoken4,
		       linetoken5, linetoken6, linetoken7,
		       linetoken8, linetoken9, linetoken10,
		       aircraft_directory, command_line );
            break;
          } // end ice map
         

	case fog_flag:
          {
	    parse_fog( linetoken2, linetoken3, linetoken4,
		       linetoken5, linetoken6, linetoken7,
		       linetoken8, linetoken9, linetoken10,
		       aircraft_directory, command_line );
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
	    parse_record( linetoken2, linetoken3, linetoken4, 
			  linetoken5, linetoken6, linetoken7,
			  linetoken8, linetoken9, linetoken10,
			  aircraft_directory, command_line );
            break;
          } // end record map               


        case misc_flag:
          {
	    parse_misc( linetoken2, linetoken3, linetoken4, 
			linetoken5, linetoken6, linetoken7,
			linetoken8, linetoken9, linetoken10,
			aircraft_directory, command_line );
            break;
          } // end misc map


        default:
          {
            if (linetoken1=="*")
                return;
	    if (ignore_unknown_keywords) {
	      // do nothing
	    } else {
	      // print error message
	      uiuc_warnings_errors(2, *command_line);
	    }
            break;
          }
        };
    } // end keyword map
  
  delete airplane;
}

// end menu.cpp
