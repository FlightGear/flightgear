// approachlist.hxx -- approach management class
//
// Written by Alexander Kappes, started March 2002.
// Based on navlist.hxx by Curtis Olson, started April 2000.
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


#ifndef _FG_APPROACHLIST_HXX
#define _FG_APPROACHLIST_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

#include <map>
#include <vector>

#include "approach.hxx"

SG_USING_STD(map);
SG_USING_STD(vector);


class FGApproachList {

  // convenience types
  typedef vector < FGApproach > approach_list_type;
  typedef approach_list_type::iterator approach_list_iterator;
  typedef approach_list_type::const_iterator approach_list_const_iterator;
  
  // typedef map < int, approach_list_type, less<int> > approach_map_type;
  typedef map < int, approach_list_type > approach_map_type;
  typedef approach_map_type::iterator approach_map_iterator;
  typedef approach_map_type::const_iterator approach_map_const_iterator;
  
  approach_map_type approachlist_freq;
  approach_map_type approachlist_bck;
  
public:
  
  FGApproachList();
  ~FGApproachList();
  
  // load the approach data and build the map
  bool init( SGPath path );
  
  // query the database for the specified frequency, lon and lat are
  // in degrees, elev is in meters
  bool query_freq( double lon, double lat, double elev, double freq, FGApproach *a );

  // query the database for the specified bucket number, lon and lat are
  // in degrees
  bool query_bck( double lon, double lat, double elev, FGApproach *a, int max_app, int &num_app );

  bool get_name( string apt_id  );

};


extern FGApproachList *current_approachlist;


#endif // _FG_APPROACHLIST_HXX
