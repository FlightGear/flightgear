// keyboard.hxx -- handle GLUT keyboard events
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


#ifndef _KEYBOARD_HXX
#define _KEYBOARD_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <GL/glut.h>
#include <GL/gl.h>


// Handle keyboard events
void GLUTkey(unsigned char k, int x, int y);
void GLUTkeyup(unsigned char k, int x, int y);
void GLUTspecialkey(int k, int x, int y);
void GLUTspecialkeyup(int k, int x, int y);


#endif // _KEYBOARD_HXX


