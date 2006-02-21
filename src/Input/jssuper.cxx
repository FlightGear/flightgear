// jssuper.cxx -- manage access to multiple joysticks
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


#include "jssuper.h"


jsSuper::jsSuper(void) {
  int i;

  activeJoysticks=0;
  currentJoystick=0;
  first=-1;
  last=0;
  for ( i = 0; i < MAX_JOYSTICKS; i++ )
      js[i] = new jsJoystick( i );
  
  for ( i = 0; i < MAX_JOYSTICKS; i++ )
  { 
    active[i] = ! ( js[i]->notWorking () );
    if ( active[i] ) {
         activeJoysticks++;
         if(first < 0) {
            first=i;
         }
         last=i;
    }        
  }
  
}  


int jsSuper::nextJoystick(void) { 
  // int i;
  if(!activeJoysticks) return 0;
  if(currentJoystick == last ) return 0; 
  currentJoystick++;
  while(!active[currentJoystick]){ currentJoystick++; };
  return 1;
}    
        
int jsSuper::prevJoystick(void) { 
  // int i;
  if(!activeJoysticks) return 0; 
  if(currentJoystick == first ) return 0; 
  currentJoystick--;
  while(!active[currentJoystick]){ currentJoystick--; };
  return 1;
}    




jsSuper::~jsSuper(void) {
  int i;
  for ( i = 0; i < MAX_JOYSTICKS; i++ )
      delete js[i];
}      
  
    

    
    
