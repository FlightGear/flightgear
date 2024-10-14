/*
 * SPDX-FileCopyrightText: (C) Christian Schmitt, March 2013
 * SPDX_FileComment: points of interest management routines
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <map>

#include <simgear/compiler.h>

#include <simgear/math/SGGeod.hxx>
#include <Navaids/positioned.hxx>
#include <Navaids/NavDataCache.hxx>

// forward decls
class SGPath;
class sg_gzifstream;

// load and initialize the POI database
//bool poiDBInit(const SGPath& path);

namespace flightgear
{
    
class POILoader
{
public:
    POILoader();
    ~POILoader() = default;

    // Load POIs from the specified poi.dat (or poi.dat.gz) file
    void loadPOIs(const NavDataCache::SceneryLocation& sceneryLocation,
                    std::size_t bytesReadSoFar,
                    std::size_t totalSizeOfAllDatFiles);

private:
    void throwExceptionIfStreamError(const sg_gzifstream& input_stream);


    PositionedID readPOIFromStream(std::istream& aStream, unsigned int lineNumber,
                                            FGPositioned::Type type = FGPositioned::INVALID);

    NavDataCache* _cache = nullptr;
    SGPath _path;

    using POIKey = std::tuple<FGPositioned::Type, std::string>;
        // Maps (type, ident, name) tuples already loaded to their locations.
    std::multimap<POIKey, SGVec3d> _loadedPOIs;
};


} // of namespace flightgear

