/**************************************************************************
 * mesh2ogl.c -- walk through a mesh data structure and make ogl calls
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


/* assumes -I/usr/include/mesa in compile command */
#include "gltk.h"

#include "../scenery/mesh.h"
#include "mat3.h"


/* Sets the first vector to be the cross-product of the last two
    vectors. */
static void mat3_cross_product(float result_vec[3], register float vec1[3], 
                       register float vec2[3]) {
   float tempvec[3];
   register float *temp = tempvec;

   temp[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
   temp[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
   temp[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];

   MAT3_COPY_VEC(result_vec, temp);
}


/* walk through mesh and make ogl calls */
GLint mesh_to_ogl(struct mesh *m) {
    GLint mesh;
    static GLfloat color[4] = { 0.3, 0.7, 0.2, 1.0 };

    float x1, y1, x2, y2, z11, z12, z21, z22;
    float v1[3], v2[3], normal[3]; 
    int i, j, istep, jstep, iend, jend;
    float temp;

    istep = jstep = 50;  /* Detail level 1 -- 1200 ... */

    mesh = glGenLists(1);
    glNewList(mesh, GL_COMPILE);
    glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color );
    glShadeModel( GL_FLAT ); /* glShadeModel( GL_SMOOTH ); */

    iend = m->cols - 1;
    jend = m->rows - 1;
    
    y1 = m->originy;
    y2 = y1 + (m->col_step * istep);
    
    for ( i = 0; i < iend; i += istep ) {
	x1 = m->originx;
	x2 = x1 + (m->row_step * jstep);
	for ( j = 0; j < jend; j += jstep ) {
	    z11 = 0.12 * m->mesh_data[j         * m->rows + i        ];
	    z12 = 0.12 * m->mesh_data[j         * m->rows + (i+istep)];
	    z21 = 0.12 * m->mesh_data[(j+jstep) * m->rows + i        ];
	    z22 = 0.12 * m->mesh_data[(j+jstep) * m->rows + (i+istep)];

	    /* printf("x1 = %f  y1 = %f\n", x1, y1);
	    printf("x2 = %f  y2 = %f\n", x2, y2);
	    printf("z11 = %f  z12 = %f  z21 = %f  z22 = %f\n", 
		   z11, z12, z21, z22); */

	    v1[0] = x2 - x1; v1[1] = 0;       v1[2] = z21 - z11;
	    v2[0] = x2 - x1; v2[1] = y2 - y1; v2[2] = z22 - z11;
	    mat3_cross_product(normal, v1, v2);
	    MAT3_NORMALIZE_VEC(normal,temp);
	    glNormal3fv(normal);
	    glBegin(GL_POLYGON);
	    glVertex3f(x1, y1, z11);
	    glVertex3f(x2, y1, z21);
	    glVertex3f(x2, y2, z22);
	    /* printf("(%f, %f, %f)\n", x1, y1, z11);
	    printf("(%f, %f, %f)\n", x2, y1, z21);
	    printf("(%f, %f, %f)\n", x2, y2, z22); */
	    glEnd();
	    
	    v1[0] = x2 - x1; v1[1] = y2 - y1; v1[2] = z22 - z11;
	    v2[0] = 0;       v2[1] = y2 - y1; v2[2] = z12 - z11;
	    mat3_cross_product(normal, v1, v2);
	    MAT3_NORMALIZE_VEC(normal,temp);
	    glNormal3fv(normal);
	    glBegin(GL_POLYGON);
	    glVertex3f(x1, y1, z11);
	    glVertex3f(x2, y2, z22);
	    glVertex3f(x1, y2, z12);
	    /* printf("(%f, %f, %f)\n", x1, y1, z11);
	    printf("(%f, %f, %f)\n", x2, y2, z22);
	    printf("(%f, %f, %f)\n", x1, y2, z12); */
	    glEnd();

	    x1 = x2;
	    x2 = x1 + (m->row_step * jstep);
	}
	y1 = y2;
	y2 = y1 + (m->col_step * istep);
    }

    glEndList();

    return(mesh);
}


/* $Log$
/* Revision 1.1  1997/05/16 16:05:52  curt
/* Initial revision.
/*
 */
