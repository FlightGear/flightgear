// navdb.hxx -- top level navaids management routines
//
// Written by Curtis Olson, started May 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_NAVDB_HXX
#define _FG_NAVDB_HXX


#include <simgear/compiler.h>
#include <simgear/math/SGGeod.hxx>
#include <string>
#include <map>
#include <tuple>
#include <Navaids/positioned.hxx>

// forward decls
class FGTACANList;
class SGPath;

namespace flightgear
{

class NavLoader {
  public:
    // load and initialize the navigational databases
    void loadNav(const SGPath& path, std::size_t bytesReadSoFar,
                 std::size_t totalSizeOfAllDatFiles);

    void loadCarrierNav(const SGPath& path);

    bool loadTacan(const SGPath& path, FGTACANList *channellist);

  private:
    // Maps (type, ident, name) tuples already loaded to their locations.
    std::multimap<std::tuple<FGPositioned::Type, std::string, std::string>,
        SGGeod> _loadedNavs;

    PositionedID processNavLine(const std::string& line,
                                const std::string& utf8Path,
                                unsigned int lineNum,
                                FGPositioned::Type type = FGPositioned::INVALID,
                                unsigned long version = 810);
};

} // of namespace flightgear
  
#endif // _FG_NAVDB_HXX
