/**************************************************************************
 * fg_init.h -- Flight Gear top level initialization routines
 *
 * Written by Curtis Olson, started August 1997.
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


#ifndef _FG_INIT_H
#define _FG_INIT_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


// General house keeping initializations
int fgInitGeneral ( void );

// This is the top level init routine which calls all the other
// initialization routines.  If you are adding a subsystem to flight
// gear, its initialization call should located in this routine.
int fgInitSubsystems( void );


#ifdef __cplusplus
}
#endif


#endif /* _FG_INIT_H */


/* $Log$
/* Revision 1.4  1998/04/21 17:02:41  curt
/* Prepairing for C++ integration.
/*
 * Revision 1.3  1998/02/12 21:59:50  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.2  1998/01/22 02:59:38  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.1  1997/08/23 01:46:20  curt
 * Initial revision.
 *
 */
