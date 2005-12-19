/***************************************************************************

	TITLE:		ls_model()	
	
----------------------------------------------------------------------------

	FUNCTION:	Model loop executive

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Created 15 October 1992 as part of LaRCSIM project
			by Bruce Jackson.

----------------------------------------------------------------------------

	DESIGNED BY:	Bruce Jackson
	
	CODED BY:	Bruce Jackson
	
	MAINTAINED BY:	maintainer

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY

	950306	Added parameters to call: dt, which is the step size
		to be taken this loop (caution: may vary from call to call)
		and Initialize, which if non-zero, implies an initialization
		is requested.					EBJ

	CURRENT RCS HEADER INFO:
$Header$
$Log$
Revision 1.5  2005/12/19 12:53:21  ehofman
Vassilii Khachaturov:

clean up some build warnings caught with gcc-4.0.

Revision 1.4  2003/07/25 17:53:41  mselig
UIUC code initilization mods to tidy things up a bit.

Revision 1.3  2003/05/13 18:45:06  curt
Robert Deters:

  I have attached some revisions for the UIUCModel and some LaRCsim.
  The only thing you should need to check is LaRCsim.cxx.  The file
  I attached is a revised version of 1.5 and the latest is 1.7.  Also,
  uiuc_getwind.c and uiuc_getwind.h are no longer in the LaRCsim
  directory.  They have been moved over to UIUCModel.

Revision 1.2  2003/03/31 03:05:41  m-selig
uiuc wind changes, MSS

Revision 1.1.1.1  2003/02/28 01:33:39  rob
uiuc version of FlightGear v0.9.0

Revision 1.3  2002/12/12 00:01:04  rob
*** empty log message ***

Revision 1.2  2002/10/22 21:06:49  rob
*** empty log message ***

Revision 1.2  2002/08/29 18:56:37  rob
*** empty log message ***

Revision 1.1.1.1  2002/04/24 17:08:23  rob
UIUC version of FlightGear-0.7.pre11

Revision 1.5  2002/04/01 19:37:34  curt
I have attached revisions to the UIUC code.  The revisions include the
ability to run a nonlinear model with flaps.  The files ls_model.c and
uiuc_aero.c were changed since we had some functions with the same
name.  The name changes doesn't affect the code, it just makes it a
little easier to read.  There are changes in LaRCsim.cxx so UIUC
models have engine sound.  Could you send me an email when you receive
this and/or when the changes make it to the CVS?  Thanks.

Also I noticed you have some outdated files that are no longer used in
the UIUCModel directory.  They are uiuc_initializemaps1.cpp,
uiuc_initializemaps2.cpp, uiuc_initializemaps3.cpp, and
uiuc_initializemaps4.cpp

Rob

Revision 1.4  2001/09/14 18:47:27  curt
More changes in support of UIUCModel.

Revision 1.3  2000/10/28 14:30:33  curt
Updates by Tony working on the FDM interface bus.

Revision 1.2  2000/04/10 18:09:41  curt
David Megginson made a few (mostly minor) mods to the LaRCsim files, and
it's now possible to choose the LaRCsim model at runtime, as in

  fgfs --aircraft=c172

or

  fgfs --aircraft=uiuc --aircraft-dir=Aircraft-uiuc/Boeing747

I did this so that I could play with the UIUC stuff without losing
Tony's C172 with its flaps, etc.  I did my best to respect the design
of the LaRCsim code by staying in C, making only minimal changes, and
not introducing any dependencies on the rest of FlightGear.  The
modified files are attached.

Revision 1.1.1.1  1999/06/17 18:07:33  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.3  1998/08/06 12:46:39  curt
Header change.

Revision 1.2  1998/01/19 18:40:27  curt
Tons of little changes to clean up the code and to remove fatal errors
when building with the c++ compiler.

Revision 1.1  1997/05/29 00:09:58  curt
Initial Flight Gear revision.

 * Revision 1.3  1995/03/06  18:49:46  bjax
 * Added dt and initialize flag parameters to subroutine calls. This will
 * support trim routine (to allow single throttle setting to drive
 * all four throttle positions, for example, if initialize is TRUE).
 *
 * Revision 1.2  1993/03/10  06:38:09  bjax
 * Added additional calls: inertias() and subsystems()... EBJ
 *
 * Revision 1.1  92/12/30  13:19:08  bjax
 * Initial revision
 * 

----------------------------------------------------------------------------

	REFERENCES:

----------------------------------------------------------------------------

	CALLED BY:	ls_step (in initialization), ls_loop (planned)

----------------------------------------------------------------------------

	CALLS TO:	aero(), engine(), gear()

----------------------------------------------------------------------------

	INPUTS:

----------------------------------------------------------------------------

	OUTPUTS:

--------------------------------------------------------------------------*/
#include <stdio.h>

#include "ls_types.h"
#include "ls_model.h"
#include "default_model_routines.h"

Model current_model;


void ls_model( SCALAR dt, int Initialize ) {
    switch (current_model) {
    case NAVION:
      inertias( dt, Initialize );
      subsystems( dt, Initialize );
      navion_aero( dt, Initialize );
      navion_engine( dt, Initialize );
      navion_gear( dt, Initialize );
      break;
    case C172:
      printf("here we are in C172 \n");
      if(Initialize < 0) c172_init();
      inertias( dt, Initialize );
      subsystems( dt, Initialize );
      c172_aero( dt, Initialize );
      c172_engine( dt, Initialize );
      c172_gear( dt, Initialize );
      break;
    case CHEROKEE:
      inertias( dt, Initialize );
      subsystems( dt, Initialize );
      cherokee_aero( dt, Initialize );
      cherokee_engine( dt, Initialize );
      cherokee_gear( dt, Initialize );
      break;
    case BASIC:
      //      printf("here we are in BASIC \n");
      if(Initialize < 0) basic_init();
      printf("Initialize %d \n", Initialize);
      inertias( dt, Initialize );
      subsystems( dt, Initialize );
      basic_aero( dt, Initialize );
      basic_engine( dt, Initialize );
      basic_gear( dt, Initialize );
      break;
    case UIUC:
      inertias( dt, Initialize );
      subsystems( dt, Initialize );
      // During initialization period, re-initialize velocities
      // and euler angles
      if (Initialize !=0) uiuc_init_2_wrapper();
      uiuc_network_recv_2_wrapper();
      uiuc_engine_2_wrapper( dt, Initialize );
      uiuc_wind_2_wrapper( dt, Initialize );
      uiuc_aero_2_wrapper( dt, Initialize );
      uiuc_gear_2_wrapper( dt, Initialize );
      uiuc_network_send_2_wrapper();
      uiuc_record_2_wrapper(dt);
      break;
    }
}
