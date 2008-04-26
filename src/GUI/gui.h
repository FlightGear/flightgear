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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 **************************************************************************/


#ifndef _GUI_H_
#define _GUI_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <plib/pu.h>

#include <simgear/structure/exception.hxx>

#define TR_HIRES_SNAP   1


// gui.cxx
extern void guiInit();
extern void mkDialog(const char *txt);
extern void guiErrorMessage(const char *txt);
extern void guiErrorMessage(const char *txt, const sg_throwable &throwable);

extern bool fgDumpSnapShot();
extern void fgDumpSceneGraph();
extern void fgDumpTerrainBranch();

extern puFont guiFnt;
extern fntTexFont *guiFntHandle;
extern int gui_menu_on;

// from gui_funcs.cxx
extern void reInit(puObject *);
extern void fgDumpSnapShotWrapper(puObject *);
#ifdef TR_HIRES_SNAP
extern void fgHiResDumpWrapper(puObject *);
extern void fgHiResDump();
#endif
#if defined( WIN32 ) && !defined( __CYGWIN__) && !defined(__MINGW32__)
extern void printScreen(puObject *);
#endif
extern void helpCb(puObject *);

typedef struct {
        const char *name;
        void (*fn)(puObject *);
} __fg_gui_fn_t;
extern const __fg_gui_fn_t __fg_gui_fn[];

// GLOBAL COMMON DIALOG BOX TEXT STRINGS
extern const char *gui_msg_OK;     // "OK"
extern const char *gui_msg_NO;     // "NO"
extern const char *gui_msg_YES;    // "YES"
extern const char *gui_msg_CANCEL; // "CANCEL"
extern const char *gui_msg_RESET;  // "RESET"

// mouse.cxx
extern void guiInitMouse(int width, int height);
extern void maybeToggleMouse( void );
extern void TurnCursorOn( void );
extern void TurnCursorOff( void );

// MACROS TO HELP KEEP PUI LIVE INTERFACE STACK IN SYNC
// These insure that the mouse is active when dialog is shown
// and try to the maintain the original mouse state when hidden
// These will also repair any damage done to the Panel if active

// Activate Dialog Box
inline void FG_PUSH_PUI_DIALOG( puObject *X ) {
    maybeToggleMouse(); 
    puPushLiveInterface( (puInterface *)X ) ; 
    X->reveal() ;
}

// Deactivate Dialog Box
inline void FG_POP_PUI_DIALOG( puObject *X ) {
    X->hide(); 
    puPopLiveInterface(); 
    maybeToggleMouse();
}

// Finalize Dialog Box Construction 
inline void FG_FINALIZE_PUI_DIALOG( puObject *X ) {
    ((puGroup *)X)->close();
    X->hide();
    puPopLiveInterface();
}
            
#endif // _GUI_H_
