//
// options.hxx -- class to handle command line options
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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


#ifndef _OPTIONS_HXX
#define _OPTIONS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#define FG_OPTIONS_OK 0
#define FG_OPTIONS_HELP 1
#define FG_OPTIONS_ERROR 2


class fgOPTIONS {

public:

    // ID of initial starting airport
    char airport_id[5];

    // Features
    int hud_status;    // HUD on/off

    // Rendering options
    int fog;           // Fog enabled/disabled
    int fullscreen;    // Full screen mode enabled/disabled
    int shading;       // shading method, 0 = Flat, 1 = Smooth
    int skyblend;      // Blend sky to haze (using polygons) or just clear
    int textures;      // Textures enabled/disabled
    int wireframe;     // Wireframe mode enabled/disabled

    // Scenery options
    int tile_radius;   // Square radius of rendered tiles.  for instance
                       // if tile_radius = 3 then a 3 x 3 grid of tiles will 
                       // be drawn.  Increase this to see terrain that is 
                       // further away.

    // Time options
    int time_offset;   // Offset true time by this many seconds

    // Constructor
    fgOPTIONS( void );

    // Parse the command line options
    int parse( int argc, char **argv );

    // Print usage message
    void usage ( void );

    // Destructor
    ~fgOPTIONS( void );

};


extern fgOPTIONS current_options;


#endif /* _OPTIONS_HXX */


// $Log$
// Revision 1.6  1998/05/06 03:16:26  curt
// Added an averaged global frame rate counter.
// Added an option to control tile radius.
//
// Revision 1.5  1998/05/03 00:47:32  curt
// Added an option to enable/disable full-screen mode.
//
// Revision 1.4  1998/04/30 12:34:19  curt
// Added command line rendering options:
//   enable/disable fog/haze
//   specify smooth/flat shading
//   disable sky blending and just use a solid color
//   enable wireframe drawing mode
//
// Revision 1.3  1998/04/28 01:20:23  curt
// Type-ified fgTIME and fgVIEW.
// Added a command line option to disable textures.
//
// Revision 1.2  1998/04/25 15:11:13  curt
// Added an command line option to set starting position based on airport ID.
//
// Revision 1.1  1998/04/24 00:49:21  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Trying out some different option parsing code.
// Some code reorganization.
//
//
