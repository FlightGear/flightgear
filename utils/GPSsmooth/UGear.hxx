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

SG_USING_STD(cout);
SG_USING_STD(endl);
SG_USING_STD(string);
SG_USING_STD(vector);


enum ugPacketType {
    GPS_PACKET = 0,
    IMU_PACKET = 1,
    NAV_PACKET = 2,
    SERVO_PACKET = 3
};

struct imu {
   double p,q,r;		/* angular velocities    */
   double ax,ay,az;		/* acceleration          */
   double hx,hy,hz;             /* magnetic field     	 */
   double Ps,Pt;                /* static/pitot pressure */
   // double Tx,Ty,Tz;          /* temperature           */
   double phi,the,psi;          /* attitudes             */
   short  err_type;		/* error type		 */
   double time;
};

struct gps {
   double lat,lon,alt;          /* gps position          */
   double ve,vn,vd;             /* gps velocity          */
   uint16_t ITOW;
   short  err_type;             /* error type            */
   double time;
};

struct nav {
   double lat,lon,alt;
   double ve,vn,vd;
   // float  t;
   short  err_type;
   double time;
};

struct servo {
   uint16_t chn[8];
   uint8_t status;
   double time;
};


// Manage a saved ugear log (track file)
class UGEARTrack {

private:

    vector <gps> gps_data;
    vector <imu> imu_data;
    vector <nav> nav_data;
    vector <servo> servo_data;

    // parse message and put current data into vector if message has a
    // newer time stamp than existing data.
    void parse_msg( const int id, char *buf,
		    gps *gpspacket, imu *imupacket, nav *navpacket,
		    servo *servopacket );

public:

    UGEARTrack();
    ~UGEARTrack();

    // read/parse the next message from the specified data stream,
    // returns id # if a valid message found.
    int next_message( SGIOChannel *ch, SGIOChannel *log,
                      gps *gpspacket, imu *imupacket, nav *navpacket,
		      servo *servopacket );
    int next_message( SGSerialPort *serial, SGIOChannel *log,
                      gps *gpspacket, imu *imupacket, nav *navpacket,
		      servo *servopacket );

    // load the named file into internal buffers
    bool load( const string &file );

    inline int gps_size() const { return gps_data.size(); }
    inline int imu_size() const { return imu_data.size(); }
    inline int nav_size() const { return nav_data.size(); }
    inline int servo_size() const { return servo_data.size(); }

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
        

};


gps UGEARInterpGPS( const gps A, const gps B, const double percent );
imu UGEARInterpIMU( const imu A, const imu B, const double percent );
nav UGEARInterpNAV( const nav A, const nav B, const double percent );
servo UGEARInterpSERVO( const servo A, const servo B, const double percent );


#endif // _FG_UGEAR_II_HXX
