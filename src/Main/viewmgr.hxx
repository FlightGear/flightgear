// viewmgr.hxx -- class for managing all the views in the flightgear world.
//
// Written by Curtis Olson, started October 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _VIEWMGR_HXX
#define _VIEWMGR_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <vector>

#include "viewer_lookat.hxx"
#include "viewer_rph.hxx"

FG_USING_STD(vector);


// Define a structure containing view information
class FGViewMgr {

private:

    typedef vector < FGViewer * > viewer_list;
    viewer_list views;

    int current;

public:

    // Constructor
    FGViewMgr( void );

    // Destructor
    ~FGViewMgr( void );

    // getters
    inline int size() const { return views.size(); }
    inline FGViewer *get_view( int i ) {
	if ( i < 0 ) { i = 0; }
	if ( i >= (int)views.size() ) { i = views.size() - 1; }
	return views[i];
    }
    inline FGViewer *next_view() {
	++current;
	if ( current >= (int)views.size() ) {
	    current = 0;
	}
	return views[current];
    }
    inline FGViewer *prev_view() {
	--current;
	if ( current < 0 ) {
	    current = views.size() - 1;
	}
	return views[current];
    }

    // setters
    inline void clear() { views.clear(); }
    inline void add_view( FGViewer * v ) {
	views.push_back(v);
	current = views.size() - 1;
    }
};


#endif // _VIEWMGR_HXX
