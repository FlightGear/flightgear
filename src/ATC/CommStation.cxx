#include "CommStation.hxx"

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
  
FGAirport* CommStation::airport() const
{
  return (FGAirport*) NavDataCache::instance()->loadById(mAirport);
}

double CommStation::freqMHz() const
{
    return mFreqKhz / 100.0;
}

CommStation*
CommStation::findByFreq(int freqKhz, const SGGeod& pos, FGPositioned::Filter* filt)
{
  return (CommStation*) NavDataCache::instance()->findCommByFreq(freqKhz, pos, filt).ptr();
}

} // of namespace flightgear
