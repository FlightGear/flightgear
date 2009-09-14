/* Copyright 1988, Brown Computer Graphics Group.  All Rights Reserved. */

/* --------------------------------------------------------------------------
 * This file contains routines that operate on matrices and vectors, or
 * vectors and vectors.
 * -------------------------------------------------------------------------*/

/* #include "sphigslocal.h" */

/* --------------------------  Static Routines	---------------------------- */

/* -------------------------  Internal Routines  --------------------------- */

/* --------------------------  Public Routines	---------------------------- */

/*
 * Multiplies a vector by a matrix, setting the result vector.
 * It assumes all homogeneous coordinates are 1.
 * The two vectors involved may be the same.
 */

#include <Math/mat3.h>

#ifndef TRUE
#  define TRUE 1
#endif

#ifndef FALSE
#  define FALSE 0
#endif

#if !defined( USE_XTRA_MAT3_INLINES )

void
MAT3mult_vec(double *result_vec, register double *vec, register double (*mat)[4])
{
   MAT3vec		tempvec;
   register double	*temp = tempvec;

   temp[0] =	vec[0] * mat[0][0] + vec[1] * mat[1][0] +
		vec[2] * mat[2][0] +	      mat[3][0];
   temp[1] =	vec[0] * mat[0][1] + vec[1] * mat[1][1] +
		vec[2] * mat[2][1] +	      mat[3][1];
   temp[2] =	vec[0] * mat[0][2] + vec[1] * mat[1][2] +
		vec[2] * mat[2][2] +	      mat[3][2];

   MAT3_COPY_VEC(result_vec, temp);
}
#endif // !defined( USE_XTRA_MAT3_INLINES )

/*
 * Multiplies a vector of size 4 by a matrix, setting the result vector.
 * The fourth element of the vector is the homogeneous coordinate, which
 * may or may not be 1.  If the "normalize" parameter is TRUE, then the
 * result vector will be normalized so that the homogeneous coordinate is 1.
 * The two vectors involved may be the same.
 * This returns zero if the vector was to be normalized, but couldn't be.
 */

int
MAT3mult_hvec(double *result_vec, register double *vec, register double (*mat)[4], int normalize)
{
   MAT3hvec             tempvec;
   double		norm_fac;
   register double	*temp = tempvec;
   register int 	ret = TRUE;

   temp[0] =	vec[0] * mat[0][0] + vec[1] * mat[1][0] +
		vec[2] * mat[2][0] + vec[3] * mat[3][0];
   temp[1] =	vec[0] * mat[0][1] + vec[1] * mat[1][1] +
		vec[2] * mat[2][1] + vec[3] * mat[3][1];
   temp[2] =	vec[0] * mat[0][2] + vec[1] * mat[1][2] +
		vec[2] * mat[2][2] + vec[3] * mat[3][2];
   temp[3] =	vec[0] * mat[0][3] + vec[1] * mat[1][3] +
		vec[2] * mat[2][3] + vec[3] * mat[3][3];

   /* Normalize if asked for, possible, and necessary */
   if (normalize) {
      if (MAT3_IS_ZERO(temp[3])) {
#ifndef THINK_C
	 fprintf (stderr,
		  "Can't normalize vector: homogeneous coordinate is 0");
#endif
	 ret = FALSE;
      }
      else {
	 norm_fac = 1.0 / temp[3];
	 MAT3_SCALE_VEC(result_vec, temp, norm_fac);
	 result_vec[3] = 1.0;
      }
   }
   else MAT3_COPY_HVEC(result_vec, temp);

   return(ret);
}

#if !defined( USE_XTRA_MAT3_INLINES )

/*
 * Sets the first vector to be the cross-product of the last two vectors.
 */

void
MAT3cross_product(double *result_vec, register double *vec1, register double *vec2)
{
   MAT3vec		tempvec;
   register double	*temp = tempvec;

   temp[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
   temp[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
   temp[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];

   MAT3_COPY_VEC(result_vec, temp);
}
#endif // !defined( USE_XTRA_MAT3_INLINES )

/*
 * Finds a vector perpendicular to vec and stores it in result_vec.
 * Method:  take any vector (we use <0,1,0>) and subtract the
 * portion of it pointing in the vec direction.  This doesn't
 * work if vec IS <0,1,0> or is very near it.  So if this is
 * the case, use <0,0,1> instead.
 * If "is_unit" is TRUE, the given vector is assumed to be unit length.
 */

#define SELECT	.7071	/* selection constant (roughly .5*sqrt(2) */

void
MAT3perp_vec(double *result_vec, double *vec, int is_unit)
{
   MAT3vec	norm;
   double	dot;

   MAT3_SET_VEC(result_vec, 0.0, 1.0, 0.0);

   MAT3_COPY_VEC(norm, vec);

   if (! is_unit) MAT3_NORMALIZE_VEC(norm, dot);

   /* See if vector is too close to <0,1,0>.  If so, use <0,0,1> */
   if ((dot = MAT3_DOT_PRODUCT(norm, result_vec)) > SELECT || dot < -SELECT) {
      result_vec[1] = 0.0;
      result_vec[2] = 1.0;
      dot = MAT3_DOT_PRODUCT(norm, result_vec);
   }

   /* Subtract off non-perpendicular part */
   result_vec[0] -= dot * norm[0];
   result_vec[1] -= dot * norm[1];
   result_vec[2] -= dot * norm[2];

   /* Make result unit length */
   MAT3_NORMALIZE_VEC(result_vec, dot);
}
