#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif 

#include <iostream>
#include <cstdio>

#include <simgear/constants.h>
#include <simgear/io/sg_file.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/misc/stdint.hxx>

#include "UGear.hxx"

using std::cout;
using std::endl;


#define START_OF_MSG0 147
#define START_OF_MSG1 224


UGTrack::UGTrack():
    sg_swap(false)
{
};

UGTrack::~UGTrack() {};


// swap the 1st 4 bytes with the last 4 bytes of a stargate double so
// it matches the PC representation
static double sg_swap_double( uint8_t *buf, size_t offset ) {
    double *result;
    uint8_t tmpbuf[10];

    for ( size_t i = 0; i < 4; ++i ) {
      tmpbuf[i] = buf[offset + i + 4];
    }
    for ( size_t i = 0; i < 4; ++i ) {
      tmpbuf[i + 4] = buf[offset + i];
    }

    // for ( size_t i = 0; i < 8; ++i ) {
    //   printf("%d ", tmpbuf[i]);
    // }
    // printf("\n");

    result = (double *)tmpbuf;
    return *result;
}


static bool validate_cksum( uint8_t id, uint8_t size, char *buf,
                            uint8_t cksum0, uint8_t cksum1,
			    bool ignore_checksum )
{
    if ( ignore_checksum ) {
        return true;
    }

    uint8_t c0 = 0;
    uint8_t c1 = 0;

    c0 += id;
    c1 += c0;
    // cout << "c0 = " << (unsigned int)c0 << " c1 = " << (unsigned int)c1 << endl;

    c0 += size;
    c1 += c0;
    // cout << "c0 = " << (unsigned int)c0 << " c1 = " << (unsigned int)c1 << endl;

    for ( uint8_t i = 0; i < size; i++ ) {
        c0 += (uint8_t)buf[i];
        c1 += c0;
        // cout << "c0 = " << (unsigned int)c0 << " c1 = " << (unsigned int)c1
        //      << " [" << (unsigned int)(uint8_t)buf[i] << "]" << endl;
    }

    // cout << "c0 = " << (unsigned int)c0 << " (" << (unsigned int)cksum0
    //      << ") c1 = " << (unsigned int)c1 << " (" << (unsigned int)cksum1
    //      << ")" << endl;

    if ( c0 == cksum0 && c1 == cksum1 ) {
        return true;
    } else {
        return false;
    }
}


void UGTrack::parse_msg( const int id, char *buf,
			    struct gps *gpspacket, imu *imupacket,
			    nav *navpacket, servo *servopacket,
			    health *healthpacket )
{
    if ( id == GPS_PACKET ) {
      *gpspacket = *(struct gps *)buf;
      if ( sg_swap ) {
          gpspacket->time = sg_swap_double( (uint8_t *)buf, 0 );
          gpspacket->lat  = sg_swap_double( (uint8_t *)buf, 8 );
          gpspacket->lon  = sg_swap_double( (uint8_t *)buf, 16 );
          gpspacket->alt  = sg_swap_double( (uint8_t *)buf, 24 );
          gpspacket->vn   = sg_swap_double( (uint8_t *)buf, 32 );
          gpspacket->ve   = sg_swap_double( (uint8_t *)buf, 40 );
          gpspacket->vd   = sg_swap_double( (uint8_t *)buf, 48 );
          gpspacket->ITOW = sg_swap_double( (uint8_t *)buf, 56 );
      }
    } else if ( id == IMU_PACKET ) {
      *imupacket = *(struct imu *)buf;
      if ( sg_swap ) {
          imupacket->time = sg_swap_double( (uint8_t *)buf, 0 );
          imupacket->p    = sg_swap_double( (uint8_t *)buf, 8 );
          imupacket->q    = sg_swap_double( (uint8_t *)buf, 16 );
          imupacket->r    = sg_swap_double( (uint8_t *)buf, 24 );
          imupacket->ax   = sg_swap_double( (uint8_t *)buf, 32 );
          imupacket->ay   = sg_swap_double( (uint8_t *)buf, 40 );
          imupacket->az   = sg_swap_double( (uint8_t *)buf, 48 );
          imupacket->hx   = sg_swap_double( (uint8_t *)buf, 56 );
          imupacket->hy   = sg_swap_double( (uint8_t *)buf, 64 );
          imupacket->hz   = sg_swap_double( (uint8_t *)buf, 72 );
          imupacket->Ps   = sg_swap_double( (uint8_t *)buf, 80 );
          imupacket->Pt   = sg_swap_double( (uint8_t *)buf, 88 );
          imupacket->phi  = sg_swap_double( (uint8_t *)buf, 96 );
          imupacket->the  = sg_swap_double( (uint8_t *)buf, 104 );
          imupacket->psi  = sg_swap_double( (uint8_t *)buf, 112 );
      }
      // printf("imu.time = %.4f  size = %d\n", imupacket->time, sizeof(struct imu));
    } else if ( id == NAV_PACKET ) {
      *navpacket = *(struct nav *)buf;
      if ( sg_swap ) {
          navpacket->time = sg_swap_double( (uint8_t *)buf, 0 );
          navpacket->lat  = sg_swap_double( (uint8_t *)buf, 8 );
          navpacket->lon  = sg_swap_double( (uint8_t *)buf, 16 );
          navpacket->alt  = sg_swap_double( (uint8_t *)buf, 24 );
          navpacket->vn   = sg_swap_double( (uint8_t *)buf, 32 );
          navpacket->ve   = sg_swap_double( (uint8_t *)buf, 40 );
          navpacket->vd   = sg_swap_double( (uint8_t *)buf, 48 );
      }
    } else if ( id == SERVO_PACKET ) {
      *servopacket = *(struct servo *)buf;
      if ( sg_swap ) {
          servopacket->time = sg_swap_double( (uint8_t *)buf, 0 );
      }
      // printf("servo time = %.3f %d %d\n", servopacket->time, servopacket->chn[0], servopacket->chn[1]);
    } else if ( id == HEALTH_PACKET ) {
      *healthpacket = *(struct health *)buf;
      if ( sg_swap ) {
          healthpacket->time = sg_swap_double( (uint8_t *)buf, 0 );
      }
    } else {
        cout << "unknown id = " << id << endl;
    }
}


// load the named stream log file into internal buffers
bool UGTrack::load_stream( const string &file, bool ignore_checksum ) {
    int count = 0;

    gps gpspacket;
    imu imupacket;
    nav navpacket;
    servo servopacket;
    health healthpacket;

    double gps_time = 0;
    double imu_time = 0;
    double nav_time = 0;
    double servo_time = 0;
    double health_time = 0;

    gps_data.clear();
    imu_data.clear();
    nav_data.clear();
    servo_data.clear();
    health_data.clear();

    // open the file
    SGFile input( file );
    if ( !input.open( SG_IO_IN ) ) {
        cout << "Cannot open file: " << file << endl;
        return false;
    }

    while ( ! input.eof() ) {
        // cout << "looking for next message ..." << endl;
        int id = next_message( &input, NULL, &gpspacket, &imupacket,
			       &navpacket, &servopacket, &healthpacket,
			       ignore_checksum );
        count++;

        if ( id == GPS_PACKET ) {
            if ( gpspacket.time > gps_time ) {
                gps_data.push_back( gpspacket );
                gps_time = gpspacket.time;
            } else {
	      cout << "oops gps back in time: " << gpspacket.time << " " << gps_time << endl;
            }
        } else if ( id == IMU_PACKET ) {
            if ( imupacket.time > imu_time ) {
                imu_data.push_back( imupacket );
                imu_time = imupacket.time;
            } else {
                cout << "oops imu back in time" << endl;
            }
        } else if ( id == NAV_PACKET ) {
            if ( navpacket.time > nav_time ) {
                nav_data.push_back( navpacket );
                nav_time = navpacket.time;
            } else {
                cout << "oops nav back in time" << endl;
            }
        } else if ( id == SERVO_PACKET ) {
            if ( servopacket.time > servo_time ) {
                servo_data.push_back( servopacket );
                servo_time = servopacket.time;
            } else {
                cout << "oops servo back in time" << endl;
            }
        } else if ( id == HEALTH_PACKET ) {
            if ( healthpacket.time > health_time ) {
                health_data.push_back( healthpacket );
                health_time = healthpacket.time;
            } else {
                cout << "oops health back in time" << endl;
            }
        }
    }

    cout << "processed " << count << " messages" << endl;
    return true;
}


// load the named stream log file into internal buffers
bool UGTrack::load_flight( const string &path ) {
    gps gpspacket;
    imu imupacket;
    nav navpacket;
    servo servopacket;
    health healthpacket;

    gps_data.clear();
    imu_data.clear();
    nav_data.clear();
    servo_data.clear();
    health_data.clear();

    gzFile fgps = NULL;
    gzFile fimu = NULL;
    gzFile fnav = NULL;
    gzFile fservo = NULL;
    gzFile fhealth = NULL;

    SGPath file;
    int size;

    // open the gps file
    file = path; file.append( "gps.dat.gz" );
    if ( (fgps = gzopen( file.c_str(), "r" )) == NULL ) {
        printf("Cannot open %s\n", file.c_str());
        return false;
    }

    size = sizeof( struct gps );
    printf("gps size = %d\n", size);
    while ( gzread( fgps, &gpspacket, size ) == size ) {
        gps_data.push_back( gpspacket );
    }

    // open the imu file
    file = path; file.append( "imu.dat.gz" );
    if ( (fimu = gzopen( file.c_str(), "r" )) == NULL ) {
        printf("Cannot open %s\n", file.c_str());
        return false;
    }

    size = sizeof( struct imu );
    printf("imu size = %d\n", size);
    while ( gzread( fimu, &imupacket, size ) == size ) {
        imu_data.push_back( imupacket );
    }

    // open the nav file
    file = path; file.append( "nav.dat.gz" );
    if ( (fnav = gzopen( file.c_str(), "r" )) == NULL ) {
        printf("Cannot open %s\n", file.c_str());
        return false;
    }

    size = sizeof( struct nav );
    printf("nav size = %d\n", size);
    while ( gzread( fnav, &navpacket, size ) == size ) {
        // printf("%.4f %.4f\n", navpacket.lat, navpacket.lon);
        nav_data.push_back( navpacket );
    }

    // open the servo file
    file = path; file.append( "servo.dat.gz" );
    if ( (fservo = gzopen( file.c_str(), "r" )) == NULL ) {
        printf("Cannot open %s\n", file.c_str());
        return false;
    }

    size = sizeof( struct servo );
    printf("servo size = %d\n", size);
    while ( gzread( fservo, &servopacket, size ) == size ) {
        servo_data.push_back( servopacket );
    }

    // open the health file
    file = path; file.append( "health.dat.gz" );
    if ( (fhealth = gzopen( file.c_str(), "r" )) == NULL ) {
        printf("Cannot open %s\n", file.c_str());
        return false;
    }

    size = sizeof( struct health );
    printf("health size = %d\n", size);
    while ( gzread( fhealth, &healthpacket, size ) == size ) {
        health_data.push_back( healthpacket );
    }

    return true;
}


// attempt to work around some system dependent issues.  Our read can
// return < data than we want.
int myread( SGIOChannel *ch, SGIOChannel *log, char *buf, int length ) {
    bool myeof = false;
    int result = 0;
    if ( !myeof ) {
      result = ch->read( buf, length );
      // cout << "wanted " << length << " read " << result << " bytes" << endl;
      if ( ch->get_type() == sgFileType ) {
	myeof = ((SGFile *)ch)->eof();
      }
    }

    if ( result > 0 && log != NULL ) {
        log->write( buf, result );
    }

    return result;
}

// attempt to work around some system dependent issues.  Our read can
// return < data than we want.
int serial_read( SGSerialPort *serial, SGIOChannel *log,
		 char *buf, int length )
{
    int result = 0;
    int bytes_read = 0;
    char *tmp = buf;

    while ( bytes_read < length ) {
      result = serial->read_port( tmp, length - bytes_read );
      bytes_read += result;
      tmp += result;
      // cout << "  read " << bytes_read << " of " << length << endl;
    }

    if ( bytes_read > 0 && log != NULL ) {
      log->write( buf, bytes_read );
    }

    return bytes_read;
}


// load the next message of a real time data stream
int UGTrack::next_message( SGIOChannel *ch, SGIOChannel *log,
			      gps *gpspacket, imu *imupacket, nav *navpacket,
			      servo *servopacket, health *healthpacket,
			      bool ignore_checksum )
{
    char tmpbuf[256];
    char savebuf[256];

    // cout << "in next_message()" << endl;

    bool myeof = false;

    // scan for sync characters
    uint8_t sync0, sync1;
    myread( ch, log, tmpbuf, 2 );
    sync0 = (unsigned char)tmpbuf[0];
    sync1 = (unsigned char)tmpbuf[1];
    while ( (sync0 != START_OF_MSG0 || sync1 != START_OF_MSG1) && !myeof ) {
        sync0 = sync1;
        myread( ch, log, tmpbuf, 1 ); sync1 = (unsigned char)tmpbuf[0];
        cout << "scanning for start of message "
	     << (unsigned int)sync0 << " " << (unsigned int)sync1
	     << ", eof = " << ch->eof() << endl;
        if ( ch->get_type() == sgFileType ) {
            myeof = ((SGFile *)ch)->eof();
        }
    }

    cout << "found start of message ..." << endl;

    // read message id and size
    myread( ch, log, tmpbuf, 2 );
    uint8_t id = (unsigned char)tmpbuf[0];
    uint8_t size = (unsigned char)tmpbuf[1];
    // cout << "message = " << (int)id << " size = " << (int)size << endl;

    // load message
    if ( ch->get_type() == sgFileType ) {
        int count = myread( ch, log, savebuf, size );
        if ( count != size ) {
            cout << "ERROR: didn't read enough bytes!" << endl;
        }
    } else {
#ifdef READ_ONE_BY_ONE
        for ( int i = 0; i < size; ++i ) {
            myread( ch, log, tmpbuf, 1 ); savebuf[i] = tmpbuf[0];
        }
#else
	myread( ch, log, savebuf, size );
#endif
    }

    // read checksum
    myread( ch, log, tmpbuf, 2 );
    uint8_t cksum0 = (unsigned char)tmpbuf[0];
    uint8_t cksum1 = (unsigned char)tmpbuf[1];
    
    if ( validate_cksum( id, size, savebuf, cksum0, cksum1, ignore_checksum ) )
    {
        parse_msg( id, savebuf, gpspacket, imupacket, navpacket, servopacket,
		   healthpacket );
        return id;
    }

    cout << "Check sum failure!" << endl;
    return -1;
}


// load the next message of a real time data stream
int UGTrack::next_message( SGSerialPort *serial, SGIOChannel *log,
                           gps *gpspacket, imu *imupacket, nav *navpacket,
                           servo *servopacket, health *healthpacket,
                           bool ignore_checksum )
{
    char tmpbuf[256];
    char savebuf[256];
    int result = 0;

    // cout << "in next_message()" << endl;

    bool myeof = false;

    // scan for sync characters
    int scan_count = 0;
    uint8_t sync0, sync1;
    result = serial_read( serial, log, tmpbuf, 2 );
    sync0 = (unsigned char)tmpbuf[0];
    sync1 = (unsigned char)tmpbuf[1];
    while ( (sync0 != START_OF_MSG0 || sync1 != START_OF_MSG1) && !myeof ) {
        scan_count++;
        sync0 = sync1;
        serial_read( serial, log, tmpbuf, 1 ); sync1 = (unsigned char)tmpbuf[0];
        // cout << "scanning for start of message "
	//      << (unsigned int)sync0 << " " << (unsigned int)sync1
	//      << endl;
    }

    if ( scan_count > 0 ) {
        cout << "found start of message after discarding " << scan_count
             << " bytes" << endl;
    }
    // cout << "found start of message ..." << endl;

    // read message id and size
    serial_read( serial, log, tmpbuf, 2 );
    uint8_t id = (unsigned char)tmpbuf[0];
    uint8_t size = (unsigned char)tmpbuf[1];
    // cout << "message = " << (int)id << " size = " << (int)size << endl;

    // load message
    serial_read( serial, log, savebuf, size );

    // read checksum
    serial_read( serial, log, tmpbuf, 2 );
    uint8_t cksum0 = (unsigned char)tmpbuf[0];
    uint8_t cksum1 = (unsigned char)tmpbuf[1];
    // cout << "cksum0 = " << (int)cksum0 << " cksum1 = " << (int)cksum1
    //      << endl;
    
    if ( validate_cksum( id, size, savebuf, cksum0, cksum1, ignore_checksum ) )
    {
        parse_msg( id, savebuf, gpspacket, imupacket, navpacket, servopacket,
		   healthpacket );

        return id;
    }

    cout << "Check sum failure!" << endl;
    return -1;

    
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


gps UGEARInterpGPS( const gps A, const gps B, const double percent )
{
    gps p;
    p.time = interp(A.time, B.time, percent);
    p.lat = interp(A.lat, B.lat, percent);
    p.lon = interp(A.lon, B.lon, percent);
    p.alt = interp(A.alt, B.alt, percent);
    p.ve = interp(A.ve, B.ve, percent);
    p.vn = interp(A.vn, B.vn, percent);
    p.vd = interp(A.vd, B.vd, percent);
    p.ITOW = (int)interp(A.ITOW, B.ITOW, percent);
    p.err_type = A.err_type;

    return p;
}

imu UGEARInterpIMU( const imu A, const imu B, const double percent )
{
    imu p;
    p.time = interp(A.time, B.time, percent);
    p.p = interp(A.p, B.p, percent);
    p.q = interp(A.q, B.q, percent);
    p.r = interp(A.r, B.r, percent);
    p.ax = interp(A.ax, B.ax, percent);
    p.ay = interp(A.ay, B.ay, percent);
    p.az = interp(A.az, B.az, percent);
    p.hx = interp(A.hx, B.hx, percent);
    p.hy = interp(A.hy, B.hy, percent);
    p.hz = interp(A.hz, B.hz, percent);
    p.Ps = interp(A.Ps, B.Ps, percent);
    p.Pt = interp(A.Pt, B.Pt, percent);
    p.phi = interp(A.phi, B.phi, percent, true);
    p.the = interp(A.the, B.the, percent, true);
    p.psi = interp(A.psi, B.psi, percent, true);
    p.err_type = A.err_type;

    return p;
}

nav UGEARInterpNAV( const nav A, const nav B, const double percent )
{
    nav p;
    p.time = interp(A.time, B.time, percent);
    p.lat = interp(A.lat, B.lat, percent);
    p.lon = interp(A.lon, B.lon, percent);
    p.alt = interp(A.alt, B.alt, percent);
    p.ve = interp(A.ve, B.ve, percent);
    p.vn = interp(A.vn, B.vn, percent);
    p.vd = interp(A.vd, B.vd, percent);
    p.err_type = A.err_type;

    return p;
}


servo UGEARInterpSERVO( const servo A, const servo B, const double percent )
{
    servo p;
    for ( int i = 0; i < 8; ++i ) {
      p.chn[i] = (uint16_t)interp(A.chn[i], B.chn[i], percent);
    }
    p.status = A.status;

    return p;
}


health UGEARInterpHEALTH( const health A, const health B, const double percent )
{
    health p;
    p.command_sequence = B.command_sequence;
    p.time = interp(A.time, B.time, percent);

    return p;
}

