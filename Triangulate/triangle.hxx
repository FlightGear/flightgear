// triandgle.hxx -- "Triangle" interface class
//
// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _TRIANGLE_HXX
#define _TRIANGLE_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <vector>

#include <Clipper/clipper.hxx>
#include <Math/point3d.hxx>

#include "trinodes.hxx"

FG_USING_STD(vector);


typedef vector < int > tripoly;
typedef tripoly::iterator tripoly_iterator;
typedef tripoly::const_iterator const_tripoly_iterator;

typedef vector < int > tripoly_list;
typedef tripoly_list::iterator tripoly_list_iterator;
typedef tripoly_list::const_iterator const_tripoly_list_iterator;


class FGTriangle {

private:

    FGTriNodes trinodes;
    tripoly_list polylist;

public:

    // Constructor and destructor
    FGTriangle( void );
    ~FGTriangle( void );

    // populate this class based on the specified gpc_polys list
    int build( FGgpcPolyList gpc_polys );
};


#endif // _TRIANGLE_HXX


// $Log$
// Revision 1.1  1999/03/17 23:51:59  curt
// Initial revision.
//
