/* 
	UIUC wind gradient test code v0.1
	
	Returns wind vector as a function of altitude for a simple
	parabolic gradient profile

	Glen Dimock
	Last update: 020227
*/

#include "uiuc_getwind.h"

void uiuc_getwind(double wind[3])
{
	/* Wind parameters */
	double zref = 300.; //Reference height (ft)
	double uref = 0.; //Horizontal wind velocity at ref. height (ft/sec)
	double ordref = 0.; //Horizontal wind ordinal from north (degrees)
	double zoff = 15.; //Z offset (ft) - wind is zero at and below this point
	double zcomp = 0.; //Vertical component (down is positive)


	/* Get wind vector */
	double windmag = 0; //Wind magnitude
	double a = 0; //Parabola: Altitude = a*windmag^2 + zoff
	
	a = zref/pow(uref,2.);
	if (Altitude >= zoff)
		windmag = sqrt(Altitude/a);
	else
		windmag = 0.;

	wind[0] = windmag * cos(ordref*3.14159/180.); //North component
	wind[1] = windmag * sin(ordref*3.14159/180.); //East component
	wind[2] = zcomp;

	return;
}

