/**************************************************************************
 * obj.h -- routines to handle WaveFront .obj format files.
 *
 * Written by Curtis Olson, started October 1997.
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


#ifndef OBJ_H
#define OBJ_H


#define MAXNODES 100000

extern float nodes[MAXNODES][3];
extern float normals[MAXNODES][3];
extern int stack[MAXNODES];


/* Load a .obj file */
void obj_fix(char *basename);


#endif /* OBJ_H */


/* $Log$
/* Revision 1.1  1997/12/08 19:28:55  curt
/* Initial revision.
/*
 */
