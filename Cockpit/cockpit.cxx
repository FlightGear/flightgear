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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H          
#  include <windows.h>
#endif

#include <GL/glut.h>

#include <stdlib.h>

#include <Include/fg_constants.h>

#include <Aircraft/aircraft.h>
#include <Debug/fg_debug.h>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar3d.h>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>
#include <Weather/weather.h>

#include "cockpit.hxx"

// This is a structure that contains all data related to
// cockpit/panel/hud system

static fgCOCKPIT *aircraft_cockpit;

fgCOCKPIT *fgCockpitInit( fgAIRCRAFT *cur_aircraft )
{
	fgCOCKPIT *cockpit;
	Hptr hud;

	fgPrintf( FG_COCKPIT, FG_INFO, "Initializing cockpit subsystem\n");

	cockpit = (fgCOCKPIT *)calloc(sizeof(fgCOCKPIT),1);
	if( cockpit == NULL )
		return( NULL );

	cockpit->code = 1;	/* It will be aircraft dependent */
	cockpit->status = 0;

	// If aircraft has HUD
	hud = fgHUDInit( cur_aircraft );  // Color no longer in parameter list
	if( hud == NULL )
		return( NULL );

	cockpit->hud = hud;

	aircraft_cockpit = cockpit;

	fgPrintf( FG_COCKPIT, FG_INFO,
		  "  Code %d  Status %d\n", 
		  cockpit->hud->code, cockpit->hud->status );

	return( cockpit );
}

fgCOCKPIT *fgCockpitAddHUD( fgCOCKPIT *cockpit, HUD *hud )
{
	cockpit->hud = hud;
	return(cockpit);
}

void fgCockpitUpdate( void )
{

	fgPrintf( FG_COCKPIT, FG_DEBUG,
		  "Cockpit: code %d   status %d\n", 
		  aircraft_cockpit->code, aircraft_cockpit->status );
	if( aircraft_cockpit->hud != NULL ) {
	    // That is, if the aircraft has a HUD, then draw it.
	    fgUpdateHUD( aircraft_cockpit->hud );
	}
}


/* $Log$
/* Revision 1.4  1998/05/03 00:46:45  curt
/* polar.h -> polar3d.h
/*
 * Revision 1.3  1998/04/30 12:36:02  curt
 * C++-ifying a couple source files.
 *
 * Revision 1.2  1998/04/25 22:06:26  curt
 * Edited cvs log messages in source files ... bad bad bad!
 *
 * Revision 1.1  1998/04/24 00:45:54  curt
 * C++-ifing the code a bit.
 *
 * Revision 1.13  1998/04/18 04:14:01  curt
 * Moved fg_debug.c to it's own library.
 *
 * Revision 1.12  1998/04/14 02:23:09  curt
 * Code reorganizations.  Added a Lib/ directory for more general libraries.
 *
 * Revision 1.11  1998/03/14 00:32:13  curt
 * Changed a printf() to a fgPrintf().
 *
 * Revision 1.10  1998/02/07 15:29:33  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.9  1998/02/03 23:20:14  curt
 * Lots of little tweaks to fix various consistency problems discovered by
 * Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
 * passed arguments along to the real printf().  Also incorporated HUD changes
 * by Michele America.
 *
 * Revision 1.8  1998/01/31 00:43:03  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.7  1998/01/27 00:47:51  curt
 * Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.6  1998/01/19 19:27:01  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.5  1998/01/19 18:40:19  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.4  1997/12/30 20:47:34  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.3  1997/12/15 23:54:33  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.2  1997/12/10 22:37:38  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.1  1997/08/29 18:03:20  curt
 * Initial revision.
 *
 */
