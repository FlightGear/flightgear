/**************************************************************************
 * fg_timer.h -- time handling routines
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


#ifndef FG_TIMER_H
#define FG_TIMER_H


extern unsigned long int fgSimTime;

/* this routine initializes the interval timer to generate a SIGALRM
 * after the specified interval (dt) the function f() will be called
 * at each signal */
void fgTimerInit( float dt, void (*f)( void ) );

/* This function returns the number of milleseconds since the last
   time it was called. */
int fgGetTimeInterval( void );


#endif /* FG_TIMER_H */


/* $Log$
/* Revision 1.3  1998/01/19 18:40:40  curt
/* Tons of little changes to clean up the code and to remove fatal errors
/* when building with the c++ compiler.
/*
 * Revision 1.2  1997/07/23 21:52:27  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.1  1997/06/16 19:24:20  curt
 * Initial revision.
 *
 */
