/**********************************************************************

 FILENAME:     uiuc_engine.cpp

----------------------------------------------------------------------

 DESCRIPTION:  determine the engine forces and moments
               
----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   simple and c172 models based on portions of 
               c172_engine.c, called from ls_model;
               cherokee model based on cherokee_engine.c

----------------------------------------------------------------------

 HISTORY:      01/30/2000   initial release

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
               Michael Selig      <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -engine model

----------------------------------------------------------------------

 OUTPUTS:      -F_X_engine
               -F_Z_engine
               -M_m_engine

----------------------------------------------------------------------

 CALLED BY:   uiuc_wrapper.cpp 

----------------------------------------------------------------------

 CALLS TO:     none

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

#include "uiuc_engine.h"

FG_USING_STD(cerr);

void uiuc_engine() 
{
  stack command_list;
  string linetoken1;
  string linetoken2;
  
  Throttle[3] = Throttle_pct;

  command_list = engineParts -> getCommands();

  /*
  if (command_list.begin() == command_list.end())
  {
        cerr << "ERROR: Engine not specified. Aircraft cannot fly without the engine" << endl;
        exit(-1);
  }
  */
 
  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      //cout << *command_line << endl;
      
      linetoken1 = engineParts -> getToken(*command_line, 1);
      linetoken2 = engineParts -> getToken(*command_line, 2);
      
      switch(engine_map[linetoken2])
        {
        case simpleSingle_flag:
          {
            F_X_engine = Throttle[3] * simpleSingleMaxThrust;
            break;
          }
        case c172_flag:
          {
            //c172 engine lines ... looks like 0.83 is just a thrust increase
            F_X_engine = Throttle[3] * 350 / 0.83;
            F_Z_engine = Throttle[3] * 4.9 / 0.83;
            M_m_engine = F_X_engine * 0.734 * cbar;
            break;
          }
        case cherokee_flag:
          {
            static float
              dP = (180.0-117.0)*745.7,   // Watts
              dn = (2700.0-2350.0)/60.0,  // d_rpm (I mean d_rps, in seconds)
              D  = 6.17*0.3048,           // prop diameter
              dPh = (58.0-180.0)*745.7,   // change of power as function of height
              dH = 25000.0*0.3048;

            float
              n,   // [rps]
              H,   // altitude [m]
              J,   // advance ratio (ratio of horizontal speed to prop tip speed)
              T,   // thrust [N]
              V,   // velocity [m/s]
              P,   // power [W]
              eta_engine; // engine efficiency

            /* assumption -> 0.0 <= Throttle[3] <=1.0 */
            P = fabs(Throttle[3]) * 180.0 * 745.7; /*180.0*745.7 ->max avail power [W]*/
            n = dn/dP * (P-117.0*745.7) + 2350.0/60.0;

            /*  V  [m/s]   */
            V = (V_rel_wind < 10.0 ? 10.0 : V_rel_wind*0.3048);
            J = V / n / D;

            /* Propeller efficiency */
            eta_engine = (J < 0.7 ? ((0.8-0.55)/(.7-.3)*(J-0.3) + 0.55) : 
                          (J > 0.85 ? ((0.6-0.8)/(1.0-0.85)*(J-0.85) + 0.8) : 0.8));

            /* power on Altitude */
            H = Altitude * 0.3048;       /* H == Altitude [m] */
            P *= (dPh/dH * H + 180.0*745.7) / (180.0*745.7);
            T = eta_engine * P/V;        /* Thrust [N] */ 

            /*assumption: Engine's line of thrust passes through cg */
            F_X_engine = T * 0.2248;     /* F_X_engine in lb */
            F_Y_engine = 0.0;
            F_Z_engine = 0.0;
            break;
          }
        };
    }
  return;
}

// end uiuc_engine.cpp
