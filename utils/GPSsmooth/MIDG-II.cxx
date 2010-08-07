#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif 

#include <simgear/compiler.h>

#include <iostream>

#include <simgear/constants.h>
#include <simgear/io/sg_file.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/misc/stdint.hxx>

#include "MIDG-II.hxx"

using std::cout;
using std::endl;


MIDGTrack::MIDGTrack() {};
MIDGTrack::~MIDGTrack() {};




static uint32_t read_swab( char *buf, size_t offset, size_t size ) {
    uint32_t result = 0;

    char *ptr = buf + offset;

    // MIDG data is big endian so swap if needed.
    if ( sgIsLittleEndian() ) {
        if ( size == 4 ) {
            sgEndianSwap( (uint32_t *)ptr );
        } else if ( size == 2 ) {
            sgEndianSwap( (uint16_t *)ptr );
        }
    }

    if ( size == 4 ) {
        result = *(uint32_t *)ptr;
    } else if ( size == 2 ) {
        result = *(uint16_t *)ptr;
    } else if ( size == 1 ) {
        result = *(uint8_t *)ptr;
    } else {
        cout << "unknown size in read_swab()" << endl;
    }

    return result;
}


static bool validate_cksum( uint8_t id, uint8_t size, char *buf,
                            uint8_t cksum0, uint8_t cksum1 )
{
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
        //      << " [" << (unsigned int)buf[i] << "]" << endl;
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


void MIDGTrack::parse_msg( const int id, char *buf, MIDGpos *pos, MIDGatt *att )
{
    if ( id == 1 ) {
        uint32_t ts;
        uint16_t status;
        int16_t temp;

        // cout << "message 1 =" << endl;

        // timestamp
        ts = (uint32_t)read_swab( buf, 0, 4 );
        // cout << "  time stamp = " << ts << endl;
            
        // status
        status = (uint16_t)read_swab( buf, 4, 2 );
        // cout << "  status = " << status << endl;

        // temp
        temp = (int16_t)read_swab( buf, 6, 2 );
        // cout << "  temp = " << temp << endl;

    } else if ( id == 2 ) {
        uint32_t ts;
        int16_t p, q, r;
        int16_t ax, ay, az;
        int16_t mx, my, mz;
        uint8_t flags;

        // cout << "message 2 =" << endl;

        // timestamp
        ts = (uint32_t)read_swab( buf, 0, 4 );
        // cout << "  time stamp = " << ts << endl;

        // p, q, r
        p = (int16_t)read_swab( buf, 4, 2 );
        q = (int16_t)read_swab( buf, 6, 2 );
        r = (int16_t)read_swab( buf, 8, 2 );
        // cout << "  pqr = " << p << "," << q << "," << r << endl;

        // ax, ay, az
        ax = (int16_t)read_swab( buf, 10, 2 );
        ay = (int16_t)read_swab( buf, 12, 2 );
        az = (int16_t)read_swab( buf, 14, 2 );
        // cout << "  ax ay az = " << ax << "," << ay << "," << az << endl;

        // mx, my, mz
        mx = (int16_t)read_swab( buf, 16, 2 );
        my = (int16_t)read_swab( buf, 18, 2 );
        mz = (int16_t)read_swab( buf, 20, 2 );
        // cout << "  mx my mz = " << mx << "," << my << "," << mz << endl;

        // flags
        flags = (uint8_t)read_swab( buf, 22, 1 );
        // cout << "  GPS 1PPS flag = " << (int)(flags & (1 << 6))
        //      << " Timestamp is gps = " << (int)(flags & (1 << 7)) << endl;

    } else if ( id == 3 ) {
        uint32_t ts;
        int16_t mx, my, mz;
        uint8_t flags;

        // cout << "message 3 =" << endl;

        // timestamp
        ts = (uint32_t)read_swab( buf, 0, 4 );
        // cout << "  time stamp = " << ts << endl;

        // mx, my, mz
        mx = (int16_t)read_swab( buf, 4, 2 );
        my = (int16_t)read_swab( buf, 6, 2 );
        mz = (int16_t)read_swab( buf, 8, 2 );
        // cout << "  mx my mz = " << mx << "," << my << "," << mz << endl;

        // flags
        flags = (uint8_t)read_swab( buf, 10, 1 );
        // cout << "  GPS 1PPS flag = " << (int)(flags & (1 << 6)) << endl;

    } else if ( id == 10 ) {
        uint32_t ts;
        int16_t p, q, r;
        int16_t ax, ay, az;
        int16_t yaw, pitch, roll;
        int32_t Qw, Qx, Qy, Qz;
        uint8_t flags;

        // cout << "message 10 =" << endl;

        // timestamp
        ts = (uint32_t)read_swab( buf, 0, 4 );
        // cout << "  att time stamp = " << ts << endl;
        att->midg_time = MIDGTime( ts );

        // p, q, r
        p = (int16_t)read_swab( buf, 4, 2 );
        q = (int16_t)read_swab( buf, 6, 2 );
        r = (int16_t)read_swab( buf, 8, 2 );
        // cout << "  pqr = " << p << "," << q << "," << r << endl;

        // ax, ay, az
        ax = (int16_t)read_swab( buf, 10, 2 );
        ay = (int16_t)read_swab( buf, 12, 2 );
        az = (int16_t)read_swab( buf, 14, 2 );
        // cout << "  ax ay az = " << ax << "," << ay << "," << az << endl;

        // yaw, pitch, roll
        yaw =   (int16_t)read_swab( buf, 16, 2 );
        pitch = (int16_t)read_swab( buf, 18, 2 );
        roll =  (int16_t)read_swab( buf, 20, 2 );
        // cout << "  yaw, pitch, roll = " << yaw << "," << pitch << ","
        //      << roll << endl;
        att->yaw_rad = ( (double)yaw / 100.0 ) * SG_PI / 180.0;
        att->pitch_rad = ( (double)pitch / 100.0 ) * SG_PI / 180.0;
        att->roll_rad = ( (double)roll / 100.0 ) * SG_PI / 180.0;

        // Qw, Qx, Qy, Qz
        Qw = (int32_t)read_swab( buf, 22, 4 );
        Qx = (int32_t)read_swab( buf, 26, 4 );
        Qy = (int32_t)read_swab( buf, 30, 4 );
        Qz = (int32_t)read_swab( buf, 34, 4 );
        // cout << "  Qw,Qx,Qy,Qz = " << Qw << "," << Qx << "," << Qy << ","
        //      << Qz << endl;

        // flags
        flags = (uint8_t)read_swab( buf, 38, 1 );
        // cout << "  External hdg measurement applied = "
        //      << (int)(flags & (1 << 3)) << endl
        //      << "  Magnatometer measurement applied = "
        //      << (int)(flags & (1 << 4)) << endl
        //      << "  DGPS = " << (int)(flags & (1 << 5)) << endl
        //      << "  Timestamp is gps = " << (int)(flags & (1 << 6)) << endl
        //      << "  INS mode = " << (int)(flags & (1 << 7))
        //      << endl;

    } else if ( id == 12 ) {
        uint32_t ts;
        int32_t posx, posy, posz;
        int32_t velx, vely, velz;
        uint8_t flags;

        // cout << "message 12 =" << endl;

        // timestamp
        ts = (uint32_t)read_swab( buf, 0, 4 );
        // cout << "  pos time stamp = " << ts << endl;
        pos->midg_time = MIDGTime( ts );

        // posx, posy, posz
        posx = (int32_t)read_swab( buf, 4, 4 );
        posy = (int32_t)read_swab( buf, 8, 4 );
        posz = (int32_t)read_swab( buf, 12, 4 );
        // cout << "  pos = " << posx << "," << posy << "," << posz << endl;

        double xyz[3];
        xyz[0] = (double)posx/100; xyz[1] = (double)posy/100; xyz[2] = (double)posz/100;
        double lat, lon, alt;
        sgCartToGeod(xyz, &lat, &lon, &alt);
        pos->lat_deg = lat * 180.0 / SG_PI;
        pos->lon_deg = lon * 180.0 / SG_PI;
        pos->altitude_msl = alt;
        // cout << "  lon = " << pos->lon_deg << " lat = " << pos->lat_deg
        //      << " alt = " << pos->altitude_msl << endl;

        // velx, vely, velz
        velx = (int32_t)read_swab( buf, 16, 4 );
        vely = (int32_t)read_swab( buf, 20, 4 );
        velz = (int32_t)read_swab( buf, 24, 4 );
        // cout << "  vel = " << velx << "," << vely << "," << velz << endl;
        double tmp1 = velx*velx + vely*vely + velz*velz;
        double vel_cms = sqrt( tmp1 );
        double vel_ms = vel_cms / 100.0;
        pos->speed_kts = vel_ms * SG_METER_TO_NM * 3600;

        // flags
        flags = (uint8_t)read_swab( buf, 28, 1 );
        // cout << "  ENU pos rel to 1st fix = " << (int)(flags & (1 << 0)) << endl
        //      << "  Velocity format = " << (int)(flags & (1 << 1)) << endl
        //      << "  bit 2 = " << (int)(flags & (1 << 2)) << endl
        //      << "  bit 3 = " << (int)(flags & (1 << 3)) << endl
        //      << "  GPS pos/vel valid = " << (int)(flags & (1 << 4)) << endl
        //      << "  DGPS = " << (int)(flags & (1 << 5)) << endl
        //      << "  Timestamp is gps = " << (int)(flags & (1 << 6)) << endl
        //      << "  Solution src (0=gps, 1=ins) = " << (int)(flags & (1 << 7))
        //      << endl;

    } else if ( id == 20 ) {
        uint32_t gps_ts, gps_week;
        uint16_t details;
        int32_t gps_posx, gps_posy, gps_posz;
        int32_t gps_velx, gps_vely, gps_velz;
        int16_t pdop, pacc, sacc;

        // cout << "message 20 =" << endl;

        // timestamp -- often slightly off from midg time stamp so
        // let's not use gps ts to determine if we need to push the
        // previous data or not, just roll it into the current data
        // independent of time stamp.
        gps_ts = (uint32_t)read_swab( buf, 0, 4 );
        // pt->midg_time = MIDGTime( ts );

        gps_week = (uint16_t)read_swab( buf, 4, 2 );
        // cout << "  gps time stamp = " << gps_ts << " week = " << gps_week
        //      <<  endl;

        // details
        details = (uint16_t)read_swab( buf, 6, 2 );
        // cout << "  details = " << details <<  endl;

        // gps_posx, gps_posy, gps_posz
        gps_posx = (int32_t)read_swab( buf, 8, 4 );
        gps_posy = (int32_t)read_swab( buf, 12, 4 );
        gps_posz = (int32_t)read_swab( buf, 16, 4 );
        // cout << "  gps_pos = " << gps_posx << "," << gps_posy << ","
        //      << gps_posz << endl;

        // gps_velx, gps_vely, gps_velz
        gps_velx = (int32_t)read_swab( buf, 20, 4 );
        gps_vely = (int32_t)read_swab( buf, 24, 4 );
        gps_velz = (int32_t)read_swab( buf, 28, 4 );
        // cout << "  gps_vel = " << gps_velx << "," << gps_vely << ","
        //      << gps_velz << endl;

        // position dop
        pdop = (uint16_t)read_swab( buf, 32, 2 );
        // cout << "  pdop = " << pdop <<  endl;
       
        // position accuracy
        pacc = (uint16_t)read_swab( buf, 34, 2 );
        // cout << "  pacc = " << pacc <<  endl;
       
        // speed accuracy
        sacc = (uint16_t)read_swab( buf, 36, 2 );
        // cout << "  sacc = " << sacc <<  endl;
       
    } else {
        cout << "unknown id = " << id << endl;
    }
}


// load the specified file, return the number of records loaded
bool MIDGTrack::load( const string &file ) {
    int count = 0;

    MIDGpos pos;
    MIDGatt att;

    uint32_t pos_time = 1;
    uint32_t att_time = 1;

    pos_data.clear();
    att_data.clear();

    // open the file
    SGFile input( file );
    if ( !input.open( SG_IO_IN ) ) {
        cout << "Cannot open file: " << file << endl;
        return false;
    }

    while ( ! input.eof() ) {
        // cout << "looking for next message ..." << endl;
        int id = next_message( &input, NULL, &pos, &att );
        count++;

        if ( id == 10 ) {
            if ( att.get_msec() > att_time ) {
                att_data.push_back( att );
                att_time = att.get_msec();
            } else {
                cout << "oops att back in time" << endl;
            }
        } else if ( id == 12 ) {
            if ( pos.get_msec() > pos_time ) {
                pos_data.push_back( pos );
                pos_time = pos.get_msec();
            } else {
                cout << "oops pos back in time" << endl;
            }
        }
    }

    cout << "processed " << count << " messages" << endl;
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
int serial_read( SGSerialPort *serial, char *buf, int length ) {
    int result = 0;
    int bytes_read = 0;
    char *tmp = buf;

    while ( bytes_read < length ) {
      result = serial->read_port( tmp, length - bytes_read );
      bytes_read += result;
      tmp += result;
      // cout << "  read " << bytes_read << " of " << length << endl;
    }

    return bytes_read;
}

// load the next message of a real time data stream
int MIDGTrack::next_message( SGIOChannel *ch, SGIOChannel *log,
                             MIDGpos *pos, MIDGatt *att )
{
    char tmpbuf[256];
    char savebuf[256];

    // cout << "in next_message()" << endl;

    bool myeof = false;

    // scan for sync characters
    uint8_t sync0, sync1;
    myread( ch, log, tmpbuf, 1 ); sync0 = (unsigned char)tmpbuf[0];
    myread( ch, log, tmpbuf, 1 ); sync1 = (unsigned char)tmpbuf[0];
    while ( (sync0 != 129 || sync1 != 161) && !myeof ) {
        sync0 = sync1;
        myread( ch, log, tmpbuf, 1 ); sync1 = (unsigned char)tmpbuf[0];
        // cout << "scanning for start of message "
	//      << (unsigned int)sync0 << " " << (unsigned int)sync1
	//      << ", eof = " << ch->eof() << endl;
        if ( ch->get_type() == sgFileType ) {
            myeof = ((SGFile *)ch)->eof();
        }
    }

    // cout << "found start of message ..." << endl;

    // read message id and size
    myread( ch, log, tmpbuf, 1 ); uint8_t id = (unsigned char)tmpbuf[0];
    myread( ch, log, tmpbuf, 1 ); uint8_t size = (unsigned char)tmpbuf[0];
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
    myread( ch, log, tmpbuf, 1 ); uint8_t cksum0 = (unsigned char)tmpbuf[0];
    myread( ch, log, tmpbuf, 1 ); uint8_t cksum1 = (unsigned char)tmpbuf[0];
    
    if ( validate_cksum( id, size, savebuf, cksum0, cksum1 ) ) {
        parse_msg( id, savebuf, pos, att );
        return id;
    }

    cout << "Check sum failure!" << endl;
    return -1;
}


// load the next message of a real time data stream
int MIDGTrack::next_message( SGSerialPort *serial, SGIOChannel *log,
                             MIDGpos *pos, MIDGatt *att )
{
    char tmpbuf[256];
    char savebuf[256];
    int result = 0;

    cout << "in next_message()" << endl;

    bool myeof = false;

    // scan for sync characters
    uint8_t sync0, sync1;
    result = serial_read( serial, tmpbuf, 2 );
    sync0 = (unsigned char)tmpbuf[0];
    sync1 = (unsigned char)tmpbuf[1];
    while ( (sync0 != 129 || sync1 != 161) && !myeof ) {
        sync0 = sync1;
        serial_read( serial, tmpbuf, 1 ); sync1 = (unsigned char)tmpbuf[0];
        cout << "scanning for start of message "
	     << (unsigned int)sync0 << " " << (unsigned int)sync1
	     << endl;
    }

    cout << "found start of message ..." << endl;

    // read message id and size
    serial_read( serial, tmpbuf, 2 );
    uint8_t id = (unsigned char)tmpbuf[0];
    uint8_t size = (unsigned char)tmpbuf[1];
    // cout << "message = " << (int)id << " size = " << (int)size << endl;

    // load message
    serial_read( serial, savebuf, size );

    // read checksum
    serial_read( serial, tmpbuf, 2 );
    uint8_t cksum0 = (unsigned char)tmpbuf[0];
    uint8_t cksum1 = (unsigned char)tmpbuf[1];
    
    if ( validate_cksum( id, size, savebuf, cksum0, cksum1 ) ) {
        parse_msg( id, savebuf, pos, att );

	//
	// FIXME
	// WRITE DATA TO LOG FILE
	//

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


MIDGpos MIDGInterpPos( const MIDGpos A, const MIDGpos B, const double percent )
{
    MIDGpos p;
    p.midg_time = MIDGTime((uint32_t)interp(A.midg_time.get_msec(),
                                            B.midg_time.get_msec(),
                                            percent));
    p.lat_deg = interp(A.lat_deg, B.lat_deg, percent);
    p.lon_deg = interp(A.lon_deg, B.lon_deg, percent);
    p.altitude_msl = interp(A.altitude_msl, B.altitude_msl, percent);
    p.fix_quality = (int)interp(A.fix_quality, B.fix_quality, percent);
    p.num_satellites = (int)interp(A.num_satellites, B.num_satellites, percent);
    p.hdop = interp(A.hdop, B.hdop, percent);
    p.speed_kts = interp(A.speed_kts, B.speed_kts, percent);
    p.course_true = interp(A.course_true, B.course_true, percent, true);

    return p;
}

MIDGatt MIDGInterpAtt( const MIDGatt A, const MIDGatt B, const double percent )
{
    MIDGatt p;
    p.midg_time = MIDGTime((uint32_t)interp(A.midg_time.get_msec(),
                                            B.midg_time.get_msec(),
                                            percent));
    p.yaw_rad = interp(A.yaw_rad, B.yaw_rad, percent, true);
    p.pitch_rad = interp(A.pitch_rad, B.pitch_rad, percent, true);
    p.roll_rad = interp(A.roll_rad, B.roll_rad, percent, true);

    return p;
}
