// weather.cxx -- routines to model weather
//
// Written by Curtis Olson, started July 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@me.umn.edu
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
#include <simgear/xgl/xgl.h>

#include <math.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/fg_random.h>

#include <Aircraft/aircraft.hxx>
#include <Weather/weather.hxx>


// This is a record containing current weather info
FGWeather current_weather;


FGWeather::FGWeather() {
}


FGWeather::~FGWeather() {
}


// Initialize the weather modeling subsystem
void FGWeather::Init( ) {
    FG_LOG( FG_GENERAL, FG_INFO, "Initializing weather subsystem");

    // Configure some wind
    // FG_V_north_airmass = 15; // ft/s =~ 10mph

    // set_visibility( 45000.0 );    // in meters
    set_visibility( 32000.0 );       // about 20 miles (in meters)
}


// Update the weather parameters for the current position
void FGWeather::Update( void ) {
    FGInterface *f;

    f = current_aircraft.fdm_state;

    // Add some random turbulence
    // f->set_U_gust( fg_random() * 5.0 - 2.5 );
    // f->set_V_gust( fg_random() * 5.0 - 2.5 );
    // f->set_W_gust( fg_random() * 5.0 - 2.5 );
}


