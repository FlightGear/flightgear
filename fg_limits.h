/**************************************************************************
 * limits.h -- program wide limits
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


#ifndef _LIMITS_H
#define _LIMITS_H


/* Maximum number of engines for a single aircraft */
#define FG_MAX_ENGINES 10

#define MAXPATH 80  // Arbitrary limit on path names to make
                    // a limit on pathways supported.
                    
#endif /* _LIMITS_H */


/* $Log$
/* Revision 1.2  1998/02/12 21:59:43  curt
/* Incorporated code changes contributed by Charlie Hotchkiss
/* <chotchkiss@namg.us.anritsu.com>
/*
 * Revision 1.1  1998/01/27 00:46:51  curt
 * prepended "fg_" on the front of these to avoid potential conflicts with
 * system include files.
 *
 * Revision 1.2  1998/01/22 02:59:35  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.1  1997/12/15 21:02:16  curt
 * Moved to .../FlightGear/Src/Include/
 *
 * Revision 1.4  1997/07/23 21:52:11  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.3  1997/05/31 19:16:24  curt
 * Elevator trim added.
 *
 * Revision 1.2  1997/05/27 17:48:10  curt
 * Added GNU copyright.
 *
 * Revision 1.1  1997/05/16 16:08:00  curt
 * Initial revision.
 *
 */
