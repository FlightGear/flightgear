/**************************************************************************
 * gui.h
 *
 * Written 1998 by Durk Talsma, started Juni, 1998.  For the flight gear
 * project.
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
 **************************************************************************/


#ifndef _GUI_H_
#define _GUI_H_

#include <plib/pu.h>

extern void guiInit();
extern void guiMotionFunc ( int x, int y );
extern void guiMouseFunc(int button, int updown, int x, int y);
extern void maybeToggleMouse( void );
extern void BusyCursor( int restore );

extern void guiToggleMenu(void);
extern void mkDialog(char *txt);
extern void ConfirmExitDialog(void);
extern void guiFixPanel( void );

extern puFont guiFnt;
extern fntTexFont *guiFntHandle;


// MACROS TO HELP KEEP PUI LIVE INTERFACE STACK IN SYNC
// These insure that the mouse is active when dialog is shown
// and try to the maintain the original mouse state when hidden
// These will also repair any damage done to the Panel if active

// Activate Dialog Box
#define FG_PUSH_PUI_DIALOG( X ) \
    maybeToggleMouse(); \
    puPushGroup( (X) ) ; \
    puPushLiveInterface( (X) ) ; \
    ( X )->reveal()

// Deactivate Dialog Box
#define FG_POP_PUI_DIALOG( X ) \
    (X)->hide(); \
    puPopLiveInterface(); \
    puPopGroup(); \
    guiFixPanel(); \
    maybeToggleMouse();

// Finalize Dialog Box Construction 
#define FG_FINALIZE_PUI_DIALOG( X ) \
    ( X )->close(); \
    ( X )->hide(); \
    puPopLiveInterface();
            
#endif // _GUI_H_
