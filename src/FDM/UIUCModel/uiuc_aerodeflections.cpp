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

void uiuc_aerodeflections()
{

  // for now, consider deflections to be equal
  // damin = damax
  aileron = - Lat_control * damax * DEG_TO_RAD;

  // for now, consider deflections to be equal
  // demin = demax
  elevator = Long_control * demax * DEG_TO_RAD + Long_trim;

  // for now, consider deflections to be equal
  // drmin = drmax
  rudder = - Rudder_pedal * drmax * DEG_TO_RAD; 

  return;
}

// end uiuc_aerodeflections.cpp
