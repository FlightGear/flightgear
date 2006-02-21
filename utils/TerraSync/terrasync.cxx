// terrasync.cxx -- "JIT" scenery fetcher
//
// Written by Curtis Olson, started November 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#include <stdlib.h>             // atoi() atof() abs() system()

#include <simgear/compiler.h>

#include STL_IOSTREAM
#include STL_STRING

#include <plib/netSocket.h>
#include <plib/ul.h>

#include <simgear/bucket/newbucket.hxx>

SG_USING_STD(string);
SG_USING_STD(cout);
SG_USING_STD(endl);

static string server = "scenery.flightgear.org";
static string source_module = "Scenery";
static string source_base = server + (string)"::" + source_module;
static string dest_base = "/dest/scenery/dir";


// display usage
static void usage( const string& prog ) {
    cout << "Usage: " << prog
         << " -p <port> [ -s <rsync_source> ] -d <rsync_dest>" << endl;
}


// parse message
static void parse_message( const string &msg, int *lat, int *lon ) {
    double dlat, dlon;
    string text = msg;

    // find GGA string and advance to start of lat
    string::size_type pos = text.find( "$GPGGA" );
    string tmp = text.substr( pos + 7 );
    pos = text.find( "," );
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
}


// sync area
static void sync_area( int lat, int lon ) {
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

    char command[512];
    char container_dir[512];
    char dir[512];

    // Sync Terrain
    snprintf( container_dir, 512, "%s/Terrain/%c%03d%c%02d",
              dest_base.c_str(), EW, abs(baselon), NS, abs(baselat) );
    snprintf( command, 512, "mkdir -p %s", container_dir );
    cout << command << endl;
    system( command );

    snprintf( dir, 512, "Terrain/%c%03d%c%02d/%c%03d%c%02d",
              EW, abs(baselon), NS, abs(baselat),
              EW, abs(lon), NS, abs(lat) );

    snprintf( command, 512,
              "rsync --verbose --archive --delete --perms --owner --group %s/%s/ %s/%s",
              source_base.c_str(), dir, dest_base.c_str(), dir );
    cout << command << endl;
    system( command );

    // Sync Objects
    snprintf( container_dir, 512, "%s/Objects/%c%03d%c%02d",
              dest_base.c_str(), EW, abs(baselon), NS, abs(baselat) );
    snprintf( command, 512, "mkdir -p %s", container_dir );
    cout << command << endl;
    system( command );

    snprintf( dir, 512, "Objects/%c%03d%c%02d/%c%03d%c%02d",
              EW, abs(baselon), NS, abs(baselat),
              EW, abs(lon), NS, abs(lat) );

    snprintf( command, 512,
              "rsync --verbose --archive --delete --perms --owner --group %s/%s/ %s/%s",
              source_base.c_str(), dir, dest_base.c_str(), dir );
    cout << command << endl;
    system( command );
}


// sync areas
static void sync_areas( int lat, int lon, int lat_dir, int lon_dir ) {
    // do current 1x1 degree area first
    sync_area( lat, lon );

    if ( lat_dir == 0 && lon_dir == 0 ) {
        // now do surrounding 8 1x1 degree areas.
        for ( int i = lat - 1; i <= lat + 1; ++i ) {
            for ( int j = lon - 1; j <= lon + 1; ++j ) {
                if ( i != lat || j != lon ) {
                    sync_area( i, j );
                }
            }
        }
    } else {
        if ( lat_dir != 0 ) {
            sync_area( lat + lat_dir, lon );
            sync_area( lat + lat_dir, lon - 1 );
            sync_area( lat + lat_dir, lon + 1 );
        }
        if ( lon_dir != 0 ) {
            sync_area( lat, lon + lon_dir );
            sync_area( lat - 1, lon + lon_dir );
            sync_area( lat + 1, lon + lon_dir );
        }
    }
}


int main( int argc, char **argv ) {
    int port = 5501;
    char host[256] = "";        // accept messages from anyone

    // parse arguments
    int i = 1;
    while ( i < argc ) {
        if ( (string)argv[i] == "-p" ) {
            ++i;
            port = atoi( argv[i] );
        } else if ( (string)argv[i] == "-s" ) {
            ++i;
            source_base = argv[i];
        } else if ( (string)argv[i] == "-d" ) {
            ++i;
            dest_base = argv[i];
        } else {
            usage( argv[0] );
            exit(-1);        
        }
        ++i;
    }

    // Must call this before any other net stuff
    netInit( &argc,argv );

    netSocket s;

    if ( ! s.open( false ) ) {  // open a UDP socket
        printf("error opening socket\n");
        return -1;
    }

    s.setBlocking( false );

    if ( s.bind( host, port ) == -1 ) {
        printf("error binding to port %d\n", port);
        return -1;
    }

    char msg[256];
    int maxlen = 256;
    int len;
    int lat, lon;
    int last_lat = -9999;
    int last_lon = -9999;
    bool recv_msg = false;

    while ( true ) {
        recv_msg = false;
        while ( (len = s.recv(msg, maxlen, 0)) >= 0 ) {
            msg[len] = '\0';
            recv_msg = true;

            parse_message( msg, &lat, &lon );
            cout << "pos = " << lat << "," << lon << endl;
        }

        if ( recv_msg ) {
            if ( lat != last_lat || lon != last_lon ) {
                int lat_dir, lon_dir, dist;
                if ( last_lat == -9999 || last_lon == -9999 ) {
                    lat_dir = lon_dir = 0;
                } else {
                    dist = lat - last_lat;
                    if ( dist != 0 ) {
                        lat_dir = dist / abs(dist);
                    } else {
                        lat_dir = 0;
                    }
                    dist = lon - last_lon;
                    if ( dist != 0 ) {
                        lon_dir = dist / abs(dist);
                    } else {
                        lon_dir = 0;
                    }
                }
                cout << "lat = " << lat << " lon = " << lon << endl;
                cout << "lat_dir = " << lat_dir << " " << " lon_dir = " << lon_dir << endl;
                sync_areas( lat, lon, lat_dir, lon_dir );
            }

            last_lat = lat;
            last_lon = lon;
        }

        ulSleep( 1 );
    }
        
    return 0;
}
