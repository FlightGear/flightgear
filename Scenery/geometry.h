/**************************************************************************
 * geometry.h -- data structures and routines for handling vrml geometry
 *
 * Written by Curtis Olson, started June 1997.
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


#ifndef GEOMETRY_H
#define GEOMETRY_H


#define VRML_BOX       0
#define VRML_CYLINDER  1
#define VRML_CONE      2
#define VRML_ELEV_GRID 3


/* Begin a new vrml geometry statement */
int vrmlInitGeometry(char *type);

/* Set current vrml geometry option name */
void vrmlGeomOptionName(char *name);

/* Set current vrml geometry value */
void vrmlGeomOptionsValue(char *value);

/* We've finished parsing the current geometry.  Now do whatever needs
 * to be done with it (like generating the OpenGL call list for
 * instance */
void vrmlHandleGeometry( void );

/* Finish up the current vrml geometry statement */
int vrmlFreeGeometry( void );


#endif /* GEOMETRY_H */


/* $Log$
/* Revision 1.3  1998/01/19 18:40:36  curt
/* Tons of little changes to clean up the code and to remove fatal errors
/* when building with the c++ compiler.
/*
 * Revision 1.2  1997/07/23 21:52:25  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.1  1997/06/29 21:16:48  curt
 * More twiddling with the Scenery Management system.
 *
 * Revision 1.1  1997/06/22 21:42:35  curt
 * Initial revision of VRML (subset) parser.
 *
 */
