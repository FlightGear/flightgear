// terrasync.cxx -- "JIT" scenery fetcher
//
// Written by Curtis Olson, started November 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
// Copyright (C) 2008  Alexander R. Perry <alex.perry@ieee.org>
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

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif


#include <stdlib.h>             // atoi() atof() abs() system()

#include <simgear/compiler.h>

#include <iostream>
#include <string>

#include <plib/netSocket.h>
#include <plib/ul.h>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>

using std::string;
using std::cout;
using std::endl;

const char* svn_base =
  "http://terrascenery.googlecode.com/svn/trunk/data/Scenery";
const char* rsync_base = "scenery.flightgear.org::Scenery";
const char* source_base = NULL;
const char* dest_base = "terrasyncdir";
bool use_svn = false;

const char* svn_cmd = "svn checkout";
const char* rsync_cmd = 
    "rsync --verbose --archive --delete --perms --owner --group";


// display usage
static void usage( const string& prog ) {
    cout << "Usage: " << endl
         << prog << " -p <port> "
	 << "[ -R ] [ -s <rsync_source> ] -d <dest>" << endl
         << prog << " -p <port> "
         << "  -S   [ -s <svn_source> ] -d <dest>" << endl;
}


const int nowhere = -9999;

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

    if ((dlon == 0) && (dlat == 0)) {
      *lon = nowhere;
      *lat = nowhere;
    }

}

// sync one directory tree
void sync_tree(char* dir) {
    int rc;
    char command[512];
    SGPath path( dest_base );

    path.append( dir );
    rc = path.create_dir( 0755 );
    if (rc) {
        cout << "Return code = " << rc << endl;
        exit(1);
    }

    if (use_svn) {
        snprintf( command, 512,
            "%s %s/%s %s/%s", svn_cmd,
            source_base, dir,
	    dest_base, dir );
    } else {
      snprintf( command, 512,
          "%s %s/%s/ %s/%s/", rsync_cmd,
          source_base, dir,
	  dest_base, dir );
    }
    cout << command << endl;
    rc = system( command );
    if (rc) {
        cout << "Return code = " << rc << endl;
        if (rc == 5120) exit(1);
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
    
    const char* terrainobjects[3] = { "Terrain", "Objects", 0 };
    const char** tree;
    char dir[512];
  
    for (tree = &terrainobjects[0]; *tree; tree++) {
        snprintf( dir, 512, "%s/%c%03d%c%02d/%c%03d%c%02d",
	      *tree,
    	      EW, abs(baselon), NS, abs(baselat),
    	      EW, abs(lon), NS, abs(lat) );
	sync_tree(dir);
    }
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
        } else if ( (string)argv[i] == "-R" ) {
	    use_svn = false;
        } else if ( (string)argv[i] == "-S" ) {
	    use_svn = true;
        } else if ( (string)argv[i] == "-T" ) {
          sync_areas( 37, -123, 0, 0 );
	  exit(0);
        } else {
            usage( argv[0] );
            exit(-1);        
        }
        ++i;
    }

    // Use the appropriate default for the "-s" flag
    if (source_base == NULL) {
        if (use_svn)
	    source_base = svn_base;
	else
	    source_base = rsync_base;
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
    int last_lat = nowhere;
    int last_lon = nowhere;
    bool recv_msg = false;
    int synced_other = 0;

    while ( true ) {
        recv_msg = false;
        while ( (len = s.recv(msg, maxlen, 0)) >= 0 ) {
            msg[len] = '\0';
            recv_msg = true;

            parse_message( msg, &lat, &lon );
            cout << "pos in msg = " << lat << "," << lon << endl;
        }

        if ( recv_msg ) {
            if ( lat != last_lat || lon != last_lon ) {
                int lat_dir, lon_dir, dist;
                if ( last_lat == nowhere || last_lon == nowhere ) {
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
                cout << "lat_dir = " << lat_dir << "  "
		     << "lon_dir = " << lon_dir << endl;
                sync_areas( lat, lon, lat_dir, lon_dir );
            } else if ( last_lat == nowhere || last_lon == nowhere ) {
	        cout << "Waiting for FGFS to finish startup" << endl;
	    } else {
	        switch (synced_other++) {
		case 0:
		    sync_tree((char*) "Airports/K");
		    break;
		case 1:
		    sync_tree((char*) "Airports");
		    break;
		default:
		    cout << "Done non-tile syncs" << endl;
		    break;
		}
	    }

            last_lat = lat;
            last_lon = lon;
        } 

        ulSleep( 1 );
    } // while true
        
    return 0;
}
