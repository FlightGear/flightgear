/* Copyright 1988, Brown Computer Graphics Group.  All Rights Reserved. */

#include <stdio.h>
/* #include "mat3err.h" */
#include "mat3.h"

/* -----------------------------  Constants  ------------------------------ */

#define FALSE		0
#define TRUE		1

#define CNULL ((char *) NULL)

/* ------------------------------  Macros  -------------------------------- */

#define ALLOCN(P,T,N,M) \
   if ((P = (T *) malloc((unsigned) (N) * sizeof(T))) == NULL) \
      ERR_ERROR(MAT3_errid, ERR_FATAL, (ERR_ALLOC1, M)); \
   else

#define FREE(P)    free((char *) (P))

#define ABS(A)		((A) > 0   ? (A) : -(A))
#define MIN(A,B)	((A) < (B) ? (A) :  (B))
#define MAX(A,B)	((A) > (B) ? (A) :  (B))

#define SWAP(A,B,T)	(T = A, A = B, B = T)

/* Is N within EPS of zero ? */
#define IS_ZERO(N,EPS)	((N) < EPS && (N) > -EPS)

/* Macros for lu routines */
#define LU_PERMUTE(p,i,j)  { int LU_T; LU_T = p[i]; p[i] = p[j]; p[j] = LU_T; }

/* -------------------------  Internal Entries ---------------------------- */

/* -------------------------  Global Variables ---------------------------- */

/* extern ERRid	*MAT3_errid; */
