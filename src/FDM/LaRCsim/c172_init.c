/***************************************************************************

	TITLE:	navion_init.c
	
----------------------------------------------------------------------------

	FUNCTION:	Initializes navion math model

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Renamed navion_init.c originally created on 930111 by Bruce Jackson

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
#include "ls_constants.h"
#include "c172_aero.h"

void c172_init( void ) {

  Throttle[3] = 0.2; 
  
  Dx_pilot = 0; Dy_pilot = 0; Dz_pilot = 0;
  Mass=2300*INVG;
  I_xx=948;
  I_yy=1346;
  I_zz=1967;
  I_xz=0;


  Flap_Position=Flap_handle;
  Flaps_In_Transit=0;


  
  
}
