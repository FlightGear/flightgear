// keyboard.cxx -- handle GLUT keyboard events
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997 - 1999  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <GL/glut.h>

#include <plib/pu.h>

#include <simgear/compiler.h>

#include <Input/input.hxx>


/**
 * Construct the modifiers.
 */
static inline int get_mods ()
{
  int glut_modifiers = glutGetModifiers();
  int modifiers = 0;

  if (glut_modifiers & GLUT_ACTIVE_SHIFT)
    modifiers |= FGInput::FG_MOD_SHIFT;
  if (glut_modifiers & GLUT_ACTIVE_CTRL)
    modifiers |= FGInput::FG_MOD_CTRL;
  if (glut_modifiers & GLUT_ACTIVE_ALT)
    modifiers |= FGInput::FG_MOD_ALT;

  return modifiers;
}


/**
 * Keyboard event handler for Glut.
 *
 * <p>Pass the value on to the FGInput module unless PUI wants it.</p>
 *
 * @param k The integer value for the key pressed.
 * @param x (unused)
 * @param y (unused)
 */
void GLUTkey(unsigned char k, int x, int y)
{
				// Give PUI a chance to grab it first.
  if (!puKeyboard(k, PU_DOWN))
				// This is problematic; it allows
				// (say) P and [SHIFT]P to be
				// distinguished, but is that a good
				// idea?
    current_input.doKey(k, get_mods(), x, y);
}


/**
 * Special key handler for Glut.
 *
 * <p>Pass the value on to the FGInput module unless PUI wants it.
 * The key value will have 256 added to it.</p>
 *
 * @param k The integer value for the key pressed (will have 256 added
 * to it).
 * @param x (unused)
 * @param y (unused)
 */
void GLUTspecialkey(int k, int x, int y)
{
				// Give PUI a chance to grab it first.
  if (!puKeyboard(k + PU_KEY_GLUT_SPECIAL_OFFSET, PU_DOWN))
    current_input.doKey(k + 256, get_mods(), x, y);
}


// end of keyboard.cxx
