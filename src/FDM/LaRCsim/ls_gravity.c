/***************************************************************************

	TITLE:	ls_gravity
		
----------------------------------------------------------------------------

	FUNCTION:	Gravity model for LaRCsim

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Created by Bruce Jackson on September 25, 1992.

----------------------------------------------------------------------------

	DESIGNED BY:	Bruce Jackson
		
	CODED BY:	Bruce Jackson
		
	MAINTAINED BY:	Bruce Jackson

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
		
	DATE	PURPOSE							BY
		
	940111	Changed include files to "ls_types.h" and 
		"ls_constants.h" from "ls_eom.h"; also changed DATA types
		to SCALAR types.					EBJ
		
										
$Header$
$Log$
Revision 1.1  2002/09/10 01:14:02  curt
Initial revision

Revision 1.1.1.1  1999/06/17 18:07:34  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.3  1998/08/06 12:46:39  curt
Header change.

Revision 1.2  1998/01/19 18:40:26  curt
Tons of little changes to clean up the code and to remove fatal errors
when building with the c++ compiler.

Revision 1.1  1997/05/29 00:09:56  curt
Initial Flight Gear revision.

 * Revision 1.2  1994/01/11  18:50:35  bjax
 * Corrected include files (was ls_eom.h) and DATA types changed
 * to SCALARs. EBJ
 *
 * Revision 1.1  1992/12/30  13:18:46  bjax
 * Initial revision
 *		

----------------------------------------------------------------------------

		REFERENCES:	Stevens, Brian L.; and Lewis, Frank L.: "Aircraft 
				Control and Simulation", Wiley and Sons, 1992.
				ISBN 0-471-

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
#include "ls_gravity.h"
#include <math.h>

#define GM	1.4076431E16
#define J2	1.08263E-3

void ls_gravity( SCALAR radius, SCALAR lat, SCALAR *gravity )
{

	SCALAR	radius_ratio, rrsq, sinsqlat;
	
	radius_ratio = radius/EQUATORIAL_RADIUS;
	rrsq = radius_ratio*radius_ratio;
	sinsqlat = sin(lat)*sin(lat);
	*gravity = (GM/(radius*radius))
		*sqrt(2.25*rrsq*rrsq*J2*J2*(5*sinsqlat*sinsqlat -2*sinsqlat + 1)
				+ 3*rrsq*J2*(1 - 3*sinsqlat) + 1);

}
