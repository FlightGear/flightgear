/**************************************************************************
 * event.h -- Flight Gear periodic event scheduler
 *
 * Written by Curtis Olson, started December 1997.
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


#define FG_EVENT_SUSP 0
#define FG_EVENT_READY 1
#define FG_EVENT_QUEUED 2


/* Initialize the scheduling subsystem */
void fgEventInit();

/* Register an event with the scheduler, returns a pointer into the
 * event table */
int fgEventRegister(char *desc, void (*event)(), int status, int interval);

/* Update the scheduling parameters for an event */
void fgEventUpdate();

/* Delete a scheduled event */
void fgEventDelete();

/* Temporarily suspend scheduling of an event */
void fgEventSuspend();

/* Resume scheduling and event */
void fgEventResume();

/* Dump scheduling stats */
void fgEventPrintStats();

/* Add pending jobs to the run queue and run the job at the front of
 * the queue */
void fgEventProcess();


/* $Log$
/* Revision 1.1  1997/12/30 04:19:22  curt
/* Initial revision.
/*
 */
