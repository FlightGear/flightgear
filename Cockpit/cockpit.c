/**************************************************************************
 * cockpit.c -- routines to draw a cockpit (initial draft)
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
 

#include <GL/glut.h>

#include <stdlib.h>
#include "cockpit.h"

#include "../constants.h"

#include "../Aircraft/aircraft.h"
#include "../Scenery/mesh.h"
#include "../Scenery/scenery.h"
#include "../Math/mat3.h"
#include "../Math/polar.h"
#include "../Time/fg_timer.h"
#include "../Math/fg_random.h"
#include "../Weather/weather.h"

// #define DEBUG

/* This is a structure that contains all data related to cockpit/panel/hud system */
static struct COCKPIT *aircraft_cockpit;

struct COCKPIT *fgCockpitInit( struct AIRCRAFT cur_aircraft )
{
	struct COCKPIT *cockpit;
	Hptr hud;
	
	cockpit = (struct COCKPIT *)calloc(sizeof(struct COCKPIT),1);
	if( cockpit == NULL )
		return( NULL );
		
	cockpit->code = 1234;
	cockpit->status = 0;
	
	/* If aircraft has HUD */
	hud = fgHUDInit( cur_aircraft, 3 );
	if( hud == NULL )
		return( NULL );
	
	cockpit->hud = hud;
	
	aircraft_cockpit = cockpit;
	
	printf( "Code %d  Status %d\n", cockpit->hud->code, cockpit->hud->status );
		
	return( cockpit );
}

struct COCKPIT *fgCockpitAddHUD( struct COCKPIT *cockpit, struct HUD *hud )
{
	cockpit->hud = hud;
}

void fgCockpitUpdate()
{

	printf( "Cockpit: code %d   status %d\n", aircraft_cockpit->code, aircraft_cockpit->status );
	if( aircraft_cockpit->hud != NULL )			// That is, if the aircraft has a HUD,
		fgUpdateHUD( aircraft_cockpit->hud );	// then draw it.
                
}


/* $Log$
/* Revision 1.1  1997/08/29 18:03:20  curt
/* Initial revision.
/*
 */
