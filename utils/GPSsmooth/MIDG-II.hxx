#ifndef _FG_MIDG_II_HXX
#define _FG_MIDG_II_HXX


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <iostream>
#include <string>
#include <vector>

#include <simgear/misc/stdint.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/serial/serial.hxx>

using std::cout;
using std::endl;
using std::string;
using std::vector;


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

    vector <MIDGpos> pos_data;
    vector <MIDGatt> att_data;

    // parse message and put current data into vector if message has a
    // newer time stamp than existing data.
    void parse_msg( const int id, char *buf, MIDGpos *pos, MIDGatt *att );

public:

    MIDGTrack();
    ~MIDGTrack();

    // read/parse the next message from the specified data stream,
    // returns id # if a valid message found.
    int next_message( SGIOChannel *ch, SGIOChannel *log,
                      MIDGpos *pos, MIDGatt *att );
    int next_message( SGSerialPort *serial, SGIOChannel *log,
                      MIDGpos *pos, MIDGatt *att );

    // load the named file into internal buffers
    bool load( const string &file );

    inline int pos_size() const { return pos_data.size(); }
    inline int att_size() const { return att_data.size(); }

    inline MIDGpos get_pospt( const unsigned int i )
    {
        if ( i < pos_data.size() ) {
            return pos_data[i];
        } else {
            return MIDGpos();
        }
    }
    inline MIDGatt get_attpt( const unsigned int i )
    {
        if ( i < att_data.size() ) {
            return att_data[i];
        } else {
            return MIDGatt();
        }
    }
        

};


MIDGpos MIDGInterpPos( const MIDGpos A, const MIDGpos B, const double percent );
MIDGatt MIDGInterpAtt( const MIDGatt A, const MIDGatt B, const double percent );


#endif // _FG_MIDG_II_HXX
