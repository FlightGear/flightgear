/**************************************************************************
 * cockpit.h -- cockpit defines and prototypes (initial draft)
 *
 * Written by Michele America, started September 1997.
 *
 * Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
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
 

#include "hud.h"


// And in the future (near future i hope).
// #include "panel.h"

struct COCKPIT {
	int code;
	Hptr hud;
	// As above.
	// PANEL *panel;
	int status;
};

struct COCKPIT *fgCockpitInit( struct AIRCRAFT cur_aircraft );
void fgCockpitUpdate();


/* $Log$
/* Revision 1.1  1997/08/29 18:03:21  curt
/* Initial revision.
/*
 */
