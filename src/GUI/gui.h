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

#include <plib/pu.h>

#include <simgear/structure/exception.hxx>

#define TR_HIRES_SNAP   1

namespace osg
{
class GraphicsContext;
}
// gui.cxx
extern void guiStartInit(osg::GraphicsContext*);
extern bool guiFinishInit();
extern bool openBrowser(string address);
extern void mkDialog(const char *txt);
extern void guiErrorMessage(const char *txt);
extern void guiErrorMessage(const char *txt, const sg_throwable &throwable);

extern bool fgDumpSnapShot();
extern void fgDumpSceneGraph();
extern void fgDumpTerrainBranch();
extern void fgPrintVisibleSceneInfoCommand();

extern puFont guiFnt;
extern fntTexFont *guiFntHandle;
extern int gui_menu_on;

// from gui_funcs.cxx
extern void fgDumpSnapShotWrapper();
#ifdef TR_HIRES_SNAP
extern void fgHiResDumpWrapper();
extern void fgHiResDump();
#endif

extern void helpCb();

typedef struct {
        const char *name;
        void (*fn)();
} __fg_gui_fn_t;
extern const __fg_gui_fn_t __fg_gui_fn[];

#endif // _GUI_H_
