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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "strutils.hxx"

const string whitespace = " \n\r\t";

//
string
trimleft( const string& s, const string& trimmings )
{
    string result;
    string::size_type pos = s.find_first_not_of( trimmings );
    if ( pos != string::npos )
    {
        result.assign( s.substr( pos ) );
    }

    return result;
}

//
string
trimright( const string& s, const string& trimmings )
{
    string result;

    string::size_type pos = s.find_last_not_of( trimmings );
    if ( pos == string::npos )
    {
	// Not found, return the original string.
	result = s;
    }
    else
    {
        result.assign( s.substr( 0, pos+1 ) );
    }

    return result;
}

//
string
trim( const string& s, const string& trimmings )
{
    return trimright( trimleft( s, trimmings ), trimmings );
}

