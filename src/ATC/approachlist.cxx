// approachlist.cxx -- Approach data management class
//
// Written by Alexander Kappes, started March 2002.
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
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "approachlist.hxx"


FGApproachList *current_approachlist;


// Constructor
FGApproachList::FGApproachList( void ) {
}


// Destructor
FGApproachList::~FGApproachList( void ) {
}


// load the approach data and build the map
bool FGApproachList::init( SGPath path ) {

    approachlist_freq.erase( approachlist_freq.begin(), approachlist_freq.end() );
    approachlist_bck.erase( approachlist_bck.begin(), approachlist_bck.end() );

    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // read in each line of the file

    // in >> skipeol;
    in >> skipcomment;

    cout << " APPROACH " << endl;
#ifdef __MWERKS__
    char c = 0;
    while ( in.get(c) && c != '\0' ) {
        in.putback(c);
#else
    while ( !in.eof() ) {
#endif

	FGApproach a;
        in >> a;
	if ( a.get_type() == '[' ) {
	    break;
	}
	//cout << " type = " << a.get_type() << endl;
	//cout << " lon = " << a.get_lon() << endl;
	//cout << " lat = " << a.get_lat() << endl;
	//cout << " elev = " << a.get_elev() << endl;
	//cout << " freq = " << a.get_freq() << endl;
 	//cout << " Airport Code = " << a.GetIdent() << endl; 
 	//cout << " Name = " << a.get_name() << endl; 

	approachlist_freq[a.get_freq()].push_back(a);
	approachlist_bck[a.get_bucket()].push_back(a);
        in >> skipcomment;

    }

    return true;
}


// query the database for the specified frequency, lon and lat are in
// degrees, elev is in meters
bool FGApproachList::query_freq( double lon, double lat, double elev, double freq,
			    FGApproach *a )
{
  lon *= SGD_DEGREES_TO_RADIANS;
  lat *= SGD_DEGREES_TO_RADIANS;

  approach_list_type stations = approachlist_freq[(int)(freq*100.0 + 0.5)];
  
  approach_list_iterator current = stations.begin();
  approach_list_iterator last = stations.end();
  
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
    //cout << "  Aircraft: lon = " << lon << "  lat = " << lat 
    //     << "  elev = " << elev << endl;
    //cout << "  Airport:  lon = " << current->get_lon() 
    //     << "  lat = " << current->get_lat() 
    //     << "  elev = " << current->get_elev() 
    //     << "  z = " << current->get_z() << endl;
    
    // match up to twice the published range so we can model
    // reduced signal strength
    if ( d < (2 * current->get_range() * SG_NM_TO_METER 
	      * 2 * current->get_range() * SG_NM_TO_METER ) ) {
      //cout << "matched = " << current->GetIdent() << endl;
      *a = *current;
      return true;
    }
  }
  return false;
}

// query the database for the specified frequency, lon and lat are in
// degrees, elev is in meters
bool FGApproachList::query_bck( double lon, double lat, double elev, FGApproach *a, 
				int max_app, int &num_app)
{

  // get bucket number for plane position
  SGBucket buck(lon, lat);

  //cout << "plane bucket" << bucket << endl;

  // get neigboring buckets
  double max_range = 100;
  int bx = int ( max_range*SG_NM_TO_METER / buck.get_width_m() / 2);
  int by = int ( max_range*SG_NM_TO_METER / buck.get_height_m() / 2 );

  // loop over bucket range 
  for ( int i=-bx; i<bx; i++) {
    for ( int j=-by; j<by; j++) {
      buck = sgBucketOffset(lon, lat, i, j);
      long int bucket = buck.gen_index();
      //cout << "bucket number = " << bucket << endl;
      approach_list_type stations = approachlist_bck[bucket];
      approach_list_iterator current = stations.begin();
      approach_list_iterator last = stations.end();
 
      double rlon = lon * SGD_DEGREES_TO_RADIANS;
      double rlat = lat * SGD_DEGREES_TO_RADIANS;
 
      // double az1, az2, s;
      Point3D aircraft = sgGeodToCart( Point3D(rlon, rlat, elev) );
      Point3D station;
      double d;
      for ( ; current != last ; ++current ) {
	station = Point3D(current->get_x(), current->get_y(), current->get_z());
	d = aircraft.distance3Dsquared( station );
	/*
	  cout << "  dist = " << sqrt(d)
	  << "  range = " << current->get_range() * SG_NM_TO_METER << endl;
	  cout << "  Aircraft: lon = " << lon
	  << "  lat = " << lat/SGD_DEGREES_TO_RADIANS 
	  << "  bucket = " << bucket
	  << "  elev = " << elev << endl;
	  cout << "  Airport:  Id = " << current->GetIdent() 
	  << "  lon = " << current->get_lon() 
	  << "  lat = " << current->get_lat() 
	  << "  elev = " << current->get_elev() 
	  << "  bucket = " << current->get_bucket() 
	  << "  z = " << current->get_z() << endl;
	*/
	// match up to twice the published range so we can model
	// reduced signal strength
	if ( d < (current->get_range() * SG_NM_TO_METER 
		  * current->get_range() * SG_NM_TO_METER ) ) {
	  //cout << "matched = " << current->GetIdent() << endl;
	  if (num_app < max_app) {
	    a[num_app] = *current;
	    num_app += 1;
	  }
	  else {
	    cout << "Approachlist error: Too many stations in range" << endl; 
	  }
	  
	  //return true;
	}
      }
    }
  }
  return true;	//DCL - added this to prevent a compiler warning
}


bool FGApproachList::get_name( string apt_id )
{
  string name;
  double freq = 125.22;

  approach_list_type stations = approachlist_freq[(int)(freq*100.0 + 0.5)];
  
  approach_list_iterator current = stations.begin();
  approach_list_iterator last    = stations.end();

  if(current != last) {  
    cout << "ApproachList" << endl;
    cout << "name" << current->get_name() << endl;
    cout << "bucket" << current->get_bucket() << endl;
  }

  return 0;

}