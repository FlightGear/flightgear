/***************************************************************************

  	TITLE:		ls_trim.c

----------------------------------------------------------------------------

	FUNCTION:  	Trims the simulated aircraft by using certain
			controls to null out a similar number of outputs.

        This routine used modified Newton-Raphson method to find the vector
	of control corrections, delta_U, to drive a similar-sized vector of
	output errors, Y, to near-zero.  Nearness to zero is to within a
	tolerance specified by the Criteria vector.  An optional Weight
	vector can be used to improve the numerical properties of the
	Jacobian matrix (called H_Partials).

	Using a single-sided difference method, each control is
	independently perturbed and the change in each output of
	interest is calculated, forming a Jacobian matrix H (variable
	name is H_Partials):

			dY = H dU


	The columns of H correspond to the control effect; the rows of
	H correspond to the outputs affected.

	We wish to find dU such that for U = U0 + dU,
	
			Y = Y0 + dY = 0
			or dY = -Y0

	One solution is dU = inv(H)*(-Y0); however, inverting H
	directly is not numerically sound, since it may be singular
	(especially if one of the controls is on a limit, or the
	problem is poorly posed).  An alternative is to either weight
	the elements of dU to make them more normalized; another is to
	multiply both sides by the transpose of H and invert the
	resulting [H' H].  This routine does both:

                        -Y0  =      H dU
		     W (-Y0) =    W H dU	premultiply by W
		  H' W (-Y0) = H' W H dU        premultiply by H'

              dU = [inv(H' W H)][ H' W (-Y0)]   Solve for dU

	As a further refinement, dU is limited to a smallish magnitude
	so that Y approaches 0 in small steps (to avoid an overshoot
	if the problem is inherently non-linear).

	Lastly, this routine can be easily fooled by "local minima",
	or depressions in the solution space that don't lead to a Y =
	0 solution.  The only advice we can offer is to "go somewheres
	else and try again"; often approaching a trim solution from a
	different (non-trimmed) starting point will prove beneficial.


----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Created from old CASTLE SHELL$TRIM.PAS
			on 6 FEB 95, which was based upon an Ames
			CASPRE routine called BQUIET.

----------------------------------------------------------------------------

	DESIGNED BY:	E. B. Jackson

	CODED BY:	same

	MAINTAINED BY:	same

----------------------------------------------------------------------------

	MODIFICATION HISTORY:

	DATE	PURPOSE						BY

	950307	Modified to make use of ls_get_sym_val and ls_put_sym_val
		routines.					EBJ
	950329	Fixed bug in making use of more than 3 controls;
		removed call by ls_trim_get_set() to ls_trim_init(). EBJ

	CURRENT RCS HEADER:

$Header$
$Log$
Revision 1.1  2002/09/10 01:14:02  curt
Initial revision

Revision 1.2  2001/07/30 20:53:54  curt
Various MSVC tweaks and warning fixes.

Revision 1.1.1.1  1999/06/17 18:07:33  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

 * Revision 1.9  1995/03/29  16:09:56  bjax
 * Fixed bug in having more than three trim controls; removed unnecessary
 * call to ls_trim_init in ls_trim_get_set. EBJ
 *
 * Revision 1.8  1995/03/16  12:28:40  bjax
 * Fixed problem where ls_trim() returns non-zero if
 * symbols are not loaded - implies vehicle trimmed when
 * actually no trim attempt is made. This results in storing of non-
 * trimmed initial conditions in sims without defined trim controls.
 *
 * Revision 1.7  1995/03/15  12:17:12  bjax
 * Added flag marker line to ls_trim_put_set() routine output.
 *
 * Revision 1.6  1995/03/08  11:49:07  bjax
 * Minor improvements to ls_trim_get_set; deleted weighting parameter
 * for output definition; added comment lines to settings file output.
 *
 * Revision 1.5  1995/03/07  22:38:04  bjax
 * Removed ls_generic.h; this version relies entirely on symbol table routines to
 * set and get variable values. Added additional fields to Control record structure;
 * created Output record with appropriate fields. Added ls_trim_put_set() and
 * ls_trim_get_set() routines. Heavily modified initialization routine; most of this
 * logic now resides in ls_trim_get_set(). Renamed all routines so that they being
 * with "ls_trim_" to avoid conflicts.
 *  EBJ
 *
 * Revision 1.4  1995/03/07  13:04:16  bjax
 * Configured to use ls_get_sym_val() and ls_set_sym_val().
 *
 * Revision 1.3  1995/03/03  01:59:53  bjax
 * Moved definition of SYMBOL_NAME and SYMBOL_TYPE to ls_sym.h
 * and removed from this module. EBJ
 *
 * Revision 1.2  1995/02/27  19:53:41  bjax
 * Moved symbol routines to ls_sym.c to declutter this file. EBJ
 *
 * Revision 1.1  1995/02/27  18:14:10  bjax
 * Initial revision
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

#ifdef __SUNPRO_CC
#  define _REENTRANT
#endif

#include <string.h>
#include "ls_constants.h"
#include "ls_types.h"
#include "ls_sym.h"
#include "ls_matrix.h"
#include "ls_interface.h"


#ifndef TRUE
#define FALSE  0
#define TRUE  !FALSE
#endif

#define MAX_NUMBER_OF_CONTROLS 10
#define MAX_NUMBER_OF_OUTPUTS 10
#define STEP_LIMIT 0.01
#define NIL_POINTER 0L

#define FACILITY_NAME_STRING "trim"
#define CURRENT_VERSION 10


typedef struct
{
    symbol_rec	Symbol;
    double	Min_Val, Max_Val, Curr_Val, Authority;
    double	Percent, Requested_Percent, Pert_Size;
    int		Ineffective, At_Limit;
} control_rec;

typedef struct
{
    symbol_rec	Symbol;
    double	Curr_Val, Weighting, Trim_Criteria;
    int		Uncontrollable;
} output_rec;


static	int		Symbols_loaded = 0;
static  int 		Index;
static  int 		Trim_Cycles;
static  int 		First;
static  int 		Trimmed;
static	double		Gain;

static  int		Number_of_Controls;
static	int		Number_of_Outputs;
static  control_rec	Controls[ MAX_NUMBER_OF_CONTROLS ];
static	output_rec	Outputs[ MAX_NUMBER_OF_OUTPUTS ];

static  double		**H_Partials;

static	double		Baseline_Output[ MAX_NUMBER_OF_OUTPUTS ];
static	double		Saved_Control, Saved_Control_Percent;

static  double		Cost, Previous_Cost;




int ls_trim_init()
/*  Initialize partials matrix */
{
    int i, error;
    // int result;

    Index = -1;
    Trim_Cycles = 0;
    Gain = 1;
    First = 1;
    Previous_Cost = 0.0;
    Trimmed = 0;

    for (i=0;i<Number_of_Controls;i++)
	{
	    Controls[i].Curr_Val = ls_get_sym_val( &Controls[i].Symbol, &error );
	    if (error) Controls[i].Symbol.Addr = NIL_POINTER;
	    Controls[i].Requested_Percent =
		(Controls[i].Curr_Val - Controls[i].Min_Val)
		/Controls[i].Authority;
	}

    H_Partials = nr_matrix( 1, Number_of_Controls, 1, Number_of_Controls );
    if (H_Partials == 0) return -1;

    return 0;
}

void ls_trim_get_vals()
/* Load the Output vector, and calculate control percent used */
{
    int i, error;

    for (i=0;i<Number_of_Outputs;i++)
	{
	    Outputs[i].Curr_Val = ls_get_sym_val( &Outputs[i].Symbol, &error );
	    if (error) Outputs[i].Symbol.Addr = NIL_POINTER;
	}

    Cost = 0.0;
    for (i=0;i<Number_of_Controls;i++)
	{
	    Controls[i].Curr_Val = ls_get_sym_val( &Controls[i].Symbol, &error );
	    if (error) Controls[i].Symbol.Addr = NIL_POINTER;
	    Controls[i].Percent =
		(Controls[i].Curr_Val - Controls[i].Min_Val)
		/Controls[i].Authority;
	}


}

void ls_trim_move_controls()
/* This routine moves the current control to specified percent of authority */
{
    int i;

    for(i=0;i<Number_of_Controls;i++)
	{
	    Controls[i].At_Limit = 0;
	    if (Controls[i].Requested_Percent <= 0.0)
		{
		    Controls[i].Requested_Percent = 0.0;
		    Controls[i].At_Limit = 1;
		}
	    if (Controls[i].Requested_Percent >= 1.0)
		{
		    Controls[i].Requested_Percent = 1.0;
		    Controls[i].At_Limit = 1;
		}
	    Controls[i].Curr_Val = Controls[i].Min_Val +
		(Controls[i].Max_Val - Controls[i].Min_Val) *
		Controls[i].Requested_Percent;
	}
}

void ls_trim_put_controls()
/* Put current control requests out to controls themselves */
{
    int i;

    for (i=0;i<Number_of_Controls;i++)
	    if (Controls[i].Symbol.Addr)
		ls_set_sym_val( &Controls[i].Symbol, Controls[i].Curr_Val );
}

void ls_trim_calc_cost()
/* This routine calculates the current distance, or cost, from trim */
{
    int i;

    Cost = 0.0;
    for(i=0;i<Number_of_Outputs;i++)
	Cost += pow((Outputs[i].Curr_Val/Outputs[i].Trim_Criteria),2.0);
}

void ls_trim_save_baseline_outputs()
{
    int i, error;

    for (i=0;i<Number_of_Outputs;i++)
	    Baseline_Output[i] = ls_get_sym_val( &Outputs[i].Symbol, &error );
}

int  ls_trim_eval_outputs()
{
    int i, trimmed;

    trimmed = 1;
    for(i=0;i<Number_of_Outputs;i++)
	if( fabs(Outputs[i].Curr_Val) > Outputs[i].Trim_Criteria) trimmed = 0;
    return trimmed;
}

void ls_trim_calc_h_column()
{
    int i;
    double delta_control, delta_output;

    delta_control = (Controls[Index].Curr_Val - Saved_Control)/Controls[Index].Authority;

    for(i=0;i<Number_of_Outputs;i++)
	{
	    delta_output = Outputs[i].Curr_Val - Baseline_Output[i];
	    H_Partials[i+1][Index+1] = delta_output/delta_control;
	}
}

void ls_trim_do_step()
{
    int i, j, l, singular;
    double 	**h_trans_w_h;
    double	delta_req_mag, scaling;
    double	delta_U_requested[ MAX_NUMBER_OF_CONTROLS ];
    double	temp[ MAX_NUMBER_OF_CONTROLS ];

    /* Identify ineffective controls (whose partials are all near zero) */

    for (j=0;j<Number_of_Controls;j++)
	{
	    Controls[j].Ineffective = 1;
	    for(i=0;i<Number_of_Outputs;i++)
		if (fabs(H_Partials[i+1][j+1]) > EPS) Controls[j].Ineffective = 0;
	}

    /* Identify uncontrollable outputs */

    for (j=0;j<Number_of_Outputs;j++)
	{
	    Outputs[j].Uncontrollable = 1;
	    for(i=0;i<Number_of_Controls;i++)
		if (fabs(H_Partials[j+1][i+1]) > EPS) Outputs[j].Uncontrollable = 0;
	}

    /* Calculate well-conditioned partials matrix [ H' W H ] */

    h_trans_w_h = nr_matrix(1, Number_of_Controls, 1, Number_of_Controls);
    if (h_trans_w_h == 0)
	{
	    fprintf(stderr, "Memory error in ls_trim().\n");
	    exit(1);
	}
    for (l=1;l<=Number_of_Controls;l++)
	for (j=1;j<=Number_of_Controls;j++)
	    {
		h_trans_w_h[l][j] = 0.0;
		for (i=1;i<=Number_of_Outputs;i++)
		    h_trans_w_h[l][j] +=
			H_Partials[i][l]*H_Partials[i][j]*Outputs[i-1].Weighting;
	    }

    /* Invert the partials array  [ inv( H' W H ) ]; note: h_trans_w_h is replaced
       with its inverse during this function call */

    singular = nr_gaussj( h_trans_w_h, Number_of_Controls, 0, 0 );

    if (singular) /* Can't invert successfully */
	{
	    nr_free_matrix( h_trans_w_h, 1, Number_of_Controls,
			                 1, Number_of_Controls );
	    fprintf(stderr, "Singular matrix in ls_trim().\n");
	    return;
	}

    /* Form right hand side of equality: temp = [ H' W (-Y) ] */

    for(i=0;i<Number_of_Controls;i++)
	{
	    temp[i] = 0.0;
	    for(j=0;j<Number_of_Outputs;j++)
		temp[i] -= H_Partials[j+1][i+1]*Baseline_Output[j]*Outputs[j].Weighting;
	}

    /* Solve for dU = [inv( H' W H )][ H' W (-Y)] */
    for(i=0;i<Number_of_Controls;i++)
	{
	    delta_U_requested[i] = 0.0;
	    for(j=0;j<Number_of_Controls;j++)
		delta_U_requested[i] += h_trans_w_h[i+1][j+1]*temp[j];
	}

    /* limit step magnitude to certain size, but not direction */

    delta_req_mag = 0.0;
    for(i=0;i<Number_of_Controls;i++)
	delta_req_mag += delta_U_requested[i]*delta_U_requested[i];
    delta_req_mag = sqrt(delta_req_mag);
    scaling = STEP_LIMIT/delta_req_mag;
    if (scaling < 1.0)
	for(i=0;i<Number_of_Controls;i++)
	    delta_U_requested[i] *= scaling;

    /* Convert deltas to percent of authority */

    for(i=0;i<Number_of_Controls;i++)
	Controls[i].Requested_Percent = Controls[i].Percent + delta_U_requested[i];

    /* free up temporary matrix */

    nr_free_matrix( h_trans_w_h, 1, Number_of_Controls,
		                 1, Number_of_Controls );

}



int ls_trim()
{
    const int Max_Cycles = 100;
    int Baseline;

    Trimmed = 0;
    if (Symbols_loaded) {

	ls_trim_init();    		/* Initialize Outputs & controls */
	ls_trim_get_vals();  /* Limit the current control settings */
	Baseline = TRUE;
	ls_trim_move_controls();		/* Write out the new values of controls */
	ls_trim_put_controls();
	ls_loop( 0.0, -1 );		/* Cycle the simulation once with new limited
					   controls */

	/* Main trim cycle loop follows */

	while((!Trimmed) && (Trim_Cycles < Max_Cycles))
	    {
		ls_trim_get_vals();
		if (Index == -1)
		    {
			ls_trim_calc_cost();
			/*Adjust_Gain();	*/
			ls_trim_save_baseline_outputs();
			Trimmed = ls_trim_eval_outputs();
		    }
		else
		    {
			ls_trim_calc_h_column();
			Controls[Index].Curr_Val = Saved_Control;
			Controls[Index].Percent  = Saved_Control_Percent;
			Controls[Index].Requested_Percent = Saved_Control_Percent;
		    }
		Index++;
		if (!Trimmed)
		    {
			if (Index >= Number_of_Controls)
			    {
				Baseline = TRUE;
				Index = -1;
				ls_trim_do_step();
			    }
			else
			    { /* Save the current value & pert next control */
				Baseline = FALSE;
				Saved_Control = Controls[Index].Curr_Val;
				Saved_Control_Percent = Controls[Index].Percent;

				if (Controls[Index].Percent < 
				    (1.0 - Controls[Index].Pert_Size) )
				    {
					Controls[Index].Requested_Percent =
					    Controls[Index].Percent +
					    Controls[Index].Pert_Size ;
				    }
				else
				    {
					Controls[Index].Requested_Percent =
					    Controls[Index].Percent -
					    Controls[Index].Pert_Size;
				    }
			    }
			ls_trim_move_controls();
			ls_trim_put_controls();
			ls_loop( 0.0, -1 );
			Trim_Cycles++;
		    }
	    }

	nr_free_matrix( H_Partials, 1, Number_of_Controls, 1, Number_of_Controls );
    }

    if (!Trimmed)  fprintf(stderr, "Trim unsuccessful.\n");
    return Trimmed;

}


char *ls_trim_get_set(char *buffer, char *eob)
/* This routine parses the settings file for "trim" entries. */
{

    static char *fac_name = FACILITY_NAME_STRING;
    char *bufptr, **lasts, *nullptr, null = '\0';
    char line[256];
    int n, ver, i, error, abrt;
    enum {controls_header, controls, outputs_header, outputs, done} looking_for;

    nullptr = &null;
    lasts = &nullptr;
    abrt = 0;
    looking_for = controls_header;


    n = sscanf(buffer, "%s", line);
    if (n == 0) return 0L;
    if (strncasecmp( fac_name, line, strlen(fac_name) )) return 0L;

    bufptr = strtok_r( buffer+strlen(fac_name)+1, "\n", lasts);
    if (bufptr == 0) return 0L;

    sscanf( bufptr, "%d", &ver );
    if (ver != CURRENT_VERSION) return 0L;

    while( !abrt && (eob > bufptr))
      {
	bufptr = strtok_r( 0L, "\n", lasts );
	if (bufptr == 0) return 0L;
	if (strncasecmp( bufptr, "end", 3) == 0) break;

	sscanf( bufptr, "%s", line );
	if (line[0] != '#') /* ignore comments */
	    {
		switch (looking_for)
		    {
		    case controls_header:
			{
			    if (strncasecmp( line, "controls", 8) == 0) 
				{
				    n = sscanf( bufptr, "%s%d", line, &Number_of_Controls );
				    if (n != 2) abrt = 1;
				    looking_for = controls;
				    i = 0;
				}
			    break;
			}
		    case controls:
			{
			    n = sscanf( bufptr, "%s%s%le%le%le", 
					Controls[i].Symbol.Mod_Name,
					Controls[i].Symbol.Par_Name,
					&Controls[i].Min_Val,
					&Controls[i].Max_Val,
					&Controls[i].Pert_Size); 
			    if (n != 5) abrt = 1;
			    Controls[i].Symbol.Addr = NIL_POINTER;
			    i++;
			    if (i >= Number_of_Controls) looking_for = outputs_header;
			    break;
			}
		    case outputs_header:
			{
			    if (strncasecmp( line, "outputs", 7) == 0) 
				{
				    n = sscanf( bufptr, "%s%d", line, &Number_of_Outputs );
				    if (n != 2) abrt = 1;
				    looking_for = outputs;
				    i = 0;
				}
			    break;
			}
		    case outputs:
			{
			    n = sscanf( bufptr, "%s%s%le", 
					Outputs[i].Symbol.Mod_Name,
					Outputs[i].Symbol.Par_Name,
					&Outputs[i].Trim_Criteria );
			    if (n != 3) abrt = 1;
			    Outputs[i].Symbol.Addr = NIL_POINTER;
			    i++;
			    if (i >= Number_of_Outputs) looking_for = done;
			}  
		    case done:
			{
			    break;
			}
		    }

	    }
      }

    if ((!abrt) && 
	(Number_of_Controls > 0) && 
	(Number_of_Outputs == Number_of_Controls))
	{
	    Symbols_loaded = 1;

	    for(i=0;i<Number_of_Controls;i++) /* Initialize fields in Controls data */
		{
		    Controls[i].Curr_Val = ls_get_sym_val( &Controls[i].Symbol, &error );
		    if (error) 
			Controls[i].Symbol.Addr = NIL_POINTER;
		    Controls[i].Authority = Controls[i].Max_Val - Controls[i].Min_Val;
		    if (Controls[i].Authority == 0.0) 
			Controls[i].Authority = 1.0;
		    Controls[i].Requested_Percent = 
			(Controls[i].Curr_Val - Controls[i].Min_Val)
			/Controls[i].Authority;
		    Controls[i].Pert_Size = Controls[i].Pert_Size/Controls[i].Authority;
		}

	    for (i=0;i<Number_of_Outputs;i++) /* Initialize fields in Outputs data */
		{
		    Outputs[i].Curr_Val = ls_get_sym_val( &Outputs[i].Symbol, &error );
		    if (error) Outputs[i].Symbol.Addr = NIL_POINTER;
		    Outputs[i].Weighting = 
			Outputs[0].Trim_Criteria/Outputs[i].Trim_Criteria;
		}
	}

    bufptr = *lasts;
    return bufptr;
}



void ls_trim_put_set( FILE *fp )
{
    int i;

    if (fp==0) return;
    fprintf(fp, "\n");
    fprintf(fp, "#==============================  %s\n", FACILITY_NAME_STRING);
    fprintf(fp, "\n");
    fprintf(fp, FACILITY_NAME_STRING);
    fprintf(fp, "\n");
    fprintf(fp, "%04d\n", CURRENT_VERSION);
    fprintf(fp, "  controls: %d\n", Number_of_Controls);
    fprintf(fp, "#    module    parameter   min_val   max_val   pert_size\n");
    for (i=0; i<Number_of_Controls; i++)
	fprintf(fp, "    %s\t%s\t%E\t%E\t%E\n", 
		Controls[i].Symbol.Mod_Name,
		Controls[i].Symbol.Par_Name,
		Controls[i].Min_Val,
		Controls[i].Max_Val,
		Controls[i].Pert_Size*Controls[i].Authority); 
    fprintf(fp, "  outputs: %d\n", Number_of_Outputs);
    fprintf(fp, "#    module    parameter   trim_criteria\n");
    for (i=0;i<Number_of_Outputs;i++)
	fprintf(fp, "    %s\t%s\t%E\n",
		Outputs[i].Symbol.Mod_Name,
		Outputs[i].Symbol.Par_Name,
		Outputs[i].Trim_Criteria );
    fprintf(fp, "end\n");
    return;
}
