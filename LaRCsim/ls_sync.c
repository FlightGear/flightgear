/***************************************************************************

	TITLE:	ls_sync.c
	
----------------------------------------------------------------------------

	FUNCTION:	Real-time synchronization routines for LaRCSIM

----------------------------------------------------------------------------

	MODULE STATUS:	Developmental

----------------------------------------------------------------------------

	GENEALOGY:	Written 921229 by Bruce Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	EBJ
	
	CODED BY:	EBJ
	
	MAINTAINED BY:	EBJ

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY
	
	930104  Added ls_resync() call to avoid having to pass DT as a
     	        global variable.                                EBJ
	940204	Added calculation of sim_control variable overrun to
		indicate a frame overrun has occurred.		EBJ
	940506	Added support for sim_control_.debug flag, which disables
		synchronization (there is still a local dbg flag that
		enables synch error logging)			EBJ

	CURRENT RCS HEADER:

$Header$
$Log$
Revision 1.1  1997/05/29 00:10:00  curt
Initial Flight Gear revision.

 * Revision 1.7  1994/05/06  15:34:54  bjax
 * Removed "freerun" variable, and substituted sim_control_.debug flag.
 *
 * Revision 1.6  1994/02/16  13:01:22  bjax
 * Added logic to signal frame overrun; corrected syntax on ls_catch call
 * (old style may be BSD format). EBJ
 *
 * Revision 1.5  1993/07/30  18:33:14  bjax
 * Added 'dt' parameter to call to ls_sync from ls_resync routine.
 *
 * Revision 1.4  1993/03/15  14:56:13  bjax
 * Removed call to ls_pause; this should be done in cockpit routine.
 *
 * Revision 1.3  93/03/13  20:34:09  bjax
 * Modified to allow for sync times longer than a second; added ls_pause()  EBJ
 * 
 * Revision 1.2  93/01/06  09:50:47  bjax
 * Added ls_resync() function.
 * 
 * Revision 1.1  92/12/30  13:19:51  bjax
 * Initial revision
 * 
 * Revision 1.3  93/12/31  10:34:11  bjax
 * Added $Log marker as well.
 * 

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
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include "ls_types.h"
#include "ls_sim_control.h"


extern SCALAR Simtime;

/* give the  time interval data structure FILE visibility */

static struct itimerval t, ot;

static int dbug = 0;

/*void ls_catch( sig, code, sc) /* signal handler */
/*int sig;
int code;
struct sigcontext *sc;*/
void ls_catch()
{
  static DATA lastSimtime = -99.9;

  /* printf("In ls_catch()\n"); */

  /*if (lastSimtime == Simtime) fprintf(stderr, "Overrun.\n"); */
  if (dbug) printf("ls_catch called\n");
  sim_control_.overrun = (lastSimtime == Simtime);
  lastSimtime = Simtime;
  signal(SIGALRM, ls_catch);
}

void ls_sync(dt)
float dt;

/* this routine syncs up the interval timer for a new dt value */
{
  int terr;
  int isec;
  float usec;

  if (sim_control_.debug!=0) return;
  
  isec = (int) dt;
  usec = 1000000* (dt - (float) isec);

  t.it_interval.tv_sec = isec;
  t.it_interval.tv_usec = usec;
  t.it_value.tv_sec = isec;
  t.it_value.tv_usec = usec;
  if (dbug) printf("ls_sync called\n");
  ls_catch();   /* set up for SIGALRM signal catch */
  terr = setitimer( ITIMER_REAL, &t, &ot );
  if (terr) perror("Error returned from setitimer");
}

void ls_unsync()
/* this routine unsyncs the interval timer */
{
  int terr;

  if (sim_control_.debug!=0) return;
  t.it_interval.tv_sec = 0;
  t.it_interval.tv_usec = 0;
  t.it_value.tv_sec = 0;
  t.it_value.tv_usec = 0;
if (dbug) printf("ls_unsync called\n");

  terr = setitimer( ITIMER_REAL, &t, &ot );
  if (terr) perror("Error returned from setitimer");

}

void ls_resync()
/* this routine resynchronizes the interval timer to the old
   interrupt period, stored in struct ot by a previous call
   to ls_unsync().  */
{
  float dt;

  if (sim_control_.debug!=0) return;
  if (dbug) printf("ls_resync called\n");
  dt = ((float) ot.it_interval.tv_usec)/1000000. +
       ((float) ot.it_interval.tv_sec);
  ls_sync(dt);
}

void ls_pause()
/* this routine waits for the next interrupt */
{
  if (sim_control_.debug!=0) return;
  if (dbug) printf("ls_pause called\n");
  pause();
}

