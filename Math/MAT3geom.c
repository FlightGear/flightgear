/* #include "HEADERS.h" */
/* Copyright 1988, Brown Computer Graphics Group.  All Rights Reserved. */

/* --------------------------------------------------------------------------
 * This file contains routines that perform geometry-related operations
 * on matrices.
 * -------------------------------------------------------------------------*/

#include "mat3defs.h"

/* --------------------------  Static Routines	---------------------------- */

/* -------------------------  Internal Routines  --------------------------- */

/* --------------------------  Public Routines	---------------------------- */

/*
 * This takes a matrix used to transform points, and returns a corresponding
 * matrix that can be used to transform direction vectors (between points).
 */

void
MAT3direction_matrix(result_mat, mat)
register MAT3mat result_mat, mat;
{
   register int i;

   MAT3copy(result_mat, mat);

   for (i = 0; i < 4; i++) result_mat[i][3] = result_mat[3][i] = 0.0;

   result_mat[3][3] = 1.0;
}

/*
 * This takes a matrix used to transform points, and returns a corresponding
 * matrix that can be used to transform vectors that must remain perpendicular
 * to planes defined by the points.  It is useful when you are transforming
 * some object that has both points and normals in its definition, and you
 * only have the transformation matrix for the points.	This routine returns
 * FALSE if the normal matrix is uncomputable.	Otherwise, it returns TRUE.
 *
 * Spike sez: "This is the adjoint for the non-homogeneous part of the
 *	       transformation."
 */

int
MAT3normal_matrix(result_mat, mat)
register MAT3mat result_mat, mat;
{
   register int ret;
   MAT3mat	tmp_mat;

   MAT3direction_matrix(result_mat, mat);

   if (ret = MAT3invert(tmp_mat, tmp_mat)) MAT3transpose(result_mat, tmp_mat);

   return(ret);
}

/*
 * Sets the given matrix to be a scale matrix for the given vector of
 * scale values.
 */

void
MAT3scale(result_mat, scale)
MAT3mat result_mat;
MAT3vec scale;
{
   MAT3identity(result_mat);

   result_mat[0][0] = scale[0];
   result_mat[1][1] = scale[1];
   result_mat[2][2] = scale[2];
}

/*
 * Sets up a matrix for a rotation about an axis given by the line from
 * (0,0,0) to axis, through an angle (in radians).
 * Looking along the axis toward the origin, the rotation is counter-clockwise.
 */

#define SELECT	.7071	/* selection constant (roughly .5*sqrt(2) */

void
MAT3rotate(result_mat, axis, angle_in_radians)
MAT3mat result_mat;
MAT3vec axis;
double	angle_in_radians;
{
   MAT3vec	naxis,	/* Axis of rotation, normalized 		*/
		base2,	/* 2nd unit basis vec, perp to axis		*/
		base3;	/* 3rd unit basis vec, perp to axis & base2	*/
   double	dot;
   MAT3mat	base_mat,	/* Change-of-basis matrix		*/
		base_mat_trans; /* Inverse of c-o-b matrix		*/
   register int i;

   /* Step 1: extend { axis } to a basis for 3-space: { axis, base2, base3 }
    * which is orthonormal (all three have unit length, and all three are
    * mutually orthogonal). Also should be oriented, i.e. axis cross base2 =
    * base3, rather than -base3.
    *
    * Method: Find a vector linearly independent from axis. For this we
    * either use the y-axis, or, if that is too close to axis, the
    * z-axis. 'Too close' means that the dot product is too near to 1.
    */

   MAT3_COPY_VEC(naxis, axis);
   MAT3_NORMALIZE_VEC(naxis, dot);

   if (dot == 0.0) {
       /* ERR_ERROR(MAT3_errid, ERR_SEVERE,
		   (ERR_S, "Zero-length axis vector given to MAT3rotate")); */
      return;
   }

   MAT3perp_vec(base2, naxis, TRUE);
   MAT3cross_product(base3, naxis, base2);

   /* Set up the change-of-basis matrix, and its inverse */
   MAT3identity(base_mat);
   MAT3identity(base_mat_trans);
   MAT3identity(result_mat);

   for (i = 0; i < 3; i++){
      base_mat_trans[i][0] = base_mat[0][i] = naxis[i];
      base_mat_trans[i][1] = base_mat[1][i] = base2[i];
      base_mat_trans[i][2] = base_mat[2][i] = base3[i];
   }

   /* If T(u) = uR, where R is base_mat, then T(x-axis) = naxis,
    * T(y-axis) = base2, and T(z-axis) = base3. The inverse of base_mat is
    * its transpose.  OK?
    */

   result_mat[1][1] =	result_mat[2][2] = cos(angle_in_radians);
   result_mat[2][1] = -(result_mat[1][2] = sin(angle_in_radians));

   MAT3mult(result_mat, base_mat_trans, result_mat);
   MAT3mult(result_mat, result_mat,	base_mat);
}

/*
 * Sets the given matrix to be a translation matrix for the given vector of
 * translation values.
 */

void
MAT3translate(result_mat, trans)
MAT3mat result_mat;
MAT3vec trans;
{
   MAT3identity(result_mat);

   result_mat[3][0] = trans[0];
   result_mat[3][1] = trans[1];
   result_mat[3][2] = trans[2];
}

/*
 * Sets the given matrix to be a shear matrix for the given x and y shear
 * values.
 */

void
MAT3shear(result_mat, xshear, yshear)
MAT3mat result_mat;
double	xshear, yshear;
{
   MAT3identity(result_mat);

   result_mat[2][0] = xshear;
   result_mat[2][1] = yshear;
}

