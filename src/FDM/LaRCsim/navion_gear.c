/***************************************************************************

	TITLE:	gear
	
----------------------------------------------------------------------------

	FUNCTION:	Landing gear model for example simulation

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Created 931012 by E. B. Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	E. B. Jackson
	
	CODED BY:	E. B. Jackson
	
	MAINTAINED BY:	E. B. Jackson

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY

	931218	Added navion.h header to allow connection with
		aileron displacement for nosewheel steering.	EBJ
	940511	Connected nosewheel to rudder pedal; adjusted gain.
	
	CURRENT RCS HEADER:

$Header$
$Log$
Revision 1.1  2002/09/10 01:14:02  curt
Initial revision

Revision 1.4  2001/07/30 20:53:54  curt
Various MSVC tweaks and warning fixes.

Revision 1.3  2000/06/12 18:52:37  curt
Added differential braking (Alex and David).

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

Revision 1.1.1.1  1999/06/17 18:07:34  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.6  1998/10/17 01:34:16  curt
C++ ifying ...

Revision 1.5  1998/09/29 02:03:00  curt
Added a brake + autopilot mods.

Revision 1.4  1998/08/06 12:46:40  curt
Header change.

Revision 1.3  1998/02/03 23:20:18  curt
Lots of little tweaks to fix various consistency problems discovered by
Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
passed arguments along to the real printf().  Also incorporated HUD changes
by Michele America.

Revision 1.2  1998/01/19 18:40:29  curt
Tons of little changes to clean up the code and to remove fatal errors
when building with the c++ compiler.

Revision 1.1  1997/05/29 00:10:02  curt
Initial Flight Gear revision.


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
#include <math.h>
#include "ls_types.h"
#include "ls_constants.h"
#include "ls_generic.h"
#include "ls_cockpit.h"


static void sub3( DATA v1[],  DATA v2[], DATA result[] )
{
    result[0] = v1[0] - v2[0];
    result[1] = v1[1] - v2[1];
    result[2] = v1[2] - v2[2];
}

static void add3( DATA v1[],  DATA v2[], DATA result[] )
{
    result[0] = v1[0] + v2[0];
    result[1] = v1[1] + v2[1];
    result[2] = v1[2] + v2[2];
}

static void cross3( DATA v1[],  DATA v2[], DATA result[] )
{
    result[0] = v1[1]*v2[2] - v1[2]*v2[1];
    result[1] = v1[2]*v2[0] - v1[0]*v2[2];
    result[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

static void multtrans3x3by3( DATA m[][3], DATA v[], DATA result[] )
{
    result[0] = m[0][0]*v[0] + m[1][0]*v[1] + m[2][0]*v[2];
    result[1] = m[0][1]*v[0] + m[1][1]*v[1] + m[2][1]*v[2];
    result[2] = m[0][2]*v[0] + m[1][2]*v[1] + m[2][2]*v[2];
}

static void mult3x3by3( DATA m[][3], DATA v[], DATA result[] )
{
    result[0] = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2];
    result[1] = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2];
    result[2] = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2];
}

static void clear3( DATA v[] )
{
    v[0] = 0.; v[1] = 0.; v[2] = 0.;
}

void navion_gear( SCALAR dt, int Initialize ) {
char rcsid[] = "$Id$";

  /*
   * Aircraft specific initializations and data goes here
   */
   
#define NUM_WHEELS 3

    static int num_wheels = NUM_WHEELS;		    /* number of wheels  */
    static DATA d_wheel_rp_body_v[NUM_WHEELS][3] =  /* X, Y, Z locations */
    {
	{ 10.,  0., 4. },				/* in feet */
	{ -1.,  3., 4. }, 
	{ -1., -3., 4. }
    };
    static DATA spring_constant[NUM_WHEELS] =	    /* springiness, lbs/ft */
	{ 1500., 5000., 5000. };
    static DATA spring_damping[NUM_WHEELS] =	    /* damping, lbs/ft/sec */
	{ 100.,  150.,  150. };		
    static DATA percent_brake[NUM_WHEELS] =	    /* percent applied braking */
	{ 0.,  0.,  0. };			    /* 0 = none, 1 = full */
    static DATA caster_angle_rad[NUM_WHEELS] =	    /* steerable tires - in */
	{ 0., 0., 0.};				    /* radians, +CW */	
  /*
   * End of aircraft specific code
   */
    
  /*
   * Constants & coefficients for tyres on tarmac - ref [1]
   */
   
    /* skid function looks like:
     * 
     *           mu  ^
     *               |
     *       max_mu  |       +		
     *               |      /|		
     *   sliding_mu  |     / +------	
     *               |    /		
     *               |   /		
     *               +--+------------------------> 
     *               |  |    |      sideward V
     *               0 bkout skid
     *	               V     V
     */
  
  
    static DATA sliding_mu   = 0.5;	
    static DATA rolling_mu   = 0.01;	
    static DATA max_brake_mu = 0.6;	
    static DATA max_mu	     = 0.8;	
    static DATA bkout_v	     = 0.1;
    static DATA skid_v       = 1.0;
  /*
   * Local data variables
   */
   
    DATA d_wheel_cg_body_v[3];		/* wheel offset from cg,  X-Y-Z	*/
    DATA d_wheel_cg_local_v[3];		/* wheel offset from cg,  N-E-D	*/
    DATA d_wheel_rwy_local_v[3];	/* wheel offset from rwy, N-E-U */
    // DATA v_wheel_body_v[3];		/* wheel velocity,	  X-Y-Z	*/
    DATA v_wheel_local_v[3];		/* wheel velocity,	  N-E-D	*/
    DATA f_wheel_local_v[3];		/* wheel reaction force,  N-E-D	*/
    DATA temp3a[3], temp3b[3], tempF[3], tempM[3];	
    DATA reaction_normal_force;		/* wheel normal (to rwy) force	*/
    DATA cos_wheel_hdg_angle, sin_wheel_hdg_angle;	/* temp storage */
    DATA v_wheel_forward, v_wheel_sideward,  abs_v_wheel_sideward;
    DATA forward_mu, sideward_mu;	/* friction coefficients	*/
    DATA beta_mu;			/* breakout friction slope	*/
    DATA forward_wheel_force, sideward_wheel_force;

    int i;				/* per wheel loop counter */
  
  /*
   * Execution starts here
   */
   
    beta_mu = max_mu/(skid_v-bkout_v);
    clear3( F_gear_v );		/* Initialize sum of forces...	*/
    clear3( M_gear_v );		/* ...and moments		*/
    
  /*
   * Put aircraft specific executable code here
   */
   
    percent_brake[1] = Brake_pct[0];
    percent_brake[2] = Brake_pct[1];
    
    caster_angle_rad[0] = 0.03*Rudder_pedal;
    
    for (i=0;i<num_wheels;i++)	    /* Loop for each wheel */
    {
	/*========================================*/
	/* Calculate wheel position w.r.t. runway */
	/*========================================*/
	
	    /* First calculate wheel location w.r.t. cg in body (X-Y-Z) axes... */
	
	sub3( d_wheel_rp_body_v[i], D_cg_rp_body_v, d_wheel_cg_body_v );
	
	    /* then converting to local (North-East-Down) axes... */
	
	multtrans3x3by3( T_local_to_body_m,  d_wheel_cg_body_v, d_wheel_cg_local_v );
	
	    /* Runway axes correction - third element is Altitude, not (-)Z... */
	
	d_wheel_cg_local_v[2] = -d_wheel_cg_local_v[2]; /* since altitude = -Z */
	
	    /* Add wheel offset to cg location in local axes */
	
	add3( d_wheel_cg_local_v, D_cg_rwy_local_v, d_wheel_rwy_local_v );
	
	    /* remove Runway axes correction so right hand rule applies */
	
	d_wheel_cg_local_v[2] = -d_wheel_cg_local_v[2]; /* now Z positive down */
	
	/*============================*/
	/* Calculate wheel velocities */
	/*============================*/
	
	    /* contribution due to angular rates */
	    
	cross3( Omega_body_v, d_wheel_cg_body_v, temp3a );
	
	    /* transform into local axes */
	  
	multtrans3x3by3( T_local_to_body_m, temp3a, temp3b );

	    /* plus contribution due to cg velocities */

	add3( temp3b, V_local_rel_ground_v, v_wheel_local_v );
	
	
	/*===========================================*/
	/* Calculate forces & moments for this wheel */
	/*===========================================*/
	
	    /* Add any anticipation, or frame lead/prediction, here... */
	    
		    /* no lead used at present */
		
	    /* Calculate sideward and forward velocities of the wheel 
		    in the runway plane					*/
	    
	cos_wheel_hdg_angle = cos(caster_angle_rad[i] + Psi);
	sin_wheel_hdg_angle = sin(caster_angle_rad[i] + Psi);
	
	v_wheel_forward  = v_wheel_local_v[0]*cos_wheel_hdg_angle
			 + v_wheel_local_v[1]*sin_wheel_hdg_angle;
	v_wheel_sideward = v_wheel_local_v[1]*cos_wheel_hdg_angle
			 - v_wheel_local_v[0]*sin_wheel_hdg_angle;

	    /* Calculate normal load force (simple spring constant) */
	
	reaction_normal_force = 0.;
	if( d_wheel_rwy_local_v[2] < 0. ) 
	{
	    reaction_normal_force = spring_constant[i]*d_wheel_rwy_local_v[2]
				  - v_wheel_local_v[2]*spring_damping[i];
	    if (reaction_normal_force > 0.) reaction_normal_force = 0.;
		/* to prevent damping component from swamping spring component */
	}
	
	    /* Calculate friction coefficients */
	    
	forward_mu = (max_brake_mu - rolling_mu)*percent_brake[i] + rolling_mu;
	abs_v_wheel_sideward = sqrt(v_wheel_sideward*v_wheel_sideward);
	sideward_mu = sliding_mu;
	if (abs_v_wheel_sideward < skid_v) 
	    sideward_mu = (abs_v_wheel_sideward - bkout_v)*beta_mu;
	if (abs_v_wheel_sideward < bkout_v) sideward_mu = 0.;

	    /* Calculate foreward and sideward reaction forces */
	    
	forward_wheel_force  =   forward_mu*reaction_normal_force;
	sideward_wheel_force =  sideward_mu*reaction_normal_force;
	if(v_wheel_forward < 0.) forward_wheel_force = -forward_wheel_force;
	if(v_wheel_sideward < 0.) sideward_wheel_force = -sideward_wheel_force;
	
	    /* Rotate into local (N-E-D) axes */
	
	f_wheel_local_v[0] = forward_wheel_force*cos_wheel_hdg_angle
			  - sideward_wheel_force*sin_wheel_hdg_angle;
	f_wheel_local_v[1] = forward_wheel_force*sin_wheel_hdg_angle
			  + sideward_wheel_force*cos_wheel_hdg_angle;
	f_wheel_local_v[2] = reaction_normal_force;	  
	   
	    /* Convert reaction force from local (N-E-D) axes to body (X-Y-Z) */
	
	mult3x3by3( T_local_to_body_m, f_wheel_local_v, tempF );
	
	    /* Calculate moments from force and offsets in body axes */

	cross3( d_wheel_cg_body_v, tempF, tempM );
	
	/* Sum forces and moments across all wheels */
	
	add3( tempF, F_gear_v, F_gear_v );
	add3( tempM, M_gear_v, M_gear_v );
	
    }
}
