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
Revision 1.5  2003/07/25 17:53:47  mselig
UIUC code initilization mods to tidy things up a bit.

Revision 1.4  2003/06/20 19:53:56  ehofman
Get rid of a multiple defined symbol warning" src/FDM/LaRCsim/ls_step.c
"

Revision 1.3  2003/06/09 02:50:23  mselig
mods made to setup for some initializations in uiuc code

Revision 1.2  2003/05/25 12:14:46  ehofman
Rename some defines to prevent a namespace clash

Revision 1.1.1.1  2002/09/10 01:14:02  curt
Initial revision of FlightGear-0.9.0

Revision 1.5  2001/09/14 18:47:27  curt
More changes in support of UIUCModel.

Revision 1.4  2001/03/24 05:03:12  curt
SG-ified logstream.

Revision 1.3  2000/10/23 22:34:55  curt
I tested:
LaRCsim c172 on-ground and in-air starts, reset: all work
UIUC Cessna172 on-ground and in-air starts work as expected, reset
results in an aircraft that is upside down but does not crash FG.   I
don't know what it was like before, so it may well be no change.
JSBSim c172 and X15 in-air starts work fine, resets now work (and are
trimmed), on-ground starts do not -- the c172 ends up on its back.  I
suspect this is no worse than before.

I did not test:
Balloon (the weather code returns nan's for the atmosphere data --this
is in the weather module and apparently is a linux only bug)
ADA (don't know how)
MagicCarpet  (needs work yet)
External (don't know how)

known to be broken:
LaRCsim c172 on-ground starts with a negative terrain altitude (this
happens at KPAO when the scenery is not present).   The FDM inits to
about 50 feet AGL and the model falls to the ground.  It does stay
upright, however, and seems to be fine once it settles out, FWIW.

To do:
--implement set_Model on the bus
--bring Christian's weather data into JSBSim
-- add default method to bus for updating things like the sin and cos of
latitude (for Balloon, MagicCarpet)
-- lots of cleanup

The files:
src/FDM/flight.cxx
src/FDM/flight.hxx
-- all data members now declared protected instead of private.
-- eliminated all but a small set of 'setters', no change to getters.
-- that small set is declared virtual, the default implementation
provided preserves the old behavior
-- all of the vector data members are now initialized.
-- added busdump() method -- SG_LOG's  all the bus data when called,
useful for diagnostics.

src/FDM/ADA.cxx
-- bus data members now directly assigned to

src/FDM/Balloon.cxx
-- bus data members now directly assigned to
-- changed V_equiv_kts to V_calibrated_kts

src/FDM/JSBSim.cxx
src/FDM/JSBSim.hxx
-- bus data members now directly assigned to
-- implemented the FGInterface virtual setters with JSBSim specific
logic
-- changed the static FDMExec to a dynamic fdmex (needed so that the
JSBSim object can be deleted when a model change is called for)
-- implemented constructor and destructor, moved some of the logic
formerly in init() to constructor
-- added logic to bring up FGEngInterface objects and set the RPM and
throttle values.

src/FDM/LaRCsim.cxx
src/FDM/LaRCsim.hxx
-- bus data members now directly assigned to
-- implemented the FGInterface virtual setters with LaRCsim specific
logic, uses LaRCsimIC
-- implemented constructor and destructor, moved some of the logic
formerly in init() to constructor
-- moved default inertias to here from fg_init.cxx
-- eliminated the climb rate calculation.  The equivalent, climb_rate =
-1*vdown, is now in copy_from_LaRCsim().

src/FDM/LaRCsimIC.cxx
src/FDM/LaRCsimIC.hxx
-- similar to FGInitialCondition, this class has all the logic needed to
turn data like Vc and Mach into the more fundamental quantities LaRCsim
needs to initialize.
-- put it in src/FDM since it is a class

src/FDM/MagicCarpet.cxx
 -- bus data members now directly assigned to

src/FDM/Makefile.am
-- adds LaRCsimIC.hxx and cxx

src/FDM/JSBSim/FGAtmosphere.h
src/FDM/JSBSim/FGDefs.h
src/FDM/JSBSim/FGInitialCondition.cpp
src/FDM/JSBSim/FGInitialCondition.h
src/FDM/JSBSim/JSBSim.cpp
-- changes to accomodate the new bus

src/FDM/LaRCsim/atmos_62.h
src/FDM/LaRCsim/ls_geodesy.h
-- surrounded prototypes with #ifdef __cplusplus ... #endif , functions
here are needed in LaRCsimIC

src/FDM/LaRCsim/c172_main.c
src/FDM/LaRCsim/cherokee_aero.c
src/FDM/LaRCsim/ls_aux.c
src/FDM/LaRCsim/ls_constants.h
src/FDM/LaRCsim/ls_geodesy.c
src/FDM/LaRCsim/ls_geodesy.h
src/FDM/LaRCsim/ls_step.c
src/FDM/UIUCModel/uiuc_betaprobe.cpp
-- changed PI to LS_PI, eliminates preprocessor naming conflict with
weather module

src/FDM/LaRCsim/ls_interface.c
src/FDM/LaRCsim/ls_interface.h
-- added function ls_set_model_dt()

src/Main/bfi.cxx
-- eliminated calls that set the NED speeds to body components.  They
are no longer needed and confuse the new bus.

src/Main/fg_init.cxx
-- eliminated calls that just brought the bus data up-to-date (e.g.
set_sin_cos_latitude). or set default values.   The bus now handles the
defaults and updates itself when the setters are called (for LaRCsim and
JSBSim).  A default method for doing this needs to be added to the bus.
-- added fgVelocityInit() to set the speed the user asked for.  Both
JSBSim and LaRCsim can now be initialized using any of:
vc,mach, NED components, UVW components.

src/Main/main.cxx
--eliminated call to fgFDMSetGroundElevation, this data is now 'pulled'
onto the bus every update()

src/Main/options.cxx
src/Main/options.hxx
-- added enum to keep track of the speed requested by the user
-- eliminated calls to set NED velocity properties to body speeds, they
are no longer needed.
-- added options for the NED components.

src/Network/garmin.cxx
src/Network/nmea.cxx
--eliminated calls that just brought the bus data up-to-date (e.g.
set_sin_cos_latitude).  The bus now updates itself when the setters are
called (for LaRCsim and JSBSim).  A default method for doing this needs
to be added to the bus.
-- changed set_V_equiv_kts to set_V_calibrated_kts.  set_V_equiv_kts no
longer exists ( get_V_equiv_kts still does, though)

src/WeatherCM/FGLocalWeatherDatabase.cpp
-- commented out the code to put the weather data on the bus, a
different scheme for this is needed.

Revision 1.2  1999/10/29 16:08:33  curt
Added flaps support to c172 model.

Revision 1.1.1.1  1999/06/17 18:07:33  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.4  1998/08/24 20:09:27  curt
Code optimization tweaks from Norman Vine.

Revision 1.3  1998/07/12 03:11:04  curt
Removed some printf()'s.
Fixed the autopilot integration so it should be able to update it's control
  positions every time the internal flight model loop is run, and not just
  once per rendered frame.
Added a routine to do the necessary stuff to force an arbitrary altitude
  change.
Gave the Navion engine just a tad more power.

Revision 1.2  1998/01/19 18:40:28  curt
Tons of little changes to clean up the code and to remove fatal errors
when building with the c++ compiler.

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

//#include <FDM/UIUCModel/uiuc_wrapper.h>

#include "ls_types.h"
#include "ls_constants.h"
#include "ls_generic.h"
#include "ls_accel.h"
#include "ls_aux.h"
#include "ls_model.h"
#include "ls_step.h"
#include "ls_geodesy.h"
#include "ls_gravity.h"
/* #include "ls_sim_control.h" */
#include <math.h>

extern Model current_model;	/* defined in ls_model.c */
extern SCALAR Simtime;		/* defined in ls_main.c */

void uiuc_init_vars() {
    static int init = 0;

    if (init==0) {
        init=-1;
        uiuc_init_aeromodel();
    }

    uiuc_initial_init();
}


void ls_step( SCALAR dt, int Initialize ) {
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
	        SCALAR  cos_Lat_geocentric, inv_Radius_to_vehicle;

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
	// Initialize UIUC aircraft model
	if (current_model == UIUC) {
	  uiuc_init_2_wrapper();
        }

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

	    // Initialize local velocities (V_north, V_east, V_down)
	    // based on transformation matrix calculated above
	    if (current_model == UIUC) {
	      uiuc_local_vel_init();
	    }

/*	Calculate local gravitation acceleration	*/

		ls_gravity( Radius_to_vehicle, Lat_geocentric, &Gravity );

/*	Initialize vehicle model 			*/

		ls_aux();
		ls_model(0.0, 0);

/* 	Calculate initial accelerations */

		ls_accel();
		
/* Initialize auxiliary variables */

		ls_aux();
		Std_Alpha_dot = 0.;
		Std_Beta_dot = 0.;

/* set flag; disable integrators */

		inited = -1;
		dt = 0.0;
		
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

    inv_Radius_to_vehicle = 1.0/Radius_to_vehicle;
    cos_Lat_geocentric = cos(Lat_geocentric);

    if ( cos_Lat_geocentric != 0) {
    	Longitude_dot = V_east/(Radius_to_vehicle*cos_Lat_geocentric);
    }
    	
    Latitude_dot = V_north*inv_Radius_to_vehicle;
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

    P_local =  V_east*inv_Radius_to_vehicle;
    Q_local = -V_north*inv_Radius_to_vehicle;
    R_local = -V_east*tan(Lat_geocentric)*inv_Radius_to_vehicle;
    
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

	if (Psi < 0 ) Psi = Psi + 2*LS_PI;

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
	
