// ExternalPipe.cxx -- a "pipe" interface to an external flight dynamics model
//
// Written by Curtis Olson, started March 2003.
//
// Copyright (C) 2003  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_MKFIFO
#  include <sys/types.h>        // mkfifo() open() umask()
#  include <sys/stat.h>         // mkfifo() open() umask()
#  include <errno.h>            // perror()
#  include <fcntl.h>            // open()
#  include <unistd.h>           // unlink()
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/io/lowlevel.hxx> // endian tests

#include <Main/fg_props.hxx>
#include <Network/native_ctrls.hxx>
#include <Network/native_fdm.hxx>

#include "ExternalPipe.hxx"


static const int MAX_BUF = 32768;

FGExternalPipe::FGExternalPipe( double dt, string name ) {
    valid = true;
    last_weight = 0.0;
    last_cg_offset = -9999.9;

    buf = new char[MAX_BUF];

#ifdef HAVE_MKFIFO
    fifo_name_1 = name + "1";
    fifo_name_2 = name + "2";

    SG_LOG( SG_IO, SG_ALERT, "ExternalPipe Inited with " << name );

    // Make the named pipe
    umask(0);
    int result;
    result = mkfifo( fifo_name_1.c_str(), 0644 );
    if ( result == -1 ) {
        SG_LOG( SG_IO, SG_ALERT, "Unable to create named pipe: "
                << fifo_name_1 );
        perror( "ExternalPipe()" );
    }
    result = mkfifo( fifo_name_2.c_str(), 0644 );
    if ( result == -1 ) {
        SG_LOG( SG_IO, SG_ALERT, "Unable to create named pipe: "
                << fifo_name_2 );
        perror( "ExternalPipe()" );
    }

    pd1 = open( fifo_name_1.c_str(), O_RDWR );
    if ( pd1 == -1 ) {
        SG_LOG( SG_IO, SG_ALERT, "Unable to open named pipe: " << fifo_name_1 );
        valid = false;
    }
    pd2 = open( fifo_name_2.c_str(), O_RDWR );
    if ( pd2 == -1 ) {
        SG_LOG( SG_IO, SG_ALERT, "Unable to open named pipe: " << fifo_name_2 );
        valid = false;
    }
#endif
}


FGExternalPipe::~FGExternalPipe() {
    delete [] buf;

    SG_LOG( SG_IO, SG_INFO, "Closing up the ExternalPipe." );
    
#ifdef HAVE_MKFIFO
    // close
    int result;
    result = close( pd1 );
    if ( result == -1 ) {
        SG_LOG( SG_IO, SG_ALERT, "Unable to close named pipe: "
                << fifo_name_1 );
        perror( "~FGExternalPipe()" );
    }
    result = close( pd2 );
    if ( result == -1 ) {
        SG_LOG( SG_IO, SG_ALERT, "Unable to close named pipe: "
                << fifo_name_2 );
        perror( "~FGExternalPipe()" );
    }
#endif
}


static int write_fifo( char cmd_type, int pd, char *cmd, int len ) {
#ifdef HAVE_MKFIFO
    char *buf = new char[len + 3];

    // write 2 byte command length + command type + command
    char hi = (len + 1) / 256;
    char lo = (len + 1) - (hi * 256);

    buf[0] = hi;
    buf[1] = lo;
    buf[2] = cmd_type;

    // strncpy( buf + 3, cmd, len );
    memcpy( buf + 3, cmd, len );

    if ( cmd_type == '1' ) {
        // cout << "writing '" << cmd << "'" << endl;
    } else if ( cmd_type == '2' ) {
        // cout << "writing controls packet" << endl;
    } else {
        // cout << "writing unknown command?" << endl;
    }

    // for ( int i = 0; i < len + 3; ++i ) {
    //     cout << " " << (int)buf[i];
    // }
    // cout << endl;

    int result = ::write( pd, buf, len + 3 );
    if ( result == -1 ) {
        perror( "write_fifo()" );
        SG_LOG( SG_IO, SG_ALERT, "Write error to named pipe: " << pd );
    }
    // cout << "wrote " << len + 3 << " bytes." << endl;

    delete [] buf;

    return result;
#else
    return 0;
#endif
}


// Initialize the ExternalPipe flight model, dt is the time increment
// for each subsequent iteration through the EOM
void FGExternalPipe::init() {
    // Explicitly call the superclass's
    // init method first.
    common_init();

    double lon = fgGetDouble( "/sim/presets/longitude-deg" );
    double lat = fgGetDouble( "/sim/presets/latitude-deg" );
    double alt = fgGetDouble( "/sim/presets/altitude-ft" );
    double ground = fgGetDouble( "/environment/ground-elevation-m" );
    double heading = fgGetDouble("/sim/presets/heading-deg");
    double speed = fgGetDouble( "/sim/presets/airspeed-kt" );
    double weight = fgGetDouble( "/sim/aircraft-weight-lbs" );
    double cg_offset = fgGetDouble( "/sim/aircraft-cg-offset-inches" );

#ifdef HAVE_MKFIFO

    char cmd[256];
    int result;

    sprintf( cmd, "longitude-deg=%.8f", lon );
    result = write_fifo( '1', pd1, cmd, strlen(cmd) );

    sprintf( cmd, "latitude-deg=%.8f", lat );
    result = write_fifo( '1', pd1, cmd, strlen(cmd) );

    sprintf( cmd, "altitude-ft=%.8f", alt );
    result = write_fifo( '1', pd1, cmd, strlen(cmd) );

    sprintf( cmd, "ground-m=%.8f", ground );
    result = write_fifo( '1', pd1, cmd, strlen(cmd) );

    sprintf( cmd, "speed-kts=%.8f", speed );
    result = write_fifo( '1', pd1, cmd, strlen(cmd) );

    sprintf( cmd, "heading-deg=%.8f", heading );
    result = write_fifo( '1', pd1, cmd, strlen(cmd) );

    if ( weight > 1000.0 ) {
        sprintf( cmd, "aircraft-weight-lbs=%.2f", weight );
        result = write_fifo( '1', pd1, cmd, strlen(cmd) );
    }
    last_weight = weight;

    if ( cg_offset > -5.0 || cg_offset < 5.0 ) {
        sprintf( cmd, "aircraft-cg-offset-inches=%.2f", cg_offset );
        result = write_fifo( '1', pd1, cmd, strlen(cmd) );
    }
    last_cg_offset = cg_offset;

    SG_LOG( SG_IO, SG_INFO, "before sending reset command." );

    if( fgGetBool("/sim/presets/onground") ) {
        sprintf( cmd, "reset=ground" );
    } else {
        sprintf( cmd, "reset=air" );
    }
    result = write_fifo( '1', pd1, cmd, strlen(cmd) );

    SG_LOG( SG_IO, SG_INFO, "Remote FDM init() finished." );
#endif
}


// Run an iteration of the EOM.
void FGExternalPipe::update( double dt ) {
#ifdef HAVE_MKFIFO
    // SG_LOG( SG_IO, SG_INFO, "Start FGExternalPipe::udpate()" );

    int length;
    int result;
    
    if ( is_suspended() ) {
        return;
    }

    int iterations = _calc_multiloop(dt);

    double weight = fgGetDouble( "/sim/aircraft-weight-lbs" );
    static double last_weight = 0.0;
    if ( fabs( weight - last_weight ) > 0.01 ) {
        char cmd[256];
        sprintf( cmd, "aircraft-weight-lbs=%.2f", weight );
        result = write_fifo( '1', pd1, cmd, strlen(cmd) );
    }
    last_weight = weight;

    double cg_offset = fgGetDouble( "/sim/aircraft-cg-offset-inches" );
    if ( fabs( cg_offset - last_cg_offset ) > 0.01 ) {
        char cmd[256];
        sprintf( cmd, "aircraft-cg-offset-inches=%.2f", cg_offset );
        result = write_fifo( '1', pd1, cmd, strlen(cmd) );
    }
    last_cg_offset = cg_offset;

    // Send control positions to remote fdm
    length = sizeof(ctrls);
    FGProps2NetCtrls( &ctrls, true, false );
    char *ptr = buf;
    *((int *)ptr) = iterations;
    // cout << "iterations = " << iterations << endl;
    ptr += sizeof(int);
    memcpy( ptr, (char *)(&ctrls), length );
    // cout << "writing control structure, size = "
    //      << length + sizeof(int) << endl;

    result = write_fifo( '2', pd1, buf, length + sizeof(int) );

    // Read fdm values
    length = sizeof(fdm);
    // cout << "about to read fdm data from remote fdm." << endl;
    result = read( pd2, (char *)(& fdm), length );
    if ( result == -1 ) {
        SG_LOG( SG_IO, SG_ALERT, "Read error from named pipe: "
                << fifo_name_2 );
    } else {
        // cout << "  read successful." << endl;
    }
    FGNetFDM2Props( &fdm, false );
#endif
}
