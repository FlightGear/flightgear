/***************************************************************************

	TITLE:		ls_sim_control.h
	
----------------------------------------------------------------------------

	FUNCTION:	LaRCSim simulation control parameters header file

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Created 18 DEC 1993 by Bruce Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	B. Jackson
	
	CODED BY:	B. Jackson
	
	MAINTAINED BY:	guess who

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY
	
	940204	Added "overrun" flag to indicate non-real-time frame.
	940210	Added "vision" flag to indicate use of shared memory.
	940513	Added "max_tape_channels" and "max_time_slices" EBJ
	950308	Increased size of time_stamp and date_string to include
		terminating null char. 				EBJ
	950314	Addedf "paused" flag to make this global (was local to
		ls_cockpit routine).				EBJ
	950406	Removed tape_channels parameter, and added end_time, model_hz,
		and term_update_hz parameters.			EBJ	

$Header$
$Log$
Revision 1.1  2002/09/10 01:14:02  curt
Initial revision

Revision 1.1.1.1  1999/06/17 18:07:33  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.4  1998/08/06 12:46:39  curt
Header change.

Revision 1.3  1998/01/22 02:59:33  curt
Changed #ifdef FILE_H to #ifdef _FILE_H

Revision 1.2  1998/01/06 01:20:17  curt
Tweaks to help building with MSVC++

Revision 1.1  1997/05/29 00:09:59  curt
Initial Flight Gear revision.

 * Revision 1.11  1995/04/07  01:39:09  bjax
 * Removed tape_channels and added end_time, model_hz, and term_update_hz.
 *
 * Revision 1.10  1995/03/15  12:33:29  bjax
 * Added 'paused' flag.
 *
 * Revision 1.9  1995/03/08  12:34:21  bjax
 * Increased size of date_string and time_stamp by 1 to include terminating null;
 * added userid field and include of stdio.h. EBJ
 *
 * Revision 1.8  1994/05/13  20:41:43  bjax
 * Increased size of time_stamp to 8 chars to allow for colons.
 * Added fields "tape_channels" and "time_slices" to allow user to change.
 *
 * Revision 1.7  1994/05/10  15:18:49  bjax
 * Modified write_cmp2 flag to write_asc1 flag, since XPLOT 4.00 doesn't
 * support cmp2.  Also added RCS header and log entries in header.
 *
		

--------------------------------------------------------------------------*/


#ifndef _LS_SIM_CONTROL_H
#define _LS_SIM_CONTROL_H


#include <stdio.h>

#ifndef SIM_CONTROL

typedef struct {

  enum { batch, terminal, GLmouse, cockpit } sim_type;
  char simname[64];	/* name of simulation */
  int run_number;	/* run number of this session			  */
  char date_string[7]; 	/* like "931220" */
  char time_stamp[9];  	/* like "13:00:00" */
#ifdef COMPILE_THIS_CODE_THIS_USELESS_CODE
  char userid[L_cuserid]; /* who is running this sim */
#endif /* COMPILE_THIS_CODE_THIS_USELESS_CODE */
  long time_slices;	/* number of points that can be recorded (circ buff) */
  int write_av;		/* will be writing out an Agile_VU file after run */
  int write_mat;	/* will be writing out a matrix script of session */
  int write_tab;	/* will be writing out a tab-delimited time history */
  int write_asc1;	/* will be writing out a GetData ASCII 1 file */
  int save_spacing;	/* spacing between data points when recording
			   data to memory; 0 = every point, 1 = every 
			   other point; 2 = every fourth point, etc. */
  int write_spacing;	/* spacing between data points when writing
			   output files; 0 = every point, 1 = every 
			   other point; 2 = every fourth point, etc. */
  int overrun;		/* indicates, if non-zero, a frame overrun
			   occurred in the previous frame. Suitable for
			   setting a display flag or writing an error
			   message.					*/
  int vision;		/* indicates, if non-zero, marriage to LaRC VISION
			   software (developed A. Dare and J. Burley of the 
			   former Cockpit Technologies Branch) */
  int debug;		/* indicates, if non-zero, to operate in debug mode
			   which implies disable double-buffering and synch.
			   attempts to avoid errors */
  int paused;		/* indicates simulation is paused */
  float end_time;	/* end of simulation run value */
  float model_hz;	/* current inner loop frame rate */
  float term_update_hz; /* current terminal refresh frequency */

} SIM_CONTROL;

extern SIM_CONTROL sim_control_;

#endif


#endif /* _LS_SIM_CONTROL_H */


/*------------------------  end of ls_sim_control.h ----------------------*/
