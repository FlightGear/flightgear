/* Copyright 1988, Brown Computer Graphics Group.  All Rights Reserved. */

/* -------------------------------------------------------------------------
		       Public MAT3 include file
   ------------------------------------------------------------------------- */

#ifndef MAT3_HAS_BEEN_INCLUDED
#define MAT3_HAS_BEEN_INCLUDED

/* -----------------------------  Constants  ------------------------------ */

/*
 * Make sure the math library .h file is included, in case it wasn't.
 */

#ifndef HUGE
#include <math.h>
#endif
#include <stdio.h>


#define MAT3_DET0	-1			/* Indicates singular mat */
#define MAT3_EPSILON	1e-12			/* Close enough to zero   */
#define MAT3_PI 	3.141592653589793	/* Pi			  */

/* ------------------------------  Types  --------------------------------- */

typedef double MAT3mat[4][4];		/* 4x4 matrix			 */
typedef double MAT3vec[3];		/* Vector			 */
typedef double MAT3hvec[4];             /* Vector with homogeneous coord */

/* ------------------------------  Macros  -------------------------------- */

/* Tests if a number is within EPSILON of zero */
#define MAT3_IS_ZERO(N) 	((N) < MAT3_EPSILON && (N) > -MAT3_EPSILON)

/* Sets a vector to the three given values */
#define MAT3_SET_VEC(V,X,Y,Z)	((V)[0]=(X), (V)[1]=(Y), (V)[2]=(Z))

/* Tests a vector for all components close to zero */
#define MAT3_IS_ZERO_VEC(V)	(MAT3_IS_ZERO((V)[0]) && \
				 MAT3_IS_ZERO((V)[1]) && \
				 MAT3_IS_ZERO((V)[2]))

/* Dot product of two vectors */
#define MAT3_DOT_PRODUCT(V1,V2) \
			((V1)[0]*(V2)[0] + (V1)[1]*(V2)[1] + (V1)[2]*(V2)[2])

/* Copy one vector to other */
#define MAT3_COPY_VEC(TO,FROM)	((TO)[0] = (FROM)[0], \
				 (TO)[1] = (FROM)[1], \
				 (TO)[2] = (FROM)[2])

/* Normalize vector to unit length, using TEMP as temporary variable.
 * TEMP will be zero if vector has zero length */
#define MAT3_NORMALIZE_VEC(V,TEMP) \
	if ((TEMP = sqrt(MAT3_DOT_PRODUCT(V,V))) > MAT3_EPSILON) { \
	   TEMP = 1.0 / TEMP; \
	   MAT3_SCALE_VEC(V,V,TEMP); \
	} else TEMP = 0.0

/* Scale vector by given factor, storing result vector in RESULT_V */
#define MAT3_SCALE_VEC(RESULT_V,V,SCALE) \
	MAT3_SET_VEC(RESULT_V, (V)[0]*(SCALE), (V)[1]*(SCALE), (V)[2]*(SCALE))

/* Adds vectors V1 and V2, storing result in RESULT_V */
#define MAT3_ADD_VEC(RESULT_V,V1,V2) \
	MAT3_SET_VEC(RESULT_V, (V1)[0]+(V2)[0], (V1)[1]+(V2)[1], \
			       (V1)[2]+(V2)[2])

/* Subtracts vector V2 from V1, storing result in RESULT_V */
#define MAT3_SUB_VEC(RESULT_V,V1,V2) \
	MAT3_SET_VEC(RESULT_V, (V1)[0]-(V2)[0], (V1)[1]-(V2)[1], \
			       (V1)[2]-(V2)[2])

/* Multiplies vectors V1 and V2, storing result in RESULT_V */
#define MAT3_MULT_VEC(RESULT_V,V1,V2) \
	MAT3_SET_VEC(RESULT_V, (V1)[0]*(V2)[0], (V1)[1]*(V2)[1], \
			       (V1)[2]*(V2)[2])

/* Sets RESULT_V to the linear combination of V1 and V2, scaled by
 * SCALE1 and SCALE2, respectively */
#define MAT3_LINEAR_COMB(RESULT_V,SCALE1,V1,SCALE2,V2) \
	MAT3_SET_VEC(RESULT_V,	(SCALE1)*(V1)[0] + (SCALE2)*(V2)[0], \
				(SCALE1)*(V1)[1] + (SCALE2)*(V2)[1], \
				(SCALE1)*(V1)[2] + (SCALE2)*(V2)[2])

/* Several of the vector macros are useful for homogeneous-coord vectors */
#define MAT3_SET_HVEC(V,X,Y,Z,W) ((V)[0]=(X), (V)[1]=(Y), \
				  (V)[2]=(Z), (V)[3]=(W))

#define MAT3_COPY_HVEC(TO,FROM) ((TO)[0] = (FROM)[0], \
				 (TO)[1] = (FROM)[1], \
				 (TO)[2] = (FROM)[2], \
				 (TO)[3] = (FROM)[3])

#define MAT3_SCALE_HVEC(RESULT_V,V,SCALE) \
	MAT3_SET_HVEC(RESULT_V, (V)[0]*(SCALE), (V)[1]*(SCALE), \
				(V)[2]*(SCALE), (V)[3]*(SCALE))

#define MAT3_ADD_HVEC(RESULT_V,V1,V2) \
	MAT3_SET_HVEC(RESULT_V, (V1)[0]+(V2)[0], (V1)[1]+(V2)[1], \
				(V1)[2]+(V2)[2], (V1)[3]+(V2)[3])

#define MAT3_SUB_HVEC(RESULT_V,V1,V2) \
	MAT3_SET_HVEC(RESULT_V, (V1)[0]-(V2)[0], (V1)[1]-(V2)[1], \
				(V1)[2]-(V2)[2], (V1)[3]-(V2)[3])

#define MAT3_MULT_HVEC(RESULT_V,V1,V2) \
	MAT3_SET_HVEC(RESULT_V, (V1)[0]*(V2)[0], (V1)[1]*(V2)[1], \
				(V1)[2]*(V2)[2], (V1)[3]*(V2)[3])

/* ------------------------------  Entries  ------------------------------- */


/* In MAT3geom.c */
void 		MAT3direction_matrix (MAT3mat result_mat, MAT3mat mat);
int 		MAT3normal_matrix (MAT3mat result_mat, MAT3mat mat);
void		MAT3rotate (MAT3mat result_mat, MAT3vec axis, double angle_in_radians);
void		MAT3translate (MAT3mat result_mat, MAT3vec trans);
void		MAT3scale (MAT3mat result_mat, MAT3vec scale);
void		MAT3shear(MAT3mat result_mat, double xshear, double yshear);

/* In MAT3mat.c */
void		MAT3identity(MAT3mat);
void		MAT3zero(MAT3mat);
void		MAT3copy (MAT3mat to, MAT3mat from);
void		MAT3mult (MAT3mat result, MAT3mat, MAT3mat);
void		MAT3transpose (MAT3mat result, MAT3mat);
int			MAT3invert (MAT3mat result, MAT3mat);
void		MAT3print (MAT3mat, FILE *fp);
void		MAT3print_formatted (MAT3mat, FILE *fp, 
			 	     char *title, char *head, char *format, char *tail);
extern int		MAT3equal();
extern double		MAT3trace();
extern int		MAT3power();
extern int		MAT3column_reduce();
extern int		MAT3kernel_basis();

/* In MAT3vec.c */
void		MAT3mult_vec(MAT3vec result_vec, MAT3vec vec, MAT3mat mat);
int		MAT3mult_hvec (MAT3hvec result_vec, MAT3hvec vec, MAT3mat mat, int normalize);
void		MAT3cross_product(MAT3vec result,MAT3vec,MAT3vec);
void		MAT3perp_vec(MAT3vec result_vec, MAT3vec vec, int is_unit);

#endif /* MAT3_HAS_BEEN_INCLUDED */

