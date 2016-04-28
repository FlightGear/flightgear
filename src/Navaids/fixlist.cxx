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

#include <stdlib.h>             // atof()

#include <algorithm>
#include <string>               // std::getline()

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx> // simgear::strutils::split()
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

const unsigned int LINES_IN_FIX_DAT = 119724;
  
void loadFixes(const SGPath& path)
{
  sg_gzifstream in( path );
  const std::string utf8path = path.utf8Str();

  if ( !in.is_open() ) {
      throw sg_io_exception("Cannot open file:", path);
  }
  
  // toss the first two lines of the file
  in >> skipeol;
  in >> skipeol;
  
  NavDataCache* cache = NavDataCache::instance();
  unsigned int lineNumber = 3;

  // read in each remaining line of the file
  for (std::string line; std::getline(in, line); lineNumber++) {
    std::vector<std::string> fields = simgear::strutils::split(line);
    std::vector<std::string>::size_type nb_fields = fields.size();
    const std::string endOfData = "99"; // special code in the fix.dat spec
    double lat, lon;

    if (nb_fields == 0) {       // blank line
      continue;
    } else if (nb_fields == 1) {
      if (fields[0] == endOfData)
        break;
      else {
        SG_LOG(SG_NAVAID, SG_WARN, utf8path << ": malformed line #" <<
               lineNumber << ": only one field, but it is not '99'");
        continue;
      }
    } else if (nb_fields != 3) {
      SG_LOG(SG_NAVAID, SG_WARN, utf8path << ": malformed line #" <<
             lineNumber << ": expected 3 fields, but got " << fields.size());
      continue;
    }

    lat = atof(fields[0].c_str());
    lon = atof(fields[1].c_str());

    cache->insertFix(fields[2], SGGeod::fromDeg(lon, lat));

      if ((lineNumber % 100) == 0) {
          // every 100 lines
          unsigned int percent = (lineNumber * 100) / LINES_IN_FIX_DAT;
          cache->setRebuildPhaseProgress(NavDataCache::REBUILD_FIXES, percent);
      }
  }

}

} // of namespace flightgear;
