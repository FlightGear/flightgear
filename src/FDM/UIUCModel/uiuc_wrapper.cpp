/********************************************************************** 
 * 
 * FILENAME:     uiuc_wrapper.cpp 
 *
 * ---------------------------------------------------------------------- 
 *
 * DESCRIPTION:  A wrapper(interface) between the UIUC Aeromodel (C++ files) 
 *               and the LaRCsim FDM (C files)
 *
 * ----------------------------------------------------------------------
 * 
 * STATUS:       alpha version
 *
 * ----------------------------------------------------------------------
 * 
 * REFERENCES:   
 * 
 * ----------------------------------------------------------------------
 * 
 * HISTORY:      01/26/2000   initial release
 * 
 * ----------------------------------------------------------------------
 * 
 * AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
 * 
 * ----------------------------------------------------------------------
 * 
 * VARIABLES:
 * 
 * ----------------------------------------------------------------------
 * 
 * INPUTS:       *
 * 
 * ----------------------------------------------------------------------
 * 
 * OUTPUTS:      *
 * 
 * ----------------------------------------------------------------------
 * 
 * CALLED BY:    *
 * 
 * ----------------------------------------------------------------------
 * 
 * CALLS TO:     *
 * 
 * ----------------------------------------------------------------------
 * 
 * COPYRIGHT:    (C) 2000 by Michael Selig
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA or view http://www.gnu.org/copyleft/gpl.html.
 * 
 ***********************************************************************/


#include "uiuc_aircraft.h"
#include "uiuc_aircraftdir.h"
#include "uiuc_coefficients.h"
#include "uiuc_engine.h"
#include "uiuc_aerodeflections.h"
#include "uiuc_recorder.h"
#include "uiuc_menu.h"
#include "../LaRCsim/ls_generic.h"


extern "C" void uiuc_init_aeromodel ();
extern "C" void uiuc_force_moment(double dt);
extern "C" void uiuc_engine_routine();

AIRCRAFT *aircraft_ = new AIRCRAFT;
AIRCRAFTDIR *aircraftdir_ = new AIRCRAFTDIR;

void uiuc_init_aeromodel ()
{
  string aircraft;

  if (aircraft_dir != "")
    aircraft = aircraft_dir + "/";

  aircraft += "aircraft.dat";
  cout << "We are using "<< aircraft << endl;
  uiuc_initializemaps(); // Initialize the <string,int> maps
  uiuc_menu(aircraft);   // Read the specified aircraft file 
}

void uiuc_force_moment(double dt)
{
  double qS = Dynamic_pressure * Sw;
  double qScbar = qS * cbar;
  double qSb = qS * bw;

  uiuc_aerodeflections();
  uiuc_coefficients();
  
  /* Calculate the wind axis forces */
  F_X_wind = -1 * CD * qS;
  F_Y_wind = CY * qS;
  F_Z_wind = -1 * CL * qS;
    
  /* wind-axis to body-axis transformation */
  F_X_aero = F_X_wind * Cos_alpha * Cos_beta - F_Y_wind * Cos_alpha * Sin_beta - F_Z_wind * Sin_alpha;
  F_Y_aero = F_X_wind * Sin_beta + F_Y_wind * Cos_beta;
  F_Z_aero = F_X_wind * Sin_alpha * Cos_beta - F_Y_wind * Sin_alpha * Sin_beta + F_Z_wind * Cos_alpha;
  
  /* Moment calculations  */
  M_l_aero = Cl * qSb;
  M_m_aero = Cm * qScbar;
  M_n_aero = Cn * qSb; 

  uiuc_recorder(dt);
}

void uiuc_engine_routine()
{
  uiuc_engine();
}

//end uiuc_wrapper.cpp
