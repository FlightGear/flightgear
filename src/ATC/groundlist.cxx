// groundlist.cxx -- ATC Ground data management class
//
// Written by David Luff, started November 2002.
// Based on navlist.cxx by Curtis Olson, started April 2000.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>

#include "groundlist.hxx"

FGGroundList *current_groundlist;

// Constructor
FGGroundList::FGGroundList( void ) {
}


// Destructor
FGGroundList::~FGGroundList( void ) {
}


// load the navaids and build the map
bool FGGroundList::init( SGPath path ) {
	
	groundlist.erase( groundlist.begin(), groundlist.end() );
	
	sg_gzifstream in( path.str() );
	if ( !in.is_open() ) {
		SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
		exit(-1);
	}
	
	// read in each line of the file
	
	in >> skipcomment;
	
	#ifdef __MWERKS__
	char c = 0;
	while ( in.get(c) && c != '\0' ) {
		in.putback(c);
		#else
		while ( !in.eof() ) {
			#endif
			
			FGGround g;
			in >> g;
			if ( g.get_type() == '[' ) {
				break;
			}
			
			//cout << "id = " << t.GetIdent() << endl;
			//cout << " type = " << t.get_type() << endl;
			//cout << " lon = " << t.get_lon() << endl;
			//cout << " lat = " << t.get_lat() << endl;
			//cout << " elev = " << t.get_elev() << endl;
			//cout << " freq = " << t.get_freq() << endl;
			//cout << " range = " << t.get_range() << endl;
			
			groundlist[g.get_freq()].push_back(g);
			in >> skipcomment;
		}
		
		return true;
	}
	
	
	// query the database for the specified frequency, lon and lat are in
	// degrees, elev is in meters
	bool FGGroundList::query( double lon, double lat, double elev, double freq,
	FGGround *g )
	{
		lon *= SGD_DEGREES_TO_RADIANS;
		lat *= SGD_DEGREES_TO_RADIANS;
		//cout << "lon = " << lon << '\n';
		//cout << "lat = " << lat << '\n';
		//cout << "elev = " << elev << '\n';
		//cout << "freq = " << freq << '\n';
		
		ground_list_type stations = groundlist[(int)(freq*100.0 + 0.5)];
		
		ground_list_iterator current = stations.begin();
		ground_list_iterator last = stations.end();
		
		// double az1, az2, s;
		Point3D aircraft = sgGeodToCart( Point3D(lon, lat, elev) );
		Point3D station;
		double d;
		for ( ; current != last ; ++current ) {
			//cout << "testing " << current->GetIdent() << endl;
			station = Point3D(current->get_x(), current->get_y(), current->get_z());
			//cout << "aircraft = " << aircraft << endl;
			//cout << "station = " << station << endl;
			
			d = aircraft.distance3Dsquared( station );
			
			//cout << "  dist = " << sqrt(d)
			//     << "  range = " << current->get_range() * SG_NM_TO_METER << endl;
			
			// match up to twice the published range so we can model
			// reduced signal strength
			if ( d < (2 * current->get_range() * SG_NM_TO_METER 
				* 2 * current->get_range() * SG_NM_TO_METER ) ) {
					//cout << "matched = " << current->GetIdent() << endl;
					*g = *current;
					return true;
				}
		}
		
		return false;
	}
