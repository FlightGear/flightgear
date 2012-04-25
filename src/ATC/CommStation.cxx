#include "CommStation.hxx"

#include <map>

namespace {

typedef std::multimap<int, flightgear::CommStation*> FrequencyMap;
static FrequencyMap static_frequencies;

typedef std::pair<FrequencyMap::const_iterator, FrequencyMap::const_iterator> FrequencyMapRange;

} // of anonymous namespace

namespace flightgear {

CommStation::CommStation(const std::string& name, FGPositioned::Type t, const SGGeod& pos, int range, int freq) :
    FGPositioned(t, name, pos),
    mRangeNM(range),
    mFreqKhz(freq),
    mAirport(NULL)
{
    static_frequencies.insert(std::make_pair(freq, this));
  
    init(true);
}

void CommStation::setAirport(FGAirport* apt)
{
    mAirport = apt;
}

double CommStation::freqMHz() const
{
    return mFreqKhz / 100.0;
}

CommStation*
CommStation::findByFreq(int freqKhz, const SGGeod& pos, FGPositioned::Filter* filt)
{
    FrequencyMapRange range = static_frequencies.equal_range(freqKhz);
    FGPositioned::List results;
    for (; range.first != range.second; ++range.first) {
        CommStation* sta = range.first->second;
        if (filt && !filt->pass(sta)) {
            continue; // filtered out
        }
        
        results.push_back(sta);
    }
    
    if (results.empty()) {
        return NULL;
    }
    
    FGPositioned::sortByRange(results, pos);
    return (CommStation*) results.front().ptr();
}

} // of namespace flightgear
