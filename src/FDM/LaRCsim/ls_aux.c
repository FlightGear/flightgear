/***************************************************************************

    TITLE:		ls_aux
		
----------------------------------------------------------------------------

    FUNCTION:	Atmospheric and auxilary relationships for LaRCSim EOM

----------------------------------------------------------------------------

    MODULE STATUS:	developmental

----------------------------------------------------------------------------

    GENEALOGY:	Created 9208026	as part of C-castle simulation project.

----------------------------------------------------------------------------

    DESIGNED BY:	B. Jackson
    
    CODED BY:		B. Jackson
    
    MAINTAINED BY:	B. Jackson

----------------------------------------------------------------------------

    MODIFICATION HISTORY:
    
    DATE    PURPOSE	
    
    931006  Moved calculations of auxiliary accelerations from here
	    to ls_accel.c and corrected minus sign in front of A_Y_Pilot
	    contribution from Q_body*P_body*D_X_pilot term.	    EBJ
    931014  Changed calculation of Alpha from atan to atan2 so sign is correct.
								    EBJ
    931220  Added calculations for static and total temperatures & pressures,
	    as well as dynamic and impact pressures and calibrated airspeed.
								    EBJ
    940111  Changed #included header files from old "ls_eom.h" to newer
	    "ls_types.h", "ls_constants.h" and "ls_generic.h".	    EBJ

    950207  Changed use of "abs" to "fabs" in calculation of signU. EBJ
    
    950228  Fixed bug in calculation of beta_dot.		    EBJ

    CURRENT RCS HEADER INFO:

$Header$
$Log$
Revision 1.4  2003/05/25 12:14:46  ehofman
Rename some defines to prevent a namespace clash

Revision 1.3  2003/05/13 18:45:06  curt
Robert Deters:

  I have attached some revisions for the UIUCModel and some LaRCsim.
  The only thing you should need to check is LaRCsim.cxx.  The file
  I attached is a revised version of 1.5 and the latest is 1.7.  Also,
  uiuc_getwind.c and uiuc_getwind.h are no longer in the LaRCsim
  directory.  They have been moved over to UIUCModel.

Revision 1.2  2003/03/31 03:05:39  m-selig
uiuc wind changes, MSS

Revision 1.1.1.1  2003/02/28 01:33:39  rob
uiuc version of FlightGear v0.9.0

Revision 1.2  2002/10/22 21:06:49  rob
*** empty log message ***

Revision 1.1.1.1  2002/04/24 17:08:23  rob
UIUC version of FlightGear-0.7.pre11

Revision 1.3  2001/03/24 05:03:12  curt
SG-ified logstream.

Revision 1.2  2000/10/23 22:34:54  curt
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

Revision 1.1.1.1  1999/06/17 18:07:33  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.4  1998/08/24 20:09:26  curt
Code optimization tweaks from Norman Vine.

Revision 1.3  1998/08/06 12:46:38  curt
Header change.

Revision 1.2  1998/01/19 18:40:24  curt
Tons of little changes to clean up the code and to remove fatal errors
when building with the c++ compiler.

Revision 1.1  1997/05/29 00:09:54  curt
Initial Flight Gear revision.

 * Revision 1.12  1995/02/28  17:57:16  bjax
 * Corrected calculation of beta_dot. EBJ
 *
 * Revision 1.11  1995/02/07  21:09:47  bjax
 * Corrected calculation of "signU"; was using divide by
 * abs(), which returns an integer; now using fabs(), which
 * returns a double.  EBJ
 *
 * Revision 1.10  1994/05/10  20:09:42  bjax
 * Fixed a major problem with dx_pilot_from_cg, etc. not being calculated locally.
 *
 * Revision 1.9  1994/01/11  18:44:33  bjax
 * Changed header files to use ls_types, ls_constants, and ls_generic.
 *
 * Revision 1.8  1993/12/21  14:36:33  bjax
 * Added calcs of pressures, temps and calibrated airspeeds.
 *
 * Revision 1.7  1993/10/14  11:25:38  bjax
 * Changed calculation of Alpha to use 'atan2' instead of 'atan' so alphas
 * larger than +/- 90 degrees are calculated correctly.			EBJ
 *
 * Revision 1.6  1993/10/07  18:45:56  bjax
 * A little cleanup; no significant changes. EBJ
 *
 * Revision 1.5  1993/10/07  18:42:22  bjax
 * Moved calculations of auxiliary accelerations here from ls_aux, and
 * corrected sign on Q_body*P_body*d_x_pilot term of A_Y_pilot calc.  EBJ
 *
 * Revision 1.4  1993/07/16  18:28:58  bjax
 * Changed call from atmos_62 to ls_atmos. EBJ
 *
 * Revision 1.3  1993/06/02  15:02:42  bjax
 * Changed call to geodesy calcs from ls_geodesy to ls_geoc_to_geod.
 *
 * Revision 1.1  92/12/30  13:17:39  bjax
 * Initial revision
 * 


----------------------------------------------------------------------------

    REFERENCES:	[ 1] Shapiro, Ascher H.: "The Dynamics and Thermodynamics
			of Compressible Fluid Flow", Volume I, The Ronald 
			Press, 1953.

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

#include "ls_aux.h"

#include "atmos_62.h"
#include "ls_geodesy.h"
#include "ls_gravity.h"

#include <math.h>

void ls_aux( void ) {

	SCALAR	dx_pilot_from_cg, dy_pilot_from_cg, dz_pilot_from_cg;
	/* SCALAR inv_Mass; */
	SCALAR	v_XZ_plane_2, signU, v_tangential;
	/* SCALAR inv_radius_ratio; */
	SCALAR	cos_rwy_hdg, sin_rwy_hdg;
	SCALAR	mach2, temp_ratio, pres_ratio;
	SCALAR  tmp;
	
    /* update geodetic position */

	ls_geoc_to_geod( Lat_geocentric, Radius_to_vehicle, 
				&Latitude, &Altitude, &Sea_level_radius );
	Longitude = Lon_geocentric - Earth_position_angle;

    /* Calculate body axis velocities */

	/* Form relative velocity vector */

	V_north_rel_ground = V_north;
	V_east_rel_ground  = V_east 
	  - OMEGA_EARTH*Sea_level_radius*cos( Lat_geocentric );
	V_down_rel_ground  = V_down;

	V_north_rel_airmass = V_north_rel_ground - V_north_airmass;
	V_east_rel_airmass  = V_east_rel_ground  - V_east_airmass;
	V_down_rel_airmass  = V_down_rel_ground  - V_down_airmass;
	
	U_body = T_local_to_body_11*V_north_rel_airmass 
	  + T_local_to_body_12*V_east_rel_airmass
	    + T_local_to_body_13*V_down_rel_airmass + U_gust;
	V_body = T_local_to_body_21*V_north_rel_airmass 
	  + T_local_to_body_22*V_east_rel_airmass
	    + T_local_to_body_23*V_down_rel_airmass + V_gust;
	W_body = T_local_to_body_31*V_north_rel_airmass 
	  + T_local_to_body_32*V_east_rel_airmass
	    + T_local_to_body_33*V_down_rel_airmass + W_gust;
				
	V_rel_wind = sqrt(U_body*U_body + V_body*V_body + W_body*W_body);


    /* Calculate alpha and beta rates	*/

	v_XZ_plane_2 = (U_body*U_body + W_body*W_body);
	
	if (U_body == 0)
		signU = 1;
	else
		signU = U_body/fabs(U_body);
		
	if( (v_XZ_plane_2 == 0) || (V_rel_wind == 0) )
	{
		Std_Alpha_dot = 0;
		Std_Beta_dot = 0;
	}
	else
	{
		Std_Alpha_dot = (U_body*W_dot_body - W_body*U_dot_body)/
		  v_XZ_plane_2;
		Std_Beta_dot = (signU*v_XZ_plane_2*V_dot_body 
		  - V_body*(U_body*U_dot_body + W_body*W_dot_body))
		    /(V_rel_wind*V_rel_wind*sqrt(v_XZ_plane_2));
	}

    /* Calculate flight path and other flight condition values */

	if (U_body == 0) 
		Std_Alpha = 0;
	else
		Std_Alpha = atan2( W_body, U_body );
		
	Cos_alpha = cos(Std_Alpha);
	Sin_alpha = sin(Std_Alpha);
	
	if (V_rel_wind == 0)
		Std_Beta = 0;
	else
		Std_Beta = asin( V_body/ V_rel_wind );
		
	Cos_beta = cos(Std_Beta);
	Sin_beta = sin(Std_Beta);
	
	V_true_kts = V_rel_wind * V_TO_KNOTS;
	
	V_ground_speed = sqrt(V_north_rel_ground*V_north_rel_ground
			      + V_east_rel_ground*V_east_rel_ground );

	V_rel_ground = sqrt(V_ground_speed*V_ground_speed
			    + V_down_rel_ground*V_down_rel_ground );
	
	v_tangential = sqrt(V_north*V_north + V_east*V_east);
	
	V_inertial = sqrt(v_tangential*v_tangential + V_down*V_down);
	
	if( (V_ground_speed == 0) && (V_down == 0) )
	  Gamma_vert_rad = 0;
	else
	  Gamma_vert_rad = atan2( -V_down, V_ground_speed );
		
	if( (V_north_rel_ground == 0) && (V_east_rel_ground == 0) )
	  Gamma_horiz_rad = 0;
	else
	  Gamma_horiz_rad = atan2( V_east_rel_ground, V_north_rel_ground );
	
	if (Gamma_horiz_rad < 0) 
	  Gamma_horiz_rad = Gamma_horiz_rad + 2*LS_PI;
	
    /* Calculate local gravity	*/
	
	ls_gravity( Radius_to_vehicle, Lat_geocentric, &Gravity );
	
    /* call function for (smoothed) density ratio, sonic velocity, and
	   ambient pressure */

	ls_atmos(Altitude, &Sigma, &V_sound, 
		 &Static_temperature, &Static_pressure);
	
	Density = Sigma*SEA_LEVEL_DENSITY;
	
	Mach_number = V_rel_wind / V_sound;
	
	V_equiv	= V_rel_wind*sqrt(Sigma);
	
	V_equiv_kts = V_equiv*V_TO_KNOTS;

    /* calculate temperature and pressure ratios (from [1]) */

	mach2 = Mach_number*Mach_number;
	temp_ratio = 1.0 + 0.2*mach2; 
	tmp = 3.5;
	pres_ratio = pow( temp_ratio, tmp );

	Total_temperature = temp_ratio*Static_temperature;
	Total_pressure    = pres_ratio*Static_pressure;

    /* calculate impact and dynamic pressures */
	
	Impact_pressure = Total_pressure - Static_pressure; 

	Dynamic_pressure = 0.5*Density*V_rel_wind*V_rel_wind;

    /* calculate calibrated airspeed indication */

	V_calibrated = sqrt( 2.0*Dynamic_pressure / SEA_LEVEL_DENSITY );
	V_calibrated_kts = V_calibrated*V_TO_KNOTS;
	
	Centrifugal_relief = 1 - v_tangential/(Radius_to_vehicle*Gravity);
	
/* Determine location in runway coordinates */

	Radius_to_rwy = Sea_level_radius + Runway_altitude;
	cos_rwy_hdg = cos(Runway_heading*DEG_TO_RAD);
	sin_rwy_hdg = sin(Runway_heading*DEG_TO_RAD);
	
	D_cg_north_of_rwy = Radius_to_rwy*(Latitude - Runway_latitude);
	D_cg_east_of_rwy = Radius_to_rwy*cos(Runway_latitude)
		*(Longitude - Runway_longitude);
	D_cg_above_rwy	= Radius_to_vehicle - Radius_to_rwy;
	
	X_cg_rwy = D_cg_north_of_rwy*cos_rwy_hdg 
	  + D_cg_east_of_rwy*sin_rwy_hdg;
	Y_cg_rwy =-D_cg_north_of_rwy*sin_rwy_hdg 
	  + D_cg_east_of_rwy*cos_rwy_hdg;
	H_cg_rwy = D_cg_above_rwy;
	
	dx_pilot_from_cg = Dx_pilot - Dx_cg;
	dy_pilot_from_cg = Dy_pilot - Dy_cg;
	dz_pilot_from_cg = Dz_pilot - Dz_cg;

	D_pilot_north_of_rwy = D_cg_north_of_rwy 
	  + T_local_to_body_11*dx_pilot_from_cg 
	    + T_local_to_body_21*dy_pilot_from_cg
	      + T_local_to_body_31*dz_pilot_from_cg;
	D_pilot_east_of_rwy  = D_cg_east_of_rwy 
	  + T_local_to_body_12*dx_pilot_from_cg 
	    + T_local_to_body_22*dy_pilot_from_cg
	      + T_local_to_body_32*dz_pilot_from_cg;
	D_pilot_above_rwy    = D_cg_above_rwy 
	  - T_local_to_body_13*dx_pilot_from_cg 
	    - T_local_to_body_23*dy_pilot_from_cg
	      - T_local_to_body_33*dz_pilot_from_cg;
							
	X_pilot_rwy =  D_pilot_north_of_rwy*cos_rwy_hdg
	  + D_pilot_east_of_rwy*sin_rwy_hdg;
	Y_pilot_rwy = -D_pilot_north_of_rwy*sin_rwy_hdg
	  + D_pilot_east_of_rwy*cos_rwy_hdg;
	H_pilot_rwy =  D_pilot_above_rwy;
								
/* Calculate Euler rates */
	
	Sin_phi	= sin(Phi);
	Cos_phi = cos(Phi);
	Sin_theta = sin(Theta);
	Cos_theta = cos(Theta);
	Sin_psi = sin(Psi);
	Cos_psi = cos(Psi);
	
	if( Cos_theta == 0 )
	  Psi_dot = 0;
	else
	  Psi_dot = (Q_total*Sin_phi + R_total*Cos_phi)/Cos_theta;
	
	Theta_dot = Q_total*Cos_phi - R_total*Sin_phi;
	Phi_dot = P_total + Psi_dot*Sin_theta;
	
/* end of ls_aux */

}
/*************************************************************************/
