/***************************************************************************

        TITLE:          basic_engine.c
        
----------------------------------------------------------------------------

        FUNCTION:       dummy engine routine

----------------------------------------------------------------------------

        MODULE STATUS:  incomplete

----------------------------------------------------------------------------

        GENEALOGY:
                                

----------------------------------------------------------------------------

        DESIGNED BY:    designer
        
        CODED BY:       programmer
        
        MAINTAINED BY:  maintainer

----------------------------------------------------------------------------

        MODIFICATION HISTORY:
        
        DATE    PURPOSE                                         BY

        CURRENT RCS HEADER INFO:

$Header$

 * Revision 1.1  92/12/30  13:21:46  bjax
 * Initial revision
 * 

----------------------------------------------------------------------------

        REFERENCES:

----------------------------------------------------------------------------

        CALLED BY:      ls_model();

----------------------------------------------------------------------------

        CALLS TO:       none

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
#include "basic_aero.h"

extern SIM_CONTROL      sim_control_;

void basic_engine( SCALAR dt, int init ) {
  
  // c172
  //    F_X_engine = Throttle_pct * 421;
  // neo 2m slope
  F_X_engine = Throttle_pct * 8;
  F_Y_engine = 0.0;
  F_Z_engine = 0.0;
  M_l_engine = 0.0;
  M_m_engine = 0.0;
  M_n_engine = 0.0;
  
  
  //  printf("Throttle_pct %f\n", Throttle_pct);
  //  printf("F_X_engine %f\n", F_X_engine);

  
}


