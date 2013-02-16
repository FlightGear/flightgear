// ExternalPipe.cxx -- a "pipe" interface to an external flight dynamics model
//
// Written by Curtis Olson, started March 2003.
//
// Copyright (C) 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_MKFIFO
#  include <sys/types.h>        // mkfifo() umask()
#  include <sys/stat.h>         // mkfifo() umask()
#  include <errno.h>            // perror()
#  include <unistd.h>           // unlink()
#endif

#include <cstring>
#include <stdio.h>              // FILE*, fopen(), fread(), fwrite(), et. al.
#include <iostream>             // for cout, endl

#include <simgear/debug/logstream.hxx>
#include <simgear/io/lowlevel.hxx> // endian tests
#include <simgear/misc/strutils.hxx> // split()

#include <Main/fg_props.hxx>
#include <Network/native_ctrls.hxx>
#include <Network/native_fdm.hxx>
#include <Scenery/scenery.hxx>

#include "ExternalPipe.hxx"

using std::cout;
using std::endl;

static const int MAX_BUF = 32768;

FGExternalPipe::FGExternalPipe( double dt, string name, string protocol ) {
    valid = true;
    last_weight = 0.0;
    last_cg_offset = -9999.9;

    buf = new char[MAX_BUF];

    // clear property request list
    property_names.clear();
    nodes.clear();

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

    pd1 = fopen( fifo_name_1.c_str(), "w" );
    if ( pd1 == NULL ) {
        SG_LOG( SG_IO, SG_ALERT, "Unable to open named pipe: " << fifo_name_1 );
        valid = false;
    }
    pd2 = fopen( fifo_name_2.c_str(), "r" );
    if ( pd2 == NULL ) {
        SG_LOG( SG_IO, SG_ALERT, "Unable to open named pipe: " << fifo_name_2 );
        valid = false;
    }
#endif

    _protocol = protocol;

    if ( _protocol != "binary" && _protocol != "property" ) {
        SG_LOG( SG_IO, SG_ALERT, "Constructor(): Unknown ExternalPipe protocol."
                << "  Must be 'binary' or 'property'."
                << "  (assuming binary)" );
        _protocol = "binary";
    }
}


FGExternalPipe::~FGExternalPipe() {
    delete [] buf;

    SG_LOG( SG_IO, SG_INFO, "Closing up the ExternalPipe." );
    
#ifdef HAVE_MKFIFO
    // close
    int result;
    result = fclose( pd1 );
    if ( result ) {
        SG_LOG( SG_IO, SG_ALERT, "Unable to close named pipe: "
                << fifo_name_1 );
        perror( "~FGExternalPipe()" );
    }
    result = fclose( pd2 );
    if ( result ) {
        SG_LOG( SG_IO, SG_ALERT, "Unable to close named pipe: "
                << fifo_name_2 );
        perror( "~FGExternalPipe()" );
    }
#endif
}


static int write_binary( char cmd_type, FILE *pd, char *cmd, int len ) {
#ifdef HAVE_MKFIFO
    char *buf = new char[len + 3];

    // write 2 byte command length + command type + command
    unsigned char hi = (len + 1) / 256;
    unsigned char lo = (len + 1) - (hi * 256);

    // cout << "len = " << len << " hi = " << (int)hi << " lo = "
    //      << (int)lo << endl;

    buf[0] = hi;
    buf[1] = lo;
    buf[2] = cmd_type;

    memcpy( buf + 3, cmd, len );

    if ( cmd_type == '1' ) {
        cout << "writing ";
        cout << (int)hi << " ";
        cout << (int)lo << " '";
        for ( int i = 2; i < len + 3; ++i ) {
            cout << buf[i];
        }
        cout << "' (" << cmd << ")" << endl;
    } else if ( cmd_type == '2' ) {
        // cout << "writing controls packet" << endl;
    } else {
        cout << "writing unknown command?" << endl;
    }

    // for ( int i = 0; i < len + 3; ++i ) {
    //     cout << " " << (int)buf[i];
    // }
    // cout << endl;

    int result = fwrite( buf, len + 3, 1, pd  );
    if ( result != 1 ) {
        perror( "write_binary()" );
        SG_LOG( SG_IO, SG_ALERT, "Write error to named pipe: " << pd );
    }
    // cout << "wrote " << len + 3 << " bytes." << endl;

    delete [] buf;

    return result;
#else
    return 0;
#endif
}


static int write_property( FILE *pd, char *cmd ) {
    int len = strlen(cmd);

#ifdef HAVE_MKFIFO
    char *buf = new char[len + 1];

    memcpy( buf, cmd, len );
    buf[len] = '\n';

    int result = fwrite( buf, len + 1, 1, pd );
    if ( result == len + 1 ) {
        perror( "write_property()" );
        SG_LOG( SG_IO, SG_ALERT, "Write error to named pipe: " << pd );
    }
    // cout << "wrote " << len + 1 << " bytes." << endl;

    delete [] buf;

    return result;
#else
    return 0;
#endif
}


// Wrapper for the ExternalPipe flight model initialization.  dt is
// the time increment for each subsequent iteration through the EOM
void FGExternalPipe::init() {
    // Explicitly call the superclass's
    // init method first.
    common_init();

    if ( _protocol == "binary" ) {
        init_binary();
    } else if ( _protocol == "property" ) {
        init_property();
    } else {
        SG_LOG( SG_IO, SG_ALERT, "Init():  Unknown ExternalPipe protocol."
                << "  Must be 'binary' or 'property'."
                << "  (assuming binary)" );
    }
}


// Initialize the ExternalPipe flight model using the binary protocol,
// dt is the time increment for each subsequent iteration through the
// EOM
void FGExternalPipe::init_binary() {
    cout << "init_binary()" << endl;

    double lon = fgGetDouble( "/sim/presets/longitude-deg" );
    double lat = fgGetDouble( "/sim/presets/latitude-deg" );
    double alt = fgGetDouble( "/sim/presets/altitude-ft" );
    double ground = get_Runway_altitude_m();
    double heading = fgGetDouble("/sim/presets/heading-deg");
    double speed = fgGetDouble( "/sim/presets/airspeed-kt" );
    double weight = fgGetDouble( "/sim/aircraft-weight-lbs" );
    double cg_offset = fgGetDouble( "/sim/aircraft-cg-offset-inches" );

    char cmd[256];
    int result;

    sprintf( cmd, "longitude-deg=%.8f", lon );
    result = write_binary( '1', pd1, cmd, strlen(cmd) );

    sprintf( cmd, "latitude-deg=%.8f", lat );
    result = write_binary( '1', pd1, cmd, strlen(cmd) );

    sprintf( cmd, "altitude-ft=%.8f", alt );
    result = write_binary( '1', pd1, cmd, strlen(cmd) );

    sprintf( cmd, "ground-m=%.8f", ground );
    result = write_binary( '1', pd1, cmd, strlen(cmd) );

    sprintf( cmd, "speed-kts=%.8f", speed );
    result = write_binary( '1', pd1, cmd, strlen(cmd) );

    sprintf( cmd, "heading-deg=%.8f", heading );
    result = write_binary( '1', pd1, cmd, strlen(cmd) );

    if ( weight > 1000.0 ) {
        sprintf( cmd, "aircraft-weight-lbs=%.2f", weight );
        result = write_binary( '1', pd1, cmd, strlen(cmd) );
    }
    last_weight = weight;

    if ( cg_offset > -5.0 || cg_offset < 5.0 ) {
        sprintf( cmd, "aircraft-cg-offset-inches=%.2f", cg_offset );
        result = write_binary( '1', pd1, cmd, strlen(cmd) );
    }
    last_cg_offset = cg_offset;

    SG_LOG( SG_IO, SG_ALERT, "before sending reset command." );

    if( fgGetBool("/sim/presets/onground") ) {
        sprintf( cmd, "reset=ground" );
    } else {
        sprintf( cmd, "reset=air" );
    }
    result = write_binary( '1', pd1, cmd, strlen(cmd) );

    fflush( pd1 );

    SG_LOG( SG_IO, SG_ALERT, "Remote FDM init() finished." );

    (void) result; // ignore result
}


// Initialize the ExternalPipe flight model using the property
// protocol, dt is the time increment for each subsequent iteration
// through the EOM
void FGExternalPipe::init_property() {
    cout << "init_property()" << endl;

    double lon = fgGetDouble( "/sim/presets/longitude-deg" );
    double lat = fgGetDouble( "/sim/presets/latitude-deg" );
    double alt = fgGetDouble( "/sim/presets/altitude-ft" );
    double ground = get_Runway_altitude_m();
    double heading = fgGetDouble("/sim/presets/heading-deg");
    double speed = fgGetDouble( "/sim/presets/airspeed-kt" );
    double weight = fgGetDouble( "/sim/aircraft-weight-lbs" );
    double cg_offset = fgGetDouble( "/sim/aircraft-cg-offset-inches" );

    char cmd[256];
    int result;

    sprintf( cmd, "init longitude-deg=%.8f", lon );
    result = write_property( pd1, cmd );

    sprintf( cmd, "init latitude-deg=%.8f", lat );
    result = write_property( pd1, cmd );

    sprintf( cmd, "init altitude-ft=%.8f", alt );
    result = write_property( pd1, cmd );

    sprintf( cmd, "init ground-m=%.8f", ground );
    result = write_property( pd1, cmd );

    sprintf( cmd, "init speed-kts=%.8f", speed );
    result = write_property( pd1, cmd );

    sprintf( cmd, "init heading-deg=%.8f", heading );
    result = write_property( pd1, cmd );

    if ( weight > 1000.0 ) {
        sprintf( cmd, "init aircraft-weight-lbs=%.2f", weight );
        result = write_property( pd1, cmd );
    }
    last_weight = weight;

    if ( cg_offset > -5.0 || cg_offset < 5.0 ) {
        sprintf( cmd, "init aircraft-cg-offset-inches=%.2f", cg_offset );
        result = write_property( pd1, cmd );
    }
    last_cg_offset = cg_offset;

    SG_LOG( SG_IO, SG_ALERT, "before sending reset command." );

    if( fgGetBool("/sim/presets/onground") ) {
        sprintf( cmd, "reset ground" );
    } else {
        sprintf( cmd, "reset air" );
    }
    result = write_property( pd1, cmd );

    fflush( pd1 );

    SG_LOG( SG_IO, SG_ALERT, "Remote FDM init() finished." );

    (void) result; // ignore result
}


// Wrapper for the ExternalPipe update routines.  dt is the time
// increment for each subsequent iteration through the EOM
void FGExternalPipe::update( double dt ) {
    if ( _protocol == "binary" ) {
        update_binary(dt);
    } else if ( _protocol == "property" ) {
        update_property(dt);
    } else {
        SG_LOG( SG_IO, SG_ALERT, "Init():  Unknown ExternalPipe protocol."
                << "  Must be 'binary' or 'property'."
                << "  (assuming binary)" );
    }
}


// Run an iteration of the EOM.
void FGExternalPipe::update_binary( double dt ) {
#ifdef HAVE_MKFIFO
    SG_LOG( SG_IO, SG_INFO, "Start FGExternalPipe::udpate_binary()" );

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
        result = write_binary( '1', pd1, cmd, strlen(cmd) );
    }
    last_weight = weight;

    double cg_offset = fgGetDouble( "/sim/aircraft-cg-offset-inches" );
    if ( fabs( cg_offset - last_cg_offset ) > 0.01 ) {
        char cmd[256];
        sprintf( cmd, "aircraft-cg-offset-inches=%.2f", cg_offset );
        result = write_binary( '1', pd1, cmd, strlen(cmd) );
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

    result = write_binary( '2', pd1, buf, length + sizeof(int) );
    fflush( pd1 );

    // Read fdm values
    length = sizeof(fdm);
    // cout << "about to read fdm data from remote fdm." << endl;
    result = fread( (char *)(& fdm), length, 1, pd2 );
    if ( result != 1 ) {
        SG_LOG( SG_IO, SG_ALERT, "Read error from named pipe: "
                << fifo_name_2 << " expected 1 item, but got " << result );
    } else {
        // cout << "  read successful." << endl;
        FGNetFDM2Props( &fdm, false );
    }
#endif
}


// Process remote FDM "set" commands

void FGExternalPipe::process_set_command( const string_list &tokens ) {
    if ( tokens[1] == "geodetic_position" ) {
        double lat_rad = atof( tokens[2].c_str() );
        double lon_rad = atof( tokens[3].c_str() );
        double alt_m   = atof( tokens[4].c_str() );
        _updateGeodeticPosition( lat_rad, lon_rad,
                                                alt_m * SG_METER_TO_FEET );

        double agl_m = alt_m - get_Runway_altitude_m();
        _set_Altitude_AGL( agl_m * SG_METER_TO_FEET );
    } else if ( tokens[1] == "euler_angles" ) {
        double phi_rad   = atof( tokens[2].c_str() );
        double theta_rad = atof( tokens[3].c_str() );
        double psi_rad   = atof( tokens[4].c_str() );
        _set_Euler_Angles( phi_rad, theta_rad, psi_rad );
    } else if ( tokens[1] == "euler_rates" ) {
        double phidot   = atof( tokens[2].c_str() );
        double thetadot = atof( tokens[3].c_str() );
        double psidot   = atof( tokens[4].c_str() );
        _set_Euler_Rates( phidot, thetadot, psidot );
    } else if ( tokens[1] == "ned" ) {
	double north_fps = atof( tokens[2].c_str() );
	double east_fps = atof( tokens[3].c_str() );
	double down_fps = atof( tokens[4].c_str() );
	_set_Velocities_Local( north_fps, east_fps, down_fps );
    } else if ( tokens[1] == "alpha" ) {
        _set_Alpha( atof(tokens[2].c_str()) );
    } else if ( tokens[1] == "beta" ) {
        _set_Beta( atof(tokens[2].c_str()) );

#if 0
    _set_V_calibrated_kts( net->vcas );
    _set_Climb_Rate( net->climb_rate );
    _set_Velocities_Local( net->v_north,
                                          net->v_east,
                                          net->v_down );
    _set_Velocities_Wind_Body( net->v_wind_body_north,
                                              net->v_wind_body_east,
                                              net->v_wind_body_down );

    _set_Accels_Pilot_Body( net->A_X_pilot,
                                           net->A_Y_pilot,
                                           net->A_Z_pilot );
#endif
    } else {
        fgSetString( tokens[1].c_str(), tokens[2].c_str() );
    }
}


// Run an iteration of the EOM.
void FGExternalPipe::update_property( double dt ) {
    // cout << "update_property()" << endl;

#ifdef HAVE_MKFIFO
    // SG_LOG( SG_IO, SG_INFO, "Start FGExternalPipe::udpate()" );

    int result;
    char cmd[256];

    if ( is_suspended() ) {
        return;
    }

    int iterations = _calc_multiloop(dt);

    double weight = fgGetDouble( "/sim/aircraft-weight-lbs" );
    static double last_weight = 0.0;
    if ( fabs( weight - last_weight ) > 0.01 ) {
        sprintf( cmd, "init aircraft-weight-lbs=%.2f", weight );
        result = write_property( pd1, cmd );
    }
    last_weight = weight;

    double cg_offset = fgGetDouble( "/sim/aircraft-cg-offset-inches" );
    if ( fabs( cg_offset - last_cg_offset ) > 0.01 ) {
        sprintf( cmd, "init aircraft-cg-offset-inches=%.2f", cg_offset );
        result = write_property( pd1, cmd );
    }
    last_cg_offset = cg_offset;

    // Send requested property values to fdm
    for ( unsigned int i = 0; i < nodes.size(); i++ ) {
        sprintf( cmd, "set %s %s", property_names[i].c_str(),
                 nodes[i]->getStringValue() );
        // cout << "  sending " << cmd << endl;
        result = write_property( pd1, cmd );
    }

    sprintf( cmd, "update %d", iterations );
    write_property( pd1, cmd );

    fflush( pd1 );

    // Read FDM response
    // cout << "ready to read fdm response" << endl;
    bool done = false;
    while ( !done ) {
        if ( fgets( cmd, 256, pd2 ) == NULL ) {
            cout << "Error reading data" << endl;
        } else {
            // cout << "  read " << strlen(cmd) << " bytes" << endl;
            // cout << cmd << endl;
        }

        // chop trailing newline
        cmd[strlen(cmd)-1] = '\0';

        // cout << cmd << endl;
        string_list tokens = simgear::strutils::split( cmd, " " );
    
        if ( tokens[0] == "request" ) {
            // save the long form name
            property_names.push_back( tokens[1] );

            // now do the property name lookup and cache the pointer
            SGPropertyNode *node = fgGetNode( tokens[1].c_str() );
            if ( node == NULL ) {
                // node doesn't exist so create with requested type
                node = fgGetNode( tokens[1].c_str(), true );
                if ( tokens[2] == "bool" ) {
                    node->setBoolValue(false);
                } else if ( tokens[2] == "int" ) {
                    node->setIntValue(0);
                } else if ( tokens[2] == "double" ) {
                    node->setDoubleValue(0.0);
                } else if ( tokens[2] == "string" ) {
                    node->setStringValue("");
                } else {
                    cout << "Unknown data type: " << tokens[2]
                         << " for " << tokens[1] << endl;
                }
            }
            nodes.push_back( node );
        } else if ( tokens[0] == "set" ) {
            process_set_command( tokens );
        } else if ( tokens[0] == "update" ) {
            done = true;
        } else {
            cout << "unknown command = " << cmd << endl;
        }
    }

    (void) result; // ignore result
#endif
}


