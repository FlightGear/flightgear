/***************************************************************************

	TITLE:		LaRCsim.c
	
----------------------------------------------------------------------------

	FUNCTION:	Top level routine for LaRCSIM.  Includes
	                global variable declarations.

----------------------------------------------------------------------------

	MODULE STATUS:	Developmental

----------------------------------------------------------------------------

	GENEALOGY:	Written 921230 by Bruce Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	EBJ
	
	CODED BY:	EBJ
	
	MAINTAINED BY:	EBJ

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY

	930111  Added "progname" variable to keep name of invoking command.
                                                                	EBJ
	931012	Removed altitude < 0. test to support gear development. EBJ
	931214	Added various pressures (Impact, Dynamic, Static, etc.) EBJ
	931215	Adopted new generic variable structure.			EBJ
	931218	Added command line options decoding.			EBJ
	940110	Changed file type of matrix file to ".m"		EBJ
	940513	Renamed this routine "LaRCsim.c" from "ls_main.c"	EBJ
	940513  Added time_stamp routine,  t_stamp.			EBJ
	950225	Added options flag, 'i', to set I/O output rate.	EBJ
	950306	Added calls to ls_get_settings() and ls_put_settings()  EBJ
	950314	Options flag 'i' now reads IC file; 'o' is output rate  EBJ
	950406	Many changes: added definition of default value macros;
		removed local variables term_update_hz, model_dt, endtime,
		substituted sim_control_ globals for these; removed
		initialization of sim_control_.tape_channels; moved optarg
		to generic extern; added mod_end_time & mod_buf_size flags
		and temporary buffer_time and data_rate locals to
		ls_checkopts(); added additional command line switches '-s'
		and '-b'; made psuedo-mandatory file names for data output
		switches; considerable rewrite of logic for setting data
		buffer length and interleave parameters; updated '-h' help
		output message; added protection logic to calculations of
		these parameters; added check of return value on first call
		to ls_cockpit() so <esc> abort works from initial pause
		state; added call to ls_unsync() immediately following
		first ls_sync() call, if paused (to avoid alarm clock
		timeout); moved call to ls_record() into non-paused
		multiloop path (was filling buffer with identical data
		during pause); put check of paused flag before calling sync
		routine ls_pause(); and added call to exit() on termination.


$Header$
$Log$
Revision 1.2  1997/05/30 03:54:12  curt
Made a bit more progress towards integrating the LaRCsim flight model.

Revision 1.1  1997/05/29 00:09:51  curt
Initial Flight Gear revision.

 * Revision 1.4.1.7  1995/04/07  01:04:37  bjax
 * Many changes made to support storage of sim options from run to run,
 * as well as restructuring storage buffer sizing and some loop logic
 * changes. See the modification log for details.
 *
 * Revision 1.4.1.6  1995/03/29  16:12:09  bjax
 * Added argument to -o switch; changed run loop to pass dt=0
 * if in paused mode. EBj
 *
 * Revision 1.4.1.5  1995/03/15  12:30:20  bjax
 * Set paused flag to non-zero by default; moved 'i' I/O rate flag
 * switch to 'o'; made 'i' an initial conditions file switch; added
 * null string to ls_get_settings() call so that default settings
 * file will be read. EBJ
 *
 * Revision 1.4.1.4  1995/03/08  12:31:34  bjax
 * Added userid retrieval and proper termination of time & date strings.
 *
 * Revision 1.4.1.3  1995/03/08  12:00:21  bjax
 * Moved setting of default options to ls_setdefopts from
 * ls_checkopts; rearranged order of ls_get_settings() call
 * to between ls_setdefopts and ls_checkopts, so command
 * line options will override settings file options.
 * EBJ
 *
 * Revision 1.4.1.2  1995/03/06  18:48:49  bjax
 * Added calles to ls_get_settings() and ls_put_settings(); added
 * passing of dt and init flags in ls_model(). EBJ
 *
 * Revision 1.4.1.1  1995/03/03  02:23:08  bjax
 * Beta version for LaRCsim, version 1.4
 *
 * Revision 1.3.2.7  1995/02/27  20:00:21  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.2.6  1995/02/25  16:52:31  bjax
 * Added 'i' option to set I/O iteration rate. EBJ
 *
 * Revision 1.3.2.5  1995/02/06  19:33:15  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.2.4  1995/02/06  19:30:30  bjax
 * Oops, should really compile these before checking in. Fixed capitailzation of
 * Initialize in ls_loop parameter.
 *
 * Revision 1.3.2.3  1995/02/06  19:25:44  bjax
 * Moved main simulation loop into subroutine ls_loop. EBJ
 *
 * Revision 1.3.2.2  1994/05/20  21:46:45  bjax
 * A little better logic on checking for option arguments.
 *
 * Revision 1.3.2.1  1994/05/20  19:29:51  bjax
 * Added options arguments to command line.
 *
 * Revision 1.3.1.16  1994/05/17  15:08:45  bjax
 * Corrected so that full name to directyr and file is saved
 * in new global variable "fullname"; this allows symbol table
 * to be extracted when in another default directory.
 *
 * Revision 1.3.1.15  1994/05/17  14:50:24  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.14  1994/05/17  14:50:23  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.13  1994/05/17  14:50:21  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.12  1994/05/17  14:50:20  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.11  1994/05/17  13:56:24  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.10  1994/05/17  13:23:03  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.9  1994/05/17  13:20:03  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.8  1994/05/17  13:19:23  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.7  1994/05/17  13:18:29  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.6  1994/05/17  13:16:30  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.5  1994/05/17  13:03:44  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.4  1994/05/17  13:03:38  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.3  1994/05/17  12:49:08  bjax
 * Rebuilt LaRCsim
 *
 * Revision 1.3.1.2  1994/05/17  12:48:45  bjax
 * *** empty log message ***
 *
 * Revision 1.3.1.1  1994/05/13  20:39:17  bjax
 * Top of 1.3 branch.
 *
 * Revision 1.2  1994/05/13  19:51:50  bjax
 * Skip rev
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

#include "ls_types.h"
#include "ls_constants.h"
#include "ls_generic.h"
#include "ls_sim_control.h"
#include "ls_cockpit.h"
/* #include "ls_tape.h" */
#ifndef linux
# include <libgen.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

/* global variable declarations */

/* TAPE		*Tape; */
GENERIC 	generic_;
SIM_CONTROL	sim_control_;
COCKPIT         cockpit_;

SCALAR 		Simtime;

/* #define DEFAULT_TERM_UPDATE_HZ 20 */ /* original value */
#define DEFAULT_TERM_UPDATE_HZ 20
#define DEFAULT_MODEL_HZ 120
#define DEFAULT_END_TIME 3600.
#define DEFAULT_SAVE_SPACING 8
#define DEFAULT_WRITE_SPACING 1
#define MAX_FILE_NAME_LENGTH 80

/* global variables */

char    *progname;
char	*fullname;

/* file variables - default simulation settings */

static float io_dt;
static float speedup;
static char  asc1name[MAX_FILE_NAME_LENGTH] = "run.asc1";
static char  tabname[MAX_FILE_NAME_LENGTH]  = "run.dat";
static char  fltname[MAX_FILE_NAME_LENGTH]  = "run.flt";
static char  matname[MAX_FILE_NAME_LENGTH]  = "run.m";



void ls_stamp()
{
    char rcsid[] = "$Id$";
    char revid[] = "$Revision$";
    char dateid[] = "$Date$";
    struct tm *nowtime;
    time_t nowtime_t;
    long date;
    
    printf("\nLaRCsim %s, %s\n\n", revid, dateid);	    /* report version of LaRCsim */
    
    nowtime_t = time( 0 );
    nowtime = localtime( &nowtime_t ); /* set fields to correct time values */
    date = (nowtime->tm_year)*10000 
	 + (nowtime->tm_mon + 1)*100
	 + (nowtime->tm_mday);
    sprintf(sim_control_.date_string, "%06d\0", date);
    sprintf(sim_control_.time_stamp, "%02d:%02d:%02d\0", 
	nowtime->tm_hour, nowtime->tm_min, nowtime->tm_sec);
    cuserid( sim_control_.userid );	/* set up user id */

    return;
}

void ls_setdefopts()
{
    /* set default values for most options */

    sim_control_.debug = 0;		/* change to non-zero if in dbx! */
    sim_control_.vision = 0;
    sim_control_.write_av = 0;		/* write Agile-Vu '.flt' file */
    sim_control_.write_mat = 0;		/* write matrix-x/matlab script */
    sim_control_.write_tab = 0;		/* write tab delim. history file */
    sim_control_.write_asc1 = 0;	/* write GetData file */
    sim_control_.sim_type = GLmouse;	/* hook up to mouse */
    sim_control_.save_spacing = DEFAULT_SAVE_SPACING;	
					/* interpolation on recording */
    sim_control_.write_spacing = DEFAULT_WRITE_SPACING;	
					/* interpolation on output */
    sim_control_.end_time = DEFAULT_END_TIME;
    sim_control_.model_hz = DEFAULT_MODEL_HZ;
    sim_control_.term_update_hz = DEFAULT_TERM_UPDATE_HZ;
    sim_control_.time_slices = DEFAULT_END_TIME*DEFAULT_MODEL_HZ/DEFAULT_SAVE_SPACING;
    sim_control_.paused = 1;

    speedup = 1.0;
}


/* return result codes from ls_checkopts */

#define OPT_OK 0
#define OPT_ERR 1

extern char *optarg;
extern int optind;

int ls_checkopts(argc, argv)	/* check and set options flags */
  int argc;
  char *argv[];
  {
    int c;
    int opt_err = 0;
    int mod_end_time = 0;
    int mod_buf_size = 0;
    float buffer_time, data_rate;

    /* set default values */

    buffer_time = sim_control_.time_slices * sim_control_.save_spacing / sim_control_.model_hz;
    data_rate   = sim_control_.model_hz / sim_control_.save_spacing;

    while ((c = getopt(argc, argv, "Aa:b:de:f:hi:kmo:r:s:t:x:")) != EOF)
	switch (c) {
	    case 'A':
		if (sim_control_.sim_type == GLmouse)
		  {
		    fprintf(stderr, "Cannot specify both keyboard (k) and ACES (A) cockpits option\n");
		    fprintf(stderr, "Keyboard operation assumed.\n");
		    break;
		  }
		sim_control_.sim_type = cockpit;
		break;
	    case 'a':
		sim_control_.write_av = 1;
		if (optarg != NULL)
		if (*optarg != '-') 
		    strncpy(fltname, optarg, MAX_FILE_NAME_LENGTH);
		else
		    optind--;
		break;
	    case 'b':	
		buffer_time = atof(optarg);
		if (buffer_time <= 0.) opt_err = -1;
		mod_buf_size++;
		break;
	    case 'd':
		sim_control_.debug = 1;
		break;
	    case 'e':
		sim_control_.end_time = atof(optarg);
		mod_end_time++;
		break;
	    case 'f':
		sim_control_.model_hz = atof(optarg);
		break;
	    case 'h': 
		opt_err = 1;
		break;
	    case 'i':
		/* ls_get_settings( optarg ); */
		break;
	    case 'k':
		sim_control_.sim_type = GLmouse;
		break;
	    case 'm':
		sim_control_.vision = 1;
		break;
	    case 'o': 
		sim_control_.term_update_hz = atof(optarg);
		if (sim_control_.term_update_hz <= 0.) opt_err = 1;
		break;
	    case 'r':
		sim_control_.write_mat = 1;
		if (optarg != NULL)
		if (*optarg != '-') 
		    strncpy(matname, optarg, MAX_FILE_NAME_LENGTH);
		else
		    optind--;
		break;
	    case 's':
		data_rate = atof(optarg);
		if (data_rate <= 0.) opt_err = -1;
		break;
	    case 't':
		sim_control_.write_tab = 1;
		if (optarg != NULL)
		if (*optarg != '-') 
		    strncpy(tabname, optarg, MAX_FILE_NAME_LENGTH);
		else
		    optind--;
		break;
	    case 'x':
		sim_control_.write_asc1 = 1;
		if (optarg != NULL)
		if (*optarg != '-') 
		    strncpy(asc1name, optarg, MAX_FILE_NAME_LENGTH);
		else
		    optind--;
		break;
	    default:
		opt_err = 1;
	    
	}

    if (opt_err)
      {
	fprintf(stderr, "Usage: %s [-options]\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "  where [-options] is zero or more of the following:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  [A|k]           Run mode: [A]CES cockpit   [default]\n");
	fprintf(stderr, "                         or [k]eyboard\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  [i <filename>]  [i]nitial conditions filename\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  [f <value>]     Iteration rate [f]requency, Hz (default is %5.2f Hz)\n", 
						sim_control_.model_hz);
	fprintf(stderr, "\n");
	fprintf(stderr, "  [o <value>]     Display [o]utput frequency, Hz (default is %5.2f Hz)\n", 
						sim_control_.term_update_hz);
	fprintf(stderr, "\n");
	fprintf(stderr, "  [s <value>]     Data storage frequency, Hz (default is %5.2f Hz)\n",
						data_rate);
	fprintf(stderr, "\n");
	fprintf(stderr, "  [e <value>]     [e]nd time in seconds (default %5.1f seconds)\n", 
						sim_control_.end_time);
	fprintf(stderr, "\n");
	fprintf(stderr, "  [b <value>]     circular time history storage [b]uffer size, in seconds \n");
	fprintf(stderr, "                  (default %5.1f seconds) (normally same as end time)\n", 
						sim_control_.time_slices*sim_control_.save_spacing/
							sim_control_.model_hz);
	fprintf(stderr, "\n");
	fprintf(stderr, "  [atxr [<filename>]] Output: [a]gile-vu  (default name: %s )\n", fltname);
	fprintf(stderr, "                       and/or [t]ab delimited ( '' name: %s )\n", tabname);
	fprintf(stderr, "                       and/or [x]plot     (default name: %s)\n", asc1name);
	fprintf(stderr, "                       and/or mat[r]ix script ( '' name: %s   )\n", matname);
	fprintf(stderr, "\n");
	return OPT_ERR;
      }

/* calculate additional controls */

    io_dt = 1.0/sim_control_.term_update_hz;

    sim_control_.save_spacing = (int) (0.5 + sim_control_.model_hz / data_rate);
    if (sim_control_.save_spacing < 1) sim_control_.save_spacing = 1;

    sim_control_.time_slices = buffer_time * sim_control_.model_hz / sim_control_.save_spacing;
    if (sim_control_.time_slices < 2) sim_control_.time_slices = 2;
	 
    return OPT_OK;
  }


void ls_loop( dt, initialize )

SCALAR dt;
int initialize;

{
    /* printf ("  In ls_loop()\n"); */
    ls_step( dt, initialize );
    /* if (sim_control_.sim_type == cockpit ) ls_ACES();  */
    ls_aux();
    ls_model( dt, initialize );
    ls_accel();
}



/* Stub ls_cockpit routines for testing */
#define FALSE 0
#define TRUE 1

int ls_cockpit() {
    int abort = FALSE;

    sim_control_.paused = 0;

    Throttle_pct = 0.85;

    printf("Mach = %.2f  ", Mach_number);
    printf("%.4f,%.4f,%.2f  ", Latitude, Longitude, Altitude);
    printf("%.2f,%.2f,%.2f\n", Phi, Theta, Psi);

    return(abort);
}

int ls_cockpit_exit() {
    exit(0);
}




main(argc, argv)
int argc;
char *argv[];
{
	int	i, multiloop, abrt;
	SCALAR  model_dt;

	fullname = argv[0];		/* point to full directory & path name of our program */
	progname = basename(argv[0]);    /* point to name of program that invoked us */
	strcpy(sim_control_.simname, progname); /* really should check for overflow*/

	ls_setdefopts();		/* set default options */
	
	/* Number_of_Continuous_States = 22; */

	generic_.geodetic_position_v[0] = 2.793445E-05;
	generic_.geodetic_position_v[1] = 3.262070E-07;
	generic_.geodetic_position_v[2] = 3.758099E+00;
	generic_.v_local_v[0]   = 7.287719E+00;
	generic_.v_local_v[1]   = 1.521770E+03;
	generic_.v_local_v[2]   = -1.265722E-05;
	generic_.euler_angles_v[0]      = -2.658474E-06;
	generic_.euler_angles_v[1]      = 7.401790E-03;
	generic_.euler_angles_v[2]      = 1.391358E-03;
	generic_.omega_body_v[0]        = 7.206685E-05;
	generic_.omega_body_v[1]        = 0.000000E+00;
	generic_.omega_body_v[2]        = 9.492658E-05;
	generic_.earth_position_angle   = 0.000000E+00;
	generic_.mass   = 8.547270E+01;
	generic_.i_xx   = 1.048000E+03;
	generic_.i_yy   = 3.000000E+03;
	generic_.i_zz   = 3.530000E+03;
	generic_.i_xz   = 0.000000E+00;
	generic_.d_cg_rp_body_v[0]      = 0.000000E+00;
	generic_.d_cg_rp_body_v[1]      = 0.000000E+00;
	generic_.d_cg_rp_body_v[2]      = 0.000000E+00;

	/* ls_get_settings( "" ); */	/* let settings file override them */

	if (ls_checkopts(argc, argv) == OPT_ERR) return 1;	/* and then command line opts */

	ls_stamp();   /* ID stamp; record time and date of run */

	if (speedup == 0.0) {
	    fprintf(stderr, "%s: Cannot run with speedup of 0.\n", progname);
	    return 1;
	}

	model_dt = 1./sim_control_.model_hz;
	
	if (io_dt < model_dt) {
	    fprintf(stderr, "%s: Can't run I/O faster than model.\n", progname);
	    return 1;
	}
	
	multiloop = (int) (io_dt/model_dt/fabs(speedup));
	model_dt = io_dt/multiloop;

	ls_init();

	/* ls_record(); */
	if (speedup > 0) {
	    abrt = ls_cockpit();
	    if (abrt) {
		ls_cockpit_exit();
		exit( EXIT_SUCCESS );
	    }
	    ls_sync(io_dt);
	    if (sim_control_.paused) ls_unsync();
	}

	do {
	    for(i=0;i<multiloop;i++) {
		if (sim_control_.paused) {
		    ls_loop( 0.0, 0);
		} else {
		    ls_loop( model_dt, 0);
		    /* ls_record(); */
		}
	    }
	    if (speedup > 0) {
		abrt = ls_cockpit();
		if (!sim_control_.paused) ls_pause();
		if (abrt) Simtime = sim_control_.end_time+model_dt;
	    }
	} while (Simtime < sim_control_.end_time);

	if (speedup > 0) {
	    ls_unsync();
	    ls_cockpit_exit();
	}
	
	/* if (sim_control_.write_mat ) ls_writemat(  matname  ); */
	/* if (sim_control_.write_av  ) ls_writeav(   fltname  ); */
	/* if (sim_control_.write_tab ) ls_writetab(  tabname  ); */
	/* if (sim_control_.write_asc1) ls_writeasc1( asc1name ); */

	/* ls_put_settings(); */

	exit( EXIT_SUCCESS );
}

/*
Mon Feb  6 14:33:15 EST 1995
bjax
*/
/*
Mon Feb 27 15:00:20 EST 1995
bjax
*/
