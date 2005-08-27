#ifndef _FG_MIDG_II_HXX
#define _FG_MIDG_II_HXX


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>

#ifdef HAVE_STDINT_H
# include <stdint.h>
#elif defined( _MSC_VER ) || defined(__MINGW32__) || defined(sun)
typedef signed short     int16_t;
typedef signed int       int32_t;
typedef unsigned short   uint16_t;
typedef unsigned int     uint32_t;
#else
# error "Port me! Platforms that don't have <stdint.h> need to define int8_t, et. al."
#endif


SG_USING_STD(cout);
SG_USING_STD(endl);
SG_USING_STD(string);
SG_USING_STD(vector);


// encapsulate a midg integer time (fixme, assumes all times in a track
// are from the same day, so we don't handle midnight roll over)
class MIDGTime {

public:

    uint32_t msec;
    double seconds;

    inline MIDGTime( const int dd, const int hh, const int mm,
                     const double ss )
    {
        seconds = dd*86400.0 + hh*3600.0 + mm*60.0 + ss;
        msec = (uint32_t)(seconds * 1000);
    }
    inline MIDGTime( const uint32_t midgtime_msec ) {
        msec = midgtime_msec;
        seconds = (double)midgtime_msec / 1000.0;
        // cout << midgtime << " = " << seconds << endl;
    }
    inline ~MIDGTime() {}

    inline double get_seconds() const { return seconds; }
    inline uint32_t get_msec() const { return msec; }
    inline double diff_seconds( const MIDGTime t ) const {
        return seconds - t.seconds;        
    }
};



// base class for MIDG data types
class MIDGpoint {

public:

    MIDGTime midg_time;

    MIDGpoint() :
        midg_time(MIDGTime(0))
    { }

    inline double get_seconds() const { return midg_time.get_seconds(); }
    inline uint32_t get_msec() const { return midg_time.get_msec(); }
};


// encapsulate the interesting midg data for a moment in time
class MIDGpos : public MIDGpoint {

public:

    double lat_deg;
    double lon_deg;
    double altitude_msl;
    int fix_quality;
    int num_satellites;
    double hdop;
    double speed_kts;
    double course_true;

    MIDGpos() :
        lat_deg(0.0),
        lon_deg(0.0),
        altitude_msl(0.0),
        fix_quality(0),
        num_satellites(0),
        hdop(0.0),
        speed_kts(0.0),
        course_true(0.0)
    { }
};


// encapsulate the interesting midg data for a moment in time
class MIDGatt : public MIDGpoint {

public:

    double yaw_rad;
    double pitch_rad;
    double roll_rad;

    MIDGatt() :
        yaw_rad(0.0),
        pitch_rad(0.0),
        roll_rad(0.0)
    { }
};


// Manage a saved midg log (track file)
class MIDGTrack {

private:

    vector <MIDGpos> posdata;
    vector <MIDGatt> attdata;
    FILE *fd;

    // parse message and put current data into vector if message has a
    // newer time stamp than existing data.
    void parse_msg( const int id, char *buf, MIDGpos *pos, MIDGatt *att );

public:

    MIDGTrack();
    ~MIDGTrack();

    int load( const string &file );

    inline int possize() const { return posdata.size(); }
    inline int attsize() const { return attdata.size(); }

    inline MIDGpos get_pospt( const unsigned int i )
    {
        if ( i < posdata.size() ) {
            return posdata[i];
        } else {
            return MIDGpos();
        }
    }
    inline MIDGatt get_attpt( const unsigned int i )
    {
        if ( i < attdata.size() ) {
            return attdata[i];
        } else {
            return MIDGatt();
        }
    }
        

};


MIDGpos MIDGInterpPos( const MIDGpos A, const MIDGpos B, const double percent );
MIDGatt MIDGInterpAtt( const MIDGatt A, const MIDGatt B, const double percent );


#endif // _FG_MIDG_II_HXX
