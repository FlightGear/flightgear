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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <stdlib.h> // atof(), atoi()

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/strutils.hxx>

#include STL_STRING

#include "simple.hxx"
#include "runways.hxx"

#include "apt_loader.hxx"


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
    string line;
    char tmp[2048];

    double rwy_lon_accum = 0.0;
    double rwy_lat_accum = 0.0;
    int rwy_count = 0;

    while ( ! in.eof() ) {
	in.getline(tmp, 2048);
	line = tmp;
	SG_LOG( SG_GENERAL, SG_BULK, "-> '" << line << "'" );
        if ( line.length() ) {
            token = simgear::strutils::split( line );
            if ( token.size() ) {
                SG_LOG( SG_GENERAL, SG_BULK, "token[0] " << token[0] );
            }
        } else {
            token.clear();
        }

        if ( !line.length() || !token.size() ) {
            // empty line, skip
        } else if ( (token[0] == "#") || (token[0] == "//") ) {
	    // comment, skip
        } else if ( token[0] == "I" ) {
            // First line, indicates IBM (i.e. DOS line endings I
            // believe.)

            // move past this line and read and discard the next line
            // which is the version and copyright information
            in.getline(tmp, 2048);
            vector<string> vers_token = simgear::strutils::split( tmp );
            SG_LOG( SG_GENERAL, SG_INFO, "Data file version = "
                    << vers_token[0] );
	} else if ( token[0] == "1" /* Airport */ ||
                    token[0] == "16" /* Seaplane base */ ||
                    token[0] == "17" /* Heliport */ ) {

            string id = token[4];
            double elev = atof( token[1].c_str() );
            SG_LOG( SG_GENERAL, SG_BULK, "Next airport = " << id << " "
                    << elev );

            if ( !last_apt_id.empty()) {
                if ( rwy_count > 0 ) {
                    double lat = rwy_lat_accum / (double)rwy_count;
                    double lon = rwy_lon_accum / (double)rwy_count;
                    airports->add( last_apt_id, lon, lat, last_apt_elev,
                                   last_apt_name, false );
		} else {
		    if ( !last_apt_id.length() ) {
			SG_LOG(SG_GENERAL, SG_ALERT,
                               "ERROR: No runways for " << last_apt_id
                               << " skipping." );
		    }
		}
            }

            last_apt_id = id;
            last_apt_elev = atof( token[1].c_str() );
            last_apt_name = "";

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
        } else if ( token[0] == "10" ) {
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
        } else if ( token[0] == "18" ) {
            // beacon entry (ignore)
        } else if ( token[0] == "14" ) {
            // control tower entry (ignore)
        } else if ( token[0] == "19" ) {
            // windsock entry (ignore)
        } else if ( token[0] == "15" ) {
            // custom startup locations (ignore)
        } else if ( token[0] == "50" || token[0] == "51" || token[0] == "52" 
                    || token[0] == "53" || token[0] == "54" || token[0] == "55" 
                    || token[0] == "56" )
        {
            // frequency entries (ignore)
        } else if ( token[0] == "99" ) {
            SG_LOG( SG_GENERAL, SG_DEBUG, "End of file reached" );
        } else {
            SG_LOG( SG_GENERAL, SG_ALERT, 
                    "Unknown line in file: " << line );
            exit(-1);
        }
    }

    if ( !last_apt_id.empty()) {
        if ( rwy_count > 0 ) {
            double lat = rwy_lat_accum / (double)rwy_count;
            double lon = rwy_lon_accum / (double)rwy_count;
            airports->add( last_apt_id, lon, lat, last_apt_elev,
                           last_apt_name, false );
        } else {
            if ( !last_apt_id.length() ) {
                SG_LOG(SG_GENERAL, SG_ALERT,
                       "ERROR: No runways for " << last_apt_id
                       << " skipping." );
            }
        }
    }

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
            const FGAirport &a = airports->search( ident );
            if ( a.getId() == ident ) {
                airports->has_metar( ident );
            }
        }
    }

    SG_LOG(SG_GENERAL, SG_INFO, "[FINISHED LOADING]");

    return true;
}
