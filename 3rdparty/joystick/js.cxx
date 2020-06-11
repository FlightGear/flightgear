/*
     PLIB - A Suite of Portable Game Libraries
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

*/

#include "config.h"

#include "FlightGear_js.h"

#include <simgear/debug/logstream.hxx>

float jsJoystick::fudge_axis ( float value, int axis ) const
{
  if ( value < center[axis] )
  {
    float xx = (      value    - center[ axis ] ) /
               ( center [ axis ] - min [ axis ] ) ;

    if ( xx < -saturate [ axis ] )
                              return -1.0f ;

    if ( xx > -dead_band [ axis ] )
                              return 0.0f ;

    xx = (        xx         + dead_band [ axis ] ) /
         ( saturate [ axis ] - dead_band [ axis ] ) ;

    return ( xx < -1.0f ) ? -1.0f : xx ;
  }
  else
  {
    float xx = (     value    - center [ axis ] ) /
               ( max [ axis ] - center [ axis ] ) ;

    if ( xx > saturate [ axis ] )
                              return 1.0f ;

    if ( xx < dead_band [ axis ] )
                              return 0.0f ;

    xx = (        xx         - dead_band [ axis ] ) /
         ( saturate [ axis ] - dead_band [ axis ] ) ;

    return ( xx > 1.0f ) ? 1.0f : xx ;
  }
}


void jsJoystick::read ( int *buttons, float *axes )
{
  if ( error )
  {
    if ( buttons )
      *buttons = 0 ;

    if ( axes )
      for ( int i = 0 ; i < num_axes ; i++ )
        axes[i] = 0.0f ;

    return ;
  }

  float raw_axes [ _JS_MAX_AXES ] ;

  rawRead ( buttons, raw_axes ) ;

  if ( axes )
    for ( int i = 0 ; i < num_axes ; i++ )
      axes[i] = fudge_axis ( raw_axes[i], i ) ;
}

void jsSetError(int level, const std::string& msg)
{
  SG_LOG(SG_INPUT, static_cast<sgDebugPriority>(level), msg);
}
