/***************************************************************************

  TITLE:	uiuc_aero	
		
----------------------------------------------------------------------------

  FUNCTION:	aerodynamics, engine and gear model

----------------------------------------------------------------------------

  MODULE STATUS:	developmental

----------------------------------------------------------------------------

  GENEALOGY:		Equations based on Part 1 of Roskam's S&C text

----------------------------------------------------------------------------

  DESIGNED BY:		Bipin Sehgal	
		
  CODED BY:		Bipin Sehgal
		
  MAINTAINED BY:	Bipin Sehgal

----------------------------------------------------------------------------

  MODIFICATION HISTORY:
		
  DATE		PURPOSE												BY
  3/17/00   Initial test release  
  3/09/01   Added callout to UIUC gear function.   (DPM)
  

----------------------------------------------------------------------------

  CALLED BY:

----------------------------------------------------------------------------

  CALLS TO:

----------------------------------------------------------------------------

  INPUTS:	

----------------------------------------------------------------------------

  OUTPUTS:

--------------------------------------------------------------------------*/


#include <math.h>
#include "ls_types.h"
#include "ls_generic.h"
#include "ls_constants.h"
#include "ls_cockpit.h"
#include <FDM/UIUCModel/uiuc_wrapper.h>


void uiuc_aero( SCALAR dt, int Initialize ) 
{
    static int init = 0;

    if (init==0)
    {
      init = -1; 
      uiuc_init_aeromodel();
    }

    uiuc_force_moment(dt);
}


void uiuc_engine( SCALAR dt, int Initialize ) 
{
    uiuc_engine_routine();
}


void uiuc_gear ()
{
    uiuc_gear_routine();
}
