/***************************************************************************

	TITLE:	ls_step
	
----------------------------------------------------------------------------

	FUNCTION:	Integration routine for equations of motion 
			(vehicle states)

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Written 920802 by Bruce Jackson.  Based upon equations
			given in reference [1] and a Matrix-X/System Build block
			diagram model of equations of motion coded by David Raney
			at NASA-Langley in June of 1992.

----------------------------------------------------------------------------

	DESIGNED BY:	Bruce Jackson
	
	CODED BY:	Bruce Jackson
	
	MAINTAINED BY:	

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY

	921223  Modified calculation of Phi and Psi to use the "atan2" routine
	        rather than the "atan" to allow full circular angles.
		"atan" limits to +/- pi/2.                      EBJ
		
	940111	Changed from oldstyle include file ls_eom.h; also changed
		from DATA to SCALAR type.			EBJ

	950207  Initialized Alpha_dot and Beta_dot to zero on first pass; calculated
		thereafter.					EBJ

	950224	Added logic to avoid adding additional increment to V_east
		in case V_east already accounts for rotating earth. 
								EBJ

	CURRENT RCS HEADER:

$Header$
$Log$
Revision 1.1  1997/05/29 00:09:59  curt
Initial Flight Gear revision.

 * Revision 1.5  1995/03/02  20:24:13  bjax
 * Added logic to avoid adding additional increment to V_east
 * in case V_east already accounts for rotating earth. EBJ
 *
 * Revision 1.4  1995/02/07  20:52:21  bjax
 * Added initialization of Alpha_dot and Beta_dot to zero on first
 * pass; they get calculated by ls_aux on next pass...  EBJ
 *
 * Revision 1.3  1994/01/11  19:01:12  bjax
 * Changed from DATA to SCALAR type; also fixed header files (was ls_eom.h)
 *
 * Revision 1.2  1993/06/02  15:03:09  bjax
 * Moved initialization of geocentric position to subroutine ls_geod_to_geoc.
 *
 * Revision 1.1  92/12/30  13:16:11  bjax
 * Initial revision
 * 

----------------------------------------------------------------------------

	REFERENCES:
	
		[ 1]	McFarland, Richard E.: "A Standard Kinematic Model
			for Flight Simulation at NASA-Ames", NASA CR-2497,
			January 1975
			 
		[ 2]	ANSI/AIAA R-004-1992 "Recommended Practice: Atmos-
			pheric and Space Flight Vehicle Coordinate Systems",
			February 1992
			

----------------------------------------------------------------------------

	CALLED BY:

----------------------------------------------------------------------------

	CALLS TO:	None.

----------------------------------------------------------------------------

	INPUTS:	State derivatives

----------------------------------------------------------------------------

	OUTPUTS:	States

--------------------------------------------------------------------------*/

#include "ls_types.h"
#include "ls_constants.h"
#include "ls_generic.h"
/* #include "ls_sim_control.h" */
#include <math.h>

extern SCALAR Simtime;		/* defined in ls_main.c */

void ls_step( dt, Initialize )

SCALAR dt;
int Initialize;

{
	static	int	inited = 0;
		SCALAR	dth;
	static	SCALAR	v_dot_north_past, v_dot_east_past, v_dot_down_past;
	static	SCALAR	latitude_dot_past, longitude_dot_past, radius_dot_past;
	static	SCALAR	p_dot_body_past, q_dot_body_past, r_dot_body_past;
		SCALAR	p_local_in_body, q_local_in_body, r_local_in_body;
		SCALAR	epsilon, inv_eps, local_gnd_veast;
		SCALAR	e_dot_0, e_dot_1, e_dot_2, e_dot_3;
	static	SCALAR	e_0, e_1, e_2, e_3;
	static	SCALAR	e_dot_0_past, e_dot_1_past, e_dot_2_past, e_dot_3_past;

/*  I N I T I A L I Z A T I O N   */


	if ( (inited == 0) || (Initialize != 0) )
	{
/* Set past values to zero */
	v_dot_north_past = v_dot_east_past = v_dot_down_past      = 0;
	latitude_dot_past = longitude_dot_past = radius_dot_past  = 0;
	p_dot_body_past = q_dot_body_past = r_dot_body_past       = 0;
	e_dot_0_past = e_dot_1_past = e_dot_2_past = e_dot_3_past = 0;
	
/* Initialize geocentric position from geodetic latitude and altitude */
	
	ls_geod_to_geoc( Latitude, Altitude, &Sea_level_radius, &Lat_geocentric);
	Earth_position_angle = 0;
	Lon_geocentric = Longitude;
	Radius_to_vehicle = Altitude + Sea_level_radius;

/* Correct eastward velocity to account for earths' rotation, if necessary */

	local_gnd_veast = OMEGA_EARTH*Sea_level_radius*cos(Lat_geocentric);
	if( fabs(V_east - V_east_rel_ground) < 0.8*local_gnd_veast )
    	V_east = V_east + local_gnd_veast;

/* Initialize quaternions and transformation matrix from Euler angles */

	    e_0 = cos(Psi*0.5)*cos(Theta*0.5)*cos(Phi*0.5) 
		+ sin(Psi*0.5)*sin(Theta*0.5)*sin(Phi*0.5);
	    e_1 = cos(Psi*0.5)*cos(Theta*0.5)*sin(Phi*0.5) 
		- sin(Psi*0.5)*sin(Theta*0.5)*cos(Phi*0.5);
	    e_2 = cos(Psi*0.5)*sin(Theta*0.5)*cos(Phi*0.5) 
		+ sin(Psi*0.5)*cos(Theta*0.5)*sin(Phi*0.5);
	    e_3 =-cos(Psi*0.5)*sin(Theta*0.5)*sin(Phi*0.5) 
		+ sin(Psi*0.5)*cos(Theta*0.5)*cos(Phi*0.5);
	    T_local_to_body_11 = e_0*e_0 + e_1*e_1 - e_2*e_2 - e_3*e_3;
	    T_local_to_body_12 = 2*(e_1*e_2 + e_0*e_3);
	    T_local_to_body_13 = 2*(e_1*e_3 - e_0*e_2);
	    T_local_to_body_21 = 2*(e_1*e_2 - e_0*e_3);
	    T_local_to_body_22 = e_0*e_0 - e_1*e_1 + e_2*e_2 - e_3*e_3;
	    T_local_to_body_23 = 2*(e_2*e_3 + e_0*e_1);
	    T_local_to_body_31 = 2*(e_1*e_3 + e_0*e_2);
	    T_local_to_body_32 = 2*(e_2*e_3 - e_0*e_1);
	    T_local_to_body_33 = e_0*e_0 - e_1*e_1 - e_2*e_2 + e_3*e_3;

/*	Calculate local gravitation acceleration	*/

		ls_gravity( Radius_to_vehicle, Lat_geocentric, &Gravity );

/*	Initialize vehicle model 			*/

		ls_aux();
		ls_model();

/* 	Calculate initial accelerations */

		ls_accel();
		
/* Initialize auxiliary variables */

		ls_aux();
		Alpha_dot = 0.;
		Beta_dot = 0.;

/* set flag; disable integrators */

		inited = -1;
		dt = 0;
		
	}

/* Update time */

	dth = 0.5*dt;
	Simtime = Simtime + dt;

/*  L I N E A R   V E L O C I T I E S   */

/* Integrate linear accelerations to get velocities */
/*    Using predictive Adams-Bashford algorithm     */

    V_north = V_north + dth*(3*V_dot_north - v_dot_north_past);
    V_east  = V_east  + dth*(3*V_dot_east  - v_dot_east_past );
    V_down  = V_down  + dth*(3*V_dot_down  - v_dot_down_past );
    
/* record past states */

    v_dot_north_past = V_dot_north;
    v_dot_east_past  = V_dot_east;
    v_dot_down_past  = V_dot_down;
    
/* Calculate trajectory rate (geocentric coordinates) */

    if (cos(Lat_geocentric) != 0)
    	Longitude_dot = V_east/(Radius_to_vehicle*cos(Lat_geocentric));
    	
    Latitude_dot = V_north/Radius_to_vehicle;
    Radius_dot = -V_down;
    	
/*  A N G U L A R   V E L O C I T I E S   A N D   P O S I T I O N S  */
    
/* Integrate rotational accelerations to get velocities */

    P_body = P_body + dth*(3*P_dot_body - p_dot_body_past);
    Q_body = Q_body + dth*(3*Q_dot_body - q_dot_body_past);
    R_body = R_body + dth*(3*R_dot_body - r_dot_body_past);

/* Save past states */

    p_dot_body_past = P_dot_body;
    q_dot_body_past = Q_dot_body;
    r_dot_body_past = R_dot_body;
    
/* Calculate local axis frame rates due to travel over curved earth */

    P_local =  V_east/Radius_to_vehicle;
    Q_local = -V_north/Radius_to_vehicle;
    R_local = -V_east*tan(Lat_geocentric)/Radius_to_vehicle;
    
/* Transform local axis frame rates to body axis rates */

    p_local_in_body = T_local_to_body_11*P_local + T_local_to_body_12*Q_local + T_local_to_body_13*R_local;
    q_local_in_body = T_local_to_body_21*P_local + T_local_to_body_22*Q_local + T_local_to_body_23*R_local;
    r_local_in_body = T_local_to_body_31*P_local + T_local_to_body_32*Q_local + T_local_to_body_33*R_local;
    
/* Calculate total angular rates in body axis */

    P_total = P_body - p_local_in_body;
    Q_total = Q_body - q_local_in_body;
    R_total = R_body - r_local_in_body;
    
/* Transform to quaternion rates (see Appendix E in [2]) */

    e_dot_0 = 0.5*( -P_total*e_1 - Q_total*e_2 - R_total*e_3 );
    e_dot_1 = 0.5*(  P_total*e_0 - Q_total*e_3 + R_total*e_2 );
    e_dot_2 = 0.5*(  P_total*e_3 + Q_total*e_0 - R_total*e_1 );
    e_dot_3 = 0.5*( -P_total*e_2 + Q_total*e_1 + R_total*e_0 );

/* Integrate using trapezoidal as before */

	e_0 = e_0 + dth*(e_dot_0 + e_dot_0_past);
	e_1 = e_1 + dth*(e_dot_1 + e_dot_1_past);
	e_2 = e_2 + dth*(e_dot_2 + e_dot_2_past);
	e_3 = e_3 + dth*(e_dot_3 + e_dot_3_past);
	
/* calculate orthagonality correction  - scale quaternion to unity length */
	
	epsilon = sqrt(e_0*e_0 + e_1*e_1 + e_2*e_2 + e_3*e_3);
	inv_eps = 1/epsilon;
	
	e_0 = inv_eps*e_0;
	e_1 = inv_eps*e_1;
	e_2 = inv_eps*e_2;
	e_3 = inv_eps*e_3;

/* Save past values */

	e_dot_0_past = e_dot_0;
	e_dot_1_past = e_dot_1;
	e_dot_2_past = e_dot_2;
	e_dot_3_past = e_dot_3;
	
/* Update local to body transformation matrix */

	T_local_to_body_11 = e_0*e_0 + e_1*e_1 - e_2*e_2 - e_3*e_3;
	T_local_to_body_12 = 2*(e_1*e_2 + e_0*e_3);
	T_local_to_body_13 = 2*(e_1*e_3 - e_0*e_2);
	T_local_to_body_21 = 2*(e_1*e_2 - e_0*e_3);
	T_local_to_body_22 = e_0*e_0 - e_1*e_1 + e_2*e_2 - e_3*e_3;
	T_local_to_body_23 = 2*(e_2*e_3 + e_0*e_1);
	T_local_to_body_31 = 2*(e_1*e_3 + e_0*e_2);
	T_local_to_body_32 = 2*(e_2*e_3 - e_0*e_1);
	T_local_to_body_33 = e_0*e_0 - e_1*e_1 - e_2*e_2 + e_3*e_3;
	
/* Calculate Euler angles */

	Theta = asin( -T_local_to_body_13 );

	if( T_local_to_body_11 == 0 )
	Psi = 0;
	else
	Psi = atan2( T_local_to_body_12, T_local_to_body_11 );

	if( T_local_to_body_33 == 0 )
	Phi = 0;
	else
	Phi = atan2( T_local_to_body_23, T_local_to_body_33 );

/* Resolve Psi to 0 - 359.9999 */

	if (Psi < 0 ) Psi = Psi + 2*PI;

/*  L I N E A R   P O S I T I O N S   */

/* Trapezoidal acceleration for position */

	Lat_geocentric    = Lat_geocentric    + dth*(Latitude_dot  + latitude_dot_past );
	Lon_geocentric    = Lon_geocentric    + dth*(Longitude_dot + longitude_dot_past);
	Radius_to_vehicle = Radius_to_vehicle + dth*(Radius_dot    + radius_dot_past );
	Earth_position_angle = Earth_position_angle + dt*OMEGA_EARTH;
	
/* Save past values */

	latitude_dot_past  = Latitude_dot;
	longitude_dot_past = Longitude_dot;
	radius_dot_past    = Radius_dot;
	
/* end of ls_step */
}
/*************************************************************************/
	
