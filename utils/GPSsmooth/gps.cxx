#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <iostream>

#include <simgear/constants.h>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/strutils.hxx>

#include "gps.hxx"

using std::cout;
using std::endl;


GPSTrack::GPSTrack() {};
GPSTrack::~GPSTrack() {};


// load the specified file, return the number of records loaded
int GPSTrack::load( const string &file ) {
    int count = 0;

    data.clear();

    // openg the file
    sg_gzifstream in( file );
    if ( !in.is_open() ) {
        cout << "Cannot open file: " << file << endl;
        return 0;
    }

    vector <string> tokens;
    GPSPoint p;

    while ( ! in.eof() ) {
        char tmp[2049];

	in.getline(tmp, 2048);

        tokens.clear();
        tokens = simgear::strutils::split(tmp, ",");

        int dd;
        double raw, min;

        if ( tokens[0] == "$GPRMC" && tokens.size() == 13 ) {
            double raw_time = atof(tokens[1].c_str());
            GPSTime gps_time = GPSTime( raw_time );
            if ( (gps_time.get_time() > p.gps_time.get_time()) &&
                 (p.gps_time.get_time() > 1.0) )
            {
                // new data cycle store last data before continuing
                data.push_back( p );
                count++;
            }

            p.gps_time = gps_time;

            raw = atof( tokens[3].c_str() );
            dd = (int)(raw / 100.00);
            min = raw - dd * 100.0;
            p.lat_deg = dd + min / 60.0;
            if ( tokens[4] == "S" ) {
                p.lat_deg = -p.lat_deg;
            }
            raw = atof( tokens[5].c_str() );
            dd = (int)(raw / 100.00);
            min = raw - dd * 100.0;
            p.lon_deg = dd + min / 60.0;
            if ( tokens[6] == "W" ) {
                p.lon_deg = -p.lon_deg;
            }

            static double max_speed = 0.0;
            p.speed_kts = atof( tokens[7].c_str() );
            if ( p.speed_kts > max_speed ) {
                max_speed = p.speed_kts;
                cout << "max speed = " << max_speed << endl;
            }
            p.course_true = atof( tokens[8].c_str() ) * SGD_DEGREES_TO_RADIANS;

        } else if ( tokens[0] == "$GPGGA" && tokens.size() == 15 ) {
            double raw_time = atof(tokens[1].c_str());
            GPSTime gps_time = GPSTime( raw_time );
            if ( fabs(gps_time.get_time() - p.gps_time.get_time()) > 0.0001 &&
                 (p.gps_time.get_time() > 1.0) ) {
                // new data cycle store last data before continuing
                data.push_back( p );
                count++;
            }

            p.gps_time = gps_time;

            raw = atof( tokens[2].c_str() );
            dd = (int)(raw / 100.00);
            min = raw - dd * 100.0;
            p.lat_deg = dd + min / 60.0;
            if ( tokens[3] == "S" ) {
                p.lat_deg = -p.lat_deg;
            }
            raw = atof( tokens[4].c_str() );
            dd = (int)(raw / 100.00);
            min = raw - dd * 100.0;
            p.lon_deg = dd + min / 60.0;
            if ( tokens[5] == "W" ) {
                p.lon_deg = -p.lon_deg;
            }

            p.fix_quality = atoi( tokens[6].c_str() );
            p.num_satellites = atoi( tokens[7].c_str() );
            p.hdop = atof( tokens[8].c_str() );

            static double max_alt = 0.0;
            double alt = atof( tokens[9].c_str() );
            if ( alt > max_alt ) {
                max_alt = alt;
                cout << "max alt = " << max_alt << endl;
            }

            if ( tokens[10] == "F" || tokens[10] == "f" ) {
                alt *= SG_FEET_TO_METER;
            }
            p.altitude_msl = alt;
        }
    }

    return count;
}


static double interp( double a, double b, double p, bool rotational = false ) {
    double diff = b - a;
    if ( rotational ) {
        // special handling of rotational data
        if ( diff > SGD_PI ) {
            diff -= SGD_2PI;
        } else if ( diff < -SGD_PI ) {
            diff += SGD_2PI;
        }
    }
    return a + diff * p;
}


GPSPoint GPSInterpolate( const GPSPoint A, const GPSPoint B,
                         const double percent ) {
    GPSPoint p;
    p.gps_time = GPSTime((int)interp(A.gps_time.get_time(),
                                     B.gps_time.get_time(),
                                     percent));
    p.lat_deg = interp(A.lat_deg, B.lat_deg, percent);
    p.lon_deg = interp(A.lon_deg, B.lon_deg, percent);
    p.fix_quality = (int)interp(A.fix_quality, B.fix_quality, percent);
    p.num_satellites = (int)interp(A.num_satellites, B.num_satellites, percent);
    p.hdop = interp(A.hdop, B.hdop, percent);
    p.altitude_msl = interp(A.altitude_msl, B.altitude_msl, percent);
    p.speed_kts = interp(A.speed_kts, B.speed_kts, percent);
    p.course_true = interp(A.course_true, B.course_true, percent, true);

    return p;
}
