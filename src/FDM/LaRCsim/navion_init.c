/***************************************************************************

	TITLE:	navion_init.c
	
----------------------------------------------------------------------------

	FUNCTION:	Initializes navion math model

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Added to navion package 930111 by Bruce Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	EBJ
	
	CODED BY:	EBJ
	
	MAINTAINED BY:	EBJ

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY

	950314	Removed initialization of state variables, since this is
		now done (version 1.4b1) in ls_init.		EBJ
	950406	Removed #include of "shmdefs.h"; shmdefs.h is a duplicate
		of "navion.h". EBJ
	
	CURRENT RCS HEADER:

----------------------------------------------------------------------------

	REFERENCES:

----------------------------------------------------------------------------

	CALLED BY:

----------------------------------------------------------------------------

	CALLS TO:

----------------------------------------------------------------------------

	INPUTS:

----------------------------------------------------------------------------

	OUTPUTS:

--------------------------------------------------------------------------*/
#include "ls_types.h"
#include "ls_generic.h"
#include "ls_cockpit.h"

void navion_init( void ) {

  Throttle[3] = 0.2; Rudder_pedal = 0; Lat_control = 0; Long_control = 0;
  
  Dx_pilot = 0; Dy_pilot = 0; Dz_pilot = 0;
  
}
