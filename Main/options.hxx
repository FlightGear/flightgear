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

    // HUD on/off
    int hud_status;

    // Offset true time by this many seconds
    int time_offset;

    // Textures enabled/disabled
    int use_textures;

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
