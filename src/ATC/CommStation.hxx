#ifndef FG_ATC_COMM_STATION_HXX
#define FG_ATC_COMM_STATION_HXX

#include <Navaids/positioned.hxx>

class FGAirport;

namespace flightgear
{

class CommStation : public FGPositioned
{
public:
    CommStation(const std::string& name, FGPositioned::Type t, const SGGeod& pos, int range, int freq);

    void setAirport(FGAirport* apt);
    FGAirport* airport() const { return mAirport; }
    
    int rangeNm() const
        { return mRangeNM; }
        
    int freqKHz() const
        { return mFreqKhz; }
        
    double freqMHz() const;
    
    static CommStation* findByFreq(int freqKhz, const SGGeod& pos, FGPositioned::Filter* filt = NULL);
private:
    int mRangeNM;
    int mFreqKhz;
    FGAirport* mAirport;
}; 
    
} // of namespace flightgear

#endif // of FG_ATC_COMM_STATION_HXX

