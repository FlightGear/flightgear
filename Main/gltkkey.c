/**************************************************************************
 * tkglkey.c -- handle tkgl keyboard events
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include <stdio.h>

/* assumes -I/usr/include/mesa in compile command */
#include "gltk.h"

#include "gltkkey.h"
#include "../aircraft/aircraft.h"


/* Handle keyboard events */
GLenum key(int k, GLenum mask) {
    struct control_params *c;

    c = &current_aircraft.controls;

    switch (k) {
    case TK_UP:
	c->elev -= 0.01;
	return GL_TRUE;
    case TK_DOWN:
	c->elev += 0.01;
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
    case TK_ESCAPE:
	tkQuit();
    }

    printf("Key hit = %d\n", k);

    return GL_FALSE;
}


/* $Log$
/* Revision 1.1  1997/05/16 16:05:51  curt
/* Initial revision.
/*
 */
