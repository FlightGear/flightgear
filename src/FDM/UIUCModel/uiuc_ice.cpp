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
               04/25/2000   (JS) added uiuc_ice_eta function
                            (removed from uiuc_coefficients)

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       uiuc_ice_eta:
               -Simtime
               -icing times
               -final icing severity (eta_ice_final)

               uiuc_ice_filter:
               -clean aero coefficient
               -icing parameter for that coefficient (kC)
               -icing severity (eta_ice)

----------------------------------------------------------------------

 OUTPUTS:      uiuc_ice_eta:
               -icing severity (eta_ice)

               uiuc_ice_filter:
               -iced aero coefficient

----------------------------------------------------------------------

 CALLED BY:    uiuc_coefficients
               uiuc_coef_drag
	       uiuc_coef_lift
	       uiuc_coef_pitch
	       uiuc_coef_sideforce
	       uiuc_coef_roll
	       uiuc_coef_yaw

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

#include "uiuc_ice.h"


void uiuc_ice_eta()
{
  double slope = 0;

  if (Simtime >= iceTime)
    {
      // set ice_on flag
      ice_on = true;

      // slowly increase icing severity over period of transientTime
      if (Simtime < (iceTime + transientTime))
	{
	  slope = eta_ice_final / transientTime;
	  eta_ice = slope * (Simtime - iceTime);
	}
      else
	{
	  eta_ice = eta_ice_final;
	}
    }
  return;
}


double uiuc_ice_filter( double Ca_clean, double kCa )
{
  double Ca_iced = 0;

  //cout << "Ice Model Engaged" << endl;

  Ca_iced = Ca_clean * (1 + kCa * eta_ice);

  return Ca_iced;
}

// end uiuc_ice.cpp
