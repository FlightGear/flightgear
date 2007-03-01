/**************************************************************************
 * ls_interface.c -- the FG interface to the LaRCsim routines
 *                   This is a heavily modified version of LaRCsim.c
 *                   As a result there is much old baggage left in this file.
 *
 * Originally Written 921230 by Bruce Jackson
 * Modified by Curtis Olson, started May 1997.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/

/* Original headers follow: */

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
$Original log: LaRCsim.c,v $
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

/* #include <sys/types.h> */
/* #include <sys/stat.h> */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "ls_types.h"
#include "ls_constants.h"
#include "ls_geodesy.h"
#include "ls_generic.h"
#include "ls_sim_control.h"
#include "ls_cockpit.h"
#include "ls_interface.h"
#include "ls_step.h"
#include "ls_accel.h"
#include "ls_aux.h"
#include "ls_model.h"
#include "ls_init.h"

// #include <Flight/flight.h>
// #include <Aircraft/aircraft.h>
// #include <Debug/fg_debug.h>


/* global variable declarations */

/* TAPE		*Tape; */
GENERIC 	generic_;
SIM_CONTROL	sim_control_;
COCKPIT         cockpit_;

SCALAR 		Simtime;

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

static double model_dt;
static double speedup;
static char  asc1name[MAX_FILE_NAME_LENGTH] = "run.asc1";
static char  tabname[MAX_FILE_NAME_LENGTH]  = "run.dat";
static char  fltname[MAX_FILE_NAME_LENGTH]  = "run.flt";
static char  matname[MAX_FILE_NAME_LENGTH]  = "run.m";



void ls_stamp( void ) {
    char rcsid[] = "$Id$";
    char revid[] = "$Revision$";
    char dateid[] = "$Date$";
    struct tm *nowtime;
    time_t nowtime_t;
    long date;
    
    /* report version of LaRCsim*/
    printf("\nLaRCsim %s, %s\n\n", revid, dateid); 
    
    nowtime_t = time( 0 );
    nowtime = localtime( &nowtime_t ); /* set fields to correct time values */
    date = (nowtime->tm_year % 100)*10000
	 + (nowtime->tm_mon + 1)*100
	 + (nowtime->tm_mday);
    sprintf(sim_control_.date_string, "%06ld\0", date);
    sprintf(sim_control_.time_stamp, "%02d:%02d:%02d\0", 
	nowtime->tm_hour, nowtime->tm_min, nowtime->tm_sec);
#ifdef COMPILE_THIS_CODE_THIS_USELESS_CODE
    cuserid( sim_control_.userid );	/* set up user id */
#endif /* COMPILE_THIS_CODE_THIS_USELESS_CODE */
    return;
}

void ls_setdefopts( void ) {
    /* set default values for most options */

    sim_control_.debug = 0;		/* change to non-zero if in dbx! */
    sim_control_.vision = 0;
    sim_control_.write_av = 0;		/* write Agile-Vu '.flt' file */
    sim_control_.write_mat = 0;		/* write matrix-x/matlab script */
    sim_control_.write_tab = 0;		/* write tab delim. history file */
    sim_control_.write_asc1 = 0;	/* write GetData file */
    sim_control_.save_spacing = DEFAULT_SAVE_SPACING;	
					/* interpolation on recording */
    sim_control_.write_spacing = DEFAULT_WRITE_SPACING;	
					/* interpolation on output */
    sim_control_.end_time = DEFAULT_END_TIME;
    sim_control_.model_hz = DEFAULT_MODEL_HZ;
    sim_control_.term_update_hz = DEFAULT_TERM_UPDATE_HZ;
    sim_control_.time_slices = (long int)(DEFAULT_END_TIME * DEFAULT_MODEL_HZ / 
	DEFAULT_SAVE_SPACING);
    sim_control_.paused = 0;

    speedup = 1.0;
}


/* return result codes from ls_checkopts */

#define OPT_OK 0
#define OPT_ERR 1

#ifdef COMPILE_THIS_CODE_THIS_USELESS_CODE

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

    buffer_time = sim_control_.time_slices * sim_control_.save_spacing / 
	sim_control_.model_hz;
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

    sim_control_.save_spacing = (int) (0.5 + sim_control_.model_hz / data_rate);
    if (sim_control_.save_spacing < 1) sim_control_.save_spacing = 1;

    sim_control_.time_slices = buffer_time * sim_control_.model_hz / 
	sim_control_.save_spacing;
    if (sim_control_.time_slices < 2) sim_control_.time_slices = 2;
	 
    return OPT_OK;
  }
#endif /* COMPILE_THIS_CODE_THIS_USELESS_CODE */

void ls_set_model_dt(double dt) {
  model_dt = dt;
}  

void ls_loop( SCALAR dt, int initialize ) {
    /* printf ("  In ls_loop()\n"); */
    ls_step( dt, initialize );
    /* if (sim_control_.sim_type == cockpit ) ls_ACES();  */
    ls_aux();
    ls_model( dt, initialize );
    ls_accel();
}



int ls_cockpit( void ) {
    // fgCONTROLS *c;

    sim_control_.paused = 0;

    // c = current_aircraft.controls;

    // Lat_control = FG_Aileron;
    // Long_control = FG_Elevator;
    // Long_trim = FG_Elev_Trim;
    // Rudder_pedal = FG_Rudder;
    // Throttle_pct = FG_Throttle[0];

    /* printf("Mach = %.2f  ", Mach_number);
    printf("%.4f,%.4f,%.2f  ", Latitude, Longitude, Altitude);
    printf("%.2f,%.2f,%.2f\n", Phi, Theta, Psi); */

    return( 0 );
}


/* Initialize the LaRCsim flight model, dt is the time increment for
   each subsequent iteration through the EOM */
int ls_toplevel_init(double dt, char * aircraft) {
    model_dt = dt;

    ls_setdefopts();		/* set default options */
	
    ls_stamp();   /* ID stamp; record time and date of run */

    if (speedup == 0.0) {
	fprintf(stderr, "%s: Cannot run with speedup of 0.\n", progname);
	return 1;
    }

    /* printf("LS pre Init pos = %.2f\n", Latitude); */

    ls_init(aircraft);

    /* printf("LS post Init pos = %.2f\n", Latitude); */

    if (speedup > 0) {
	/* Initialize (get) cockpit (controls) settings */
	ls_cockpit();
    }

    return(1);
}


/* Run an iteration of the EOM (equations of motion) */
int ls_update(int multiloop) {
    int	i;

    if (speedup > 0) {
	ls_cockpit();
    }

    for ( i = 0; i < multiloop; i++ ) {
	ls_loop( model_dt, 0);
    }

    return 1;
}


/* Set the altitude (force) */
int ls_ForceAltitude(double alt_feet) {
    Altitude = alt_feet;
    ls_geod_to_geoc( Latitude, Altitude, &Sea_level_radius, &Lat_geocentric);
    Radius_to_vehicle = Altitude + Sea_level_radius;

    return 0;
}


/* Flight Gear Modification Log
 *
 * $Log$
 * Revision 1.3  2007/03/01 17:53:24  mfranz
 * Hans Ulrich NIEDERMANN:
 *
 * """
 * Fix Y2K bug triggering string overflow
 *
 * sim_control_.date_string is a char[7], so it can contain "yymmdd" and
 * the terminating '\0'. However, nowtime->tm_year is 107 for the year 2007,
 * so you'll end up with a 7 digit number and the string written to
 * sim_control_.date_string is longer than sim_control_.date_string is.
 * Ouch!
 * """
 *
 * mf: ... and sim_control_.date_string isn't even used.
 *
 * Revision 1.2  2006-02-21 17:45:03  mfranz
 * new FSF address (see http://www.gnu.org/licenses/gpl.html)
 *
 * Revision 1.1.1.1  2002-09-10 01:14:02  curt
 * Initial revision of FlightGear-0.9.0
 *
 * Revision 1.6  2002/01/30 15:17:27  david
 * Fixes from Cameron Moore:
 *
 * I've attached 3 diffs against files in FlightGear to fix some printf
 * format strings.  The changes are pretty straight forward.  Let me know
 * if you have any questions.  (BTW, I'm using gcc 2.95.4)
 *
 * Revision 1.5  2001/05/21 18:44:59  curt
 * Tile pager tweaks.
 * MSVC++ tweaks.
 *
 * Revision 1.4  2001/03/24 05:03:12  curt
 * SG-ified logstream.
 *
 * Revision 1.3  2000/10/23 22:34:54  curt
 * I tested:
 * LaRCsim c172 on-ground and in-air starts, reset: all work
 * UIUC Cessna172 on-ground and in-air starts work as expected, reset
 * results in an aircraft that is upside down but does not crash FG.   I
 * don't know what it was like before, so it may well be no change.
 * JSBSim c172 and X15 in-air starts work fine, resets now work (and are
 * trimmed), on-ground starts do not -- the c172 ends up on its back.  I
 * suspect this is no worse than before.
 *
 * I did not test:
 * Balloon (the weather code returns nan's for the atmosphere data --this
 * is in the weather module and apparently is a linux only bug)
 * ADA (don't know how)
 * MagicCarpet  (needs work yet)
 * External (don't know how)
 *
 * known to be broken:
 * LaRCsim c172 on-ground starts with a negative terrain altitude (this
 * happens at KPAO when the scenery is not present).   The FDM inits to
 * about 50 feet AGL and the model falls to the ground.  It does stay
 * upright, however, and seems to be fine once it settles out, FWIW.
 *
 * To do:
 * --implement set_Model on the bus
 * --bring Christian's weather data into JSBSim
 * -- add default method to bus for updating things like the sin and cos of
 * latitude (for Balloon, MagicCarpet)
 * -- lots of cleanup
 *
 * The files:
 * src/FDM/flight.cxx
 * src/FDM/flight.hxx
 * -- all data members now declared protected instead of private.
 * -- eliminated all but a small set of 'setters', no change to getters.
 * -- that small set is declared virtual, the default implementation
 * provided preserves the old behavior
 * -- all of the vector data members are now initialized.
 * -- added busdump() method -- SG_LOG's  all the bus data when called,
 * useful for diagnostics.
 *
 * src/FDM/ADA.cxx
 * -- bus data members now directly assigned to
 *
 * src/FDM/Balloon.cxx
 * -- bus data members now directly assigned to
 * -- changed V_equiv_kts to V_calibrated_kts
 *
 * src/FDM/JSBSim.cxx
 * src/FDM/JSBSim.hxx
 * -- bus data members now directly assigned to
 * -- implemented the FGInterface virtual setters with JSBSim specific
 * logic
 * -- changed the static FDMExec to a dynamic fdmex (needed so that the
 * JSBSim object can be deleted when a model change is called for)
 * -- implemented constructor and destructor, moved some of the logic
 * formerly in init() to constructor
 * -- added logic to bring up FGEngInterface objects and set the RPM and
 * throttle values.
 *
 * src/FDM/LaRCsim.cxx
 * src/FDM/LaRCsim.hxx
 * -- bus data members now directly assigned to
 * -- implemented the FGInterface virtual setters with LaRCsim specific
 * logic, uses LaRCsimIC
 * -- implemented constructor and destructor, moved some of the logic
 * formerly in init() to constructor
 * -- moved default inertias to here from fg_init.cxx
 * -- eliminated the climb rate calculation.  The equivalent, climb_rate =
 * -1*vdown, is now in copy_from_LaRCsim().
 *
 * src/FDM/LaRCsimIC.cxx
 * src/FDM/LaRCsimIC.hxx
 * -- similar to FGInitialCondition, this class has all the logic needed to
 * turn data like Vc and Mach into the more fundamental quantities LaRCsim
 * needs to initialize.
 * -- put it in src/FDM since it is a class
 *
 * src/FDM/MagicCarpet.cxx
 *  -- bus data members now directly assigned to
 *
 * src/FDM/Makefile.am
 * -- adds LaRCsimIC.hxx and cxx
 *
 * src/FDM/JSBSim/FGAtmosphere.h
 * src/FDM/JSBSim/FGDefs.h
 * src/FDM/JSBSim/FGInitialCondition.cpp
 * src/FDM/JSBSim/FGInitialCondition.h
 * src/FDM/JSBSim/JSBSim.cpp
 * -- changes to accomodate the new bus
 *
 * src/FDM/LaRCsim/atmos_62.h
 * src/FDM/LaRCsim/ls_geodesy.h
 * -- surrounded prototypes with #ifdef __cplusplus ... #endif , functions
 * here are needed in LaRCsimIC
 *
 * src/FDM/LaRCsim/c172_main.c
 * src/FDM/LaRCsim/cherokee_aero.c
 * src/FDM/LaRCsim/ls_aux.c
 * src/FDM/LaRCsim/ls_constants.h
 * src/FDM/LaRCsim/ls_geodesy.c
 * src/FDM/LaRCsim/ls_geodesy.h
 * src/FDM/LaRCsim/ls_step.c
 * src/FDM/UIUCModel/uiuc_betaprobe.cpp
 * -- changed PI to LS_PI, eliminates preprocessor naming conflict with
 * weather module
 *
 * src/FDM/LaRCsim/ls_interface.c
 * src/FDM/LaRCsim/ls_interface.h
 * -- added function ls_set_model_dt()
 *
 * src/Main/bfi.cxx
 * -- eliminated calls that set the NED speeds to body components.  They
 * are no longer needed and confuse the new bus.
 *
 * src/Main/fg_init.cxx
 * -- eliminated calls that just brought the bus data up-to-date (e.g.
 * set_sin_cos_latitude). or set default values.   The bus now handles the
 * defaults and updates itself when the setters are called (for LaRCsim and
 * JSBSim).  A default method for doing this needs to be added to the bus.
 * -- added fgVelocityInit() to set the speed the user asked for.  Both
 * JSBSim and LaRCsim can now be initialized using any of:
 * vc,mach, NED components, UVW components.
 *
 * src/Main/main.cxx
 * --eliminated call to fgFDMSetGroundElevation, this data is now 'pulled'
 * onto the bus every update()
 *
 * src/Main/options.cxx
 * src/Main/options.hxx
 * -- added enum to keep track of the speed requested by the user
 * -- eliminated calls to set NED velocity properties to body speeds, they
 * are no longer needed.
 * -- added options for the NED components.
 *
 * src/Network/garmin.cxx
 * src/Network/nmea.cxx
 * --eliminated calls that just brought the bus data up-to-date (e.g.
 * set_sin_cos_latitude).  The bus now updates itself when the setters are
 * called (for LaRCsim and JSBSim).  A default method for doing this needs
 * to be added to the bus.
 * -- changed set_V_equiv_kts to set_V_calibrated_kts.  set_V_equiv_kts no
 * longer exists ( get_V_equiv_kts still does, though)
 *
 * src/WeatherCM/FGLocalWeatherDatabase.cpp
 * -- commented out the code to put the weather data on the bus, a
 * different scheme for this is needed.
 *
 * Revision 1.2  2000/04/10 18:09:41  curt
 * David Megginson made a few (mostly minor) mods to the LaRCsim files, and
 * it's now possible to choose the LaRCsim model at runtime, as in
 *
 *   fgfs --aircraft=c172
 *
 * or
 *
 *   fgfs --aircraft=uiuc --aircraft-dir=Aircraft-uiuc/Boeing747
 *
 * I did this so that I could play with the UIUC stuff without losing
 * Tony's C172 with its flaps, etc.  I did my best to respect the design
 * of the LaRCsim code by staying in C, making only minimal changes, and
 * not introducing any dependencies on the rest of FlightGear.  The
 * modified files are attached.
 *
 * Revision 1.1.1.1  1999/06/17 18:07:33  curt
 * Start of 0.7.x branch
 *
 * Revision 1.2  1999/04/27 19:28:04  curt
 * Changes for the MacOS port contributed by Darrell Walisser.
 *
 * Revision 1.1.1.1  1999/04/05 21:32:45  curt
 * Start of 0.6.x branch.
 *
 * Revision 1.25  1999/01/19 20:57:02  curt
 * MacOS portability changes contributed by "Robert Puyol" <puyol@abvent.fr>
 *
 * Revision 1.24  1998/12/14 13:27:47  curt
 * Removed some old, outdated, no longer needed code.
 *
 * Revision 1.23  1998/10/16 23:27:44  curt
 * C++-ifying.
 *
 * Revision 1.22  1998/09/29 02:02:59  curt
 * Added a brake + autopilot mods.
 *
 * Revision 1.21  1998/08/22  14:49:56  curt
 * Attempting to iron out seg faults and crashes.
 * Did some shuffling to fix a initialization order problem between view
 * position, scenery elevation.
 *
 * Revision 1.20  1998/07/12 03:11:03  curt
 * Removed some printf()'s.
 * Fixed the autopilot integration so it should be able to update it's control
 *   positions every time the internal flight model loop is run, and not just
 *   once per rendered frame.
 * Added a routine to do the necessary stuff to force an arbitrary altitude
 *   change.
 * Gave the Navion engine just a tad more power.
 *
 * Revision 1.19  1998/05/11 18:17:28  curt
 * Output message tweaking.
 *
 * Revision 1.18  1998/04/21 16:59:38  curt
 * Integrated autopilot.
 * Prepairing for C++ integration.
 *
 * Revision 1.17  1998/02/23 19:07:58  curt
 * Incorporated Durk's Astro/ tweaks.  Includes unifying the sun position
 * calculation code between sun display, and other FG sections that use this
 * for things like lighting.
 *
 * Revision 1.16  1998/02/07 15:29:38  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.15  1998/01/22 22:03:47  curt
 * Removed #include <sys/stat.h>
 *
 * Revision 1.14  1998/01/19 19:27:04  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.13  1998/01/19 18:40:26  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.12  1998/01/06 01:20:16  curt
 * Tweaks to help building with MSVC++
 *
 * Revision 1.11  1998/01/05 22:19:26  curt
 * #ifdef'd out some unused code that was problematic for MSVC++ to compile.
 *
 * Revision 1.10  1997/12/10 22:37:43  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.9  1997/08/27 03:30:08  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.8  1997/06/21 17:12:50  curt
 * Capitalized subdirectory names.
 *
 * Revision 1.7  1997/05/31 19:16:28  curt
 * Elevator trim added.
 *
 * Revision 1.6  1997/05/31 04:13:53  curt
 * WE CAN NOW FLY!!!
 *
 * Continuing work on the LaRCsim flight model integration.
 * Added some MSFS-like keyboard input handling.
 *
 * Revision 1.5  1997/05/30 23:26:25  curt
 * Added elevator/aileron controls.
 *
 * Revision 1.4  1997/05/30 19:30:15  curt
 * The LaRCsim flight model is starting to look like it is working.
 *
 * Revision 1.3  1997/05/30 03:54:12  curt
 * Made a bit more progress towards integrating the LaRCsim flight model.
 *
 * Revision 1.2  1997/05/29 22:39:59  curt
 * Working on incorporating the LaRCsim flight model.
 *
 * Revision 1.1  1997/05/29 00:09:57  curt
 * Initial Flight Gear revision.
 *
 */
