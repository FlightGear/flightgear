// trieles.hxx -- "Triangle" element management class
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


#ifndef _TRIELES_HXX
#define _TRIELES_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <vector>

FG_USING_STD(vector);


// a segment is two integer pointers into the node list
class FGTriEle {
    int n1, n2, n3;

    double attribute;

public:

    // Constructor and destructor
    inline FGTriEle( void ) { };
    inline FGTriEle( int i1, int i2, int i3, double a ) {
	n1 = i1; n2 = i2; n3 = i3; attribute = a;
    }

    inline ~FGTriEle( void ) { };

    inline int get_n1() const { return n1; }
    inline void set_n1( int i ) { n1 = i; }
    inline int get_n2() const { return n2; }
    inline void set_n2( int i ) { n2 = i; }
    inline int get_n3() const { return n3; }
    inline void set_n3( int i ) { n3 = i; }

    inline double get_attribute() const { return attribute; }
    inline void set_attribute( double a ) { attribute = a; }
};


typedef vector < FGTriEle > triele_list;
typedef triele_list::iterator triele_list_iterator;
typedef triele_list::const_iterator const_triele_list_iterator;


#endif // _TRIELES_HXX


// $Log$
// Revision 1.3  1999/03/27 05:30:14  curt
// Handle corner nodes separately from the rest of the fitted nodes.
// Add fitted nodes in after corners and polygon nodes since the fitted nodes
//   are less important.  Subsequent nodes will "snap" to previous nodes if
//   they are "close enough."
// Need to manually divide segments to prevent "T" intersetions which can
//   confound the triangulator.  Hey, I got to use a recursive method!
// Pass along correct triangle attributes to output file generator.
// Do fine grained node snapping for corners and polygons, but course grain
//   node snapping for fitted terrain nodes.
//
// Revision 1.2  1999/03/23 22:02:53  curt
// Refinements in naming and organization.
//
// Revision 1.1  1999/03/22 23:58:57  curt
// Initial revision.
//
