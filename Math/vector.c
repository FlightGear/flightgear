/**************************************************************************
 * vector.c -- additional vector routines
 *
 * Written by Curtis Olson, started December 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include <math.h>
#include <stdio.h>

#include "vector.h"

#include "../Math/mat3.h"


/* Map a vector onto the plane specified by normal */
void map_vec_onto_cur_surface_plane(MAT3vec normal, MAT3vec v0, MAT3vec vec,
				    MAT3vec result)
{
    MAT3vec u1, v, tmp;

    /* calculate a vector "u1" representing the shortest distance from
     * the plane specified by normal and v0 to a point specified by
     * "vec".  "u1" represents both the direction and magnitude of
     * this desired distance. */

    /* u1 = ( (normal <dot> vec) / (normal <dot> normal) ) * normal */

    MAT3_SCALE_VEC( u1,
		    normal,
		    ( MAT3_DOT_PRODUCT(normal, vec) /
		      MAT3_DOT_PRODUCT(normal, normal)
		      )
		    );

    /*
    printf("  vec = %.2f, %.2f, %.2f\n", vec[0], vec[1], vec[2]);
    printf("  v0 = %.2f, %.2f, %.2f\n", v0[0], v0[1], v0[2]);
    printf("  u1 = %.2f, %.2f, %.2f\n", u1[0], u1[1], u1[2]);
    */

    /* calculate the vector "v" which is the vector "vec" mapped onto
       the plane specified by "normal" and "v0". */

    /* v = v0 + vec - u1 */

    MAT3_ADD_VEC(tmp, v0, vec);
    MAT3_SUB_VEC(v, tmp, u1);
    /* printf("  v = %.2f, %.2f, %.2f\n", v[0], v[1], v[2]); */

    /* Calculate the vector "result" which is "v" - "v0" which is a
     * directional vector pointing from v0 towards v */

    /* result = v - v0 */

    MAT3_SUB_VEC(result, v, v0);
    /* printf("  result = %.2f, %.2f, %.2f\n", 
       result[0], result[1], result[2]); */
}

/* $Log$
/* Revision 1.1  1997/12/22 04:13:17  curt
/* Initial revision.
/*
 */





