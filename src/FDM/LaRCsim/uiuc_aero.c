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
		
  MAINTAINED BY:	Rob Deters and Glen Dimock

----------------------------------------------------------------------------

  MODIFICATION HISTORY:
		
  DATE		PURPOSE												BY
  3/17/00   Initial test release  
  3/09/01   Added callout to UIUC gear function.   (DPM)
  6/18/01   Added call out to UIUC record routine (RD)
  11/12/01  Changed from uiuc_init_aeromodel() to uiuc_initial_init(). (RD)
  2/24/02   Added uiuc_network_routine() (GD)
  12/11/02  Divided uiuc_network_routine into uiuc_network_recv_routine and
            uiuc_network_send_routine (RD)

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


void uiuc_init_2_wrapper()
{
    static int init = 0;

    // On first time through initialize UIUC aircraft model
    if (init==0) {
        init=-1;
	uiuc_defaults_inits();
        uiuc_init_aeromodel();
    }

    // Re-initialize velocities and euler angles since LaRCsim tends
    // to change them
    uiuc_initial_init();
}

void uiuc_local_vel_init()
{
  uiuc_vel_init();
}

void uiuc_aero_2_wrapper( SCALAR dt, int Initialize ) 
{
    uiuc_force_moment(dt);
}


void uiuc_wind_2_wrapper( SCALAR dt, int Initialize ) 
{
  if (Initialize == 0)
    uiuc_wind_routine(dt);
}

void uiuc_engine_2_wrapper( SCALAR dt, int Initialize ) 
{

    uiuc_engine_routine();
}


void uiuc_gear_2_wrapper ()
{
    uiuc_gear_routine();
}

void uiuc_record_2_wrapper(SCALAR dt)
{
  uiuc_record_routine(dt);
}

void uiuc_network_recv_2_wrapper()
{
  uiuc_network_recv_routine();
}

void uiuc_network_send_2_wrapper()
{
  uiuc_network_send_routine();
}
