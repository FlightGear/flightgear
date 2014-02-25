// terrasync.cxx -- "JIT" scenery fetcher
//
// Written by Curtis Olson, started November 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
// Copyright (C) 2008  Alexander R. Perry <alex.perry@ieee.org>
// Copyright (C) 2011  Thorsten Brehm <brehmt@gmail.com>
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
#include <config.h>
#endif

#include <simgear/simgear_config.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef HAVE_UNISTD_H
#include "unistd.h"
#endif

#ifdef __MINGW32__
#   include <time.h>
#elif defined(_MSC_VER)
#   include <io.h>
#endif

#include <stdlib.h>             // atoi() atof() abs() system()
#include <signal.h>             // signal()

#include <simgear/compiler.h>

#include <iostream>
#include <fstream>
#include <string>

#include <simgear/io/raw_socket.hxx>
#include <simgear/scene/tsync/terrasync.hxx>

#if defined(_MSC_VER) || defined(__MINGW32__)
    typedef void (__cdecl * sighandler_t)(int);
#elif defined( __APPLE__ ) || defined (__FreeBSD__)
    typedef sig_t sighandler_t;
#endif

int termination_triggering_signals[] = {
#if defined(_MSC_VER) || defined(__MINGW32__)
    SIGINT, SIGILL, SIGFPE, SIGSEGV, SIGTERM, SIGBREAK, SIGABRT,
#else
    SIGHUP, SIGINT, SIGQUIT, SIGKILL, SIGTERM,
#endif
    0};  // zero terminated

using namespace std;

const char* svn_base =
    "http://terrascenery.googlecode.com/svn/trunk/data/Scenery";
const char* rsync_base = "scenery.flightgear.org::Scenery";

bool terminating = false;
sighandler_t prior_signal_handlers[32];

simgear::Socket theSocket;
simgear::SGTerraSync* pTerraSync = NULL;

/** display usage information */
static void
usage( const string& prog ) {
    cout << "Usage:  terrasync [options]\n"
            "Options:\n"
            " -d <dest>       destination directory [required]\n"
            " -R              transport using pipe to rsync\n"
            " -S              transport using built-in svn\n"
            " -e              transport using external svn client\n"
            " -p <port>       listen on UDP port [default: 5501]\n"
            " -s <source>     source base [default: '']\n"
#ifndef _MSC_VER
            " -pid <pidfile>  write PID to file\n"
#endif
            " -v              be more verbose\n";

#ifndef _MSC_VER
    cout << "\n"
            "Example:\n"
            "  pid=$(cat $pidfile 2>/dev/null)\n"
            "  if test -n \"$pid\" && kill -0 $pid ; then\n"
            "      echo \"terrasync already running: $pid\"\n"
            "  else\n"
            "      nice /games/sport/fgs/utils/TerraSync/terrasync         \\\n"
            "        -v -pid $pidfile -S -p 5500 -d /games/orig/terrasync &\n"
            "  fi\n";
#endif
}

/** Signal handler for termination requests (Ctrl-C) */
void
terminate_request_handler(int param)
{
    char msg[] = "\nReceived signal XX, intend to exit soon.\n"
         "Repeat the signal to force immediate termination.\n";
    msg[17] = '0' + param / 10;
    msg[18] = '0' + param % 10;
    if (write(1, msg, sizeof(msg) - 1) == -1)
    {
        // need to act write's return value to avoid GCC compiler warning
        // "'write' declared with attribute warn_unused_result"
        terminating = true; // dummy operation
    }
    terminating = true;
    signal(param, prior_signal_handlers[param]);
    theSocket.close();
    if (pTerraSync)
        pTerraSync->unbind();
}

/** Parse command-line options. */
void
parseOptions(int argc, char** argv, SGPropertyNode_ptr config,
             bool& testing, int& verbose, int& port, const char* &pidfn)
{
    // parse arguments
    for (int i=1; i < argc; ++i )
    {
        string arg = argv[i];
        if ( arg == "-p" ) {
            ++i;
            port = atoi( argv[i] );
        } else if ( arg.find("-pid") == 0 ) {
            ++i;
            pidfn = argv[i];
            cout << "pidfn: " << pidfn << endl;
        } else if ( arg == "-s" ) {
            ++i;
            config->setStringValue("svn-server", argv[i]);
        } else if ( arg == "-d" ) {
            ++i;
            config->setStringValue("scenery-dir", argv[i]);
        } else if ( arg == "-R" ) {
            config->setBoolValue("use-svn", false);
        } else if ( arg == "-S" ) {
            config->setBoolValue("use-built-in-svn", true);
        } else if ( arg == "-e" ) {
            config->setBoolValue("use-built-in-svn", false);
        } else if ( arg == "-v" ) {
            verbose++;
        } else if ( arg == "-T" ) {
            testing = true;
        } else if ( arg == "-h" ) {
            usage( argv[0] );
            exit(0);
        } else {
            cerr << "Unrecognized command-line option '" << arg << "'" << endl;
            usage( argv[0] );
            exit(-1);
        }
    }
}

/** parse a single NMEA-0183 position message */
static void
parse_message( int verbose, const string &msg, int *lat, int *lon )
{
    double dlat, dlon;
    string text = msg;

    if (verbose>=3)
        cout << "message: '" << text << "'" << endl;

    // find GGA string and advance to start of lat (see NMEA-0183 for protocol specs)
    string::size_type pos = text.find( "$GPGGA" );
    if ( pos == string::npos )
    {
        *lat = simgear::NOWHERE;
        *lon = simgear::NOWHERE;
        return;
    }
    string tmp = text.substr( pos + 7 );
    pos = tmp.find( "," );
    tmp = tmp.substr( pos + 1 );
    // cout << "-> " << tmp << endl;

    // find lat then advance to start of hemisphere
    pos = tmp.find( "," );
    string lats = tmp.substr( 0, pos );
    dlat = atof( lats.c_str() ) / 100.0;
    tmp = tmp.substr( pos + 1 );

    // find N/S hemisphere and advance to start of lon
    if ( tmp.substr( 0, 1 ) == "S" ) {
        dlat = -dlat;
    }
    pos = tmp.find( "," );
    tmp = tmp.substr( pos + 1 );

    // find lon
    pos = tmp.find( "," );
    string lons = tmp.substr( 0, pos );
    dlon = atof( lons.c_str() ) / 100.0;
    tmp = tmp.substr( pos + 1 );

    // find E/W hemisphere and advance to start of lon
    if ( tmp.substr( 0, 1 ) == "W" ) {
        dlon = -dlon;
    }

    if ( dlat < 0 ) {
        *lat = (int)dlat - 1;
    } else {
        *lat = (int)dlat;
    }

    if ( dlon < 0 ) {
        *lon = (int)dlon - 1;
    } else {
        *lon = (int)dlon;
    }

    if ((dlon == 0) && (dlat == 0)) {
        *lon = simgear::NOWHERE;
        *lat = simgear::NOWHERE;
    }
}

void
syncArea( int lat, int lon )
{
    if ( lat < -90 || lat > 90 || lon < -180 || lon > 180 )
        return;
    char NS, EW;
    int baselat, baselon;

    if ( lat < 0 ) {
        int base = (int)(lat / 10);
        if ( lat == base * 10 ) {
            baselat = base * 10;
        } else {
            baselat = (base - 1) * 10;
        }
        NS = 's';
    } else {
        baselat = (int)(lat / 10) * 10;
        NS = 'n';
    }
    if ( lon < 0 ) {
        int base = (int)(lon / 10);
        if ( lon == base * 10 ) {
            baselon = base * 10;
        } else {
            baselon = (base - 1) * 10;
        }
        EW = 'w';
    } else {
        baselon = (int)(lon / 10) * 10;
        EW = 'e';
    }

    ostringstream dir;
    dir << setfill('0')
    << EW << setw(3) << abs(baselon) << NS << setw(2) << abs(baselat) << "/"
    << EW << setw(3) << abs(lon)     << NS << setw(2) << abs(lat);
    
    pTerraSync->syncAreaByPath(dir.str());
}

void 
syncAreas( int lat, int lon, int lat_dir, int lon_dir )
{
    if ( lat_dir == 0 && lon_dir == 0 ) {
        
        // do surrounding 8 1x1 degree areas.
        for ( int i = lat - 1; i <= lat + 1; ++i ) {
            for ( int j = lon - 1; j <= lon + 1; ++j ) {
                if ( i != lat || j != lon ) {
                    syncArea( i, j );
                }
            }
        }
    } else {
        if ( lat_dir != 0 ) {
            syncArea( lat + lat_dir, lon - 1 );
            syncArea( lat + lat_dir, lon + 1 );
            syncArea( lat + lat_dir, lon );
        }
        if ( lon_dir != 0 ) {
            syncArea( lat - 1, lon + lon_dir );
            syncArea( lat + 1, lon + lon_dir );
            syncArea( lat, lon + lon_dir );
        }
    }

    // do current 1x1 degree area first
    syncArea( lat, lon );
}

bool 
schedulePosition(int lat, int lon)
{
    bool Ok = false;
    if ((lat == simgear::NOWHERE) || (lon == simgear::NOWHERE)) {
        return Ok;
    }
    
    static int last_lat = simgear::NOWHERE;
    static int last_lon = simgear::NOWHERE;
    if ((lat == last_lat) && (lon == last_lon)) {
        Ok = true;
        return Ok;
    }

    int lat_dir=0;
    int lon_dir=0;
    
    if ( last_lat != simgear::NOWHERE && last_lon != simgear::NOWHERE )
    {
        int dist = lat - last_lat;
        if ( dist != 0 )
        {
            lat_dir = dist / abs(dist);
        }
        else
        {
            lat_dir = 0;
        }
        dist = lon - last_lon;
        if ( dist != 0 )
        {
            lon_dir = dist / abs(dist);
        } else
        {
            lon_dir = 0;
        }
    }

    cout << "Scenery update for " <<
           "lat = " << lat << ", lon = " << lon <<
           ", lat_dir = " << lat_dir << ",  " <<
           "lon_dir = " << lon_dir << endl;

    syncAreas( lat, lon, lat_dir, lon_dir );
    Ok = true;

    last_lat = lat;
    last_lon = lon;

    return Ok;
}
    
/** Monitor UDP socket and process NMEA position updates. */
int
processRequests(SGPropertyNode_ptr config, bool testing, int verbose, int port)
{
    const char* host = "localhost";
    char msg[256];
    int len;
    int lat, lon;
    bool connected = false;

    // Must call this before any other net stuff
    simgear::Socket::initSockets();

    // open UDP socket
    if ( !theSocket.open( false ) )
    {
        cerr << "error opening socket" << endl;
        return -1;
    }

    if ( theSocket.bind( host, port ) == -1 )
    {
        cerr << "error binding to port " << port << endl;
        return -1;
    }

    theSocket.setBlocking(true);

    while ( (!terminating)&&
            (!config->getBoolValue("stalled", false)) )
    {
        if (verbose >= 4)
            cout << "main loop" << endl;
        // main loop
        bool recv_msg = false;
        if ( testing )
        {
            // Testing without FGFS communication
            lat = 37;
            lon = -123;
            recv_msg = true;
        } else
        {
            if (verbose && pTerraSync->isIdle())
            {
                cout << "Idle; waiting for FlightGear position data" << endl;
            }
            len = theSocket.recv(msg, sizeof(msg)-1, 0);
            if (len >= 0)
            {
                msg[len] = '\0';
                recv_msg = true;
                if (verbose>=2)
                    cout << "recv length: " << len << endl;
                parse_message( verbose, msg, &lat, &lon );
                if ((!connected)&&
                    (lat != simgear::NOWHERE)&&
                    (lon != simgear::NOWHERE))
                {
                    cout << "Valid position data received. Connected successfully." << endl;
                    connected = true;
                }
            }
        }

        if ( recv_msg )
        {
            schedulePosition(lat, lon);
        }

        if ( testing )
        {
            if (pTerraSync->isIdle())
                terminating = true;
            else
                SGTimeStamp::sleepForMSec(1000);
        }
    } // while !terminating

    return 0;
}


int main( int argc, char **argv )
{
    int port = 5501;
    int verbose = 0;
    int exit_code = 0;
    bool testing = false;
    const char* pidfn = "";

    // default configuration
    sglog().setLogLevels( SG_ALL, SG_ALERT);
    SGPropertyNode_ptr root = new SGPropertyNode();
    SGPropertyNode_ptr config = root->getNode("/sim/terrasync", true);
    config->setStringValue("scenery-dir", "terrasyncdir");
    config->setStringValue("svn-server", svn_base);
    config->setStringValue("rsync-server", rsync_base);
    config->setBoolValue("use-built-in-svn", true);
    config->setBoolValue("use-svn", true);
    config->setBoolValue("enabled", true);
    config->setIntValue("max-errors", -1); // -1 = infinite

    // parse command-line arguments
    parseOptions(argc, argv, config, testing, verbose, port, pidfn);

    if (verbose)
        sglog().setLogLevels( SG_ALL, SG_INFO);

#ifndef _MSC_VER
    // create PID file
    if (*pidfn)
    {
        ofstream pidstream;
        pidstream.open(pidfn);
        if (!pidstream.good())
        {
            cerr << "Cannot open pid file '" << pidfn << "': ";
            perror(0);
            exit(2);
        }
        pidstream << getpid() << endl;
        pidstream.close();
    }
#endif

    // install signal handlers
    for (int* sigp=termination_triggering_signals; *sigp; sigp++)
    {
        prior_signal_handlers[*sigp] =
            signal(*sigp, *terminate_request_handler);
        if (verbose>=2)
            cout << "Intercepting signal " << *sigp << endl;
    }

    {
        pTerraSync = new simgear::SGTerraSync;
        pTerraSync->setRoot(root);
        pTerraSync->bind();
        pTerraSync->init();

        // now monitor and process position updates
        exit_code = processRequests(config, testing, verbose, port);

        pTerraSync->unbind();
        delete pTerraSync;
        pTerraSync = NULL;
    }

    return exit_code;
}
