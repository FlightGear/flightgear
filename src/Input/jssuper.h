// jssuper.h -- manage access to multiple joysticks
//
// Written by Tony Peden, started May 2001
//
// Copyright (C) 2001  Tony Peden (apeden@earthlink.net)
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

#ifndef _JSSUPER_H
#define _JSSUPER_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>		// plib/js.h should really include this !!!!!!
#include <plib/js.h>

#define MAX_JOYSTICKS 8

class jsSuper {
  private:
    int activeJoysticks;
    int active[MAX_JOYSTICKS];
    int currentJoystick;
    int first, last;
    jsJoystick* js[MAX_JOYSTICKS];
    
  public:  
    jsSuper(void);
    ~jsSuper(void);
    
    inline int getNumJoysticks(void) { return activeJoysticks; }
    
    inline int atFirst(void) { return currentJoystick == first; }
    inline int atLast(void) { return currentJoystick == last; }
    
    inline void firstJoystick(void) { currentJoystick=first; }
    inline void lastJoystick(void) { currentJoystick=last; }

    int nextJoystick(void);
    int prevJoystick(void);

    inline jsJoystick* getJoystick(int Joystick) 
            { currentJoystick=Joystick; return js[Joystick]; }

    inline jsJoystick* getJoystick(void) { return js[currentJoystick]; }
    
    inline int getCurrentJoystickId(void) { return currentJoystick; }
};
    
#endif    
