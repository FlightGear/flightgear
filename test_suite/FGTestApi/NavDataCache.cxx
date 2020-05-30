/*
 * Copyright (C) 2018 Edward d'Auvergne
 *
 * This file is part of the program FlightGear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "NavDataCache.hxx"

#include <Navaids/NavDataCache.hxx>


namespace FGTestApi {

namespace setUp {

void initNavDataCache()
{
    if (flightgear::NavDataCache::instance())
        return;
    
    flightgear::NavDataCache* cache = flightgear::NavDataCache::createInstance();
    if (cache->isRebuildRequired()) {
        std::cerr << "Navcache rebuild for testing" << std::flush;

        while (cache->rebuild() != flightgear::NavDataCache::REBUILD_DONE) {
          SGTimeStamp::sleepForMSec(1000);
          std::cerr << "." << std::flush;
        }
    }
}

}  // End of namespace setUp.

}  // End of namespace FGTestApi.
