/**************************************************************************
 * fg_debug.h -- Flight Gear debug utility functions
 *
 * Written by Paul Bleisch, started January 1998. 
 *
 * Copyright (C) 1998 Paul Bleisch, pbleisch@acm.org
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
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef _FG_DEBUG_H
#define _FG_DEBUG_H

#include <stdio.h>

/* NB:  To add a dbg_class, add it here, and add it to the structure
   in fg_debug.c
*/
typedef enum {
  FG_NONE    = 0x00000000,

  FG_TERRAIN = 0x00000001,
  FG_ASTRO   = 0x00000002,
  FG_FLIGHT  = 0x00000004,
  FG_INPUT   = 0x00000008,
  FG_GL      = 0x00000010,
  FG_VIEW    = 0x00000020,
  FG_COCKPIT = 0x00000040,
  FG_GENERAL = 0x00000080,
  FG_MATH    = 0x00000100,
  FG_EVENT   = 0x00000200,
  FG_AIRCRAFT= 0x00000400,

  FG_ALL     = 0xFFFFFFFF
} fgDebugClass;

/* NB:  To add a priority, add it here.
*/
typedef enum {
  FG_BULK,	/* For frequent messages */ 
  FG_DEBUG, 	/* Less frequent debug type messages */
  FG_INFO,      /* Informatory messages */
  FG_WARN,	/* Possible impending problem */
  FG_ALERT, 	/* Very possible impending problem */
  FG_EXIT,      /* Problem (no core) */
  FG_ABORT      /* Abandon ship (core) */
} fgDebugPriority;

/* Initialize the debuggin stuff. */
void fgInitDebug( void );

/* fgPrintf

   Expects:
   class      fgDebugClass mask for this message.
   prio       fgDebugPriority of this message.
   fmt        printf like string format
   ...        var args for fmt

   Returns:
   number of items in fmt handled.

   This function works like the standard C library function printf() with
   the addition of message classes and priorities (see fgDebugClasses
   and fgDebugPriorities).  These additions allow us to classify messages
   and disable sets of messages at runtime.  Only messages with a prio
   greater than or equal to fg_DebugPriority and in the current debug class 
   (fg_DebugClass) are printed.
*/
int fgPrintf( fgDebugClass dbg_class, fgDebugPriority prio, char *fmt, ... ); 


/* fgSetDebugLevels()

   Expects:
   dbg_class      Bitmask representing classes to display.
   prio       Minimum priority of messages to display.
*/
void fgSetDebugLevels( fgDebugClass dbg_class, fgDebugPriority prio );

/* fgSetDebugOutput()

   Expects:
   file       A FILE* to a stream to send messages to.  

   It is assumed the file stream is open and writable.  The system
   defaults to stderr.  The current stream is flushed but not
   closed.
*/
void fgSetDebugOutput( FILE *out );


/* fgRegisterDebugCallback

   Expects:
   callback   A function that takes parameters as defined by the 
              fgDebugCallback type.

   Returns:
   a pointer to the previously registered callback (if any)
  
   Install a user defined debug log callback.   This callback is called w
   whenever fgPrintf is called.  The parameters passed to the callback are
   defined above by fgDebugCallback.  outstr is the string that is to be
   printed.  If callback returns nonzero, it is assumed that the message
   was handled fully by the callback and **fgPrintf need do no further 
   processing of the message.**  Only one callback may be installed at a 
   time.
*/
typedef int (*fgDebugCallback)(fgDebugClass, fgDebugPriority, char *outstr);
fgDebugCallback fgRegisterDebugCallback( fgDebugCallback callback );


#endif /* _FG_DEBUG_H */

