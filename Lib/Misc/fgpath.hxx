//
// fgpath.hxx -- routines to abstract out path separator differences
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


#ifndef _FGPATH_HXX
#define _FGPATH_HXX


#include <Include/compiler.h>

#include STL_STRING

FG_USING_STD(string);


#ifdef MACOS
#  define FG_PATH_SEP ':'
#  define FG_BAD_PATH_SEP '/'
#else
#  define FG_PATH_SEP '/'
#  define FG_BAD_PATH_SEP ':'
#endif


class FGPath {

private:

    string path;

public:

    // default constructor
    FGPath();

    // create a path based on "path"
    FGPath( const string p );

    // destructor
    ~FGPath();

    // set path
    void set( const string p );

    // append another piece to the existing path
    void append( const string p );

    // concatenate a string to the end of the path without inserting a
    // path separator
    void concat( const string p );

    // get the path string
    inline string str() const { return path; }
    inline const char *c_str() { return path.c_str(); }
};


#endif // _FGPATH_HXX


