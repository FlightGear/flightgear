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


// a segment is two integer pointers into the node list
class FGTriEle {
    int n1, n2, n3;

public:

    // Constructor and destructor
    inline FGTriEle( void ) { };
    inline FGTriEle( int i1, int i2, int i3 ) { n1 = i1; n2 = i2; n3 = i3; }

    inline ~FGTriEle( void ) { };

    inline int get_n1() const { return n1; }
    inline void set_n1( int i ) { n1 = i; }
    inline int get_n2() const { return n2; }
    inline void set_n2( int i ) { n2 = i; }
    inline int get_n3() const { return n3; }
    inline void set_n3( int i ) { n3 = i; }
};


#endif // _TRIELES_HXX


// $Log$
// Revision 1.1  1999/03/22 23:58:57  curt
// Initial revision.
//
