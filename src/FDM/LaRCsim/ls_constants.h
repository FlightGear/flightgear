/***************************************************************************

	TITLE:	ls_constants.h
	
----------------------------------------------------------------------------

	FUNCTION:	LaRCSim constants definition header file

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Created 15 DEC 1993 by Bruce Jackson; was part of
			old ls_eom.h header file

----------------------------------------------------------------------------

	DESIGNED BY:	B. Jackson
	
	CODED BY:	B. Jackson
	
	MAINTAINED BY:	guess who

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY
			
----------------------------------------------------------------------------

	REFERENCES:
	
		[ 1]	McFarland, Richard E.: "A Standard Kinematic Model
			for Flight Simulation at NASA-Ames", NASA CR-2497,
			January 1975
			
		[ 2]	ANSI/AIAA R-004-1992 "Recommended Practice: Atmos-
			pheric and Space Flight Vehicle Coordinate Systems",
			February 1992
			
		[ 3]	Beyer, William H., editor: "CRC Standard Mathematical
			Tables, 28th edition", CRC Press, Boca Raton, FL, 1987,
			ISBN 0-8493-0628-0
			
		[ 4]	Dowdy, M. C.; Jackson, E. B.; and Nichols, J. H.:
			"Controls Analysis and Simulation Test Loop Environ-
			ment (CASTLE) Programmer's Guide, Version 1.3", 
			NATC TM 89-11, 30 March 1989.
			
		[ 5]	Halliday, David; and Resnick, Robert: "Fundamentals
			of Physics, Revised Printing", Wiley and Sons, 1974.
			ISBN 0-471-34431-1

		[ 6]	Anon: "U. S. Standard Atmosphere, 1962"
		
		[ 7]	Anon: "Aeronautical Vest Pocket Handbook, 17th edition",
			Pratt & Whitney Aircraft Group, Dec. 1977
			
		[ 8]	Stevens, Brian L.; and Lewis, Frank L.: "Aircraft 
			Control and Simulation", Wiley and Sons, 1992.
			ISBN 0-471-61397-5			

--------------------------------------------------------------------------*/

#ifndef _LS_CONSTANTS
#define _LS_CONSTANTS


#ifndef CONSTANTS

#define CONSTANTS -1

/* Define application-wide macros */

#define PATHNAME "LARCSIMPATH"
#ifndef NIL_POINTER
#define NIL_POINTER 0L
#endif

/* Define constants (note: many factors will need to change for other 
	systems of measure)	*/

/* Value of Pi from ref [3] */
#define	LS_PI 3.14159265358979323846264338327950288419716939967511

/* Value of earth radius from [8], ft */
#define	EQUATORIAL_RADIUS 20925650.
#define RESQ 437882827922500.

/* Value of earth flattening parameter from ref [8] 			
	
	Note: FP = f
	      E  = 1-f
	      EPS = sqrt(1-(1-f)^2)			*/
	      
#define FP	.003352813178
#define E   .996647186
#define EPS .081819221
#define INVG .031080997

/* linear velocity of earth at equator from w*R; w=2pi/24 hrs, in ft/sec */
#define OMEGA_EARTH .00007272205217

/* miscellaneous units conversions (ref [7]) */
#define	V_TO_KNOTS	0.5921
#define DEG_TO_RAD	0.017453292
#define RAD_TO_DEG	57.29577951
#define FT_TO_METERS	0.3048
#define METERS_TO_FT	3.2808
#define K_TO_R		1.8
#define R_TO_K		0.55555556
#define NSM_TO_PSF	0.02088547
#define PSF_TO_NSM	47.8801826
#define KCM_TO_SCF	0.00194106
#define SCF_TO_KCM	515.183616


/* ENGLISH Atmospheric reference properties [6] */
#define SEA_LEVEL_DENSITY	0.002376888

#endif


#endif /* _LS_CONSTANTS_H */


/*------------------------- end of ls_constants.h -------------------------*/
