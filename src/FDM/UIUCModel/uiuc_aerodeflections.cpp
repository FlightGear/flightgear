/**********************************************************************

 FILENAME:     uiuc_aerodeflections.cpp

----------------------------------------------------------------------

 DESCRIPTION:  determine the aero control surface deflections
               elevator [rad]
               aileron [rad]
               rudder [rad]
               
----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   based on deflection portions of c172_aero.c and 
               uiuc_aero.c

----------------------------------------------------------------------

 HISTORY:      01/30/2000   initial release
               04/05/2000   (JS) added zero_Long_trim command
	       07/05/2001   (RD) removed elevator_tab addidtion to
	                    elevator calculation
	       11/12/2001   (RD) added new flap routine.  Needed for
	                    Twin Otter non-linear model

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>
               Robert Deters      <rdeters@uiuc.edu>
               Michael Selig      <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       *

----------------------------------------------------------------------

 OUTPUTS:      *

----------------------------------------------------------------------

 CALLED BY:    *

----------------------------------------------------------------------

 CALLS TO:     *

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

#include <math.h>

#include "uiuc_aerodeflections.h"

void uiuc_aerodeflections( double dt )
{
  double prevFlapHandle = 0.0f;
  double flap_transit_rate;
  bool flaps_in_transit = false;
  double demax_remain;
  double demin_remain;

  if (zero_Long_trim)
    {
      Long_trim = 0;
      //elevator_tab = 0;
    }

  if (Lat_control <= 0)
    aileron = - Lat_control * damin * DEG_TO_RAD;
  else
    aileron = - Lat_control * damax * DEG_TO_RAD;

  if (Long_trim <= 0)
    {
      elevator = Long_trim * demax * DEG_TO_RAD;
      demax_remain = demax + Long_trim * demax;
      demin_remain = -1*Long_trim * demax + demin;
      if (Long_control <= 0)
	elevator += Long_control * demax_remain * DEG_TO_RAD;
      else
	elevator += Long_control * demin_remain * DEG_TO_RAD;
    }
  else
    {
      elevator = Long_trim * demin * DEG_TO_RAD;
      demin_remain = demin - Long_trim * demin;
      demax_remain = Long_trim * demin + demax;
      if (Long_control >=0)
	elevator += Long_control * demin_remain * DEG_TO_RAD;
      else
	elevator += Long_control * demax_remain * DEG_TO_RAD;
    }

  //if ((Long_control+Long_trim) <= 0)
  //  elevator = (Long_control + Long_trim) * demax * DEG_TO_RAD;
  //else
  //  elevator = (Long_control + Long_trim) * demin * DEG_TO_RAD;

  if (Rudder_pedal <= 0)
    rudder = - Rudder_pedal * drmin * DEG_TO_RAD;
  else
    rudder = - Rudder_pedal * drmax * DEG_TO_RAD;


  // new flap routine
  // designed for the twin otter non-linear model
  flap_percent     = Flap_handle / 30.0;       // percent of flaps desired
  if (flap_percent>=0.31 && flap_percent<=0.35)
    flap_percent = 1.0 / 3.0;
  if (flap_percent>=0.65 && flap_percent<=0.69)
    flap_percent = 2.0 / 3.0;
  flap_goal        = flap_percent * flap_max;  // angle of flaps desired
  flap_moving_rate = flap_rate * dt;           // amount flaps move per time step
  
  // determine flap position with respect to the flap goal
  if (flap_pos < flap_goal)
    {
      flap_pos += flap_moving_rate;
      if (flap_pos > flap_goal)
	flap_pos = flap_goal;
    }
  else if (flap_pos > flap_goal)
    {
      flap_pos -= flap_moving_rate;
      if (flap_pos < flap_goal)
	flap_pos = flap_goal;
    }


  // old flap routine
  // check for lowest flap setting
  if (Flap_handle < dfArray[1])
    {
      Flap_handle    = dfArray[1];
      prevFlapHandle = Flap_handle;
      flap           = Flap_handle;
    }
  // check for highest flap setting
  else if (Flap_handle > dfArray[ndf])
    {
      Flap_handle      = dfArray[ndf];
      prevFlapHandle   = Flap_handle;
      flap             = Flap_handle;
    }
  // otherwise in between
  else          
    {
      if(Flap_handle != prevFlapHandle)
        {
          flaps_in_transit = true;
        }
      if(flaps_in_transit)
        {
          int iflap = 0;
          while (dfArray[iflap] < Flap_handle)
            {
              iflap++;
            }
          if (flap < Flap_handle)
            {
              if (TimeArray[iflap] > 0)
                flap_transit_rate = (dfArray[iflap] - dfArray[iflap-1]) / TimeArray[iflap+1];
              else
                flap_transit_rate = (dfArray[iflap] - dfArray[iflap-1]) / 5;
            }
          else 
            {
              if (TimeArray[iflap+1] > 0)
                flap_transit_rate = (dfArray[iflap] - dfArray[iflap+1]) / TimeArray[iflap+1];
              else
                flap_transit_rate = (dfArray[iflap] - dfArray[iflap+1]) / 5;
            }
          if(fabs (flap - Flap_handle) > dt * flap_transit_rate)
            flap += flap_transit_rate * dt;
          else
            {
              flaps_in_transit = false;
              flap = Flap_handle;
            }
        }
    }
  prevFlapHandle = Flap_handle;

  return;
}

// end uiuc_aerodeflections.cpp
