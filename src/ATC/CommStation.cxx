#include "config.h"

#include "CommStation.hxx"
#include <Airports/airport.hxx>
#include <Navaids/NavDataCache.hxx>

namespace flightgear {

CommStation::CommStation(PositionedID aGuid, const std::string& name, FGPositioned::Type t, const SGGeod& pos, int range, int freq) :
    FGPositioned(aGuid, t, name, pos),
    mRangeNM(range),
    mFreqKhz(freq),
    mAirport(0)
{  
}

void CommStation::setAirport(PositionedID apt)
{
    mAirport = apt;
}
  
FGAirportRef CommStation::airport() const
{
  return FGPositioned::loadById<FGAirport>(mAirport);
}

double CommStation::freqMHz() const
{
    return mFreqKhz / 1000.0;
}

CommStationRef
CommStation::findByFreq(int freqKhz, const SGGeod& pos, FGPositioned::Filter* filt)
{
  return (CommStation*) NavDataCache::instance()->findCommByFreq(freqKhz, pos, filt).ptr();
}

} // of namespace flightgear
