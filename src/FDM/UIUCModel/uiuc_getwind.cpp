/**********************************************************************

 FILENAME:     uiuc_getwind.cpp

----------------------------------------------------------------------

 DESCRIPTION:  sets V_airmass_north, _east, _down for use in LaRCsim

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      03/26/2003   initial release
               
----------------------------------------------------------------------

 AUTHOR(S):    Glen Dimock         <dimock@uiuc.edu>
   	       Michael Selig 	   <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:
      
----------------------------------------------------------------------

 OUTPUTS:

----------------------------------------------------------------------

 CALLED BY:    uiuc_wrapper
              
----------------------------------------------------------------------

 CALLS TO:     none

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


/* 
	UIUC wind gradient test code v0.1
	
	Returns wind vector as a function of altitude for a simple
	parabolic gradient profile

	Glen Dimock
	Last update: 020227
*/

#include "uiuc_getwind.h"
#include "uiuc_aircraft.h"

void uiuc_getwind()
{
	/* Wind parameters */
	double zref = 300.; //Reference height (ft)
	double uref = 0; //Horizontal wind velocity at ref. height (ft/sec)
	//	double uref = 11; //Horizontal wind velocity at ref. height (ft/sec)
	//	double uref = 13; //Horizontal wind velocity at ref. height (ft/sec)
	//      double uref = 20; //Horizontal wind velocity at ref. height (ft/sec), 13.63 mph
	//	double uref = 22.5; //Horizontal wind velocity at ref. height (ft/sec), 15 mph
	//	double uref = 60.; //Horizontal wind velocity at ref. height (ft/sec), 40 mph
	double ordref =-64.; //Horizontal wind ordinal from north (degrees)
	double zoff = 300.; //Z offset (ft) - wind is zero at and below this point
	double zcomp = 0.; //Vertical component (down is positive)

/*  	double zref = 300.; //Reference height (ft) */
/*  	double uref = 0.; //Horizontal wind velocity at ref. height (ft/sec) */
/*  	double ordref = 0.; //Horizontal wind ordinal from north (degrees) */
/*  	double zoff = 15.; //Z offset (ft) - wind is zero at and below this point */
/*  	double zcomp = 0.; //Vertical component (down is positive) */


	/* Get wind vector */
	double windmag = 0; //Wind magnitude
	double a = 0; //Parabola: Altitude = a*windmag^2 + zoff
        double x = pow(uref,2.);
	
        if (x) {
	    a = zref/x;
        }
	if ((Altitude >= zoff) && (a > 0))
		windmag = sqrt(Altitude/a);
	else
		windmag = 0.;

	V_north_airmass = windmag * cos(ordref*3.14159/180.); //North component
	V_east_airmass  = windmag * sin(ordref*3.14159/180.); //East component
	V_down_airmass  = zcomp;

	return;
}

