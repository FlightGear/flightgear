// splash.hxx -- draws the initial splash screen
//
// Written by Curtis Olson, started July 1998.  (With a little looking
// at Freidemann's panel code.) :-)
//
// Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
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
//
// $Id$


#ifndef _SPLASH_HXX
#define _SPLASH_HXX

#include <osg/Node>

#ifndef __cplusplus
# error This library requires C++
#endif

// Initialize the splash screen
void fgSplashInit ();

// Set progress information
void fgSplashProgress ( const char *text );

// Retrieve the splash screen node ...
osg::Node* fgCreateSplashNode();

#endif // _SPLASH_HXX


