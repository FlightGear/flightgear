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
 

#ifndef _COCKPIT_H
#define _COCKPIT_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


#include <Cockpit/hud.h>


// And in the future (near future i hope).
// #include <Cockpit/panel.h>

typedef struct  {
	int code;
	Hptr hud;
	// As above.
	// PANEL *panel;
	int status;
}fgCOCKPIT, *pfgCockpit;

fgCOCKPIT *fgCockpitInit( fgAIRCRAFT *cur_aircraft );
void fgCockpitUpdate( void );


#ifdef __cplusplus
}
#endif


#endif /* _COCKPIT_H */


/* $Log$
/* Revision 1.7  1998/04/21 17:02:34  curt
/* Prepairing for C++ integration.
/*
 * Revision 1.6  1998/02/07 15:29:33  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.5  1998/01/22 02:59:29  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.4  1998/01/19 19:27:01  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.3  1998/01/19 18:40:19  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.2  1997/12/10 22:37:39  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.1  1997/08/29 18:03:21  curt
 * Initial revision.
 *
 */
