/**************************************************************************
 * tkglkey.h -- handle tkgl keyboard events
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef TKGLKEY_H
#define TKGLKEY_H


/* assumes -I/usr/include/mesa in compile command */
#include "gltk.h"


/* Handle keyboard events */
GLenum key(int k, GLenum mask);


#endif TKGLKEY_H


/* $Log$
/* Revision 1.1  1997/05/16 16:05:53  curt
/* Initial revision.
/*
 */
