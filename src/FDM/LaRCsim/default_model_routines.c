/***************************************************************************

	TITLE:		engine.c
	
----------------------------------------------------------------------------

	FUNCTION:	dummy engine routine

----------------------------------------------------------------------------

	MODULE STATUS:	incomplete

----------------------------------------------------------------------------

	GENEALOGY:	Created 15 OCT 92 as dummy routine for checkout. EBJ

----------------------------------------------------------------------------

	DESIGNED BY:	designer
	
	CODED BY:	programmer
	
	MAINTAINED BY:	maintainer

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY

	CURRENT RCS HEADER INFO:

$Header$

$Log$
Revision 1.1  2002/09/10 01:14:01  curt
Initial revision

Revision 1.1.1.1  1999/06/17 18:07:33  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.3  1998/08/06 12:46:37  curt
Header change.

Revision 1.2  1998/01/19 18:40:23  curt
Tons of little changes to clean up the code and to remove fatal errors
when building with the c++ compiler.

Revision 1.1  1997/05/29 00:09:53  curt
Initial Flight Gear revision.

 * Revision 1.3  1994/01/11  19:10:45  bjax
 * Removed include files.
 *
 * Revision 1.2  1993/03/14  12:16:10  bjax
 * simple correction: added ';' to end of default routines. EBJ
 *
 * Revision 1.1  93/03/10  06:43:46  bjax
 * Initial revision
 * 
 * Revision 1.1  92/12/30  13:21:46  bjax
 * Initial revision
 * 

----------------------------------------------------------------------------

	REFERENCES:

----------------------------------------------------------------------------

	CALLED BY:	ls_model();

----------------------------------------------------------------------------

	CALLS TO:	none

----------------------------------------------------------------------------

	INPUTS:

----------------------------------------------------------------------------

	OUTPUTS:

--------------------------------------------------------------------------*/


#include "ls_types.h"
#include "default_model_routines.h"

void inertias( SCALAR dt, int Initialize ) 	{}
void subsystems( SCALAR dt, int Initialize )	{}
/* void engine() 	{} */
/* void gear()		{} */




