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
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <math.h>

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <Math/fg_random.h>
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
    FGState *f;

    f = current_aircraft.fdm_state;

    // Add some random turbulence
    // f->set_U_gust( fg_random() * 5.0 - 2.5 );
    // f->set_V_gust( fg_random() * 5.0 - 2.5 );
    // f->set_W_gust( fg_random() * 5.0 - 2.5 );
}


// $Log$
// Revision 1.5  1998/12/06 13:51:26  curt
// Turned "struct fgWEATHER" into "class FGWeather".
//
// Revision 1.4  1998/12/05 15:54:31  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.3  1998/11/23 21:49:11  curt
// Borland portability tweaks.
//
// Revision 1.2  1998/11/06 21:18:29  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.1  1998/10/17 01:34:36  curt
// C++ ifying ...
//
// Revision 1.18  1998/10/02 12:46:50  curt
// Added an "auto throttle"
//
// Revision 1.17  1998/07/20 12:51:57  curt
// Default visibility to about 20 miles.
//
// Revision 1.16  1998/06/12 01:00:59  curt
// Build only static libraries.
// Declare memmove/memset for Sloaris.
// Added support for exponetial fog, which solves for the proper density to
// achieve the desired visibility range.
//
// Revision 1.15  1998/04/25 22:06:34  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.14  1998/02/09 15:07:54  curt
// Minor tweaks.
//
// Revision 1.13  1998/01/27 00:48:08  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.12  1998/01/19 19:27:22  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.11  1998/01/19 18:40:41  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.10  1997/12/30 22:22:46  curt
// Further integration of event manager.
//
// Revision 1.9  1997/12/30 20:48:03  curt
// Integrated new event manager with subsystem initializations.
//
// Revision 1.8  1997/12/11 04:43:58  curt
// Fixed sun vector and lighting problems.  I thing the moon is now lit
// correctly.
//
// Revision 1.7  1997/12/10 22:37:56  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.6  1997/08/27 03:30:38  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.5  1997/08/22 21:34:42  curt
// Doing a bit of reorganizing and house cleaning.
//
// Revision 1.4  1997/08/02 16:23:55  curt
// Misc. tweaks.
//
// Revision 1.3  1997/07/31 22:52:41  curt
// Working on redoing internal coordinate systems & scenery transformations.
//
// Revision 1.2  1997/07/30 16:12:44  curt
// Moved fg_random routines from Util/ to Math/
//
// Revision 1.1  1997/07/19 23:03:57  curt
// Initial revision.
//

