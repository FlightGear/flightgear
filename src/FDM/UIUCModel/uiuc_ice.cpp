/**********************************************************************

 FILENAME:     uiuc_ice.cpp

----------------------------------------------------------------------

 DESCRIPTION:  reads in clean coefficient and icing severity 
               parameters and returns iced coefficient

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

------------------------------------------------string ----------------------

 INPUTS:       -clean aero coefficient
               -icing parameter for that coefficient (kC)
               -icing severity (eta)

----------------------------------------------------------------------

 OUTPUTS:      -iced aero coefficient

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

#include "uiuc_ice.h"


double uiuc_ice( double Ca_clean, double kCa, double eta_temp )
{
  double Ca_iced = 0;

  //cout << "Ice Model Engaged" << endl;

  Ca_iced = Ca_clean * (1 + kCa * eta_temp);

  return Ca_iced;
}

// end uiuc_ice.cpp
