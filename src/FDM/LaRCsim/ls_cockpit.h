/***************************************************************************

	TITLE:		ls_cockpit.h
	
----------------------------------------------------------------------------

	FUNCTION:	Header for cockpit IO

----------------------------------------------------------------------------

	MODULE STATUS:	Developmental

----------------------------------------------------------------------------

	GENEALOGY:	Created 20 DEC 93 by E. B. Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	E. B. Jackson
	
	CODED BY:	E. B. Jackson
	
	MAINTAINED BY:	E. B. Jackson

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY

	950314	Added "throttle_pct" field to cockpit header for both
		display and trim purposes.			EBJ
	
	CURRENT RCS HEADER:

$Header$
$Log$
Revision 1.1  2002/09/10 01:14:01  curt
Initial revision

Revision 1.3  2000/06/12 18:52:37  curt
Added differential braking (Alex and David).

Revision 1.2  1999/10/29 16:08:32  curt
Added flaps support to c172 model.

Revision 1.1.1.1  1999/06/17 18:07:34  curt
Start of 0.7.x branch

Revision 1.2  1999/04/22 18:47:25  curt
Wrap with extern "C" { } if building with __cplusplus compiler.

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.5  1998/10/17 01:34:14  curt
C++ ifying ...

Revision 1.4  1998/08/06 12:46:39  curt
Header change.

Revision 1.3  1998/01/22 02:59:32  curt
Changed #ifdef FILE_H to #ifdef _FILE_H

Revision 1.2  1997/05/31 19:16:27  curt
Elevator trim added.

Revision 1.1  1997/05/29 00:09:54  curt
Initial Flight Gear revision.

 * Revision 1.3  1995/03/15  12:32:10  bjax
 * Added throttle_pct field.
 *
 * Revision 1.2  1995/02/28  20:37:02  bjax
 * Changed name of gear_sel_down switch to gear_sel_up to reflect
 * correct sense of switch.  EBJ
 *
 * Revision 1.1  1993/12/21  14:39:04  bjax
 * Initial revision
 *

--------------------------------------------------------------------------*/


#ifndef _LS_COCKPIT_H
#define _LS_COCKPIT_H

#ifdef __cplusplus
extern "C" { 
#endif

typedef struct {
    float   long_stick, lat_stick, rudder_pedal;
    float   flap_handle;
    float   long_trim;
    float   throttle[4];
    short   forward_trim, aft_trim, left_trim, right_trim;
    short   left_pb_on_stick, right_pb_on_stick, trig_pos_1, trig_pos_2;
    short   sb_extend, sb_retract, gear_sel_up;
    float   throttle_pct;
    float   brake_pct[2];
} COCKPIT;

extern COCKPIT cockpit_;

#define Left_button	cockpit_.left_pb_on_stick
#define Right_button	cockpit_.right_pb_on_stick
#define Rudder_pedal	cockpit_.rudder_pedal
#define Flap_handle	cockpit_.flap_handle
#define Throttle	cockpit_.throttle
#define Throttle_pct	cockpit_.throttle_pct
#define First_trigger	cockpit_.trig_pos_1
#define Second_trigger	cockpit_.trig_pos_2
#define Long_control	cockpit_.long_stick
#define Long_trim       cockpit_.long_trim
#define Lat_control	cockpit_.lat_stick
#define Fwd_trim	cockpit_.forward_trim
#define Aft_trim	cockpit_.aft_trim
#define Left_trim	cockpit_.left_trim
#define Right_trim	cockpit_.right_trim
#define SB_extend	cockpit_.sb_extend
#define SB_retract	cockpit_.sb_retract
#define Gear_sel_up	cockpit_.gear_sel_up
#define Brake_pct       cockpit_.brake_pct


#ifdef __cplusplus
}
#endif

#endif /* _LS_COCKPIT_H */
