/***************************************************************************

	TITLE:		ls_matrix.h
	
----------------------------------------------------------------------------

	FUNCTION:	Header file for general real matrix routines.
				
	The routines in this module have come more or less from ref [1].
	Note that, probably due to the heritage of ref [1] (which has a 
	FORTRAN version that was probably written first), the use of 1 as
	the first element of an array (or vector) is used. This is accomplished
	in memory by allocating, but not using, the 0 elements in each dimension.
	While this wastes some memory, it allows the routines to be ported more
	easily from FORTRAN (I suspect) as well as adhering to conventional 
	matrix notation.  As a result, however, traditional ANSI C convention
	(0-base indexing) is not followed; as the authors of ref [1] point out,
	there is some question of the portability of the resulting routines
	which sometimes access negative indexes. See ref [1] for more details.

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Created 950222 E. B. Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	from Numerical Recipes in C, by Press, et. al.
	
	CODED BY:	Bruce Jackson
	
	MAINTAINED BY:	

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY
	
	CURRENT RCS HEADER:

$Header$
$Log$
Revision 1.1  2002/09/10 01:14:02  curt
Initial revision

Revision 1.1.1.1  1999/06/17 18:07:34  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.1  1998/06/27 22:34:58  curt
Initial revision.

 * Revision 1.1  1995/02/27  20:02:18  bjax
 * Initial revision
 *

----------------------------------------------------------------------------

	REFERENCES:	[1] Press, William H., et. al, Numerical Recipes in 
			    C, 2nd edition, Cambridge University Press, 1992

----------------------------------------------------------------------------

	CALLED BY:

----------------------------------------------------------------------------

	CALLS TO:

----------------------------------------------------------------------------

	INPUTS:

----------------------------------------------------------------------------

	OUTPUTS:

--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define NR_END 1

/* matrix creation & destruction routines */

int *nr_ivector(long nl, long nh);
double **nr_matrix(long nrl, long nrh, long ncl, long nch);

void nr_free_ivector(int *v, long nl /* , long nh */);
void nr_free_matrix(double **m, long nrl, long nrh, long ncl, long nch);


/* Gauss-Jordan inversion routine */

int nr_gaussj(double **a, int n, double **b, int m);

/* Linear equation solution by Gauss-Jordan elimination. a[1..n][1..n] is */
/* the input matrix. b[1..n][1..m] is input containing the m right-hand   */
/* side vectors. On output, a is replaced by its matrix invers, and b is  */
/* replaced by the corresponding set of solution vectors.                 */

/* Note: this routine modified by EBJ to make b optional, if m == 0 */

/* Matrix copy, multiply, and printout routines (by EBJ) */

void nr_copymat(double **orig, int n, double **copy);
void nr_multmat(double **m1, int n, double **m2, double **prod);
void nr_printmat(double **a, int n);


