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
#include <simgear/structure/exception.hxx>

#include <string>

#include "simple.hxx"
#include "runways.hxx"

#include "apt_loader.hxx"
#include <iostream>
using namespace std;

static FGPositioned::Type fptypeFromRobinType(int aType)
{
  switch (aType) {
  case 1: return FGPositioned::AIRPORT;
  case 16: return FGPositioned::SEAPORT;
  case 17: return FGPositioned::HELIPORT;
  default:
    SG_LOG(SG_GENERAL, SG_ALERT, "unsupported type:" << aType);
    cerr << "asdfasdfasdf" << endl;
    throw sg_range_exception("Unsupported airport type", "fptypeFromRobinType");
  }
}

class APTLoader
{
public:

  APTLoader()
  :  last_apt_id(""),
     last_apt_elev(0.0),
     last_apt_name(""),
     last_apt_info(""),
     last_apt_type("")
  {}

  void parseAPT(const string &aptdb_file, FGCommList *comm_list)
  {
    sg_gzifstream in( aptdb_file );

    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << aptdb_file );
        exit(-1);
    }

    string line;
    char tmp[2049];
    tmp[2048] = 0;
    
    unsigned int line_id = 0;
    unsigned int line_num = 0;

    while ( ! in.eof() ) {
      in.getline(tmp, 2048);
      line = tmp; // string copy, ack
      line_num++;

      if ( !line.size() || isspace(tmp[0])) {
        continue;
      }
      
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
        SG_LOG( SG_GENERAL, SG_INFO, "Data file version = " << tmp );
	    } else if ( line_id == 1 /* Airport */ ||
                    line_id == 16 /* Seaplane base */ ||
                    line_id == 17 /* Heliport */ ) {
        parseAirportLine(simgear::strutils::split(line));
      } else if ( line_id == 10 ) {
        parseRunwayLine(simgear::strutils::split(line));
      } else if ( line_id == 18 ) {
            // beacon entry (ignore)
      } else if ( line_id == 14 ) {
        // control tower entry
        vector<string> token(simgear::strutils::split(line));
        
        double lat = atof( token[1].c_str() );
        double lon = atof( token[2].c_str() );
        double elev = atof( token[3].c_str() );
        tower = SGGeod::fromDegFt(lon, lat, elev + last_apt_elev);
        got_tower = true;
      } else if ( line_id == 19 ) {
          // windsock entry (ignore)
      } else if ( line_id == 15 ) {
          // custom startup locations (ignore)
      } else if ( line_id == 0 ) {
          // ??
      } else if ( line_id == 50 ) {
// This assumes/requires that any code-50 line (ATIS or AWOS)
// applies to the preceding code-1 line (airport ID and name)
// and that a full set of code-10 lines (runway descriptors)
// has come between the code-1 and code-50 lines.
	    // typical code-50 lines:
	    // 50 11770 ATIS
	    // 50 11770 AWOS 3
// This code parallels code found in "operator>>" in ATC.hxx;
// FIXME: unify the code.
	    if ( rwy_count > 0 ) {
	      ATCData a;
	      a.lat = rwy_lat_accum / (double)rwy_count;
	      a.lon = rwy_lon_accum / (double)rwy_count;
	      a.elev = last_apt_elev;
	      a.range = 50;	// give all ATISs small range
	      a.ident = last_apt_id;
	      a.name = last_apt_name;
	      token.clear();
	      token = simgear::strutils::split(line);
	      // short int representing tens of kHz:
	      a.freq = atoi(token[1].c_str());
	      if (token[2] == "ATIS") a.type = ATIS;
	      else a.type = AWOS;		// ASOS same as AWOS

	      // generate cartesian coordinates
	      Point3D geod( a.lon * SGD_DEGREES_TO_RADIANS, 
		  a.lat * SGD_DEGREES_TO_RADIANS, a.elev );
	      Point3D cart = sgGeodToCart( geod );
	      a.x = cart.x();
	      a.y = cart.y();
	      a.z = cart.z();

	      comm_list->commlist_freq[a.freq].push_back(a);

	      SGBucket bucket(a.lon, a.lat);
	      int bucknum = bucket.gen_index();
	      comm_list->commlist_bck[bucknum].push_back(a);
#if 0
              SG_LOG( SG_GENERAL, SG_ALERT, 
                "Loaded ATIS/AWOS for airport: " << a.ident
	              << "  lat: "  << a.lat
	              << "  lon: "  << a.lon
	              << "  freq: " << a.freq
	              << "  type: " << a.type );
#endif
	    } else {
	      SG_LOG( SG_GENERAL, SG_ALERT, 
	      "No runways; skipping AWOS for " + last_apt_id);
	    }
        } else if ( line_id >= 51 && line_id <= 56 ) {
          // other frequency entries (ignore)
      } else if ( line_id == 99 ) {
          SG_LOG( SG_GENERAL, SG_DEBUG, "End of file reached" );
      } else {
          SG_LOG( SG_GENERAL, SG_ALERT, 
                  "Unknown line(#" << line_num << ") in file: " << line );
          exit(-1);
      }
    }

    addAirport();
  }
  
private:
  vector<string> token;
  double rwy_lat_accum;
  double rwy_lon_accum;
  double last_rwy_heading;
  int rwy_count;
  bool got_tower;
  string last_apt_id;
  string last_apt_name;
  double last_apt_elev;
  string last_apt_info;
  string last_apt_type;
  SGGeod tower;
  
  vector<FGRunwayPtr> runways;
  vector<FGTaxiwayPtr> taxiways;
  
  void addAirport()
  {  
    if (last_apt_id.empty()) {
      return;
    }

    if (!rwy_count) {
        SG_LOG(SG_GENERAL, SG_ALERT, "ERROR: No runways for " << last_apt_id
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
        tower = SGGeod::fromDegFt(lon + fudge_lon, lat + fudge_lat, last_apt_elev + tower_height);
    }

    SGGeod pos(SGGeod::fromDegFt(lon, lat, last_apt_elev));
    FGAirport* apt = new FGAirport(last_apt_id, pos, tower, last_apt_name, false,
        fptypeFromRobinType(atoi(last_apt_type.c_str())));
        
    apt->setRunwaysAndTaxiways(runways, taxiways);
  }
  
  void parseAirportLine(const vector<string>& token)
  {
    const string& id(token[4]);
    double elev = atof( token[1].c_str() );

    addAirport();
            
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
    last_apt_type = token[0];

    // clear runway list for start of next airport
    rwy_lon_accum = 0.0;
    rwy_lat_accum = 0.0;
    rwy_count = 0;
  }
  
  void parseRunwayLine(const vector<string>& token)
  {
    double lat = atof( token[1].c_str() );
    double lon = atof( token[2].c_str() );
    rwy_lat_accum += lat;
    rwy_lon_accum += lon;
    rwy_count++;

    const string& rwy_no(token[3]);

    double heading = atof( token[4].c_str() );
    double length = atoi( token[5].c_str() );
    double width = atoi( token[8].c_str() );

    last_rwy_heading = heading;

    int surface_code = atoi( token[10].c_str() );
    SGGeod pos(SGGeod::fromDegFt(lon, lat, 0.0));
    
    if (rwy_no[0] == 'x') {
      // taxiway
      FGTaxiway* t = new FGTaxiway(rwy_no, pos, heading, length, width, surface_code);
      taxiways.push_back(t);
    } else {
      // (pair of) runways
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

      FGRunway* rwy = new FGRunway(NULL, rwy_no, pos, heading, length,
                            width, displ_thresh1, stopway1, surface_code, false);
      runways.push_back(rwy);
      
      FGRunway* reciprocal = new FGRunway(NULL, FGRunway::reverseIdent(rwy_no), 
                pos, heading + 180.0, length, width, 
                displ_thresh2, stopway2, surface_code, true);
              
      runways.push_back(reciprocal);
    }
  }
};

// Load the airport data base from the specified aptdb file.  The
// metar file is used to mark the airports as having metar available
// or not.
bool fgAirportDBLoad( const string &aptdb_file, 
        FGCommList *comm_list, const std::string &metar_file )
{

   APTLoader ld;

   ld.parseAPT(aptdb_file, comm_list);

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
            FGAirport* apt = FGAirport::findByIdent(ident);
            if (apt) {
                apt->setMetar(true);
            }
        }
    }

    SG_LOG(SG_GENERAL, SG_INFO, "[FINISHED LOADING]");

    return true;
}

