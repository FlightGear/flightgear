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
// (Log is kept at end of this file)


#ifndef _WEATHER_HXX
#define _WEATHER_HXX


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
	// in meters
	visibility = v;

        // for GL_FOG_EXP
	fog_exp_density = -log(0.01 / visibility);

	// for GL_FOG_EXP2
	fog_exp2_density = sqrt( -log(0.01) ) / visibility;

	// Set correct opengl fog density
	xglFogf (GL_FOG_DENSITY, fog_exp2_density);

	// FG_LOG( FG_INPUT, FG_DEBUG, "Fog density = " << w->fog_density );
    }
};

extern FGWeather current_weather;


#endif // _WEATHER_HXX


// $Log$
// Revision 1.1  1999/04/05 21:32:48  curt
// Initial revision
//
// Revision 1.2  1998/12/06 13:51:27  curt
// Turned "struct fgWEATHER" into "class FGWeather".
//
// Revision 1.1  1998/10/17 01:34:37  curt
// C++ ifying ...
//
// Revision 1.10  1998/06/12 01:01:00  curt
// Build only static libraries.
// Declare memmove/memset for Sloaris.
// Added support for exponetial fog, which solves for the proper density to
// achieve the desired visibility range.
//
// Revision 1.9  1998/04/21 17:02:46  curt
// Prepairing for C++ integration.
//
// Revision 1.8  1998/01/22 02:59:44  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.7  1998/01/19 18:40:41  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.6  1997/12/30 22:22:47  curt
// Further integration of event manager.
//
// Revision 1.5  1997/12/10 22:37:56  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.4  1997/08/27 03:30:39  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.3  1997/08/22 21:34:43  curt
// Doing a bit of reorganizing and house cleaning.
//
// Revision 1.2  1997/07/23 21:52:30  curt
// Put comments around the text after an #endif for increased portability.
//
// Revision 1.1  1997/07/19 23:03:58  curt
// Initial revision.
//

