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
// (Log is kept at end of this file)


#ifndef _JOYSTICK_HXX
#define _JOYSTICK_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


// Initialize the joystick(s)
int fgJoystickInit( void );

#if defined( ENABLE_LINUX_JOYSTICK )
    // update the control parameters based on joystick intput
    int fgJoystickRead( void );
#endif // ENABLE_LINUX_JOYSTICK


#endif // _JOYSTICK_HXX


// $Log$
// Revision 1.3  1998/10/27 02:14:33  curt
// Changes to support GLUT joystick routines as fall back.
//
// Revision 1.2  1998/10/25 10:56:27  curt
// Completely rewritten to use Steve Baker's joystick interface class.
//
// Revision 1.1  1998/10/24 22:28:18  curt
// Renamed joystick.[ch] to joystick.[ch]xx
// Added js.hxx which is Steve's joystick interface class.
//
// Revision 1.3  1998/04/22 13:26:21  curt
// C++ - ifing the code a bit.
//
// Revision 1.2  1998/01/22 02:59:36  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.1  1997/08/29 18:06:55  curt
// Initial revision.
//

