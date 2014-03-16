#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <iostream>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::string;
using std::vector;


// encapsulate a gps integer time (fixme, assumes all times in a track
// are from the same day, so we don't handle midnight roll over)
class GPSTime {

public:

    double seconds;

    inline GPSTime( const int hh, const int mm, const double ss ) {
        seconds = hh*3600 + mm*60 + ss;
    }
    inline GPSTime( const double gpstime ) {
        double tmp = gpstime;
        int hh = (int)(tmp / 10000);
        tmp -= hh * 10000;
        int mm = (int)(tmp / 100);
        tmp -= mm * 100;
        double ss = tmp;
        seconds = hh*3600 + mm*60 + ss;
        // cout << gpstime << " = " << seconds << endl;
    }
    inline ~GPSTime() {}

    inline double get_time() const { return seconds; }
    inline double diff_sec( const GPSTime t ) const {
        return seconds - t.seconds;        
    }
};



// encapsulate the interesting gps data for a moment in time
class GPSPoint {

public:

    GPSTime gps_time;
    double lat_deg;
    double lon_deg;
    int fix_quality;
    int num_satellites;
    double hdop;
    double altitude_msl;
    double speed_kts;
    double course_true;

    GPSPoint() :
        gps_time(GPSTime(0,0,0)),
        lat_deg(0.0),
        lon_deg(0.0),
        fix_quality(0),
        num_satellites(0),
        hdop(0.0),
        altitude_msl(0.0),
        speed_kts(0.0),
        course_true(0.0)
    { }

    inline double get_time() const { return gps_time.get_time(); }
};


// Manage a saved gps log (track file)
class GPSTrack {

private:

    vector <GPSPoint> data;

public:

    GPSTrack();
    ~GPSTrack();

    int load( const string &file );

    inline int size() const { return data.size(); }

    inline GPSPoint get_point( const unsigned int i )
    {
        if ( i < data.size() ) {
            return data[i];
        } else {
            return GPSPoint();
        }
    }
        

};


GPSPoint GPSInterpolate( const GPSPoint A, const GPSPoint B,
                         const double percent );
