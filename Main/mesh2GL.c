/**************************************************************************
 * mesh2GL.c -- walk through a mesh data structure and make GL calls
 *
 * Written by Curtis Olson, started May 1997.
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


#include <GL/glut.h>

#include "../constants.h"
#include "../Scenery/mesh.h"
#include "../Scenery/scenery.h"
#include "../Math/mat3.h"
#include "../Math/polar.h"


/* walk through mesh and make ogl calls */
GLint mesh2GL(struct mesh *m) {
    GLint mesh;
    /* static GLfloat color[4] = { 0.5, 0.4, 0.25, 1.0 }; */ /* dark desert */
    static GLfloat color[4] = { 0.5, 0.5, 0.25, 1.0 };

    float x1, y1, x2, y2, z11, z12, z21, z22;
    struct fgCartesianPoint p11, p12, p21, p22;

    MAT3vec v1, v2, normal; 
    int i, j, istep, jstep, iend, jend;
    float temp;

    printf("In mesh2GL(), generating GL call list.\n");

    istep = jstep = cur_scenery_params.terrain_skip ;  /* Detail level */

    /* setup the batch transformation */
    fgRotateBatchInit(-m->originx * ARCSEC_TO_RAD, -m->originy * ARCSEC_TO_RAD);

    mesh = glGenLists(1);
    glNewList(mesh, GL_COMPILE);

    glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color );
	
    iend = m->cols - 1;
    jend = m->rows - 1;
    
    y1 = m->originy;
    y2 = y1 + (m->col_step * istep);
    
    for ( i = 0; i < iend; i += istep ) {
	x1 = m->originx;
	x2 = x1 + (m->row_step * jstep);

	glBegin(GL_TRIANGLE_STRIP);

	for ( j = 0; j < jend; j += jstep ) {
	    p11 = fgGeodetic2Cartesian(x1*ARCSEC_TO_RAD, y1*ARCSEC_TO_RAD);
	    /* printf("A geodetic is (%.2f, %.2f)\n", x1, y1); */
	    /* printf("A cart is (%.8f, %.8f, %.8f)\n", p11.x, p11.y, p11.z); */
	    p11 = fgRotateCartesianPoint(p11);
	    /* printf("A point is (%.8f, %.8f, %.8f)\n", p11.y, p11.z, z11); */

	    p12 = fgGeodetic2Cartesian(x1*ARCSEC_TO_RAD, y2*ARCSEC_TO_RAD);
	    p12 = fgRotateCartesianPoint(p12);

	    p21 = fgGeodetic2Cartesian(x2*ARCSEC_TO_RAD, y1*ARCSEC_TO_RAD);
	    p21 = fgRotateCartesianPoint(p21);

	    p22 = fgGeodetic2Cartesian(x2*ARCSEC_TO_RAD, y2*ARCSEC_TO_RAD);
	    p22 = fgRotateCartesianPoint(p22);

	    z11 = 0.001 * m->mesh_data[j         * m->rows + i        ];
	    z12 = 0.001 * m->mesh_data[j         * m->rows + (i+istep)];
	    z21 = 0.001 * m->mesh_data[(j+jstep) * m->rows + i        ];
	    z22 = 0.001 * m->mesh_data[(j+jstep) * m->rows + (i+istep)];

	    v1[0] = p22.y - p11.y; v1[1] = p22.z - p11.z; v1[2] = z22 - z11;
	    v2[0] = p12.y - p11.y; v2[1] = p12.z - p11.z; v2[2] = z12 - z11;
	    MAT3cross_product(normal, v1, v2);
	    MAT3_NORMALIZE_VEC(normal,temp);
	    glNormal3d(normal[0], normal[1], normal[2]);
	    /* printf("normal 1 = (%.2f %.2f %.2f\n", normal[0], normal[1], 
		   normal[2]); */
	    
	    if ( j == 0 ) {
		/* first time through */
		glVertex3d(p11.y, p11.z, z11);
		glVertex3d(p12.y, p12.z, z12);
	    }
	    
	    glVertex3d(p22.y, p22.z, z22);
    
	    v2[0] = p21.y - p11.y; v2[1] = p21.z - p11.z; v2[2] = z21 - z11;
	    MAT3cross_product(normal, v2, v1);
	    MAT3_NORMALIZE_VEC(normal,temp);
	    glNormal3d(normal[0], normal[1], normal[2]);
	    /* printf("normal 2 = (%.2f %.2f %.2f\n", normal[0], normal[1], 
		   normal[2]); */

	    glVertex3d(p21.y, p21.z, z21);

	    x1 = x2;
	    x2 = x1 + (m->row_step * jstep);
	}
	glEnd();

	y1 = y2;
	y2 = y1 + (m->col_step * istep);
    }

    glEndList();

    return(mesh);
}



/* $Log$
/* Revision 1.29  1997/07/11 01:29:58  curt
/* More tweaking of terrian floor.
/*
 * Revision 1.28  1997/07/10 04:26:37  curt
 * We now can interpolated ground elevation for any position in the grid.  We
 * can use this to enforce a "hard" ground.  We still need to enforce some
 * bounds checking so that we don't try to lookup data points outside the
 * grid data set.
 *
 * Revision 1.27  1997/07/09 21:31:13  curt
 * Working on making the ground "hard."
 *
 * Revision 1.26  1997/07/08 18:20:13  curt
 * Working on establishing a hard ground.
 *
 * Revision 1.25  1997/07/07 20:59:50  curt
 * Working on scenery transformations to enable us to fly fluidly over the
 * poles with no discontinuity/distortion in scenery.
 *
 * Revision 1.24  1997/07/05 20:43:35  curt
 * renamed mat3 directory to Math so we could add other math related routines.
 *
 * Revision 1.23  1997/07/03 00:51:14  curt
 * Playing with terrain color.
 *
 * Revision 1.22  1997/06/29 21:19:17  curt
 * Working on scenery management system.
 *
 * Revision 1.21  1997/06/21 17:12:54  curt
 * Capitalized subdirectory names.
 *
 * Revision 1.20  1997/06/18 04:10:32  curt
 * A couple more runway tweaks ...
 *
 * Revision 1.19  1997/06/18 02:21:24  curt
 * Hacked in a runway
 *
 * Revision 1.18  1997/06/17 04:19:17  curt
 * More timer related tweaks with respect to view direction changes.
 *
 * Revision 1.17  1997/06/16 19:32:52  curt
 * Starting to add general timer support.
 *
 * Revision 1.16  1997/06/02 03:40:07  curt
 * A tiny bit more view tweaking.
 *
 * Revision 1.15  1997/06/02 03:01:38  curt
 * Working on views (side, front, back, transitions, etc.)
 *
 * Revision 1.14  1997/05/31 19:16:26  curt
 * Elevator trim added.
 *
 * Revision 1.13  1997/05/31 04:13:53  curt
 * WE CAN NOW FLY!!!
 *
 * Continuing work on the LaRCsim flight model integration.
 * Added some MSFS-like keyboard input handling.
 *
 * Revision 1.12  1997/05/30 23:26:20  curt
 * Added elevator/aileron controls.
 *
 * Revision 1.11  1997/05/30 19:27:02  curt
 * The LaRCsim flight model is starting to look like it is working.
 *
 * Revision 1.10  1997/05/30 03:54:11  curt
 * Made a bit more progress towards integrating the LaRCsim flight model.
 *
 * Revision 1.9  1997/05/29 22:39:51  curt
 * Working on incorporating the LaRCsim flight model.
 *
 * Revision 1.8  1997/05/29 12:31:40  curt
 * Minor tweaks, moving towards general flight model integration.
 *
 * Revision 1.7  1997/05/29 02:33:24  curt
 * Updated to reflect changing interfaces in other "modules."
 *
 * Revision 1.6  1997/05/27 17:44:32  curt
 * Renamed & rearranged variables and routines.   Added some initial simple
 * timer/alarm routines so the flight model can be updated on a regular 
 * interval.
 *
 * Revision 1.5  1997/05/24 01:45:32  curt
 * Fixed surface normals for triangle mesh.
 *
 * Revision 1.4  1997/05/23 20:05:24  curt
 * First stab at using GL_TRIANGLE_STRIP's instead of GL_POLYGONS (to conserve
 * memory)
 *
 * Revision 1.3  1997/05/23 15:40:26  curt
 * Added GNU copyright headers.
 * Fog now works!
 *
 * Revision 1.2  1997/05/23 00:35:13  curt
 * Trying to get fog to work ...
 *
 * Revision 1.1  1997/05/21 15:57:52  curt
 * Renamed due to added GLUT support.
 *
 * Revision 1.3  1997/05/19 18:22:42  curt
 * Parameter tweaking ... starting to stub in fog support.
 *
 * Revision 1.2  1997/05/17 00:17:35  curt
 * Trying to stub in support for standard OpenGL.
 *
 * Revision 1.1  1997/05/16 16:05:52  curt
 * Initial revision.
 *
 */
