/**************************************************************************
 * tkglkey.c -- handle tkgl keyboard events
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
	c->elev -= 0.01;
	return;
    case GLUT_KEY_DOWN:
	c->elev += 0.01;
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
	glFogf(GL_FOG_END, fogDensity);
	printf("Fog density = %.4f\n", fogDensity);
	return;
    case 90:
	fogDensity /= 1.10;
	glFogf(GL_FOG_END, fogDensity);
	printf("Fog density = %.4f\n", fogDensity);
	return;
    case 27: /* ESC */
	exit(0);
    }

}


/* $Log$
/* Revision 1.5  1997/05/30 23:26:19  curt
/* Added elevator/aileron controls.
/*
 * Revision 1.4  1997/05/27 17:44:31  curt
 * Renamed & rearranged variables and routines.   Added some initial simple
 * timer/alarm routines so the flight model can be updated on a regular interval.
 *
 * Revision 1.3  1997/05/23 15:40:25  curt
 * Added GNU copyright headers.
 * Fog now works!
 *
 * Revision 1.2  1997/05/23 00:35:12  curt
 * Trying to get fog to work ...
 *
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
