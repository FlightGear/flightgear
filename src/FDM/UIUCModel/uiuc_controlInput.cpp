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

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.

**********************************************************************/

#include <simgear/compiler.h>

#include "uiuc_controlInput.h"

#include STL_IOSTREAM

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
          elevator = elevator + 
            uiuc_1Dinterpolation(elevator_input_timeArray,
                                 elevator_input_deArray,
                                 elevator_input_ntime,
                                 time);
        }
    }
  return;
}

// end uiuc_controlInput.cpp
