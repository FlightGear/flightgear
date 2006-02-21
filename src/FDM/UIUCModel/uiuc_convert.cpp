/**********************************************************************

 FILENAME:     uiuc_convert.cpp

----------------------------------------------------------------------

 DESCRIPTION:  reads conversion type and sets conversion factors 

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      02/22/2000   initial release

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -conversion type

----------------------------------------------------------------------

 OUTPUTS:      -conversion factor

----------------------------------------------------------------------

 CALLED BY:    uiuc_menu.cpp

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

#include "uiuc_convert.h"


double uiuc_convert( int conversionType )
{
  double factor;

  switch(conversionType)
    {
    case 0:
      {
	/* no conversion, multiply by 1 */
	factor = 1;
	break;
      }
    case 1:
      {
	/* convert from degrees to radians */
	factor = DEG_TO_RAD;
	break;
      }
    };
  return factor;
}

// end uiuc_convert.cpp
