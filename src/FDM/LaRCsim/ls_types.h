/***************************************************************************

	TITLE:	ls_types.h
	
----------------------------------------------------------------------------

	FUNCTION:	LaRCSim type definitions header file

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Created 15 DEC 1993 by Bruce Jackson from old
			ls_eom.h header

----------------------------------------------------------------------------

	DESIGNED BY:	B. Jackson
	
	CODED BY:	B. Jackson
	
	MAINTAINED BY:	guess who

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY
	19 MAY 2001 Reduce MSVC6 warnings   Geoff R. McLane
--------------------------------------------------------------------------*/

#ifndef _LS_TYPES_H
#define _LS_TYPES_H

#ifdef _MSC_VER
#pragma warning(disable: 4244) // conversion from double to float
#pragma warning(disable: 4305) // truncation from const double to float
#endif /* _MSC_VER */

/* SCALAR type is used throughout equations of motion code - sets precision */

typedef double SCALAR;

typedef SCALAR VECTOR_3[3];

/* DATA type is old style; this statement for continuity */

#define DATA SCALAR


#endif /* _LS_TYPES_H */


/* --------------------------- end of ls_types.h -------------------------*/
