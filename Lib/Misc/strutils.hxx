// String utilities.
//
// Written by Bernie Bright, 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
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

#ifndef STRUTILS_H
#define STRUTILS_H

#include <Include/compiler.h>
#include STL_STRING

#ifdef FG_HAVE_STD_INCLUDES
#  include <cstdlib>
#else
#  include <stdlib.h>
#endif

FG_USING_STD(string);

// Default characters to remove.
extern const string whitespace;

// Returns a string with trailing characters removed.
string trimleft( const string& s, const string& trimmings = whitespace );

// Returns a string with leading characters removed.
string trimright( const string& s, const string& trimmings = whitespace );

// Returns a string with leading and trailing characters removed.
string trim( const string& s, const string& trimmings = whitespace );

//-----------------------------------------------------------------------------

inline double
atof( const string& str )
{
    return ::atof( str.c_str() );
}

inline int
atoi( const string& str )
{
    return ::atoi( str.c_str() );
}

#endif // STRUTILS_H

// $Log$
// Revision 1.1  1999/04/05 21:32:33  curt
// Initial revision
//
// Revision 1.6  1999/03/02 01:01:56  curt
// Tweaks for compiling with native SGI compilers.
//
// Revision 1.5  1999/02/26 22:08:09  curt
// Added initial support for native SGI compilers.
//
// Revision 1.4  1999/01/19 20:41:47  curt
// Portability updates contributed by Bernie Bright.
//
// Revision 1.3  1998/10/16 00:50:57  curt
// Remove leading _ from a couple defines.
//
// Revision 1.2  1998/10/13 00:10:07  curt
// More portability changes to help with windoze compilation problems.
//
// Revision 1.1  1998/09/01 19:06:31  curt
// Initial revision.
//
