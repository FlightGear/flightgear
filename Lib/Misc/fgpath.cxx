//
// fgpath.cxx -- routines to abstract out path separator differences
//               between MacOS and the rest of the world
//
// Written by Curtis L. Olson, started April 1999.
//
// Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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


#include "fgpath.hxx"


// If Unix, replace all ":" with "/".  If MacOS, replace all "/" with
// ":" it should go without saying that neither of these characters
// should be used in file or directory names.

static string fix_path( const string path ) {
    string result = path;

    for ( int i = 0; i < (int)path.size(); ++i ) {
	if ( result[i] == FG_BAD_PATH_SEP ) {
	    result[i] = FG_PATH_SEP;
	}
    }

    return result;
}


// default constructor
FGPath::FGPath() {
    path = "";
}


// create a path based on "path"
FGPath::FGPath( const string p ) {
    path = fix_path( p );
}


// destructor
FGPath::~FGPath() {
}


// append to the existing path
void FGPath::append( const string p ) {
    string part = fix_path( p );

    if ( path.size() == 0 ) {
	path = part;
    } else {
	if ( part[0] != FG_PATH_SEP ) {
	    path += FG_PATH_SEP;
	}
	path += part;
    }
}
