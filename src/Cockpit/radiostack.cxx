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


#include <Aircraft/aircraft.hxx>
#include <Navaids/ilslist.hxx>
#include <Navaids/navlist.hxx>

#include "radiostack.hxx"


FGRadioStack *current_radiostack;


// Constructor
FGRadioStack::FGRadioStack() {
    need_update = true;
}


// Destructor
FGRadioStack::~FGRadioStack() {
}


// Search the database for the current frequencies given current location
void FGRadioStack::update( double lon, double lat, double elev ) {
    need_update = false;

    Point3D aircraft = fgGeodToCart( Point3D( lon, lat, elev ) );
    Point3D station;
    double az1, az2, s;

    if ( nav1_valid ) {
	// staightline distance
	station = Point3D( nav1_x, nav1_y, nav1_z );
	nav1_dist = aircraft.distance3D( station );
	
	if ( nav1_dist < nav1_effective_range * NM_TO_METER ) {
	    nav1_inrange = true;
	    
	    // wgs84 heading
	    geo_inverse_wgs_84( elev, lat * RAD_TO_DEG, lon * RAD_TO_DEG, 
				nav1_lat, nav1_lon,
				&az1, &az2, &s );
	    nav1_heading = az1;

	    // cout << " heading = " << nav1_heading
	    //      << " dist = " << nav1_dist << endl;
	} else {
	    nav1_inrange = false;
	}

	if ( nav1_loc ) {
	} else {
	    nav1_radial = nav1_sel_radial;
	}
    } else {
	nav1_inrange = false;
	// cout << "not picking up vor. :-(" << endl;
    }

    if ( nav2_valid ) {
	// staightline distance
	station = Point3D( nav2_x, nav2_y, nav2_z );
	nav2_dist = aircraft.distance3D( station );

	if ( nav2_dist < nav2_effective_range * NM_TO_METER ) {
	    nav2_inrange = true;

	    // wgs84 heading
	    geo_inverse_wgs_84( elev, lat * RAD_TO_DEG, lon * RAD_TO_DEG, 
				nav2_lat, nav2_lon,
				&az1, &az2, &s );
	    nav2_heading = az1;

	    // cout << " heading = " << nav2_heading
	    //      << " dist = " << nav2_dist << endl;
	} else {
	    nav2_inrange = false;
	}

	if ( !nav2_loc ) {
	    nav2_radial = nav2_sel_radial;
	}
    } else {
	nav2_inrange = false;
	// cout << "not picking up vor. :-(" << endl;
    }

    // adf
    if ( adf_valid ) {
	// staightline distance
	station = Point3D( adf_x, adf_y, adf_z );
	adf_dist = aircraft.distance3D( station );

	if ( adf_dist < adf_effective_range * NM_TO_METER ) {
	    adf_inrange = true;

	    // wgs84 heading
	    geo_inverse_wgs_84( elev, lat * RAD_TO_DEG, lon * RAD_TO_DEG, 
				adf_lat, adf_lon,
				&az1, &az2, &s );
	    adf_heading = az1;

	    // cout << " heading = " << nav2_heading
	    //      << " dist = " << nav2_dist << endl;
	} else {
	    adf_inrange = false;
	}
    } else {
	adf_inrange = false;
    }
}


// Update current nav/adf radio stations based on current postition
void FGRadioStack::search( double lon, double lat, double elev ) {
    // nav1
    FGILS ils;
    FGNav nav;

    if ( current_ilslist->query( lon, lat, elev, nav1_freq, &ils ) ) {
	nav1_valid = true;
	nav1_loc = true;

	nav1_lon = ils.get_loclon();
	nav1_lat = ils.get_loclat();
	nav1_elev = ils.get_gselev();
	nav1_effective_range = FG_ILS_DEFAULT_RANGE;
	nav1_target_gs = ils.get_gsangle();
	nav1_radial = ils.get_locheading();
	while ( nav1_radial <   0.0 ) { nav1_radial += 360.0; }
	while ( nav1_radial > 360.0 ) { nav1_radial -= 360.0; }
	nav1_x = ils.get_x();
	nav1_y = ils.get_y();
	nav1_z = ils.get_z();
	// cout << "Found an ils station in range" << endl;
	// cout << " id = " << ils.get_locident() << endl;
    } else if ( current_navlist->query( lon, lat, elev, nav1_freq, &nav ) ) {
	nav1_valid = true;
	nav1_loc = false;

	nav1_lon = nav.get_lon();
	nav1_lat = nav.get_lat();
	nav1_elev = nav.get_elev();
	nav1_effective_range = nav.get_range();
	nav1_target_gs = 0.0;
	nav1_radial = nav1_sel_radial;
	nav1_x = nav.get_x();
	nav1_y = nav.get_y();
	nav1_z = nav.get_z();
	// cout << "Found a vor station in range" << endl;
	// cout << " id = " << nav.get_ident() << endl;
    } else {
	nav1_valid = false;
	// cout << "not picking up vor1. :-(" << endl;
    }

    if ( current_ilslist->query( lon, lat, elev, nav2_freq, &ils ) ) {
	nav2_valid = true;
	nav2_loc = true;

	nav2_lon = ils.get_loclon();
	nav2_lat = ils.get_loclat();
	nav2_elev = ils.get_gselev();
	nav2_effective_range = FG_ILS_DEFAULT_RANGE;
	nav2_target_gs = ils.get_gsangle();
	nav2_radial = ils.get_locheading();
	while ( nav2_radial <   0.0 ) { nav2_radial += 360.0; }
	while ( nav2_radial > 360.0 ) { nav2_radial -= 360.0; }
	nav2_x = ils.get_x();
	nav2_y = ils.get_y();
	nav2_z = ils.get_z();
	// cout << "Found an ils station in range" << endl;
	// cout << " id = " << ils.get_locident() << endl;
    } else if ( current_navlist->query( lon, lat, elev, nav2_freq, &nav ) ) {
	nav2_valid = true;
	nav2_loc = false;

	nav2_lon = nav.get_lon();
	nav2_lat = nav.get_lat();
	nav2_elev = nav.get_elev();
	nav2_effective_range = nav.get_range();
	nav2_target_gs = 0.0;
	nav2_radial = nav2_sel_radial;
	nav2_x = nav.get_x();
	nav2_y = nav.get_y();
	nav2_z = nav.get_z();
	// cout << "Found a vor station in range" << endl;
	// cout << " id = " << nav.get_ident() << endl;
    } else {
	nav2_valid = false;
	// cout << "not picking up vor2. :-(" << endl;
    }

    // adf
    if ( current_navlist->query( lon, lat, elev, adf_freq, &nav ) ) {
	adf_valid = true;

	adf_lon = nav.get_lon();
	adf_lat = nav.get_lat();
	adf_elev = nav.get_elev();
	adf_effective_range = nav.get_range();
	adf_x = nav.get_x();
	adf_y = nav.get_y();
	adf_z = nav.get_z();
	// cout << "Found an adf station in range" << endl;
	// cout << " id = " << nav.get_ident() << endl;
    } else {
	adf_valid = false;
	// cout << "not picking up adf. :-(" << endl;
    }
}


// periodic radio station search wrapper
void fgRadioSearch( void ) {
    current_radiostack->search( cur_fdm_state->get_Longitude(),
				cur_fdm_state->get_Latitude(),
				cur_fdm_state->get_Altitude() * FEET_TO_METER );
}
