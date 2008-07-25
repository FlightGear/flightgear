// fg_os_common.cxx -- common functions for fg_os interface
// implemented as an osgViewer
//
// Copyright (C) 2007  Tim Moore timoore@redhat.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <plib/pu.h>
#include <osg/GraphicsContext>

#include "fg_os.hxx"
#include "globals.hxx"
#include "renderer.hxx"

// fg_os callback registration APIs
//

// Event handling and scene graph update is all handled by a
// manipulator. See FGManipulator.cpp
void fgRegisterIdleHandler(fgIdleHandler func)
{
    globals->get_renderer()->getManipulator()->setIdleHandler(func);
}

void fgRegisterDrawHandler(fgDrawHandler func)
{
    globals->get_renderer()->getManipulator()->setDrawHandler(func);
}

void fgRegisterWindowResizeHandler(fgWindowResizeHandler func)
{
    globals->get_renderer()->getManipulator()->setWindowResizeHandler(func);
}

void fgRegisterKeyHandler(fgKeyHandler func)
{
    globals->get_renderer()->getManipulator()->setKeyHandler(func);
}

void fgRegisterMouseClickHandler(fgMouseClickHandler func)
{
    globals->get_renderer()->getManipulator()->setMouseClickHandler(func);
}

void fgRegisterMouseMotionHandler(fgMouseMotionHandler func)
{
    globals->get_renderer()->getManipulator()->setMouseMotionHandler(func);
}

// Redraw "happens" every frame whether you want it or not.
void fgRequestRedraw()
{
}


