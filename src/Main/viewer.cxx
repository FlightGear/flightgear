// viewer.cxx -- class for managing a viewer in the flightgear world.
//
// Written by Curtis Olson, started August 1997.
//                          overhaul started October 2000.
//
// Copyright (C) 1997 - 2000  Curtis L. Olson  - curt@flightgear.org
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


#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/constants.h>

#include "viewer.hxx"


// Constructor
FGViewer::FGViewer( void ):
    scalingType(FG_SCALING_MAX),
    fov(55.0),
    view_offset(0.0),
    goal_view_offset(0.0),
    view_tilt(0.0),
    goal_view_tilt(0.0)
{
    sgSetVec3( pilot_offset, 0.0, 0.0, 0.0 );
    sgdZeroVec3(geod_view_pos);
    sgdZeroVec3(abs_view_pos);
    sea_level_radius = SG_EQUATORIAL_RADIUS_M; 
    //a reasonable guess for init, so that the math doesn't blow up
}


// Destructor
FGViewer::~FGViewer( void ) {
}

void
FGViewer::init ()
{
}

void
FGViewer::bind ()
{
}

void
FGViewer::unbind ()
{
}

double
FGViewer::get_h_fov()
{
    switch (scalingType) {
    case FG_SCALING_WIDTH:  // h_fov == fov
	return fov;
    case FG_SCALING_MAX:
	if (aspect_ratio < 1.0) {
	    // h_fov == fov
	    return fov;
	} else {
	    // v_fov == fov
	    return atan(tan(fov/2 * SG_DEGREES_TO_RADIANS) / aspect_ratio) *
		SG_RADIANS_TO_DEGREES * 2;
	}
    default:
	assert(false);
    }
}

double
FGViewer::get_v_fov()
{
    switch (scalingType) {
    case FG_SCALING_WIDTH:  // h_fov == fov
	return atan(tan(fov/2 * SG_DEGREES_TO_RADIANS) * aspect_ratio) *
	    SG_RADIANS_TO_DEGREES * 2;
    case FG_SCALING_MAX:
	if (aspect_ratio < 1.0) {
	    // h_fov == fov
	    return atan(tan(fov/2 * SG_DEGREES_TO_RADIANS) * aspect_ratio) *
		SG_RADIANS_TO_DEGREES * 2;
	} else {
	    // v_fov == fov
	    return fov;
	}
    default:
	assert(false);
    }
}

void
FGViewer::update (int dt)
{
  int i;
  for ( i = 0; i < dt; i++ ) {
    if ( fabs(get_goal_view_offset() - get_view_offset()) < 0.05 ) {
      set_view_offset( get_goal_view_offset() );
      break;
    } else {
      // move current_view.view_offset towards
      // current_view.goal_view_offset
      if ( get_goal_view_offset() > get_view_offset() )
	{
	  if ( get_goal_view_offset() - get_view_offset() < SGD_PI ){
	    inc_view_offset( 0.01 );
	  } else {
	    inc_view_offset( -0.01 );
	  }
	} else {
	  if ( get_view_offset() - get_goal_view_offset() < SGD_PI ){
	    inc_view_offset( -0.01 );
	  } else {
	    inc_view_offset( 0.01 );
	  }
	}
      if ( get_view_offset() > SGD_2PI ) {
	inc_view_offset( -SGD_2PI );
      } else if ( get_view_offset() < 0 ) {
	inc_view_offset( SGD_2PI );
      }
    }
  }

  for ( i = 0; i < dt; i++ ) {
    if ( fabs(get_goal_view_tilt() - get_view_tilt()) < 0.05 ) {
      set_view_tilt( get_goal_view_tilt() );
      break;
    } else {
      // move current_view.view_tilt towards
      // current_view.goal_view_tilt
      if ( get_goal_view_tilt() > get_view_tilt() )
	{
	  if ( get_goal_view_tilt() - get_view_tilt() < SGD_PI ){
	    inc_view_tilt( 0.01 );
	  } else {
	    inc_view_tilt( -0.01 );
	  }
	} else {
	  if ( get_view_tilt() - get_goal_view_tilt() < SGD_PI ){
	    inc_view_tilt( -0.01 );
	  } else {
	    inc_view_tilt( 0.01 );
	  }
	}
      if ( get_view_tilt() > SGD_2PI ) {
	inc_view_tilt( -SGD_2PI );
      } else if ( get_view_tilt() < 0 ) {
	inc_view_tilt( SGD_2PI );
      }
    }
  }
}
