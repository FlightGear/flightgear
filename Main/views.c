/**************************************************************************
 * views.c -- data structures and routines for managing and view parameters.
 *
 * Written by Curtis Olson, started August 1997.
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


#include "views.h"

#include "../Include/constants.h"

#include "../Flight/flight.h"
#include "../Math/mat3.h"
#include "../Math/polar.h"
#include "../Scenery/scenery.h"


/* This is a record containing current view parameters */
struct fgVIEW current_view;


/* Initialize a view structure */
void fgViewInit(struct fgVIEW *v) {
    v->view_offset = 0.0;
    v->goal_view_offset = 0.0;
}


/* Update the view parameters */
void fgViewUpdate(struct fgFLIGHT *f, struct fgVIEW *v) {
    MAT3vec vec, forward;
    MAT3mat R, TMP, UP, LOCAL, VIEW;

    /* calculate the cartesion coords of the current lat/lon/0 elev */
    v->cur_zero_elev = fgPolarToCart(FG_Longitude, FG_Lat_geocentric, 
				     FG_Sea_level_radius);

    /* calculate view position in current FG view coordinate system */
    v->view_pos = fgPolarToCart(FG_Longitude, FG_Lat_geocentric, 
			     FG_Radius_to_vehicle * FEET_TO_METER + 1.0);
    v->view_pos.x -= scenery.center.x;
    v->view_pos.y -= scenery.center.y;
    v->view_pos.z -= scenery.center.z;

    printf("View pos = %.4f, %.4f, %.4f\n", 
	   v->view_pos.x, v->view_pos.y, v->view_pos.z);

    /* Derive the LOCAL aircraft rotation matrix (roll, pitch, yaw) */
    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3rotate(R, vec, FG_Phi);
    /* printf("Roll matrix\n"); */
    /* MAT3print(R, stdout); */

    MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
    /* MAT3mult_vec(vec, vec, R); */
    MAT3rotate(TMP, vec, FG_Theta);
    /* printf("Pitch matrix\n"); */
    /* MAT3print(TMP, stdout); */
    MAT3mult(R, R, TMP);

    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    /* MAT3mult_vec(vec, vec, R); */
    /* MAT3rotate(TMP, vec, FG_Psi - FG_PI_2); */
    MAT3rotate(TMP, vec, -FG_Psi);
    /* printf("Yaw matrix\n");
    MAT3print(TMP, stdout); */
    MAT3mult(LOCAL, R, TMP);
    /* printf("LOCAL matrix\n"); */
    /* MAT3print(LOCAL, stdout); */

    /* Derive the local UP transformation matrix based on *geodetic*
     * coordinates */
    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3rotate(R, vec, FG_Longitude);     /* R = rotate about Z axis */
    /* printf("Longitude matrix\n"); */
    /* MAT3print(R, stdout); */

    MAT3_SET_VEC(vec, 0.0, 1.0, 0.0);
    MAT3mult_vec(vec, vec, R);
    MAT3rotate(TMP, vec, -FG_Latitude);  /* TMP = rotate about X axis */
    /* printf("Latitude matrix\n"); */
    /* MAT3print(TMP, stdout); */

    MAT3mult(UP, R, TMP);
    /* printf("Local up matrix\n"); */
    /* MAT3print(UP, stdout); */

    MAT3_SET_VEC(v->local_up, 1.0, 0.0, 0.0);
    MAT3mult_vec(v->local_up, v->local_up, UP);

    printf("    Local Up = (%.4f, %.4f, %.4f)\n", 
	   v->local_up[0], v->local_up[1], v->local_up[2]);
    
    /* Alternative method to Derive local up vector based on
     * *geodetic* coordinates */
    /* alt_up = fgPolarToCart(FG_Longitude, FG_Latitude, 1.0); */
    /* printf("    Alt Up = (%.4f, %.4f, %.4f)\n", 
       alt_up.x, alt_up.y, alt_up.z); */

    /* Derive the VIEW matrix */
    MAT3mult(VIEW, LOCAL, UP);
    /* printf("VIEW matrix\n"); */
    /* MAT3print(VIEW, stdout); */

    /* generate the current up, forward, and fwrd-view vectors */
    MAT3_SET_VEC(vec, 1.0, 0.0, 0.0);
    MAT3mult_vec(v->view_up, vec, VIEW);

    MAT3_SET_VEC(vec, 0.0, 0.0, 1.0);
    MAT3mult_vec(forward, vec, VIEW);
    printf("Forward vector is (%.2f,%.2f,%.2f)\n", forward[0], forward[1], 
	   forward[2]);

    MAT3rotate(TMP, v->view_up, v->view_offset);
    MAT3mult_vec(v->view_forward, forward, TMP);

}


/* $Log$
/* Revision 1.4  1997/12/17 23:13:36  curt
/* Began working on rendering a sky.
/*
 * Revision 1.3  1997/12/15 23:54:50  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.2  1997/12/10 22:37:48  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.1  1997/08/27 21:31:17  curt
 * Initial revision.
 *
 */
