// trisegs.hxx -- "Triangle" segment management class
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


#ifndef _TRISEGS_HXX
#define _TRISEGS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <vector>

#include "trinodes.hxx"

FG_USING_STD(vector);


// a segment is two integer pointers into the node list
class FGTriSeg {
    int n1, n2;

public:

    // Constructor and destructor
    inline FGTriSeg( void ) { };
    inline FGTriSeg( int i1, int i2 ) { 
	n1 = i1;
	n2 = i2;
    }

    inline ~FGTriSeg( void ) { };

    inline int get_n1() const { return n1; }
    inline void set_n1( int i ) { n1 = i; }
    inline int get_n2() const { return n2; }
    inline void set_n2( int i ) { n2 = i; }

    friend bool operator == (const FGTriSeg& a, const FGTriSeg& b);

};

inline bool operator == (const FGTriSeg& a, const FGTriSeg& b)
{
    return ((a.n1 == b.n1) && (a.n2 == b.n2)) 
	|| ((a.n1 == b.n2) && (a.n2 == b.n1));
}


typedef vector < FGTriSeg > triseg_list;
typedef triseg_list::iterator triseg_list_iterator;
typedef triseg_list::const_iterator const_triseg_list_iterator;


class FGTriSegments {

private:

    triseg_list seg_list;

    // Divide segment if there are other points on it, return the
    // divided list of segments
    triseg_list divide_segment( const point_list& nodes, 
				const FGTriSeg& s );

public:

    // Constructor and destructor
    FGTriSegments( void );
    ~FGTriSegments( void );

    // delete all the data out of seg_list
    inline void clear() { seg_list.clear(); }

    // Add a segment to the segment list if it doesn't already exist.
    // Returns the index (starting at zero) of the segment in the
    // list.
    int unique_add( const FGTriSeg& s );

    // Add a segment to the segment list if it doesn't already exist.
    // Returns the index (starting at zero) of the segment in the list.
    void unique_divide_and_add( const point_list& node_list, 
				const FGTriSeg& s );

    // return the master node list
    inline triseg_list get_seg_list() const { return seg_list; }

    // return the ith segment
    inline FGTriSeg get_seg( int i ) const { return seg_list[i]; }
};


#endif // _TRISEGS_HXX


