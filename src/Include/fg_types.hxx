// fg_types.hxx -- commonly used types I don't want to have to keep redefining
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


#ifndef _CONSTRUCT_TYPES_HXX
#define _CONSTRUCT_TYPES_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include STL_STRING
#include <vector>

#include <Math/point3d.hxx>

FG_USING_STD(vector);
FG_USING_STD(string);


typedef vector < int > int_list;
typedef int_list::iterator int_list_iterator;
typedef int_list::const_iterator const_int_list_iterator;

typedef vector < Point3D > point_list;
typedef point_list::iterator point_list_iterator;
typedef point_list::const_iterator const_point_list_iterator;

typedef vector < string > string_list;
typedef string_list::iterator string_list_iterator;
typedef string_list::const_iterator const_string_list_iterator;


class point2d {
public:
    union {
	double x;
	double dist;
	double lon;
    };
    union {
	double y;
	double theta;
	double lat;
    };
};


#endif // _CONSTRUCT_TYPES_HXX

