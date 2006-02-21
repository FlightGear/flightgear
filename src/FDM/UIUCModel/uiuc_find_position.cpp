/**********************************************************************

 FILENAME:     uiuc_find_positon.cpp

----------------------------------------------------------------------

 DESCRIPTION:  Determine the position of a surface/object given the
               command, increment rate, and current position.  Outputs
               new position
               
----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      03/03/2003   initial release

----------------------------------------------------------------------

 AUTHOR(S):    Robert Deters      <rdeters@uiuc.edu>
               Michael Selig      <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       command, increment rate, position

----------------------------------------------------------------------

 OUTPUTS:      position

----------------------------------------------------------------------

 CALLED BY:    uiuc_aerodeflections()

----------------------------------------------------------------------

 CALLS TO:     *

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
#include "uiuc_find_position.h"

double uiuc_find_position(double command, double increment_per_timestep,
			  double position)
{
  if (position < command) {
    position += increment_per_timestep;
    if (position > command) 
      position = command;
  } 
  else if (position > command) {
    position -= increment_per_timestep;
    if (position < command)
      position = command;
  }

  return position;
}
