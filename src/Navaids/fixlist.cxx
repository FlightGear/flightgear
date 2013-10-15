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
#include <simgear/structure/exception.hxx>

#include "fixlist.hxx"
#include <Navaids/fix.hxx>
#include <Navaids/NavDataCache.hxx>

FGFix::FGFix(PositionedID aGuid, const std::string& aIdent, const SGGeod& aPos) :
  FGPositioned(aGuid, FIX, aIdent, aPos)
{
}


namespace flightgear
{
  
void loadFixes(const SGPath& path)
{
  sg_gzifstream in( path.str() );
  if ( !in.is_open() ) {
      throw sg_io_exception("Cannot open file:", path);
  }
  
  // toss the first two lines of the file
  in >> skipeol;
  in >> skipeol;
  
  NavDataCache* cache = NavDataCache::instance();
  
  // read in each remaining line of the file
  while ( ! in.eof() ) {
    double lat, lon;
    std::string ident;
    in >> lat >> lon >> ident;
    if (lat > 95) break;
    
    cache->insertFix(ident, SGGeod::fromDeg(lon, lat));
    in >> skipcomment;
  }

}
  
} // of namespace flightgear;
