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

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>
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

#include "uiuc_aerodeflections.h"

void uiuc_aerodeflections( double dt )
{
  double prevFlapHandle, flap_transit_rate;
  bool flaps_in_transit;

  if (zero_Long_trim)
    {
      Long_trim = 0;
      //elevator_tab = 0;
    }

  if (Lat_control <= 0)
    aileron = - Lat_control * damin * DEG_TO_RAD;
  else
    aileron = - Lat_control * damax * DEG_TO_RAD;

  if ((Long_control+Long_trim) <= 0)
    elevator = (Long_control + Long_trim) * demax * DEG_TO_RAD + elevator_tab;
  else
    elevator = (Long_control + Long_trim) * demin * DEG_TO_RAD + elevator_tab;

  if (Rudder_pedal <= 0)
    rudder = - Rudder_pedal * drmin * DEG_TO_RAD;
  else
    rudder = - Rudder_pedal * drmax * DEG_TO_RAD;

  // flap routine
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
