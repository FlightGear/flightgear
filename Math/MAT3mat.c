/* #include "HEADERS.h" */
/* Copyright 1988, Brown Computer Graphics Group.  All Rights Reserved. */

/* --------------------------------------------------------------------------
 * This file contains routines that operate solely on matrices.
 * -------------------------------------------------------------------------*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef WIN32
#  include <memory.h>      /* required for memset() and memcpy() */
#endif

#include <string.h>
#include <Math/mat3defs.h>

#define USE_MEM

/* #include "macros.h" */

/* --------------------------  Static Routines	---------------------------- */

/* -------------------------  Internal Routines  --------------------------- */

/* --------------------------  Public Routines	---------------------------- */


#if !defined( USE_XTRA_MAT3_INLINES )

/*
 * Sets a matrix to identity.
 */

void
MAT3identity (register MAT3mat mat)
{
   register int i;

#ifdef USE_MEM /* WIN32 */
   memset(mat, 0x00, sizeof(MAT3mat));
#else
   bzero (mat, sizeof(MAT3mat));
#endif

   for (i = 0; i < 4; i++)
      mat[i][i] = 1.0;
}

/*
 * Sets a matrix to zero.
 */

void
MAT3zero (MAT3mat mat)
{
#ifdef USE_MEM /* WIN32 */
   memset(mat,0x00, sizeof(MAT3mat));
#else
   bzero (mat, sizeof(MAT3mat));
#endif
}


/*
 * Copies one matrix to another.
 */

void
MAT3copy(MAT3mat to, MAT3mat from)
{
#ifdef USE_MEM /* WIN32 */
    memcpy(to, from, sizeof(MAT3mat));
#else
    bcopy(from, to, sizeof(MAT3mat));
#endif
}

/*
 * This multiplies two matrices, producing a third, which may the same as
 * either of the first two.
 */

void
MAT3mult (double (*result_mat)[4], register double (*mat1)[4], register double (*mat2)[4])
{
   register int i, j;
   MAT3mat	tmp_mat;

   for (i = 0; i < 4; i++)
      for (j = 0; j < 4; j++)
         tmp_mat[i][j] = (mat1[i][0] * mat2[0][j] +
		       mat1[i][1] * mat2[1][j] +
		       mat1[i][2] * mat2[2][j] +
		       mat1[i][3] * mat2[3][j]);
   MAT3copy (result_mat, tmp_mat);
}
#endif // !defined( USE_XTRA_MAT3_INLINES )

/*
 * This returns the transpose of a matrix.  The result matrix may be
 * the same as the one to transpose.
 */

void
MAT3transpose (double (*result_mat)[4], register double (*mat)[4])
{
   register int i, j;
   MAT3mat	tmp_mat;

   for (i = 0; i < 4; i++) 
      for (j = 0; j < 4; j++) 
         tmp_mat[i][j] = mat[j][i];

   MAT3copy (result_mat, tmp_mat);
}


/*
 * This prints the given matrix to the given file pointer.
 */

void
MAT3print(double (*mat)[4], FILE *fp)
{
   MAT3print_formatted(mat, fp, CNULL, CNULL, CNULL, CNULL);
}

/*
 * This prints the given matrix to the given file pointer.
 * use the format string to pass to fprintf.  head and tail
 * are printed at the beginning and end of each line.
 */

void
MAT3print_formatted(double (*mat)[4], FILE *fp, char *title, char *head, char *format, char *tail)
{
   register int i, j;

   /* This is to allow this to be called easily from a debugger */
   if (fp == NULL) fp = stderr;

   if (title  == NULL)	title  = "MAT3 matrix:\n";
   if (head   == NULL)	head   = "  ";
   if (format == NULL)	format = "%#8.4lf  ";
   if (tail   == NULL)	tail   = "\n";

   (void) fprintf(fp, title);

   for (i = 0; i < 4; i++) {
      (void) fprintf(fp, head);
      for (j = 0; j < 4; j++) (void) fprintf(fp, format, mat[i][j]);
      (void) fprintf(fp, tail);
   }
}
