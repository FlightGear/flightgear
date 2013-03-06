#ifndef FG_ATC_COMM_STATION_HXX
#define FG_ATC_COMM_STATION_HXX

#include <Airports/airports_fwd.hxx>
#include <Navaids/positioned.hxx>

namespace flightgear
{

class CommStation : public FGPositioned
{
public:
    CommStation(PositionedID aGuid, const std::string& name, FGPositioned::Type t, const SGGeod& pos, int range, int freq);

    void setAirport(PositionedID apt);
    FGAirportRef airport() const;
    
    int rangeNm() const
        { return mRangeNM; }
        
    int freqKHz() const
        { return mFreqKhz; }
        
    double freqMHz() const;
    
    static CommStationRef findByFreq(int freqKhz, const SGGeod& pos, FGPositioned::Filter* filt = NULL);
private:
    int mRangeNM;
    int mFreqKhz;
    PositionedID mAirport;
};

} // of namespace flightgear

#endif // of FG_ATC_COMM_STATION_HXX

