/***************************************************************************

	TITLE:	ls_init.c
	
----------------------------------------------------------------------------

	FUNCTION:	Initializes simulation

----------------------------------------------------------------------------

	MODULE STATUS:	incomplete

----------------------------------------------------------------------------

	GENEALOGY:	Written 921230 by Bruce Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	EBJ
	
	CODED BY:	EBJ
	
	MAINTAINED BY:	EBJ

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY

	950314	Added get_set, put_set, and init routines.	EBJ
	
	CURRENT RCS HEADER:

$Header$
$Log$
Revision 1.1  1997/05/29 00:09:57  curt
Initial Flight Gear revision.

 * Revision 1.4  1995/03/15  12:15:23  bjax
 * Added ls_init_get_set() and ls_init_put_set() and ls_init_init()
 * routines. EBJ
 *
 * Revision 1.3  1994/01/11  19:09:44  bjax
 * Fixed header includes.
 *
 * Revision 1.2  1992/12/30  14:04:53  bjax
 * Added call to ls_step(0, 1).
 *
 * Revision 1.1  92/12/30  14:02:19  bjax
 * Initial revision
 * 
 * Revision 1.1  92/12/30  13:21:21  bjax
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
static char rcsid[] = "$Id$";

#include <string.h>
#include <stdio.h>
#include "ls_types.h"
#include "ls_sym.h"

#define MAX_NUMBER_OF_CONTINUOUS_STATES 100
#define MAX_NUMBER_OF_DISCRETE_STATES  20
#define HARDWIRED 13
#define NIL_POINTER 0L

#define FACILITY_NAME_STRING "init"
#define CURRENT_VERSION 10

typedef struct
{
    symbol_rec	Symbol;
    double	value;
} cont_state_rec;

typedef struct
{
    symbol_rec	Symbol;
    long	value;
} disc_state_rec;


extern SCALAR Simtime;

static int Symbols_loaded = 0;
static int Number_of_Continuous_States = 0;
static int Number_of_Discrete_States = 0;
static cont_state_rec	Continuous_States[ MAX_NUMBER_OF_CONTINUOUS_STATES ];
static disc_state_rec	Discrete_States[  MAX_NUMBER_OF_DISCRETE_STATES ];


void ls_init_init()
{
    int i, error;

    if (Number_of_Continuous_States == 0)
	{
	    Number_of_Continuous_States = HARDWIRED;

	    for (i=0;i<HARDWIRED;i++)
		strcpy( Continuous_States[i].Symbol.Mod_Name, "*" );

	    strcpy( Continuous_States[ 0].Symbol.Par_Name, 
		    "generic_.geodetic_position_v[0]");
	    strcpy( Continuous_States[ 1].Symbol.Par_Name, 
		    "generic_.geodetic_position_v[1]");
	    strcpy( Continuous_States[ 2].Symbol.Par_Name, 
		    "generic_.geodetic_position_v[2]");
	    strcpy( Continuous_States[ 3].Symbol.Par_Name, 
		    "generic_.v_local_v[0]");
	    strcpy( Continuous_States[ 4].Symbol.Par_Name, 
		    "generic_.v_local_v[1]");
	    strcpy( Continuous_States[ 5].Symbol.Par_Name, 
		    "generic_.v_local_v[2]");
	    strcpy( Continuous_States[ 6].Symbol.Par_Name, 
		    "generic_.euler_angles_v[0]");
	    strcpy( Continuous_States[ 7].Symbol.Par_Name, 
		    "generic_.euler_angles_v[1]");
	    strcpy( Continuous_States[ 8].Symbol.Par_Name, 
		    "generic_.euler_angles_v[2]");
	    strcpy( Continuous_States[ 9].Symbol.Par_Name, 
		    "generic_.omega_body_v[0]");
	    strcpy( Continuous_States[10].Symbol.Par_Name, 
		    "generic_.omega_body_v[1]");
	    strcpy( Continuous_States[11].Symbol.Par_Name, 
		    "generic_.omega_body_v[2]");
	    strcpy( Continuous_States[12].Symbol.Par_Name, 
		    "generic_.earth_position_angle");
	}

    /* commented out by CLO 
    for (i=0;i<Number_of_Continuous_States;i++)
	{
	    (void) ls_get_sym_val( &Continuous_States[i].Symbol, &error );
	    if (error) Continuous_States[i].Symbol.Addr = NIL_POINTER;
	}

    for (i=0;i<Number_of_Discrete_States;i++)
	{
	    (void) ls_get_sym_val( &Discrete_States[i].Symbol, &error );
	    if (error) Discrete_States[i].Symbol.Addr = NIL_POINTER;
	}
    */
}

void ls_init()
{
    int i;

    Simtime = 0;

    ls_init_init();
    /* move the states to proper values */

    /* commented out by CLO
    for(i=0;i<Number_of_Continuous_States;i++)
	if (Continuous_States[i].Symbol.Addr)
	    ls_set_sym_val( &Continuous_States[i].Symbol, 
			    Continuous_States[i].value );

    for(i=0;i<Number_of_Discrete_States;i++)
	if (Discrete_States[i].Symbol.Addr)
	    ls_set_sym_val( &Discrete_States[i].Symbol, 
			    (double) Discrete_States[i].value );
    */
  
    model_init();

    ls_step(0.0, -1);
}


char *ls_init_get_set(char *buffer, char *eob)
/* This routine parses the settings file for "init" entries. */
{

    static char *fac_name = FACILITY_NAME_STRING;
    char *bufptr;
    char line[256];
    int n, ver, i, error, abrt;

    enum {cont_states_header, cont_states, disc_states_header, disc_states, done } looking_for;

    abrt = 0;
    looking_for = cont_states_header;

    n = sscanf(buffer, "%s", line);
    if (n == 0) return 0L;
    if (strncasecmp( fac_name, line, strlen(fac_name) )) return 0L;

    bufptr = strtok( buffer+strlen(fac_name)+1, "\n");
    if (bufptr == 0) return 0L;

    sscanf( bufptr, "%d", &ver );
    if (ver != CURRENT_VERSION) return 0L;

    ls_init_init();

    while( !abrt && (eob > bufptr))
      {
	bufptr = strtok( 0L, "\n");
	if (bufptr == 0) return 0L;
	if (strncasecmp( bufptr, "end", 3) == 0) break;

	sscanf( bufptr, "%s", line );
	if (line[0] != '#') /* ignore comments */
	    {
		switch (looking_for)
		    {
		    case cont_states_header:
			{
			    if (strncasecmp( line, "continuous_states", 17) == 0) 
				{
				    n = sscanf( bufptr, "%s%d", line, 
						&Number_of_Continuous_States );
				    if (n != 2) abrt = 1;
				    looking_for = cont_states;
				    i = 0;
				}
			    break;
			}
		    case cont_states:
			{
			    n = sscanf( bufptr, "%s%s%le", 
					Continuous_States[i].Symbol.Mod_Name,
					Continuous_States[i].Symbol.Par_Name,
					&Continuous_States[i].value );
			    if (n != 3) abrt = 1;
			    Continuous_States[i].Symbol.Addr = NIL_POINTER;
			    i++;
			    if (i >= Number_of_Continuous_States) 
				looking_for = disc_states_header;
			    break;
			}
		    case disc_states_header:
			{
			    if (strncasecmp( line, "discrete_states", 15) == 0) 
				{
				    n = sscanf( bufptr, "%s%d", line, 
						&Number_of_Discrete_States );
				    if (n != 2) abrt = 1;
				    looking_for = disc_states;
				    i = 0;
				}
			    break;
			}
		    case disc_states:
			{
			    n = sscanf( bufptr, "%s%s%ld", 
					Discrete_States[i].Symbol.Mod_Name,
					Discrete_States[i].Symbol.Par_Name,
					&Discrete_States[i].value );
			    if (n != 3) abrt = 1;
			    Discrete_States[i].Symbol.Addr = NIL_POINTER;
			    i++;
			    if (i >= Number_of_Discrete_States) looking_for = done;
			}  
		    case done:
			{
			    break;
			}
		    }

	    }
      }

    Symbols_loaded = !abrt;

    return bufptr;
}



void ls_init_put_set( FILE *fp )
{
    int i;

    if (fp==0) return;
    fprintf(fp, "\n");
    fprintf(fp, "#==============================  %s\n", FACILITY_NAME_STRING);
    fprintf(fp, "\n");
    fprintf(fp, FACILITY_NAME_STRING);
    fprintf(fp, "\n");
    fprintf(fp, "%04d\n", CURRENT_VERSION);
    fprintf(fp, "  continuous_states: %d\n", Number_of_Continuous_States);
    fprintf(fp, "#    module    parameter   value\n");
    for (i=0; i<Number_of_Continuous_States; i++)
	fprintf(fp, "    %s\t%s\t%E\n", 
		Continuous_States[i].Symbol.Mod_Name,
		Continuous_States[i].Symbol.Par_Name,
		Continuous_States[i].value );

    fprintf(fp, "  discrete_states: %d\n", Number_of_Discrete_States);
    fprintf(fp, "#    module    parameter   value\n");
    for (i=0;i<Number_of_Discrete_States;i++)
	fprintf(fp, "    %s\t%s\t%ld\n",
		Discrete_States[i].Symbol.Mod_Name,
		Discrete_States[i].Symbol.Par_Name,
		Discrete_States[i].value );
    fprintf(fp, "end\n");
    return;
}

void ls_save_current_as_ic()
{
    /* Save current states as initial conditions */

    int i, error;

    /* commented out by CLO 
    for(i=0;i<Number_of_Continuous_States;i++)
	if (Continuous_States[i].Symbol.Addr)
	    Continuous_States[i].value = 
		ls_get_sym_val( &Continuous_States[i].Symbol, &error );

    for(i=0;i<Number_of_Discrete_States;i++)
	if (Discrete_States[i].Symbol.Addr)
	    Discrete_States[i].value = (long)
		ls_get_sym_val( &Discrete_States[i].Symbol, &error );
    */
}
