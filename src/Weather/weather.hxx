// weather.hxx -- routines to model weather
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


#ifndef _WEATHER_HXX
#define _WEATHER_HXX


#include <simgear/compiler.h>

#include <simgear/xgl/xgl.h>

#ifdef SG_HAVE_STD_INCLUDES
#  include <cmath>
#else
#  include <math.h>
#endif

// holds the current weather values
class FGWeather {

private:

    double visibility;
    GLfloat fog_exp_density;
    GLfloat fog_exp2_density;

public:

    FGWeather();
    ~FGWeather();

    void Init();
    void Update();
    
    inline double get_visibility() const { return visibility; }

    inline void set_visibility( double v ) {
	xglMatrixMode(GL_MODELVIEW);
	// in meters
	visibility = v;

        // for GL_FOG_EXP
	fog_exp_density = -log(0.01 / visibility);

	// for GL_FOG_EXP2
	fog_exp2_density = sqrt( -log(0.01) ) / visibility;

	// Set correct opengl fog density
	xglFogf (GL_FOG_DENSITY, fog_exp2_density);
	xglFogi( GL_FOG_MODE, GL_EXP2 );

	// SG_LOG( SG_INPUT, SG_DEBUG, "Fog density = " << fog_density );
	// SG_LOG( SG_INPUT, SG_INFO, 
	//     	   "Fog exp2 density = " << fog_exp2_density );
    }
};

extern FGWeather current_weather;


#endif // _WEATHER_HXX


