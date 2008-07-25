/**********************************************************************

 FILENAME:     uiuc_controlInput.cpp

----------------------------------------------------------------------

 DESCRIPTION:  sets control surface deflections for specified input 
               modes

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      04/08/2000   initial release
               06/18/2001   (RD) Added aileron_input and rudder_input
	       07/05/2001   (RD) Code added to allow the pilot to fly
	                    aircraft after the control surface input
			    files have been used.

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>
               Robert Deters      <rdeters@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -Simtime
               -deflection times
               -deflection angles

----------------------------------------------------------------------

 OUTPUTS:      -elevator deflection

----------------------------------------------------------------------

 CALLED BY:    uiuc_coefficients.cpp

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include <simgear/compiler.h>

#include "uiuc_controlInput.h"

#include <iostream>

void uiuc_controlInput()
{
  // elevator step input
  if (elevator_step)
    {
      if (Simtime >= elevator_step_startTime)
        {
          elevator = elevator + elevator_step_angle;
        }
    }

  // elevator singlet input
  if (elevator_singlet)
    {
      if (Simtime >= elevator_singlet_startTime && 
          Simtime <= (elevator_singlet_startTime + elevator_singlet_duration))
        {
          elevator = elevator + elevator_singlet_angle;
        }
    }

  // elevator doublet input
  if (elevator_doublet)
    {
      if (Simtime >= elevator_doublet_startTime && 
          Simtime <= (elevator_doublet_startTime + elevator_doublet_duration/2))
        {
          elevator = elevator + elevator_doublet_angle;
        }
      else if (Simtime >= (elevator_doublet_startTime + elevator_doublet_duration/2) && 
               Simtime <= (elevator_doublet_startTime + elevator_doublet_duration))
        {
          elevator = elevator - elevator_doublet_angle;
        }
    }

  // elevator input
  if (elevator_input)
    {
      double elevator_input_endTime = elevator_input_timeArray[elevator_input_ntime];
      if (Simtime >= elevator_input_startTime && 
          Simtime <= (elevator_input_startTime + elevator_input_endTime))
        {
          double time = Simtime - elevator_input_startTime;
	  if (pilot_elev_no_check)
	    {
	      elevator = 0 + elevator_tab;
	      pilot_elev_no = true;
	    }
          elevator = elevator + 
            uiuc_1Dinterpolation(elevator_input_timeArray,
                                 elevator_input_deArray,
                                 elevator_input_ntime,
                                 time);
        }
    }

  // aileron input
  if (aileron_input)
    {
      double aileron_input_endTime = aileron_input_timeArray[aileron_input_ntime];
      if (Simtime >= aileron_input_startTime && 
          Simtime <= (aileron_input_startTime + aileron_input_endTime))
        {
          double time = Simtime - aileron_input_startTime;
	  if (pilot_ail_no_check)
	    {
	      aileron = 0;
	      if (Simtime==0)             //7-25-01 (RD) Added because
		pilot_ail_no = false;     //segmentation fault is given
	      else                        //with FG 0.7.8
		pilot_ail_no = true;
	    }
          aileron = aileron + 
            uiuc_1Dinterpolation(aileron_input_timeArray,
                                 aileron_input_daArray,
                                 aileron_input_ntime,
                                 time);
        }
    }

  // rudder input
  if (rudder_input)
    {
      double rudder_input_endTime = rudder_input_timeArray[rudder_input_ntime];
      if (Simtime >= rudder_input_startTime && 
          Simtime <= (rudder_input_startTime + rudder_input_endTime))
        {
          double time = Simtime - rudder_input_startTime;
	  if (pilot_rud_no_check)
	    {
	      rudder = 0;
	      pilot_rud_no = true;
	    }
          rudder = rudder + 
            uiuc_1Dinterpolation(rudder_input_timeArray,
                                 rudder_input_drArray,
                                 rudder_input_ntime,
                                 time);
        }
    }

  if (flap_pos_input)
    {
      double flap_pos_input_endTime = flap_pos_input_timeArray[flap_pos_input_ntime];
      if (Simtime >= flap_pos_input_startTime && 
          Simtime <= (flap_pos_input_startTime + flap_pos_input_endTime))
        {
          double time = Simtime - flap_pos_input_startTime;
          flap_pos = uiuc_1Dinterpolation(flap_pos_input_timeArray,
					  flap_pos_input_dfArray,
					  flap_pos_input_ntime,
					  time);
        }
    }

  return;
}

// end uiuc_controlInput.cpp
