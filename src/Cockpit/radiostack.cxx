// radiostack.cxx -- class to manage an instance of the radio stack
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#include <Navaids/ilslist.hxx>
#include <Navaids/navlist.hxx>

#include "radiostack.hxx"


// Constructor
FGRadioStack::FGRadioStack() {
    need_update = true;
}


// Destructor
FGRadioStack::~FGRadioStack() {
}


// Update nav/adf radios based on current postition
void FGRadioStack::update( double lon, double lat, double elev ) {
    need_update = false;

    FGNav n;

    // nav1
    if ( current_navlist->query( lon, lat, elev, nav1_freq,
				 &n, &nav1_heading, &nav1_dist) ) {
	nav1_inrange = true;
	nav1_lon = n.get_lon();
	nav1_lat = n.get_lat();
	nav1_elev = n.get_elev();
	cout << "Found a vor station in range" << endl;
	cout << " id = " << n.get_ident() << endl;
	cout << " heading = " << nav1_heading
	     << " dist = " << nav1_dist << endl;
    } else {
	nav1_inrange = false;
	cout << "not picking up vor. :-(" << endl;
    }

    // nav1
    if ( current_navlist->query( lon, lat, elev, nav2_freq,
				 &n, &nav2_heading, &nav2_dist) ) {
	nav2_inrange = true;
	nav2_lon = n.get_lon();
	nav2_lat = n.get_lat();
	nav2_elev = n.get_elev();
	cout << "Found a vor station in range" << endl;
	cout << " id = " << n.get_ident() << endl;
	cout << " heading = " << nav2_heading
	     << " dist = " << nav2_dist << endl;
    } else {
	nav2_inrange = false;
	cout << "not picking up vor. :-(" << endl;
    }

    // adf
    double junk;
    if ( current_navlist->query( lon, lat, elev, adf_freq,
				 &n, &adf_heading, &junk) ) {
	adf_inrange = true;
	adf_lon = n.get_lon();
	adf_lat = n.get_lat();
	adf_elev = n.get_elev();
	cout << "Found an adf station in range" << endl;
	cout << " id = " << n.get_ident() << endl;
	cout << " heading = " << adf_heading
	     << " dist = " << junk << endl;
    } else {
	adf_inrange = false;
	cout << "not picking up adf. :-(" << endl;
    }
}

