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


/* Load a .obj file */
void obj_fix(char *infile, char *outfile);


#endif /* OBJ_H */


/* $Log$
/* Revision 1.4  1998/03/03 15:36:13  curt
/* Tweaks for compiling with g++
/*
 * Revision 1.3  1998/01/31 00:41:25  curt
 * Made a few changes converting floats to doubles.
 *
 * Revision 1.2  1998/01/09 23:03:13  curt
 * Restructured to split 1deg x 1deg dem's into 64 subsections.
 *
 * Revision 1.1  1997/12/08 19:28:55  curt
 * Initial revision.
 *
 */
