/***************************************************************************

	TITLE:		ls_matrix.c
	
----------------------------------------------------------------------------

	FUNCTION:	general real matrix routines; includes
				gaussj() for matrix inversion using
				Gauss-Jordan method with full pivoting.
				
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
Revision 1.2  2004/04/01 15:27:55  curt
Clean up various compiler warnings that have crept into the code.  This
by no means get's them all, but it's a start.

Revision 1.1.1.1  2002/09/10 01:14:02  curt
Initial revision of FlightGear-0.9.0

Revision 1.1.1.1  1999/06/17 18:07:34  curt
Start of 0.7.x branch

Revision 1.1.1.1  1999/04/05 21:32:45  curt
Start of 0.6.x branch.

Revision 1.1  1998/06/27 22:34:57  curt
Initial revision.

 * Revision 1.1  1995/02/27  19:55:44  bjax
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "ls_matrix.h"


#define SWAP(a,b) {temp=(a);(a)=(b);(b)=temp;}

static char rcsid[] = "$Id$";


int *nr_ivector(long nl, long nh)
{
    int *v;

    v=(int *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(int)));
    return v-nl+NR_END;
}



double **nr_matrix(long nrl, long nrh, long ncl, long nch)
/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
{
    long i, nrow=nrh-nrl+1, ncol=nch-ncl+1;
    double **m;

    /* allocate pointers to rows */
    m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));

    if (!m)
	{
	    fprintf(stderr, "Memory failure in routine 'nr_matrix'.\n");
	    exit(1);
	}

    m += NR_END;
    m -= nrl;

    /* allocate rows and set pointers to them */
    m[nrl] = (double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
    if (!m[nrl]) 
	{
	    fprintf(stderr, "Memory failure in routine 'matrix'\n");
	    exit(1);
	}

    m[nrl] += NR_END;
    m[nrl] -= ncl;

    for (i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;

    /* return pointer to array of pointers to rows */
    return m;
}


void nr_free_ivector(int *v, long nl /* , long nh */)
{
    free( (char *) (v+nl-NR_END));
}


void nr_free_matrix(double **m, long nrl, long nrh, long ncl, long nch)
/* free a double matrix allocated by nr_matrix() */
{
    free((char *) (m[nrl]+ncl-NR_END));
    free((char *) (m+nrl-NR_END));
}


int nr_gaussj(double **a, int n, double **b, int m)

/* Linear equation solution by Gauss-Jordan elimination. a[1..n][1..n] is */
/* the input matrix. b[1..n][1..m] is input containing the m right-hand   */
/* side vectors. On output, a is replaced by its matrix invers, and b is  */
/* replaced by the corresponding set of solution vectors.                 */

/* Note: this routine modified by EBJ to make b optional, if m == 0 */

{
    int		*indxc, *indxr, *ipiv;
    int 	i, icol = 0, irow = 0, j, k, l, ll;
    double       big, dum, pivinv, temp;

    int		bexists = ((m != 0) || (b == 0));

    indxc = nr_ivector(1,n);	/* The integer arrays ipiv, indxr, and  */
    indxr = nr_ivector(1,n);	/* indxc are used for pivot bookkeeping */
    ipiv  = nr_ivector(1,n);
    
    for (j=1;j<=n;j++) ipiv[j] = 0;

    for (i=1;i<=n;i++)		/* This is the main loop over columns	*/
	{
	    big = 0.0;
	    for (j=1;j<=n;j++)	/* This is outer loop of pivot search	*/
		if (ipiv[j] != 1)
		    for (k=1;k<=n;k++)
			{
			    if (ipiv[k] == 0)
				{
				    if (fabs(a[j][k]) >= big)
					{
					    big = fabs(a[j][k]);
					    irow = j;
					    icol = k;
					}
				}
			    else
				if (ipiv[k] > 1) return -1;
			}
	    ++(ipiv[icol]);

/*    We now have the pivot element, so we interchange rows, if needed,  */
/* to put the pivot element on the diagonal.  The columns are not        */
/* physically interchanged, only relabeled: indxc[i], the column of the  */
/* ith pivot element, is the ith column that is reduced, while indxr[i]  */
/* is the row in which that pivot element was orignally located. If      */
/* indxr[i] != indxc[i] there is an implied column interchange.  With    */
/* this form of bookkeeping, the solution b's will end up in the correct */
/* order, and the inverse matrix will be scrambed by columns.            */

	    if (irow != icol)
		{
/*		    for (l=1;1<=n;l++) SWAP(a[irow][l],a[icol][l]) */
			for (l=1;l<=n;l++) 
			  { 
			  	temp=a[irow][l]; 
			  	a[irow][l]=a[icol][l]; 
			  	a[icol][l]=temp; 
			  }
		    if (bexists) for (l=1;l<=m;l++) SWAP(b[irow][l],b[icol][l])
		}
	    indxr[i] = irow;	/* We are now ready to divide the pivot row */
	    indxc[i] = icol;	/* by the pivot element, a[irow][icol]      */
	    if (a[icol][icol] == 0.0) return -1;
	    pivinv = 1.0/a[icol][icol];
	    a[icol][icol] = 1.0;
	    for (l=1;l<=n;l++) a[icol][l] *= pivinv;
	    if (bexists) for (l=1;l<=m;l++) b[icol][l] *= pivinv;
	    for (ll=1;ll<=n;ll++)	/* Next, we reduce the rows...	*/
		if (ll != icol )	/* .. except for the pivot one  */
		    {
			dum = a[ll][icol];
			a[ll][icol] = 0.0;
			for (l=1;l<=n;l++) a[ll][l] -= a[icol][l]*dum;
	   if (bexists) for (l=1;l<=m;l++) b[ll][i] -= b[icol][l]*dum;
		    }
	}

/* This is the end of the mail loop over columns of the reduction. It
       only remains to unscrambled the solution in view of the column
       interchanges. We do this by interchanging pairs of columns in
       the reverse order that the permutation was built up. */
			
    for (l=n;l>=1;l--)
	{
	    if (indxr[l] != indxc[l])
		for (k=1;k<=n;k++)
		    SWAP(a[k][indxr[l]],a[k][indxc[l]])
	}

/* and we are done */
    
    nr_free_ivector(ipiv,1 /*,n*/ );
    nr_free_ivector(indxr,1 /*,n*/ );
    nr_free_ivector(indxc,1 /*,n*/ );

    return 0;	/* indicate success */
}

void nr_copymat(double **orig, int n, double **copy)
/* overwrites matrix 'copy' with copy of matrix 'orig' */
{
	long i, j;
	
	if ((orig==0)||(copy==0)||(n==0)) return;
	
	for (i=1;i<=n;i++)
		for (j=1;j<=n;j++)
			copy[i][j] = orig[i][j];
}

void nr_multmat(double **m1, int n, double **m2, double **prod)
{
	long i, j, k;
	
	if ((m1==0)||(m2==0)||(prod==0)||(n==0)) return;
	
	for (i=1;i<=n;i++)
		for (j=1;j<=n;j++)
			{
				prod[i][j] = 0.0;
				for(k=1;k<=n;k++) prod[i][j] += m1[i][k]*m2[k][j];
			}
}
			


void nr_printmat(double **a, int n)
{
    int i,j;
    
    printf("\n");
    for(i=1;i<=n;i++) 
      {
	  for(j=1;j<=n;j++)
	      printf("% 9.4f ", a[i][j]);
	  printf("\n");
      }
    printf("\n");
    
}


void testmat( void ) /* main() for test purposes */
{
    double **mat1, **mat2, **mat3;
    double invmaxlong;
    int loop, i, j, n = 20;
    long maxlong = RAND_MAX;
    int maxloop = 2;

    invmaxlong = 1.0/(double)maxlong;
    mat1 = nr_matrix(1, n, 1, n );
    mat2 = nr_matrix(1, n, 1, n );
    mat3 = nr_matrix(1, n, 1, n );

/*    for(i=1;i<=n;i++) mat1[i][i]= 5.0; */

	for(loop=0;loop<maxloop;loop++)
	{
  	    if (loop != 0)
  	    	for(i=1;i<=n;i++)
		    for(j=1;j<=n;j++) 
			mat1[i][j] = 2.0 - 4.0*invmaxlong*(double) rand();

		printf("Original matrix:\n");
	    nr_printmat( mat1, n );
	    
	    nr_copymat( mat1, n, mat2 );
	
	    i = nr_gaussj( mat2, n, 0, 0 );
	
	    if (i) printf("Singular matrix.\n");
	
		printf("Inverted matrix:\n");
	    nr_printmat( mat2, n );
	    
	    nr_multmat( mat1, n, mat2, mat3 );
	    
	    printf("Original multiplied by inverse:\n");
	    nr_printmat( mat3, n );
	    
	    if (loop < maxloop-1) /* sleep(1) */;
	}

    nr_free_matrix( mat1, 1, n, 1, n );
    nr_free_matrix( mat2, 1, n, 1, n );
    nr_free_matrix( mat3, 1, n, 1, n );
}
