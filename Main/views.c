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


#include <Main/views.h>

#include <Include/fg_constants.h>

#include <Flight/flight.h>
#include <Math/mat3.h>
#include <Math/polar.h>
#include <Math/vector.h>
#include <Scenery/scenery.h>
#include <Time/fg_time.h>
#include <Main/fg_debug.h>

/* This is a record containing current view parameters */
struct fgVIEW current_view;


/* Initialize a view structure */
void fgViewInit(struct fgVIEW *v) {
    fgPrintf( FG_VIEW, FG_INFO, "Initializing View parameters\n");

    v->view_offset = 0.0;
    v->goal_view_offset = 0.0;
}


/* Update the view parameters */
void fgViewUpdate(struct fgFLIGHT *f, struct fgVIEW *v, struct fgLIGHT *l) {
    MAT3vec vec, forward, v0, minus_z;
    MAT3mat R, TMP, UP, LOCAL, VIEW;
    double ntmp;

    /* calculate the cartesion coords of the current lat/lon/0 elev */
    v->cur_zero_elev = fgPolarToCart(FG_Longitude, FG_Lat_geocentric, 
				     FG_Sea_level_radius * FEET_TO_METER);
    v->cur_zero_elev.x -= scenery.center.x;
    v->cur_zero_elev.y -= scenery.center.y;
    v->cur_zero_elev.z -= scenery.center.z;

    /* calculate view position in current FG view coordinate system */
    v->abs_view_pos = fgPolarToCart(FG_Longitude, FG_Lat_geocentric, 
			     FG_Radius_to_vehicle * FEET_TO_METER + 1.0);
    v->view_pos.x = v->abs_view_pos.x - scenery.center.x;
    v->view_pos.y = v->abs_view_pos.y - scenery.center.y;
    v->view_pos.z = v->abs_view_pos.z - scenery.center.z;

    printf( "View pos = %.4f, %.4f, %.4f\n", 
	   v->abs_view_pos.x, v->abs_view_pos.y, v->abs_view_pos.z);
    fgPrintf( FG_VIEW, FG_DEBUG, "View pos = %.4f, %.4f, %.4f\n", 
	   v->view_pos.x, v->view_pos.y, v->view_pos.z);

    /* make a vector to the current view position */
    MAT3_SET_VEC(v0, v->view_pos.x, v->view_pos.y, v->view_pos.z);

    /* calculate vector to sun's position on the earth's surface */
    v->to_sun[0] = l->fg_sunpos.x - (v->view_pos.x + scenery.center.x);
    v->to_sun[1] = l->fg_sunpos.y - (v->view_pos.y + scenery.center.y);
    v->to_sun[2] = l->fg_sunpos.z - (v->view_pos.z + scenery.center.z);
    /* printf("Vector to sun = %.2f %.2f %.2f\n",
	   v->to_sun[0], v->to_sun[1], v->to_sun[2]); */

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

    /* printf("Local Up = (%.4f, %.4f, %.4f)\n",
	   v->local_up[0], v->local_up[1], v->local_up[2]); */
    
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
    /* printf("Forward vector is (%.2f,%.2f,%.2f)\n", forward[0], forward[1], 
	   forward[2]); */

    MAT3rotate(TMP, v->view_up, v->view_offset);
    MAT3mult_vec(v->view_forward, forward, TMP);

    /* Given a vector from the view position to the point on the
     * earth's surface the sun is directly over, map into onto the
     * local plane representing "horizontal". */
    map_vec_onto_cur_surface_plane(v->local_up, v0, v->to_sun, 
				   v->surface_to_sun);
    MAT3_NORMALIZE_VEC(v->surface_to_sun, ntmp);
    /* printf("Surface direction to sun is %.2f %.2f %.2f\n",
	   v->surface_to_sun[0], v->surface_to_sun[1], v->surface_to_sun[2]); */
    /* printf("Should be close to zero = %.2f\n", 
	   MAT3_DOT_PRODUCT(v->local_up, v->surface_to_sun)); */

    /* Given a vector pointing straight down (-Z), map into onto the
     * local plane representing "horizontal".  This should give us the
     * local direction for moving "south". */
    MAT3_SET_VEC(minus_z, 0.0, 0.0, -1.0);
    map_vec_onto_cur_surface_plane(v->local_up, v0, minus_z, v->surface_south);
    MAT3_NORMALIZE_VEC(v->surface_south, ntmp);
    /* printf("Surface direction directly south %.2f %.2f %.2f\n",
	   v->surface_south[0], v->surface_south[1], v->surface_south[2]); */

    /* now calculate the surface east vector */
    MAT3rotate(TMP, v->view_up, FG_PI_2);
    MAT3mult_vec(v->surface_east, v->surface_south, TMP);
    /* printf("Surface direction directly east %.2f %.2f %.2f\n",
	   v->surface_east[0], v->surface_east[1], v->surface_east[2]); */
    /* printf("Should be close to zero = %.2f\n", 
	   MAT3_DOT_PRODUCT(v->surface_south, v->surface_east)); */
}


/* $Log$
/* Revision 1.12  1998/01/29 00:50:28  curt
/* Added a view record field for absolute x, y, z position.
/*
 * Revision 1.11  1998/01/27 00:47:58  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.10  1998/01/19 19:27:09  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.9  1998/01/13 00:23:09  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.8  1997/12/30 22:22:33  curt
 * Further integration of event manager.
 *
 * Revision 1.7  1997/12/30 20:47:45  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.6  1997/12/22 04:14:32  curt
 * Aligned sky with sun so dusk/dawn effects can be correct relative to the sun.
 *
 * Revision 1.5  1997/12/18 04:07:02  curt
 * Worked on properly translating and positioning the sky dome.
 *
 * Revision 1.4  1997/12/17 23:13:36  curt
 * Began working on rendering a sky.
 *
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
