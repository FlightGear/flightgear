/**************************************************************************
 * parsevrml.h -- top level include file for accessing the vrml parser
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


#ifndef PARSEVRML_H
#define PARSEVRML_H


/* parse a VRML scenery file */
int fgParseVRML(char *file);


#endif /* PARSEVRML_H */


/* $Log$
/* Revision 1.2  1997/07/23 21:52:26  curt
/* Put comments around the text after an #endif for increased portability.
/*
 * Revision 1.1  1997/06/29 21:16:49  curt
 * More twiddling with the Scenery Management system.
 *
 * Revision 1.1  1997/06/27 02:25:13  curt
 * Initial revision.
 *
 */
