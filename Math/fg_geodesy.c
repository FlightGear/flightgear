/**************************************************************************
 * fg_geodesy.c -- routines to convert between geodetic and geocentric 
 *                 coordinate systems.
 *
 * Copied and adapted directly from LaRCsim/ls_geodesy.c
 *
 * See below for the complete original LaRCsim comments.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include <math.h>

#include <Math/fg_geodesy.h>
#include <Include/fg_constants.h>


/* ONE_SECOND is pi/180/60/60, or about 100 feet at earths' equator */
#define ONE_SECOND 4.848136811E-6


/* fgGeocToGeod(lat_geoc, radius, *lat_geod, *alt, *sea_level_r)
 *     INPUTS:	
 *         lat_geoc	Geocentric latitude, radians, + = North
 *         radius	C.G. radius to earth center, ft
 *
 *     OUTPUTS:
 *         lat_geod	Geodetic latitude, radians, + = North
 *         alt		C.G. altitude above mean sea level, ft
 *         sea_level_r	radius from earth center to sea level at
 *                      local vertical (surface normal) of C.G.
 */

void fgGeocToGeod( double lat_geoc, double radius, double
		   *lat_geod, double *alt, double *sea_level_r )
{
    double t_lat, x_alpha, mu_alpha, delt_mu, r_alpha, l_point, rho_alpha;
    double sin_mu_a, denom,delt_lambda, lambda_sl, sin_lambda_sl;

    if( ( (FG_PI_2 - lat_geoc) < ONE_SECOND )     /* near North pole */
	|| ( (FG_PI_2 + lat_geoc) < ONE_SECOND ) )   /* near South pole */
    {
	*lat_geod = lat_geoc;
	*sea_level_r = EQUATORIAL_RADIUS_KM*E;
	*alt = radius - *sea_level_r;
    } else {
	t_lat = tan(lat_geoc);
	x_alpha = E*EQUATORIAL_RADIUS_KM/sqrt(t_lat*t_lat + E*E);
	mu_alpha = atan2(sqrt(RESQ_KM - x_alpha*x_alpha),E*x_alpha);
	if (lat_geoc < 0) mu_alpha = - mu_alpha;
	sin_mu_a = sin(mu_alpha);
	delt_lambda = mu_alpha - lat_geoc;
	r_alpha = x_alpha/cos(lat_geoc);
	l_point = radius - r_alpha;
	*alt = l_point*cos(delt_lambda);
	denom = sqrt(1-EPS*EPS*sin_mu_a*sin_mu_a);
	rho_alpha = EQUATORIAL_RADIUS_KM*(1-EPS)/
	    (denom*denom*denom);
	delt_mu = atan2(l_point*sin(delt_lambda),rho_alpha + *alt);
	*lat_geod = mu_alpha - delt_mu;
	lambda_sl = atan( E*E * tan(*lat_geod) ); /* SL geoc. latitude */
	sin_lambda_sl = sin( lambda_sl );
	*sea_level_r = 
	    sqrt(RESQ_KM / (1 + ((1/(E*E))-1)*sin_lambda_sl*sin_lambda_sl));
    }
}


/* fgGeodToGeoc( lat_geod, alt, *sl_radius, *lat_geoc )
 *     INPUTS:	
 *         lat_geod	Geodetic latitude, radians, + = North
 *         alt		C.G. altitude above mean sea level, ft
 *
 *     OUTPUTS:
 *         sl_radius	SEA LEVEL radius to earth center, ft (add Altitude to
 *                      get true distance from earth center.
 *         lat_geoc	Geocentric latitude, radians, + = North
 *
 */

void fgGeodToGeoc( double lat_geod, double alt, double *sl_radius,
		      double *lat_geoc )
{
    double lambda_sl, sin_lambda_sl, cos_lambda_sl, sin_mu, cos_mu, px, py;
    
    lambda_sl = atan( E*E * tan(lat_geod) ); /* sea level geocentric latitude */
    sin_lambda_sl = sin( lambda_sl );
    cos_lambda_sl = cos( lambda_sl );
    sin_mu = sin(lat_geod);	/* Geodetic (map makers') latitude */
    cos_mu = cos(lat_geod);
    *sl_radius = 
	sqrt(RESQ_KM / (1 + ((1/(E*E))-1)*sin_lambda_sl*sin_lambda_sl));
    py = *sl_radius*sin_lambda_sl + alt*sin_mu;
    px = *sl_radius*cos_lambda_sl + alt*cos_mu;
    *lat_geoc = atan2( py, px );
}


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
Revision 1.5  1998/04/25 22:06:23  curt
Edited cvs log messages in source files ... bad bad bad!

Revision 1.4  1998/01/27 00:47:59  curt
Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
system and commandline/config file processing code.

Revision 1.3  1998/01/19 19:27:12  curt
Merged in make system changes from Bob Kuehne <rpk@sgi.com>
This should simplify things tremendously.

Revision 1.2  1997/12/15 23:54:54  curt
Add xgl wrappers for debugging.
Generate terrain normals on the fly.

Revision 1.1  1997/07/31 23:13:14  curt
Initial revision.

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


/* $Log$
/* Revision 1.5  1998/04/25 22:06:23  curt
/* Edited cvs log messages in source files ... bad bad bad!
/*
 * Revision 1.4  1998/01/27 00:47:59  curt
 * Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.3  1998/01/19 19:27:12  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.2  1997/12/15 23:54:54  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.1  1997/07/31 23:13:14  curt
 * Initial revision.
 *
 */
