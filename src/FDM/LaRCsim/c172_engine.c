/***************************************************************************

	TITLE:		engine.c
	
----------------------------------------------------------------------------

	FUNCTION:	dummy engine routine

----------------------------------------------------------------------------

	MODULE STATUS:	incomplete

----------------------------------------------------------------------------

	GENEALOGY:	This is a renamed navion_engine.c originall written by E. Bruce 
				Jackson
				

----------------------------------------------------------------------------

	DESIGNED BY:	designer
	
	CODED BY:	programmer
	
	MAINTAINED BY:	maintainer

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY

	CURRENT RCS HEADER INFO:

$Header$

 * Revision 1.1  92/12/30  13:21:46  bjax
 * Initial revision
 * 

----------------------------------------------------------------------------

	REFERENCES:

----------------------------------------------------------------------------

	CALLED BY:	ls_model();

----------------------------------------------------------------------------

	CALLS TO:	none

----------------------------------------------------------------------------

	INPUTS:

----------------------------------------------------------------------------

	OUTPUTS:

--------------------------------------------------------------------------*/
#include <math.h>
#include "ls_types.h"
#include "ls_constants.h"
#include "ls_generic.h"
#include "ls_sim_control.h"
#include "ls_cockpit.h"
#include "c172_aero.h"

extern SIM_CONTROL	sim_control_;

void c172_engine( SCALAR dt, int init ) {
    
    float v,h,pa;
    float bhp=160;
	
    Throttle[3] = Throttle_pct;

    
    if ( ! Use_External_Engine ) {
	/* do a crude engine power calc based on throttle position */
	v=V_rel_wind;
	h=Altitude;
	if(V_rel_wind < 10)
	    v=10;
	if(Altitude < 0)
	    h=0;
	pa=(0.00144*v + 0.546)*(1 - 1.6E-5*h)*bhp;
	if(pa < 0)
	    pa=0;

	F_X_engine = Throttle[3]*(pa*550)/v;
    } else {
	/* accept external settings */
    }

    /* printf("F_X_engine = %.3f\n", F_X_engine); */

    M_m_engine = F_X_engine*0.734*cbar;
    /* 0.734 - estimated (WAGged) location of thrust line in the z-axis*/

}


