/**************************************************************************
 * tkglkey.c -- handle tkgl keyboard events
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include <stdio.h>

#include <GL/glut.h>

#include "GLUTkey.h"
#include "../aircraft/aircraft.h"

extern double fogDensity;

/* Handle keyboard events */
void GLUTkey(unsigned char k, int x, int y) {
    struct control_params *c;

    c = &current_aircraft.controls;

    printf("Key hit = %d\n", k);

    switch (k) {
    case GLUT_KEY_UP:
	c->elev -= 0.1;
	return;
    case GLUT_KEY_DOWN:
	c->elev += 0.1;
	return;
    case GLUT_KEY_LEFT:
	c->aileron += 0.01;
	return;
    case GLUT_KEY_RIGHT:
	c->aileron -= 0.01;
	return;
    case 1 /* TK_END */:
	c->rudder -= 0.01;
	return;
    case 2 /* TK_PGDWN */:
	c->rudder += 0.01;
	return;
    case 3:
	c->throttle[0] -= 0.05;
	return;
    case 4:
	c->throttle[0] += 0.05;
	return;
    case 122:
	fogDensity *= 1.10;
	glFogf(GL_FOG_DENSITY, fogDensity);
	printf("Fog density = %.4f\n", fogDensity);
	return;
    case 90:
	fogDensity /= 1.10;
	glFogf(GL_FOG_DENSITY, fogDensity);
	printf("Fog density = %.4f\n", fogDensity);
	return;
    case 27: /* ESC */
	exit(0);
    }

}


/* $Log$
/* Revision 1.2  1997/05/23 00:35:12  curt
/* Trying to get fog to work ...
/*
 * Revision 1.1  1997/05/21 15:57:50  curt
 * Renamed due to added GLUT support.
 *
 * Revision 1.2  1997/05/19 18:22:41  curt
 * Parameter tweaking ... starting to stub in fog support.
 *
 * Revision 1.1  1997/05/16 16:05:51  curt
 * Initial revision.
 *
 */
