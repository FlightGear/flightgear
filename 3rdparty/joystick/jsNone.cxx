/*
     Copyright (C) 1998,2002  Steve Baker

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.

     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

     For further information visit http://plib.sourceforge.net

     $Id: jsNone.cxx 1960 2004-09-21 11:45:55Z smokydiamond $
*/

#include "FlightGear_js.h"

#ifdef __OpenBSD__
  #define TRUE JS_TRUE
#endif

struct os_specific_s ;


void jsJoystick::open ()
{
  error = TRUE ;
  num_axes = num_buttons = 0 ;
}


void jsJoystick::close ()
{
  error = TRUE ;
}


jsJoystick::jsJoystick ( int ident )
{
  error = TRUE ;
  num_axes = num_buttons = 0 ;
  os = NULL;
}


void jsJoystick::rawRead ( int *buttons, float *axes )
{
  if ( buttons != NULL ) *buttons = 0 ;
}

void jsInit ()  {}
