// genfans.hxx -- Combine individual triangles into more optimal fans.
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


#ifndef _GENFANS_HXX
#define _GENFANS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <vector>

#include <Main/construct_types.hxx>
#include <Triangulate/trieles.hxx>

FG_USING_STD(vector);


typedef vector < int_list > fan_list;
typedef fan_list::iterator fan_list_iterator;
typedef fan_list::const_iterator const_fan_list_iterator;

typedef vector < int_list > reverse_list;
typedef reverse_list::iterator reverse_list_iterator;
typedef reverse_list::const_iterator const_reverse_list_iterator;



class FGGenFans {

private:

    fan_list fans;

    // make sure the list is expanded at least to hold "n" and then
    // push "i" onto the back of the "n" list.
    void add_and_expand( reverse_list& by_node, int n, int i );

public:

    // Constructor && Destructor
    inline FGGenFans() { }
    inline ~FGGenFans() { }

    // recursive build fans from triangle list
    // fan_list greedy_build( triele_list tris );
    fan_list greedy_build( triele_list tris );

    // report average fan size
    double ave_size();
};


#endif // _GENFANS_HXX


// $Log$
// Revision 1.1  1999/03/29 13:08:35  curt
// Initial revision.
//
