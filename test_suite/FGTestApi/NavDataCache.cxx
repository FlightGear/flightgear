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
