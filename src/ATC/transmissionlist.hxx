// transmissionlist.hxx -- transmission management class
//
// Written by Alexander Kappes, started March 2002.
// Based on navlist.hxx by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_TRANSMISSIONLIST_HXX
#define _FG_TRANSMISSIONLIST_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

#include <map>
#include <vector>

#include "ATC.hxx"
#include "transmission.hxx"

SG_USING_STD(map);
SG_USING_STD(vector);

class FGTransmissionList {

  // convenience types
  typedef vector < FGTransmission > transmission_list_type;
  typedef transmission_list_type::iterator transmission_list_iterator;
  typedef transmission_list_type::const_iterator transmission_list_const_iterator;
  
  // Map of transmission lists by station type
  // typedef map < int, transmission_list_type, less<int> > transmission_map_type;
  typedef map < atc_type, transmission_list_type > transmission_map_type;
  typedef transmission_map_type::iterator transmission_map_iterator;
  typedef transmission_map_type::const_iterator transmission_map_const_iterator;
  
  transmission_map_type transmissionlist_station;
  
public:
  
  FGTransmissionList();
  ~FGTransmissionList();
  
  // load the transmission data and build the map
  bool init( SGPath path );
  
  // query the database for the specified code,
  bool query_station( const atc_type &station, FGTransmission *a, int max_trans, int &num_trans );

  // generate the transmission text given the code of the message 
  // and the parameters
  // Set ttext = true to generate the spoken transmission text, 
  // or false to generate the abridged menu entry text.
  string gen_text(const atc_type &station, const TransCode code,
		  const TransPar &tpars, const bool ttext);

};


void mkATCMenuInit (void);
void mkATCMenu (void);

extern FGTransmissionList *current_transmissionlist;


#endif // _FG_TRANSMISSIONLIST_HXX
