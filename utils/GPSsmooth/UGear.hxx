#ifndef _FG_UGEAR_II_HXX
#define _FG_UGEAR_II_HXX


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


enum ugPacketType {
    GPS_PACKET = 0,
    IMU_PACKET = 1,
    NAV_PACKET = 2,
    SERVO_PACKET = 3,
    HEALTH_PACKET = 4
};

struct imu {
   double time;
   double p,q,r;		/* angular velocities    */
   double ax,ay,az;		/* acceleration          */
   double hx,hy,hz;             /* magnetic field     	 */
   double Ps,Pt;                /* static/pitot pressure */
   // double Tx,Ty,Tz;          /* temperature           */
   double phi,the,psi;          /* attitudes             */
   uint64_t err_type;		/* error type		 */
};

struct gps {
   double time;
   double lat,lon,alt;          /* gps position          */
   double ve,vn,vd;             /* gps velocity          */
   double ITOW;
   uint64_t err_type;           /* error type            */
};

struct nav {
   double time;
   double lat,lon,alt;
   double ve,vn,vd;
   // float  t;
   uint64_t err_type;
};

struct servo {
   double time;
   uint16_t chn[8];
   uint64_t status;
};

struct health {
    double time;
    double target_roll_deg;     /* AP target roll angle */
    double target_heading_deg;  /* AP target heading angle */
    double target_pitch_deg;    /* AP target pitch angle */
    double target_climb_fps;    /* AP target climb rate */
    double target_altitude_ft;  /* AP target altitude */
    uint64_t command_sequence;  /* highest received command sequence num */
    uint64_t target_waypoint;   /* index of current waypoint target */
    uint64_t loadavg;           /* system "1 minute" load average */
    uint64_t ahrs_hz;           /* actual ahrs loop hz */
    uint64_t nav_hz;            /* actual nav loop hz */
};

// Manage a saved ugear log (track file)
class UGTrack {

private:

    vector <gps> gps_data;
    vector <imu> imu_data;
    vector <nav> nav_data;
    vector <servo> servo_data;
    vector <health> health_data;

    // parse message and put current data into vector if message has a
    // newer time stamp than existing data.
    void parse_msg( const int id, char *buf,
		    gps *gpspacket, imu *imupacket, nav *navpacket,
		    servo *servopacket, health *healthpacket );

    // activate special double swap logic for non-standard stargate
    // double format
    bool sg_swap;

public:

    UGTrack();
    ~UGTrack();

    // read/parse the next message from the specified data stream,
    // returns id # if a valid message found.
    int next_message( SGIOChannel *ch, SGIOChannel *log,
                      gps *gpspacket, imu *imupacket, nav *navpacket,
		      servo *servopacket, health * healthpacket,
		      bool ignore_checksum );
    int next_message( SGSerialPort *serial, SGIOChannel *log,
                      gps *gpspacket, imu *imupacket, nav *navpacket,
		      servo *servopacket, health *healthpacket,
		      bool ignore_checksum );

    // load the named stream log file into internal buffers
    bool load_stream( const string &file, bool ignore_checksum );

    // load the named flight files into internal buffers
    bool load_flight( const string &path );

    inline int gps_size() const { return gps_data.size(); }
    inline int imu_size() const { return imu_data.size(); }
    inline int nav_size() const { return nav_data.size(); }
    inline int servo_size() const { return servo_data.size(); }
    inline int health_size() const { return health_data.size(); }

    inline gps get_gpspt( const unsigned int i )
    {
        if ( i < gps_data.size() ) {
            return gps_data[i];
        } else {
            return gps();
        }
    }
    inline imu get_imupt( const unsigned int i )
    {
        if ( i < imu_data.size() ) {
            return imu_data[i];
        } else {
            return imu();
        }
    }
    inline nav get_navpt( const unsigned int i )
    {
        if ( i < nav_data.size() ) {
            return nav_data[i];
        } else {
            return nav();
        }
    }
    inline servo get_servopt( const unsigned int i )
    {
        if ( i < servo_data.size() ) {
            return servo_data[i];
        } else {
            return servo();
        }
    }
    inline health get_healthpt( const unsigned int i )
    {
        if ( i < health_data.size() ) {
            return health_data[i];
        } else {
            return health();
        }
    }
       

    // set stargate mode where we have to do an odd swapping of doubles to
    // account for their non-standard formate
    inline void set_stargate_swap_mode() {
        sg_swap = true;
    }
};


gps UGEARInterpGPS( const gps A, const gps B, const double percent );
imu UGEARInterpIMU( const imu A, const imu B, const double percent );
nav UGEARInterpNAV( const nav A, const nav B, const double percent );
servo UGEARInterpSERVO( const servo A, const servo B, const double percent );
health UGEARInterpHEALTH( const health A, const health B,
			  const double percent );


#endif // _FG_UGEAR_II_HXX
