/**************************************************************************
 * tilemgr.h -- routines to handle dynamic management of scenery tiles
 *
 * Written by Curtis Olson, started January 1998.
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


#ifndef _TILEMGR_H
#define _TILEMGR_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


/* Initialize the Tile Manager subsystem */
int fgTileMgrInit( void );


/* given the current lon/lat, fill in the array of local chunks.  If
 * the chunk isn't already in the cache, then read it from disk. */
int fgTileMgrUpdate( void );


/* Render the local tiles --- hack, hack, hack */
void fgTileMgrRender( void );


#ifdef __cplusplus
}
#endif


#endif /* _TILEMGR_H */


/* $Log$
/* Revision 1.6  1998/04/21 17:02:45  curt
/* Prepairing for C++ integration.
/*
 * Revision 1.5  1998/02/12 21:59:53  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.4  1998/01/22 02:59:42  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.3  1998/01/19 18:40:38  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.2  1998/01/13 00:23:11  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.1  1998/01/07 23:50:51  curt
 * "area" renamed to "tile"
 *
 * */


