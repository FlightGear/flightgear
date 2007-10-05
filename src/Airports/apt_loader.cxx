// apt_loader.cxx -- a front end loader of the apt.dat file.  This loader
//                   populates the runway and basic classes.
//
// Written by Curtis Olson, started August 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <simgear/compiler.h>

#include <stdlib.h> // atof(), atoi()
#include <string.h> // memchr()
#include <ctype.h> // isspace()

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/strutils.hxx>

#include STL_STRING

#include "simple.hxx"
#include "runways.hxx"

#include "apt_loader.hxx"

static void addAirport(FGAirportList *airports, const string& apt_id, const string& apt_name,
    int rwy_count, double rwy_lat_accum, double rwy_lon_accum, double last_rwy_heading,
    double apt_elev, SGGeod& tower, bool got_tower, int type)
{
    if (apt_id.empty())
        return;

    if (!rwy_count) {
        SG_LOG(SG_GENERAL, SG_ALERT, "ERROR: No runways for " << apt_id
                << ", skipping." );
        return;
    }

    double lat = rwy_lat_accum / (double)rwy_count;
    double lon = rwy_lon_accum / (double)rwy_count;

    if (!got_tower) {
        // tower height hard coded for now...
        const float tower_height = 50.0f;
        // make a little off the heading for 1 runway airports...
        float fudge_lon = fabs(sin(last_rwy_heading * SGD_DEGREES_TO_RADIANS)) * .003f;
        float fudge_lat = .003f - fudge_lon;
        tower = SGGeod::fromDegFt(lon + fudge_lon, lat + fudge_lat, apt_elev + tower_height);
    }

    airports->add(apt_id, SGGeod::fromDegFt(lon, lat, apt_elev), tower, apt_name, false,
            type == 1/*airport*/, type == 16/*seaport*/, type == 17/*heliport*/);
}

// Load the airport data base from the specified aptdb file.  The
// metar file is used to mark the airports as having metar available
// or not.
bool fgAirportDBLoad( FGAirportList *airports, FGRunwayList *runways,
                      const string &aptdb_file, const string &metar_file )
{
    //
    // Load the apt.dat file
    //

    sg_gzifstream in( aptdb_file );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << aptdb_file );
        exit(-1);
    }

    vector<string> token;
    string last_apt_id = "";
    double last_apt_elev = 0.0;
    string last_apt_name = "";
    string last_apt_info = "";
    string last_apt_type = "";
    SGGeod last_tower;
    bool got_tower = false;
    string line;
    char tmp[2049];
    tmp[2048] = 0;

    unsigned int line_id = 0;
    unsigned int line_num = 0;
    double rwy_lon_accum = 0.0;
    double rwy_lat_accum = 0.0;
    int rwy_count = 0;
    double last_rwy_heading = 0.0;

    while ( ! in.eof() ) {
	in.getline(tmp, 2048);
	line = tmp;
        line_num++;

        SG_LOG( SG_GENERAL, SG_BULK, "#" << line_num << " '" << line << "'" );
        if ( !line.size() || isspace(tmp[0]))
            continue;

        if (line.size() >= 3) {
            char *p = (char *)memchr(tmp, ' ', 3);
            if ( p )
                *p = 0;
        }

        line_id = atoi(tmp);
        if ( tmp[0] == 'I' ) {
            // First line, indicates IBM (i.e. DOS line endings I
            // believe.)

            // move past this line and read and discard the next line
            // which is the version and copyright information
            in.getline(tmp, 2048);
            // vector<string> vers_token = simgear::strutils::split( tmp );
            if ( strlen(tmp) > 4 ) {
               char *p = (char *)memchr(tmp, ' ', 4);
               if ( p )
                  *p = 0;
            }
            SG_LOG( SG_GENERAL, SG_INFO, "Data file version = "
                    << tmp );
	} else if ( line_id == 1 /* Airport */ ||
                    line_id == 16 /* Seaplane base */ ||
                    line_id == 17 /* Heliport */ ) {

            token.clear();
            token = simgear::strutils::split(line);
            string id = token[4];
            double elev = atof( token[1].c_str() );
            SG_LOG( SG_GENERAL, SG_BULK, "Next airport = " << id << " "
                    << elev );

            addAirport(airports, last_apt_id, last_apt_name, rwy_count, rwy_lat_accum, rwy_lon_accum,
                last_rwy_heading, last_apt_elev, last_tower, got_tower, line_id);

            last_apt_id = id;
            last_apt_elev = elev;
            last_apt_name = "";
            got_tower = false;

            // build the name
            for ( unsigned int i = 5; i < token.size() - 1; ++i ) {
                last_apt_name += token[i];
                last_apt_name += " ";
            }
            last_apt_name += token[token.size() - 1];

            last_apt_info = line;
            last_apt_type = token[0];

            // clear runway list for start of next airport
            rwy_lon_accum = 0.0;
            rwy_lat_accum = 0.0;
            rwy_count = 0;
        } else if ( line_id == 10 ) {
            token.clear();
            token = simgear::strutils::split(line);

            // runway entry
            double lat = atof( token[1].c_str() );
            double lon = atof( token[2].c_str() );
            rwy_lat_accum += lat;
            rwy_lon_accum += lon;
            rwy_count++;

            string rwy_no = token[3];

            double heading = atof( token[4].c_str() );
            double length = atoi( token[5].c_str() );
            double width = atoi( token[8].c_str() );
            
            last_rwy_heading = heading;

            string rwy_displ_threshold = token[6];
            vector<string> displ
                = simgear::strutils::split( rwy_displ_threshold, "." );
            double displ_thresh1 = atof( displ[0].c_str() );
            double displ_thresh2 = atof( displ[1].c_str() );

            string rwy_stopway = token[7];
            vector<string> stop
                = simgear::strutils::split( rwy_stopway, "." );
            double stopway1 = atof( stop[0].c_str() );
            double stopway2 = atof( stop[1].c_str() );

            string lighting_flags = token[9];
            int surface_code = atoi( token[10].c_str() );
            string shoulder_code = token[11];
            int marking_code = atoi( token[12].c_str() );
            double smoothness = atof( token[13].c_str() );
            bool dist_remaining = (atoi( token[14].c_str() ) == 1 );

            runways->add( last_apt_id, rwy_no, lon, lat, heading, length,
                          width, displ_thresh1, displ_thresh2,
                          stopway1, stopway2, lighting_flags, surface_code,
                          shoulder_code, marking_code, smoothness,
                          dist_remaining );
        } else if ( line_id == 18 ) {
            // beacon entry (ignore)
        } else if ( line_id == 14 ) {
            // control tower entry
            token.clear();
            token = simgear::strutils::split(line);

            double lat = atof( token[1].c_str() );
            double lon = atof( token[2].c_str() );
            double elev = atof( token[3].c_str() );
            last_tower = SGGeod::fromDegFt(lon, lat, elev);
            got_tower = true;
        } else if ( line_id == 19 ) {
            // windsock entry (ignore)
        } else if ( line_id == 15 ) {
            // custom startup locations (ignore)
        } else if ( line_id >= 50 && line_id <= 56 ) {
            // frequency entries (ignore)
        } else if ( line_id == 99 ) {
            SG_LOG( SG_GENERAL, SG_DEBUG, "End of file reached" );
        } else {
            SG_LOG( SG_GENERAL, SG_ALERT, 
                    "Unknown line(#" << line_num << ") in file: " << line );
            exit(-1);
        }
    }

    // add the last airport being processed if any
    addAirport(airports, last_apt_id, last_apt_name, rwy_count, rwy_lat_accum, rwy_lon_accum,
        last_rwy_heading, last_apt_elev, last_tower, got_tower, 0);


    //
    // Load the metar.dat file and update apt db with stations that
    // have metar data.
    //

    sg_gzifstream metar_in( metar_file );
    if ( !metar_in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << metar_file );
    }

    string ident;
    while ( metar_in ) {
        metar_in >> ident;
        if ( ident == "#" || ident == "//" ) {
            metar_in >> skipeol;
        } else {
            const FGAirport* a = airports->search( ident );
            if ( a ) airports->has_metar( ident );
        }
    }

    SG_LOG(SG_GENERAL, SG_INFO, "[FINISHED LOADING]");

    return true;
}
