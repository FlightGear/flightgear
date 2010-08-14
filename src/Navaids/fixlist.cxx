// fixlist.cxx -- fix list management class
//
// Written by Curtis Olson, started April 2000.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <algorithm>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "fixlist.hxx"
#include <Navaids/fix.hxx>
#include <Airports/simple.hxx>

FGFix::FGFix(const std::string& aIdent, const SGGeod& aPos) :
  FGPositioned(FIX, aIdent, aPos)
{
  init(true); // init FGPositioned
}

// Constructor
FGFixList::FGFixList( void ) {
}


// Destructor
FGFixList::~FGFixList( void ) {
}


// load the navaids and build the map
bool FGFixList::init(const SGPath& path ) {
    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // toss the first two lines of the file
    in >> skipeol;
    in >> skipeol;

    // read in each remaining line of the file
    while ( ! in.eof() ) {
      double lat, lon;
      std::string ident;
      in >> lat >> lon >> ident;
      if (lat > 95) break;

      // fix gets added to the FGPositioned spatial indices, so we don't need
      // to hold onto it here.
      new FGFix(ident, SGGeod::fromDeg(lon, lat));
      in >> skipcomment;
    }
    return true;
}
