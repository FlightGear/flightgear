/**************************************************************************
 * fg_geodesy.h -- routines to convert between geodetic and geocentric 
 *                 coordinate systems.
 *
 * Copied and adapted directly from LaRCsim/ls_geodesy.c
 *
 * See below for the complete original LaRCsim comments.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef FG_GEODESY_H
#define FG_GEODESY_H


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
		   *lat_geod, double *alt, double *sea_level_r );

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
		   double *lat_geoc );



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


#endif /* FG_GEODESY_H */


/* $Log$
/* Revision 1.1  1997/07/31 23:13:14  curt
/* Initial revision.
/*
 */
