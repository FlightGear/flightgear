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

/* assumes -I/usr/include/mesa in compile command */
#include "gltk.h"

#include "GLTKkey.h"
#include "../aircraft/aircraft.h"


/* Handle keyboard events */
GLenum GLTKkey(int k, GLenum mask) {
    struct control_params *c;

    c = &current_aircraft.controls;

    switch (k) {
    case TK_UP:
	c->elev -= 0.1;
	return GL_TRUE;
    case TK_DOWN:
	c->elev += 0.1;
	return GL_TRUE;
    case TK_LEFT:
	c->aileron += 0.01;
	return GL_TRUE;
    case TK_RIGHT:
	c->aileron -= 0.01;
	return GL_TRUE;
    case 1 /* TK_END */:
	c->rudder -= 0.01;
	return GL_TRUE;
    case 2 /* TK_PGDWN */:
	c->rudder += 0.01;
	return GL_TRUE;
    case TK_a:
	c->throttle[0] -= 0.05;
	return GL_TRUE;
    case TK_s:
	c->throttle[0] += 0.05;
	return GL_TRUE;
    case TK_ESCAPE:
	tkQuit();
    }

    printf("Key hit = %d\n", k);

    return GL_FALSE;
}


/* $Log$
/* Revision 1.3  1997/06/21 17:12:52  curt
/* Capitalized subdirectory names.
/*
 * Revision 1.2  1997/05/23 15:40:24  curt
 * Added GNU copyright headers.
 * Fog now works!
 *
 * Revision 1.1  1997/05/21 15:57:49  curt
 * Renamed due to added GLUT support.
 *
 * Revision 1.2  1997/05/19 18:22:41  curt
 * Parameter tweaking ... starting to stub in fog support.
 *
 * Revision 1.1  1997/05/16 16:05:51  curt
 * Initial revision.
 *
 */
