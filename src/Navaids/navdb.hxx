/*
 * SPDX-FileCopyrightText: Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
 * SPDX_FileComment: top level navaids management routines
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <simgear/compiler.h>
#include <simgear/math/SGGeod.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/positioned.hxx>

#include <map>
#include <string>
#include <tuple>

// forward decls
class FGTACANList;
class SGPath;

namespace flightgear
{

class NavLoader {
  public:
    // load and initialize the navigational databases
    void loadNav(const NavDataCache::SceneryLocation& sceneryLocation,
                 std::size_t bytesReadSoFar,
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
                                unsigned int version = 810);
};

} // of namespace flightgear

