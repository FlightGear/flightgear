/**************************************************************************
 * ls_interface.h -- interface to the "LaRCsim" flight model
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


#ifndef LS_INTERFACE_H
#define LS_INTERFACE_H


/* reset flight params to a specific position */ 
int fgLaRCsimInit(double dt);

/* update position based on inputs, positions, velocities, etc. */
int fgLaRCsimUpdate();


#endif LS_INTERFACE_H


/* $Log$
/* Revision 1.1  1997/05/29 00:09:58  curt
/* Initial Flight Gear revision.
/*
 */
