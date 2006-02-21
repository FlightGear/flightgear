/**********************************************************************

 FILENAME:     uiuc_fog.cpp

----------------------------------------------------------------------

 DESCRIPTION:  changes Fog variable to +/-1 or 0 using fog 
	       parameters and Simtime

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      05/20/2001   initial release

----------------------------------------------------------------------

 AUTHOR(S):    Michael Savchenko         <savchenk@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:     	-Simtime
		-Fog
		-fog_field
		-fog_next_time
		-fog_current_segment
		-fog_value
		-fog_time
	       
----------------------------------------------------------------------

 OUTPUTS:      	-Fog
		-fog_field
		-fog_next_time
		-fog_current_segment
 
----------------------------------------------------------------------

 CALLED BY:    uiuc_wrapper

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


#include "uiuc_fog.h"

void uiuc_fog()
{
  if (Simtime >= fog_next_time)
  {
    if (fog_current_segment != 0)
    {
      if (fog_value[fog_current_segment] > fog_value[fog_current_segment-1])
	Fog = 1;
      else if (fog_value[fog_current_segment] < fog_value[fog_current_segment-1])
	Fog = -1;
      else
	Fog = 0;
    }
    else
      Fog = 0;

    if (Simtime > fog_time[fog_current_segment]) {
      if (fog_current_segment == fog_segments)
      {
	fog_field = false;
	Fog = 0;
	return;
      }
      else
	fog_current_segment++;			}

    if (fog_value[fog_current_segment] == fog_value[fog_current_segment-1])
      fog_next_time = fog_time[fog_current_segment];
    else
      fog_next_time += (fog_time[fog_current_segment]-fog_time[fog_current_segment-1]) / abs(fog_value[fog_current_segment]-fog_value[fog_current_segment-1]);
  }
  else
    Fog = 0;
  
  return;
}

