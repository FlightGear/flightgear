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


#ifdef GLUT
    #include <GL/glut.h>
#elif TIGER
    /* assumes -I/usr/include/mesa in compile command */
    #include "gltk.h"
#endif

#include "../scenery/mesh.h"
#include "../mat3/mat3.h"


/* walk through mesh and make ogl calls */
GLint mesh2GL(struct mesh *m) {
    GLint mesh;

    float x1, y1, x2, y2, z11, z12, z21, z22;
    MAT3vec v1, v2, normal; 
    int i, j, istep, jstep, iend, jend;
    float temp;

    istep = jstep = 30;  /* Detail level 1 -- 1200 ... */

    mesh = glGenLists(1);
    glNewList(mesh, GL_COMPILE);

    iend = m->cols - 1;
    jend = m->rows - 1;
    
    y1 = m->originy;
    y2 = y1 + (m->col_step * istep);
    
    for ( i = 0; i < iend; i += istep ) {
	x1 = m->originx;
	x2 = x1 + (m->row_step * jstep);

	glBegin(GL_TRIANGLE_STRIP);

	for ( j = 0; j < jend; j += jstep ) {
	    z11 = 0.03 * m->mesh_data[j         * m->rows + i        ];
	    z12 = 0.03 * m->mesh_data[j         * m->rows + (i+istep)];
	    z21 = 0.03 * m->mesh_data[(j+jstep) * m->rows + i        ];
	    z22 = 0.03 * m->mesh_data[(j+jstep) * m->rows + (i+istep)];

	    v1[0] = x2 - x1; v1[1] = 0;       v1[2] = z21 - z11;
	    v2[0] = 0;       v2[1] = y2 - y1; v2[2] = z12 - z11;
	    MAT3cross_product(normal, v1, v2);
	    MAT3_NORMALIZE_VEC(normal,temp);
	    glNormal3d(normal[0], normal[1], normal[2]);

	    if ( j == 0 ) {
		/* first time through */
		glVertex3f(x1, y1, z11);
		glVertex3f(x1, y2, z12);
	    }

	    glVertex3f(x2, y1, z21);
	    
	    v1[0] = x2 - x1; v1[1] = y1 - y2; v1[2] = z21 - z12;
	    v2[0] = x2 - x1; v2[1] = 0; v2[2] = z22 - z12;
	    MAT3cross_product(normal, v1, v2);
	    MAT3_NORMALIZE_VEC(normal,temp);
	    glNormal3d(normal[0], normal[1], normal[2]);
	    glVertex3f(x2, y2, z22);

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
/* Revision 1.15  1997/06/02 03:01:38  curt
/* Working on views (side, front, back, transitions, etc.)
/*
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
