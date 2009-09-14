// joystick.cxx -- joystick support
//
// Written by Curtis Olson, started October 1998.
//
// Copyright (C) 1998  Curtis L. Olson - curt@me.umn.edu
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


#ifndef _JOYSTICK_HXX
#define _JOYSTICK_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


// Initialize the joystick(s)
int fgJoystickInit( void );

#if defined( ENABLE_PLIB_JOYSTICK )
    // update the control parameters based on joystick intput
    int fgJoystickRead( void );
#endif // ENABLE_PLIB_JOYSTICK


#endif // _JOYSTICK_HXX


