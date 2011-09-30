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

#include "apt_loader.hxx"

#include <simgear/compiler.h>

#include <stdlib.h> // atof(), atoi()
#include <string.h> // memchr()
#include <ctype.h> // isspace()

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/bucket/newbucket.hxx>

#include <string>

#include "simple.hxx"
#include "runways.hxx"
#include "pavement.hxx"

#include <ATC/CommStation.hxx>

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
    throw sg_range_exception("Unsupported airport type", "fptypeFromRobinType");
  }
}

class APTLoader
{
public:

  APTLoader()
  :  last_apt_id(""),
     last_apt_name(""),
     last_apt_elev(0.0),
     last_apt_info(""),
     last_apt_type("")
  {}



  void parseAPT(const string &aptdb_file)
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
      } else if ( line_id == 10 ) { // Runway v810
        parseRunwayLine810(simgear::strutils::split(line));
      } else if ( line_id == 100 ) { // Runway v850
        parseRunwayLine850(simgear::strutils::split(line));
      } else if ( line_id == 101 ) { // Water Runway v850
        parseWaterRunwayLine850(simgear::strutils::split(line));
      } else if ( line_id == 102 ) { // Helipad v850
        parseHelipadLine850(simgear::strutils::split(line));
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
      } else if ( line_id == 20 ) {
          // Taxiway sign (ignore)
      } else if ( line_id == 21 ) {
          // lighting objects (ignore)
      } else if ( line_id == 15 ) {
          // custom startup locations (ignore)
      } else if ( line_id == 0 ) {
          // ??
      } else if ( line_id >= 50 && line_id <= 56) {
        parseCommLine(line_id, simgear::strutils::split(line));
      } else if ( line_id == 110 ) {
        pavement = true;
        parsePavementLine850(simgear::strutils::split(line, 0, 4));
      } else if ( line_id >= 111 && line_id <= 114 ) {
        if ( pavement )
          parsePavementNodeLine850(line_id, simgear::strutils::split(line));
      } else if ( line_id >= 115 && line_id <= 116 ) {
          // other pavement nodes (ignore)
      } else if ( line_id == 120 ) {
        pavement = false;
      } else if ( line_id == 130 ) {
        pavement = false;
      } else if ( line_id == 99 ) {
          SG_LOG( SG_GENERAL, SG_DEBUG, "End of file reached" );
      } else {
          SG_LOG( SG_GENERAL, SG_ALERT, 
                  "Unknown line(#" << line_num << ") in file: " << line );
          exit( -1 );
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
  SGGeod tower;
  string last_apt_info;
  string last_apt_type;
  string pavement_ident;
  bool pavement;
  
  vector<FGRunwayPtr> runways;
  vector<FGTaxiwayPtr> taxiways;
  vector<FGPavementPtr> pavements;
  vector<flightgear::CommStation*> commStations;
  
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
    apt->setRunwaysAndTaxiways(runways, taxiways, pavements);
    apt->setCommStations(commStations);
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
  
  void parseRunwayLine810(const vector<string>& token)
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
    SGGeod pos(SGGeod::fromDegFt(lon, lat, last_apt_elev));
    
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
      
      rwy->setReciprocalRunway(reciprocal);
      reciprocal->setReciprocalRunway(rwy);
    }
  }

  void parseRunwayLine850(const vector<string>& token)
  {
    double width = atof( token[1].c_str() ) * SG_METER_TO_FEET;
    int surface_code = atoi( token[2].c_str() );

    double lat_1 = atof( token[9].c_str() );
    double lon_1 = atof( token[10].c_str() );
    SGGeod pos_1(SGGeod::fromDegFt(lon_1, lat_1, 0.0));
    rwy_lat_accum += lat_1;
    rwy_lon_accum += lon_1;
    rwy_count++;

    double lat_2 = atof( token[18].c_str() );
    double lon_2 = atof( token[19].c_str() );
    SGGeod pos_2(SGGeod::fromDegFt(lon_2, lat_2, 0.0));
    rwy_lat_accum += lat_2;
    rwy_lon_accum += lon_2;
    rwy_count++;

    double length, heading_1, heading_2, dummy;
    SGGeodesy::inverse( pos_1, pos_2, heading_1, heading_2, length );
    SGGeod pos;
    SGGeodesy::direct( pos_1, heading_1, length / 2.0, pos, dummy );
    length *= SG_METER_TO_FEET;

    last_rwy_heading = heading_1;

    const string& rwy_no_1(token[8]);
    const string& rwy_no_2(token[17]);
    if ( rwy_no_1.size() == 0 || rwy_no_2.size() == 0 )
        return;

    double displ_thresh1 = atof( token[11].c_str() );
    double displ_thresh2 = atof( token[20].c_str() );

    double stopway1 = atof( token[12].c_str() );
    double stopway2 = atof( token[21].c_str() );

    FGRunway* rwy = new FGRunway(NULL, rwy_no_1, pos, heading_1, length,
                          width, displ_thresh1, stopway1, surface_code, false);
    runways.push_back(rwy);

    FGRunway* reciprocal = new FGRunway(NULL, rwy_no_2, 
              pos, heading_2, length, width, 
              displ_thresh2, stopway2, surface_code, true);
    runways.push_back(reciprocal);
    
    rwy->setReciprocalRunway(reciprocal);
    reciprocal->setReciprocalRunway(rwy);
  }

  void parseWaterRunwayLine850(const vector<string>& token)
  {
    double width = atof( token[1].c_str() ) * SG_METER_TO_FEET;

    double lat_1 = atof( token[4].c_str() );
    double lon_1 = atof( token[5].c_str() );
    SGGeod pos_1(SGGeod::fromDegFt(lon_1, lat_1, 0.0));
    rwy_lat_accum += lat_1;
    rwy_lon_accum += lon_1;
    rwy_count++;

    double lat_2 = atof( token[7].c_str() );
    double lon_2 = atof( token[8].c_str() );
    SGGeod pos_2(SGGeod::fromDegFt(lon_2, lat_2, 0.0));
    rwy_lat_accum += lat_2;
    rwy_lon_accum += lon_2;
    rwy_count++;

    double length, heading_1, heading_2, dummy;
    SGGeodesy::inverse( pos_1, pos_2, heading_1, heading_2, length );
    SGGeod pos;
    SGGeodesy::direct( pos_1, heading_1, length / 2.0, pos, dummy );

    last_rwy_heading = heading_1;

    const string& rwy_no_1(token[3]);
    const string& rwy_no_2(token[6]);

    FGRunway* rwy = new FGRunway(NULL, rwy_no_1, pos, heading_1, length,
                          width, 0.0, 0.0, 13, false);
    runways.push_back(rwy);

    FGRunway* reciprocal = new FGRunway(NULL, rwy_no_2, 
              pos, heading_2, length, width, 
              0.0, 0.0, 13, true);
    runways.push_back(reciprocal);
    
    rwy->setReciprocalRunway(reciprocal);
    reciprocal->setReciprocalRunway(rwy);
  }

  void parseHelipadLine850(const vector<string>& token)
  {
    double length = atof( token[5].c_str() ) * SG_METER_TO_FEET;
    double width = atof( token[6].c_str() ) * SG_METER_TO_FEET;

    double lat = atof( token[2].c_str() );
    double lon = atof( token[3].c_str() );
    SGGeod pos(SGGeod::fromDegFt(lon, lat, 0.0));
    rwy_lat_accum += lat;
    rwy_lon_accum += lon;
    rwy_count++;

    double heading = atof( token[4].c_str() );

    last_rwy_heading = heading;

    const string& rwy_no(token[1]);
    int surface_code = atoi( token[7].c_str() );

    FGRunway* rwy = new FGRunway(NULL, rwy_no, pos, heading, length,
                          width, 0.0, 0.0, surface_code, false);
    runways.push_back(rwy);
  }

  void parsePavementLine850(const vector<string>& token)
  {
    if ( token.size() >= 5 ) {
      pavement_ident = token[4];
      if ( !pavement_ident.empty() && pavement_ident[pavement_ident.size()-1] == '\r' )
        pavement_ident.erase( pavement_ident.size()-1 );
    } else {
      pavement_ident = "xx";
    }
  }

  void parsePavementNodeLine850(int num, const vector<string>& token)
  {
    double lat = atof( token[1].c_str() );
    double lon = atof( token[2].c_str() );
    SGGeod pos(SGGeod::fromDegFt(lon, lat, 0.0));

    FGPavement* pvt = 0;
    if ( !pavement_ident.empty() ) {
      pvt = new FGPavement( pavement_ident, pos );
      pavements.push_back( pvt );
      pavement_ident = "";
    } else {
      pvt = pavements.back();
    }
    if ( num == 112 || num == 114 ) {
      double lat_b = atof( token[3].c_str() );
      double lon_b = atof( token[4].c_str() );
      SGGeod pos_b(SGGeod::fromDegFt(lon_b, lat_b, 0.0));
      pvt->addBezierNode(pos, pos_b, num == 114);
    } else {
      pvt->addNode(pos, num == 113);
    }
  }

  void parseCommLine(int lineId, const vector<string>& token) 
  {
    if ( rwy_count <= 0 ) {
      SG_LOG( SG_GENERAL, SG_ALERT, "No runways; skipping comm for " + last_apt_id);
    }
 
    SGGeod pos = SGGeod::fromDegFt(rwy_lon_accum / (double)rwy_count, 
        rwy_lat_accum / (double)rwy_count, last_apt_elev);
    
    // short int representing tens of kHz:
    int freqKhz = atoi(token[1].c_str());
    int rangeNm = 50;
    FGPositioned::Type ty;
    // Make sure we only pass on stations with at least a name
    if (token.size() >2){

        switch (lineId) {
            case 50:
                ty = FGPositioned::FREQ_AWOS;
                if (token[2] == "ATIS") {
                    ty = FGPositioned::FREQ_ATIS;
                }
                break;

            case 51:    ty = FGPositioned::FREQ_UNICOM; break;
            case 52:    ty = FGPositioned::FREQ_CLEARANCE; break;
            case 53:    ty = FGPositioned::FREQ_GROUND; break;
            case 54:    ty = FGPositioned::FREQ_TOWER; break;
            case 55:
            case 56:    ty = FGPositioned::FREQ_APP_DEP; break;
            default:
                throw sg_range_exception("unupported apt.dat comm station type");
        }

        commStations.push_back(new flightgear::CommStation(token[2], ty, pos, rangeNm, freqKhz));
    }
    else SG_LOG( SG_GENERAL, SG_DEBUG, "Found unnamed comm. Skipping: " << lineId);
  }

};


// Load the airport data base from the specified aptdb file.  The
// metar file is used to mark the airports as having metar available
// or not.
bool fgAirportDBLoad( const string &aptdb_file, const std::string &metar_file )
{

   APTLoader ld;
   ld.parseAPT(aptdb_file);
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

