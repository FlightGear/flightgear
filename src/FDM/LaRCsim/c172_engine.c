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

extern SIM_CONTROL	sim_control_;

void engine( SCALAR dt, int init ) {
    /* if (init) { */
    Throttle[3] = Throttle_pct;
    /* } */

    /* F_X_engine = Throttle[3]*813.4/0.2; */  /* original code */
    /* F_Z_engine = Throttle[3]*11.36/0.2; */  /* original code */
    F_X_engine = Throttle[3]*350/0.83;
    F_Z_engine = Throttle[3]*4.9/0.83;

    Throttle_pct = Throttle[3];
}


