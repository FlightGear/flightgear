/* Copyright 1988, Brown Computer Graphics Group.  All Rights Reserved. */

/* --------------------------------------------------------------------------
 * This file contains routines that operate solely on matrices.
 * -------------------------------------------------------------------------*/

#include "mat3defs.h"

/* --------------------------  Static Routines	---------------------------- */

#define SMALL 1e-20		/* Small enough to be considered zero */

/*
 * Shuffles rows in inverse of 3x3.  See comment in MAT3_inv3_second_col().
 */

static void
MAT3_inv3_swap( register double inv[3][3], int row0, int row1, int row2)
{
   register int i, tempi;
   double	temp;

#define SWAP_ROWS(a, b) \
   for (i = 0; i < 3; i++) SWAP(inv[a][i], inv[b][i], temp); \
   SWAP(a, b, tempi)

   if (row0 != 0){
      if (row1 == 0) {
	 SWAP_ROWS(row0, row1);
      }
      else {
	 SWAP_ROWS(row0, row2);
      }
   }

   if (row1 != 1) {
      SWAP_ROWS(row1, row2);
   }
}

/*
 * Does Gaussian elimination on second column.
 */

static int
MAT3_inv3_second_col (register double source[3][3], register double inv[3][3], int row0)
{
   register int row1, row2, i1, i2, i;
   double	temp;
   double	a, b;

   /* Find which row to use */
   if	   (row0 == 0)	i1 = 1, i2 = 2;
   else if (row0 == 1)	i1 = 0, i2 = 2;
   else 		i1 = 0, i2 = 1;

   /* Find which is larger in abs. val.:the entry in [i1][1] or [i2][1]	*/
   /* and use that value for pivoting.					*/

   a = source[i1][1]; if (a < 0) a = -a;
   b = source[i2][1]; if (b < 0) b = -b;
   if (a > b) 	row1 = i1;
   else		row1 = i2;
   row2 = (row1 == i1 ? i2 : i1);

   /* Scale row1 in source */
   if ((source[row1][1] < SMALL) && (source[row1][1] > -SMALL)) return(FALSE);
   temp = 1.0 / source[row1][1];
   source[row1][1]  = 1.0;
   source[row1][2] *= temp;	/* source[row1][0] is zero already */

   /* Scale row1 in inv */
   inv[row1][row1]  = temp;	/* it used to be a 1.0 */
   inv[row1][row0] *= temp;

   /* Clear column one, source, and make corresponding changes in inv */

   for (i = 0; i < 3; i++) if (i != row1) {	/* for i = all rows but row1 */
   temp = -source[i][1];
      source[i][1]  = 0.0;
      source[i][2] += temp * source[row1][2];

      inv[i][row1]  = temp * inv[row1][row1];
      inv[i][row0] += temp * inv[row1][row0];
   }

   /* Scale row2 in source */
   if ((source[row2][2] < SMALL) && (source[row2][2] > -SMALL)) return(FALSE);
   temp = 1.0 / source[row2][2];
   source[row2][2] = 1.0;	/* source[row2][*] is zero already */

   /* Scale row2 in inv */
   inv[row2][row2]  = temp;	/* it used to be a 1.0 */
   inv[row2][row0] *= temp;
   inv[row2][row1] *= temp;

   /* Clear column one, source, and make corresponding changes in inv */
   for (i = 0; i < 3; i++) if (i != row2) {	/* for i = all rows but row2 */
   temp = -source[i][2];
      source[i][2] = 0.0;
      inv[i][row0] += temp * inv[row2][row0];
      inv[i][row1] += temp * inv[row2][row1];
      inv[i][row2] += temp * inv[row2][row2];
   }

   /*
    * Now all is done except that the inverse needs to have its rows shuffled.
    * row0 needs to be moved to inv[0][*], row1 to inv[1][*], etc.
    *
    * We *didn't* do the swapping before the elimination so that we could more
    * easily keep track of what ops are needed to be done in the inverse.
    */
   MAT3_inv3_swap(inv, row0, row1, row2);

   return(TRUE);
}

/*
 * Fast inversion routine for 3 x 3 matrices.	- Written by jfh.
 *
 * This takes 30 multiplies/divides, as opposed to 39 for Cramer's Rule.
 * The algorithm consists of performing fast gaussian elimination, by never
 * doing any operations where the result is guaranteed to be zero, or where
 * one operand is guaranteed to be zero. This is done at the cost of clarity,
 * alas.
 *
 * Returns 1 if the inverse was successful, 0 if it failed.
 */

static int
MAT3_invert3 (register double source[3][3], register double inv[3][3])
{
   register int i, row0;
   double	temp;
   double	a, b, c;

   inv[0][0] = inv[1][1] = inv[2][2] = 1.0;
   inv[0][1] = inv[0][2] = inv[1][0] = inv[1][2] = inv[2][0] = inv[2][1] = 0.0;

   /* attempt to find the largest entry in first column to use as pivot */
   a = source[0][0]; if (a < 0) a = -a;
   b = source[1][0]; if (b < 0) b = -b;
   c = source[2][0]; if (c < 0) c = -c;

   if (a > b) {
      if (a > c) row0 = 0;
      else row0 = 2;
   }
   else { 
      if (b > c) row0 = 1;
      else row0 = 2;
   }

   /* Scale row0 of source */
   if ((source[row0][0] < SMALL) && (source[row0][0] > -SMALL)) return(FALSE);
   temp = 1.0 / source[row0][0];
   source[row0][0]  = 1.0;
   source[row0][1] *= temp;
   source[row0][2] *= temp;

   /* Scale row0 of inverse */
   inv[row0][row0] = temp;	/* other entries are zero -- no effort	*/

   /* Clear column zero of source, and make corresponding changes in inverse */

   for (i = 0; i < 3; i++) if (i != row0) {	/* for i = all rows but row0 */
      temp = -source[i][0];
      source[i][0]  = 0.0;
      source[i][1] += temp * source[row0][1];
      source[i][2] += temp * source[row0][2];
      inv[i][row0]  = temp * inv[row0][row0];
   }

   /*
    * We've now done gaussian elimination so that the source and
    * inverse look like this:
    *
    *	1 * *		* 0 0
    *	0 * *		* 1 0
    *	0 * *		* 0 1
    *
    * We now proceed to do elimination on the second column.
    */
   if (! MAT3_inv3_second_col(source, inv, row0)) return(FALSE);

   return(TRUE);
}

/*
 * Finds a new pivot for a non-simple 4x4.  See comments in MAT3invert().
 */

static int
MAT3_inv4_pivot (register MAT3mat src, MAT3vec r, double *s, int *swap)
{
   register int i, j;
   double	temp, max;

   *swap = -1;

   if (MAT3_IS_ZERO(src[3][3])) {

      /* Look for a different pivot element: one with largest abs value */
      max = 0.0;

      for (i = 0; i < 4; i++) {
	 if	 (src[i][3] >  max) max =  src[*swap = i][3];
	 else if (src[i][3] < -max) max = -src[*swap = i][3];
      }

      /* No pivot element available ! */
      if (*swap < 0) return(FALSE);

      else for (j = 0; j < 4; j++) SWAP(src[*swap][j], src[3][j], temp);
   }

   MAT3_SET_VEC (r, -src[0][3], -src[1][3], -src[2][3]);

   *s = 1.0 / src[3][3];

   src[0][3] = src[1][3] = src[2][3] = 0.0;
   src[3][3]			     = 1.0;

   MAT3_SCALE_VEC(src[3], src[3], *s);

   for (i = 0; i < 3; i++) {
      src[0][i] += r[0] * src[3][i];
      src[1][i] += r[1] * src[3][i];
      src[2][i] += r[2] * src[3][i];
   }

   return(TRUE);
}

/* -------------------------  Internal Routines  --------------------------- */

/* --------------------------  Public Routines	---------------------------- */

/*
 * This returns the inverse of the given matrix.  The result matrix
 * may be the same as the one to invert.
 *
 * Fast inversion routine for 4 x 4 matrices, written by jfh.
 *
 * Returns 1 if the inverse was successful, 0 if it failed.
 *
 * This routine has been specially tweaked to notice the following:
 * If the matrix has the form
 *	  * * * 0
 *	  * * * 0
 *	  * * * 0
 *	  * * * 1
 *
 * (as do many matrices in graphics), then we compute the inverse of
 * the upper left 3x3 matrix and use this to find the general inverse.
 *
 *   In the event that the right column is not 0-0-0-1, we do gaussian
 * elimination to make it so, then use the 3x3 inverse, and then do
 * our gaussian elimination.
 */

int
MAT3invert(double (*result_mat)[4], double (*mat)[4])
{
   MAT3mat		src, inv;
   register int 	i, j, simple;
   double		m[3][3], inv3[3][3], s, temp;
   MAT3vec		r, t;
   int			swap;

   MAT3copy(src, mat);
   MAT3identity(inv);

   /* If last column is not (0,0,0,1), use special code */
   simple = (mat[0][3] == 0.0 && mat[1][3] == 0.0 &&
	     mat[2][3] == 0.0 && mat[3][3] == 1.0);

   if (! simple && ! MAT3_inv4_pivot(src, r, &s, &swap)) return(FALSE);

   MAT3_COPY_VEC(t, src[3]);	/* Translation vector */

   /* Copy upper-left 3x3 matrix */
   for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) m[i][j] = src[i][j];

   if (! MAT3_invert3(m, inv3)) return(FALSE);

   for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) inv[i][j] = inv3[i][j];

   for (i = 0; i < 3; i++) for (j = 0; j < 3; j++)
      inv[3][i] -= t[j] * inv3[j][i];

   if (! simple) {

      /* We still have to undo our gaussian elimination from earlier on */
      /* add r0 * first col to last col */
      /* add r1 * 2nd	col to last col */
      /* add r2 * 3rd	col to last col */

      for (i = 0; i < 4; i++) {
	 inv[i][3] += r[0] * inv[i][0] + r[1] * inv[i][1] + r[2] * inv[i][2];
	 inv[i][3] *= s;
      }

      if (swap >= 0)
	 for (i = 0; i < 4; i++) SWAP(inv[i][swap], inv[i][3], temp);
   }

   MAT3copy(result_mat, inv);

   return(TRUE);
}
