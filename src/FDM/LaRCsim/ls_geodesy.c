/***************************************************************************

	TITLE:	ls_geodesy
	
----------------------------------------------------------------------------

	FUNCTION:	Converts geocentric coordinates to geodetic positions

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Written as part of LaRCSim project by E. B. Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	E. B. Jackson
	
	CODED BY:	E. B. Jackson
	
	MAINTAINED BY:	E. B. Jackson

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY
	
	930208	Modified to avoid singularity near polar region.	EBJ
	930602	Moved backwards calcs here from ls_step.		EBJ
	931214	Changed erroneous Latitude and Altitude variables to 
		*lat_geod and *alt in routine ls_geoc_to_geod.		EBJ
	940111	Changed header files from old ls_eom.h style to ls_types, 
		and ls_constants.  Also replaced old DATA type with new
		SCALAR type.						EBJ

	CURRENT RCS HEADER:

$Header$
$Log$
Revision 1.1  2002/09/10 01:14:02  curt
Initial revision

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

Revision 1.1.1.1  1999/06/17 18:07:34  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.3  1998/07/08 14:41:37  curt
.

Revision 1.2  1998/01/19 18:40:25  curt
Tons of little changes to clean up the code and to remove fatal errors
when building with the c++ compiler.

Revision 1.1  1997/05/29 00:09:56  curt
Initial Flight Gear revision.

 * Revision 1.5  1994/01/11  18:47:05  bjax
 * Changed include files to use types and constants, not ls_eom.h
 * Also changed DATA type to SCALAR type.
 *
 * Revision 1.4  1993/12/14  21:06:47  bjax
 * Removed global variable references Altitude and Latitude.   EBJ
 *
 * Revision 1.3  1993/06/02  15:03:40  bjax
 * Made new subroutine for calculating geodetic to geocentric; changed name
 * of forward conversion routine from ls_geodesy to ls_geoc_to_geod.
 *

----------------------------------------------------------------------------

	REFERENCES:

		[ 1]	Stevens, Brian L.; and Lewis, Frank L.: "Aircraft 
			Control and Simulation", Wiley and Sons, 1992.
			ISBN 0-471-61397-5		      


----------------------------------------------------------------------------

	CALLED BY:	ls_aux

----------------------------------------------------------------------------

	CALLS TO:

----------------------------------------------------------------------------

	INPUTS:	
		lat_geoc	Geocentric latitude, radians, + = North
		radius		C.G. radius to earth center, ft

----------------------------------------------------------------------------

	OUTPUTS:
		lat_geod	Geodetic latitude, radians, + = North
		alt		C.G. altitude above mean sea level, ft
		sea_level_r	radius from earth center to sea level at
				local vertical (surface normal) of C.G.

--------------------------------------------------------------------------*/

#include "ls_types.h"
#include "ls_constants.h"
#include "ls_geodesy.h"
#include <math.h>

/* ONE_SECOND is pi/180/60/60, or about 100 feet at earths' equator */
#define ONE_SECOND 4.848136811E-6
#define HALF_PI 0.5*LS_PI


void ls_geoc_to_geod( SCALAR lat_geoc, SCALAR radius, SCALAR *lat_geod, 
		      SCALAR *alt, SCALAR *sea_level_r )
{
	SCALAR	t_lat, x_alpha, mu_alpha, delt_mu, r_alpha, l_point, rho_alpha;
	SCALAR	sin_mu_a, denom,delt_lambda, lambda_sl, sin_lambda_sl;

	if(   ( (HALF_PI - lat_geoc) < ONE_SECOND )     /* near North pole */
	   || ( (HALF_PI + lat_geoc) < ONE_SECOND ) )   /* near South pole */
	  {
	    *lat_geod = lat_geoc;
	    *sea_level_r = EQUATORIAL_RADIUS*E;
	    *alt = radius - *sea_level_r;
	  }
	else
	  {
	    t_lat = tan(lat_geoc);
	    x_alpha = E*EQUATORIAL_RADIUS/sqrt(t_lat*t_lat + E*E);
	    mu_alpha = atan2(sqrt(RESQ - x_alpha*x_alpha),E*x_alpha);
	    if (lat_geoc < 0) mu_alpha = - mu_alpha;
	    sin_mu_a = sin(mu_alpha);
	    delt_lambda = mu_alpha - lat_geoc;
	    r_alpha = x_alpha/cos(lat_geoc);
	    l_point = radius - r_alpha;
	    *alt = l_point*cos(delt_lambda);
	    denom = sqrt(1-EPS*EPS*sin_mu_a*sin_mu_a);
	    rho_alpha = EQUATORIAL_RADIUS*(1-EPS)/
	      (denom*denom*denom);
	    delt_mu = atan2(l_point*sin(delt_lambda),rho_alpha + *alt);
	    *lat_geod = mu_alpha - delt_mu;
	    lambda_sl = atan( E*E * tan(*lat_geod) ); /* SL geoc. latitude */
	    sin_lambda_sl = sin( lambda_sl );
	    *sea_level_r = sqrt(RESQ
			   /(1 + ((1/(E*E))-1)*sin_lambda_sl*sin_lambda_sl));
	  }
}


void ls_geod_to_geoc( SCALAR lat_geod, SCALAR alt, 
		      SCALAR *sl_radius, SCALAR *lat_geoc )
{
    SCALAR lambda_sl, sin_lambda_sl, cos_lambda_sl, sin_mu, cos_mu, px, py;
    
    lambda_sl = atan( E*E * tan(lat_geod) ); /* sea level geocentric latitude */
    sin_lambda_sl = sin( lambda_sl );
    cos_lambda_sl = cos( lambda_sl );
    sin_mu = sin(lat_geod);	/* Geodetic (map makers') latitude */
    cos_mu = cos(lat_geod);
    *sl_radius = sqrt(RESQ
	/(1 + ((1/(E*E))-1)*sin_lambda_sl*sin_lambda_sl));
    py = *sl_radius*sin_lambda_sl + alt*sin_mu;
    px = *sl_radius*cos_lambda_sl + alt*cos_mu;
    *lat_geoc = atan2( py, px );
}
