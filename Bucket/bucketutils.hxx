/**************************************************************************
 * bucketutils.hxx -- support routines to handle fgBUCKET operations
 *
 * Written by Bernie Bright, started January 1998.
 *
 * Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef _BUCKETUTILS_HXX
#define _BUCKETUTILS_HXX

#include <Include/compiler.h>

#include STL_STRING

#include "bucketutils.h"

FG_USING_STD(string);

inline bool
operator== ( const fgBUCKET& b1, const fgBUCKET& b2 )
{
    return ( b1.lon == b2.lon &&
	     b1.lat == b2.lat &&
	     b1.x == b2.x &&
	     b1.y == b2.y );
}

inline string
fgBucketGenIndex( const fgBUCKET& p )
{
    char index_str[256];
    sprintf( index_str, "%ld", fgBucketGenIndex( &p ) );
    return string( index_str );
}

inline string
fgBucketGenBasePath( const fgBUCKET& p )
{
    char base_path[256];
    fgBucketGenBasePath( &p, base_path );
    return string( base_path );
}

inline ostream&
operator<< ( ostream& out, const fgBUCKET& b )
{
    return out << b.lon << "," << b.lat << " " << b.x << "," << b.y;
}

#endif /* _BUCKETUTILS_HXX */


// $Log$
// Revision 1.3  1999/03/02 01:01:42  curt
// Tweaks for compiling with native SGI compilers.
//
// Revision 1.2  1999/01/19 20:56:53  curt
// MacOS portability changes contributed by "Robert Puyol" <puyol@abvent.fr>
//
// Revision 1.1  1998/11/09 23:42:12  curt
// Initial revision.
//


