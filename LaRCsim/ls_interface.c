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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#include <sys/types.h>
/* #include <sys/stat.h> */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "ls_types.h"
#include "ls_constants.h"
#include "ls_generic.h"
#include "ls_sim_control.h"
#include "ls_cockpit.h"
#include "ls_interface.h"
#include "ls_step.h"
#include "ls_accel.h"
#include "ls_aux.h"
#include "ls_model.h"
#include "ls_init.h"
#include <Flight/flight.h>
#include <Aircraft/aircraft.h>

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
    date = (nowtime->tm_year)*10000 
	 + (nowtime->tm_mon + 1)*100
	 + (nowtime->tm_mday);
    sprintf(sim_control_.date_string, "%06d\0", date);
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


void ls_loop( SCALAR dt, int initialize ) {
    /* printf ("  In ls_loop()\n"); */
    ls_step( dt, initialize );
    /* if (sim_control_.sim_type == cockpit ) ls_ACES();  */
    ls_aux();
    ls_model( dt, initialize );
    ls_accel();
}



int ls_cockpit( void ) {
    struct fgCONTROLS *c;

    sim_control_.paused = 0;

    c = &current_aircraft.controls;

    Lat_control = FG_Aileron;
    Long_control = FG_Elevator;
    Long_trim = FG_Elev_Trim;
    Rudder_pedal = FG_Rudder;
    Throttle_pct = FG_Throttle[0];

    /* printf("Mach = %.2f  ", Mach_number);
    printf("%.4f,%.4f,%.2f  ", Latitude, Longitude, Altitude);
    printf("%.2f,%.2f,%.2f\n", Phi, Theta, Psi); */

    return( 0 );
}


/* Initialize the LaRCsim flight model, dt is the time increment for
   each subsequent iteration through the EOM */
int fgLaRCsimInit(double dt) {
    model_dt = dt;

    ls_setdefopts();		/* set default options */
	
    ls_stamp();   /* ID stamp; record time and date of run */

    if (speedup == 0.0) {
	fprintf(stderr, "%s: Cannot run with speedup of 0.\n", progname);
	return 1;
    }

    printf("LS pre Init pos = %.2f\n", Latitude);

    ls_init();

    printf("LS post Init pos = %.2f\n", Latitude);

    if (speedup > 0) {
	/* Initialize (get) cockpit (controls) settings */
	ls_cockpit();
    }

    return(1);
}


/* Run an iteration of the EOM (equations of motion) */
int fgLaRCsimUpdate(int multiloop) {
    int	i;

    if (speedup > 0) {
	ls_cockpit();
    }

    for ( i = 0; i < multiloop; i++ ) {
	ls_loop( model_dt, 0);
    }

    return(1);
}


/* Convert from the fgFLIGHT struct to the LaRCsim generic_ struct */
int fgFlight_2_LaRCsim (struct fgFLIGHT *f) {
    Mass =      FG_Mass;
    I_xx =      FG_I_xx;
    I_yy =      FG_I_yy;
    I_zz =      FG_I_zz;
    I_xz =      FG_I_xz;
    Dx_pilot =  FG_Dx_pilot;
    Dy_pilot =  FG_Dy_pilot;
    Dz_pilot =  FG_Dz_pilot;
    Dx_cg =     FG_Dx_cg;
    Dy_cg =     FG_Dy_cg;
    Dz_cg =     FG_Dz_cg;
    F_X =       FG_F_X;
    F_Y =       FG_F_Y;
    F_Z =       FG_F_Z;
    F_north =   FG_F_north;
    F_east =    FG_F_east;
    F_down =    FG_F_down;
    F_X_aero =  FG_F_X_aero;
    F_Y_aero =  FG_F_Y_aero;
    F_Z_aero =  FG_F_Z_aero;
    F_X_engine =        FG_F_X_engine;
    F_Y_engine =        FG_F_Y_engine;
    F_Z_engine =        FG_F_Z_engine;
    F_X_gear =  FG_F_X_gear;
    F_Y_gear =  FG_F_Y_gear;
    F_Z_gear =  FG_F_Z_gear;
    M_l_rp =    FG_M_l_rp;
    M_m_rp =    FG_M_m_rp;
    M_n_rp =    FG_M_n_rp;
    M_l_cg =    FG_M_l_cg;
    M_m_cg =    FG_M_m_cg;
    M_n_cg =    FG_M_n_cg;
    M_l_aero =  FG_M_l_aero;
    M_m_aero =  FG_M_m_aero;
    M_n_aero =  FG_M_n_aero;
    M_l_engine =        FG_M_l_engine;
    M_m_engine =        FG_M_m_engine;
    M_n_engine =        FG_M_n_engine;
    M_l_gear =  FG_M_l_gear;
    M_m_gear =  FG_M_m_gear;
    M_n_gear =  FG_M_n_gear;
    V_dot_north =       FG_V_dot_north;
    V_dot_east =        FG_V_dot_east;
    V_dot_down =        FG_V_dot_down;
    U_dot_body =        FG_U_dot_body;
    V_dot_body =        FG_V_dot_body;
    W_dot_body =        FG_W_dot_body;
    A_X_cg =    FG_A_X_cg;
    A_Y_cg =    FG_A_Y_cg;
    A_Z_cg =    FG_A_Z_cg;
    A_X_pilot = FG_A_X_pilot;
    A_Y_pilot = FG_A_Y_pilot;
    A_Z_pilot = FG_A_Z_pilot;
    N_X_cg =    FG_N_X_cg;
    N_Y_cg =    FG_N_Y_cg;
    N_Z_cg =    FG_N_Z_cg;
    N_X_pilot = FG_N_X_pilot;
    N_Y_pilot = FG_N_Y_pilot;
    N_Z_pilot = FG_N_Z_pilot;
    P_dot_body =        FG_P_dot_body;
    Q_dot_body =        FG_Q_dot_body;
    R_dot_body =        FG_R_dot_body;
    V_north =   FG_V_north;
    V_east =    FG_V_east;
    V_down =    FG_V_down;
    V_north_rel_ground =        FG_V_north_rel_ground;
    V_east_rel_ground = FG_V_east_rel_ground;
    V_down_rel_ground = FG_V_down_rel_ground;
    V_north_airmass =   FG_V_north_airmass;
    V_east_airmass =    FG_V_east_airmass;
    V_down_airmass =    FG_V_down_airmass;
    V_north_rel_airmass =       FG_V_north_rel_airmass;
    V_east_rel_airmass =        FG_V_east_rel_airmass;
    V_down_rel_airmass =        FG_V_down_rel_airmass;
    U_gust =    FG_U_gust;
    V_gust =    FG_V_gust;
    W_gust =    FG_W_gust;
    U_body =    FG_U_body;
    V_body =    FG_V_body;
    W_body =    FG_W_body;
    V_rel_wind =        FG_V_rel_wind;
    V_true_kts =        FG_V_true_kts;
    V_rel_ground =      FG_V_rel_ground;
    V_inertial =        FG_V_inertial;
    V_ground_speed =    FG_V_ground_speed;
    V_equiv =   FG_V_equiv;
    V_equiv_kts =       FG_V_equiv_kts;
    V_calibrated =      FG_V_calibrated;
    V_calibrated_kts =  FG_V_calibrated_kts;
    P_body =    FG_P_body;
    Q_body =    FG_Q_body;
    R_body =    FG_R_body;
    P_local =   FG_P_local;
    Q_local =   FG_Q_local;
    R_local =   FG_R_local;
    P_total =   FG_P_total;
    Q_total =   FG_Q_total;
    R_total =   FG_R_total;
    Phi_dot =   FG_Phi_dot;
    Theta_dot = FG_Theta_dot;
    Psi_dot =   FG_Psi_dot;
    Latitude_dot =      FG_Latitude_dot;
    Longitude_dot =     FG_Longitude_dot;
    Radius_dot =        FG_Radius_dot;
    Lat_geocentric =    FG_Lat_geocentric;
    Lon_geocentric =    FG_Lon_geocentric;
    Radius_to_vehicle = FG_Radius_to_vehicle;
    Latitude =  FG_Latitude;
    Longitude = FG_Longitude;
    Altitude =  FG_Altitude;
    Phi =       FG_Phi;
    Theta =     FG_Theta;
    Psi =       FG_Psi;
    T_local_to_body_11 =        FG_T_local_to_body_11;
    T_local_to_body_12 =        FG_T_local_to_body_12;
    T_local_to_body_13 =        FG_T_local_to_body_13;
    T_local_to_body_21 =        FG_T_local_to_body_21;
    T_local_to_body_22 =        FG_T_local_to_body_22;
    T_local_to_body_23 =        FG_T_local_to_body_23;
    T_local_to_body_31 =        FG_T_local_to_body_31;
    T_local_to_body_32 =        FG_T_local_to_body_32;
    T_local_to_body_33 =        FG_T_local_to_body_33;
    Gravity =   FG_Gravity;
    Centrifugal_relief =        FG_Centrifugal_relief;
    Alpha =     FG_Alpha;
    Beta =      FG_Beta;
    Alpha_dot = FG_Alpha_dot;
    Beta_dot =  FG_Beta_dot;
    Cos_alpha = FG_Cos_alpha;
    Sin_alpha = FG_Sin_alpha;
    Cos_beta =  FG_Cos_beta;
    Sin_beta =  FG_Sin_beta;
    Cos_phi =   FG_Cos_phi;
    Sin_phi =   FG_Sin_phi;
    Cos_theta = FG_Cos_theta;
    Sin_theta = FG_Sin_theta;
    Cos_psi =   FG_Cos_psi;
    Sin_psi =   FG_Sin_psi;
    Gamma_vert_rad =    FG_Gamma_vert_rad;
    Gamma_horiz_rad =   FG_Gamma_horiz_rad;
    Sigma =     FG_Sigma;
    Density =   FG_Density;
    V_sound =   FG_V_sound;
    Mach_number =       FG_Mach_number;
    Static_pressure =   FG_Static_pressure;
    Total_pressure =    FG_Total_pressure;
    Impact_pressure =   FG_Impact_pressure;
    Dynamic_pressure =  FG_Dynamic_pressure;
    Static_temperature =        FG_Static_temperature;
    Total_temperature = FG_Total_temperature;
    Sea_level_radius =  FG_Sea_level_radius;
    Earth_position_angle =      FG_Earth_position_angle;
    Runway_altitude =   FG_Runway_altitude;
    Runway_latitude =   FG_Runway_latitude;
    Runway_longitude =  FG_Runway_longitude;
    Runway_heading =    FG_Runway_heading;
    Radius_to_rwy =     FG_Radius_to_rwy;
    D_cg_north_of_rwy = FG_D_cg_north_of_rwy;
    D_cg_east_of_rwy =  FG_D_cg_east_of_rwy;
    D_cg_above_rwy =    FG_D_cg_above_rwy;
    X_cg_rwy =  FG_X_cg_rwy;
    Y_cg_rwy =  FG_Y_cg_rwy;
    H_cg_rwy =  FG_H_cg_rwy;
    D_pilot_north_of_rwy =      FG_D_pilot_north_of_rwy;
    D_pilot_east_of_rwy =       FG_D_pilot_east_of_rwy;
    D_pilot_above_rwy = FG_D_pilot_above_rwy;
    X_pilot_rwy =       FG_X_pilot_rwy;
    Y_pilot_rwy =       FG_Y_pilot_rwy;
    H_pilot_rwy =       FG_H_pilot_rwy;

    return( 0 );
}


/* Convert from the LaRCsim generic_ struct to the fgFLIGHT struct */
int fgLaRCsim_2_Flight (struct fgFLIGHT *f) {
    FG_Mass =   Mass;
    FG_I_xx =   I_xx;
    FG_I_yy =   I_yy;
    FG_I_zz =   I_zz;
    FG_I_xz =   I_xz;
    FG_Dx_pilot =       Dx_pilot;
    FG_Dy_pilot =       Dy_pilot;
    FG_Dz_pilot =       Dz_pilot;
    FG_Dx_cg =  Dx_cg;
    FG_Dy_cg =  Dy_cg;
    FG_Dz_cg =  Dz_cg;
    FG_F_X =    F_X;
    FG_F_Y =    F_Y;
    FG_F_Z =    F_Z;
    FG_F_north =        F_north;
    FG_F_east = F_east;
    FG_F_down = F_down;
    FG_F_X_aero =       F_X_aero;
    FG_F_Y_aero =       F_Y_aero;
    FG_F_Z_aero =       F_Z_aero;
    FG_F_X_engine =     F_X_engine;
    FG_F_Y_engine =     F_Y_engine;
    FG_F_Z_engine =     F_Z_engine;
    FG_F_X_gear =       F_X_gear;
    FG_F_Y_gear =       F_Y_gear;
    FG_F_Z_gear =       F_Z_gear;
    FG_M_l_rp = M_l_rp;
    FG_M_m_rp = M_m_rp;
    FG_M_n_rp = M_n_rp;
    FG_M_l_cg = M_l_cg;
    FG_M_m_cg = M_m_cg;
    FG_M_n_cg = M_n_cg;
    FG_M_l_aero =       M_l_aero;
    FG_M_m_aero =       M_m_aero;
    FG_M_n_aero =       M_n_aero;
    FG_M_l_engine =     M_l_engine;
    FG_M_m_engine =     M_m_engine;
    FG_M_n_engine =     M_n_engine;
    FG_M_l_gear =       M_l_gear;
    FG_M_m_gear =       M_m_gear;
    FG_M_n_gear =       M_n_gear;
    FG_V_dot_north =    V_dot_north;
    FG_V_dot_east =     V_dot_east;
    FG_V_dot_down =     V_dot_down;
    FG_U_dot_body =     U_dot_body;
    FG_V_dot_body =     V_dot_body;
    FG_W_dot_body =     W_dot_body;
    FG_A_X_cg = A_X_cg;
    FG_A_Y_cg = A_Y_cg;
    FG_A_Z_cg = A_Z_cg;
    FG_A_X_pilot =      A_X_pilot;
    FG_A_Y_pilot =      A_Y_pilot;
    FG_A_Z_pilot =      A_Z_pilot;
    FG_N_X_cg = N_X_cg;
    FG_N_Y_cg = N_Y_cg;
    FG_N_Z_cg = N_Z_cg;
    FG_N_X_pilot =      N_X_pilot;
    FG_N_Y_pilot =      N_Y_pilot;
    FG_N_Z_pilot =      N_Z_pilot;
    FG_P_dot_body =     P_dot_body;
    FG_Q_dot_body =     Q_dot_body;
    FG_R_dot_body =     R_dot_body;
    FG_V_north =        V_north;
    FG_V_east = V_east;
    FG_V_down = V_down;
    FG_V_north_rel_ground =     V_north_rel_ground;
    FG_V_east_rel_ground =      V_east_rel_ground;
    FG_V_down_rel_ground =      V_down_rel_ground;
    FG_V_north_airmass =        V_north_airmass;
    FG_V_east_airmass = V_east_airmass;
    FG_V_down_airmass = V_down_airmass;
    FG_V_north_rel_airmass =    V_north_rel_airmass;
    FG_V_east_rel_airmass =     V_east_rel_airmass;
    FG_V_down_rel_airmass =     V_down_rel_airmass;
    FG_U_gust = U_gust;
    FG_V_gust = V_gust;
    FG_W_gust = W_gust;
    FG_U_body = U_body;
    FG_V_body = V_body;
    FG_W_body = W_body;
    FG_V_rel_wind =     V_rel_wind;
    FG_V_true_kts =     V_true_kts;
    FG_V_rel_ground =   V_rel_ground;
    FG_V_inertial =     V_inertial;
    FG_V_ground_speed = V_ground_speed;
    FG_V_equiv =        V_equiv;
    FG_V_equiv_kts =    V_equiv_kts;
    FG_V_calibrated =   V_calibrated;
    FG_V_calibrated_kts =       V_calibrated_kts;
    FG_P_body = P_body;
    FG_Q_body = Q_body;
    FG_R_body = R_body;
    FG_P_local =        P_local;
    FG_Q_local =        Q_local;
    FG_R_local =        R_local;
    FG_P_total =        P_total;
    FG_Q_total =        Q_total;
    FG_R_total =        R_total;
    FG_Phi_dot =        Phi_dot;
    FG_Theta_dot =      Theta_dot;
    FG_Psi_dot =        Psi_dot;
    FG_Latitude_dot =   Latitude_dot;
    FG_Longitude_dot =  Longitude_dot;
    FG_Radius_dot =     Radius_dot;
    FG_Lat_geocentric = Lat_geocentric;
    FG_Lon_geocentric = Lon_geocentric;
    FG_Radius_to_vehicle =      Radius_to_vehicle;
    FG_Latitude =       Latitude;
    FG_Longitude =      Longitude;
    FG_Altitude =       Altitude;
    FG_Phi =    Phi;
    FG_Theta =  Theta;
    FG_Psi =    Psi;
    FG_T_local_to_body_11 =     T_local_to_body_11;
    FG_T_local_to_body_12 =     T_local_to_body_12;
    FG_T_local_to_body_13 =     T_local_to_body_13;
    FG_T_local_to_body_21 =     T_local_to_body_21;
    FG_T_local_to_body_22 =     T_local_to_body_22;
    FG_T_local_to_body_23 =     T_local_to_body_23;
    FG_T_local_to_body_31 =     T_local_to_body_31;
    FG_T_local_to_body_32 =     T_local_to_body_32;
    FG_T_local_to_body_33 =     T_local_to_body_33;
    FG_Gravity =        Gravity;
    FG_Centrifugal_relief =     Centrifugal_relief;
    FG_Alpha =  Alpha;
    FG_Beta =   Beta;
    FG_Alpha_dot =      Alpha_dot;
    FG_Beta_dot =       Beta_dot;
    FG_Cos_alpha =      Cos_alpha;
    FG_Sin_alpha =      Sin_alpha;
    FG_Cos_beta =       Cos_beta;
    FG_Sin_beta =       Sin_beta;
    FG_Cos_phi =        Cos_phi;
    FG_Sin_phi =        Sin_phi;
    FG_Cos_theta =      Cos_theta;
    FG_Sin_theta =      Sin_theta;
    FG_Cos_psi =        Cos_psi;
    FG_Sin_psi =        Sin_psi;
    FG_Gamma_vert_rad = Gamma_vert_rad;
    FG_Gamma_horiz_rad =        Gamma_horiz_rad;
    FG_Sigma =  Sigma;
    FG_Density =        Density;
    FG_V_sound =        V_sound;
    FG_Mach_number =    Mach_number;
    FG_Static_pressure =        Static_pressure;
    FG_Total_pressure = Total_pressure;
    FG_Impact_pressure =        Impact_pressure;
    FG_Dynamic_pressure =       Dynamic_pressure;
    FG_Static_temperature =     Static_temperature;
    FG_Total_temperature =      Total_temperature;
    FG_Sea_level_radius =       Sea_level_radius;
    FG_Earth_position_angle =   Earth_position_angle;
    FG_Runway_altitude =        Runway_altitude;
    FG_Runway_latitude =        Runway_latitude;
    FG_Runway_longitude =       Runway_longitude;
    FG_Runway_heading = Runway_heading;
    FG_Radius_to_rwy =  Radius_to_rwy;
    FG_D_cg_north_of_rwy =      D_cg_north_of_rwy;
    FG_D_cg_east_of_rwy =       D_cg_east_of_rwy;
    FG_D_cg_above_rwy = D_cg_above_rwy;
    FG_X_cg_rwy =       X_cg_rwy;
    FG_Y_cg_rwy =       Y_cg_rwy;
    FG_H_cg_rwy =       H_cg_rwy;
    FG_D_pilot_north_of_rwy =   D_pilot_north_of_rwy;
    FG_D_pilot_east_of_rwy =    D_pilot_east_of_rwy;
    FG_D_pilot_above_rwy =      D_pilot_above_rwy;
    FG_X_pilot_rwy =    X_pilot_rwy;
    FG_Y_pilot_rwy =    Y_pilot_rwy;
    FG_H_pilot_rwy =    H_pilot_rwy;

    return ( 0 );
}

/* Flight Gear Modification Log
 *
 * $Log$
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
