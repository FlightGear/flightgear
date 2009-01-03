// navdb.cxx -- top level navaids management routines
//
// Written by Curtis Olson, started May 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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
#  include "config.h"
#endif

#include <simgear/compiler.h>

#include <string>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sgstream.hxx>

#include "navrecord.hxx"
#include "navlist.hxx"
#include "navdb.hxx"
#include "Main/globals.hxx"
#include "Navaids/markerbeacon.hxx"
#include "Airports/simple.hxx"

using std::string;

static FGPositioned::Type
mapRobinTypeToFGPType(int aTy)
{
  switch (aTy) {
 // case 1:
  case 2: return FGPositioned::NDB;
  case 3: return FGPositioned::VOR;
  case 4: return FGPositioned::LOC;
  case 5: return FGPositioned::ILS;
  case 6: return FGPositioned::GS;
  case 12:
  case 13: return FGPositioned::DME;
  case 99: return FGPositioned::INVALID; // end-of-file code
  default:
    throw sg_range_exception("Got a nav.dat type we don't recognize", "FGNavRecord::createFromStream");
  }
}

static FGNavRecord* createNavFromStream(std::istream& aStream)
{
  int rawType;
  aStream >> rawType;
  if (aStream.eof()) {
    return NULL; // happens with, eg, carrier_nav.dat
  }
  
  double lat, lon, elev_ft, multiuse;
  int freq, range;
  std::string name, ident;
  aStream >> lat >> lon >> elev_ft >> freq >> range >> multiuse >> ident;
  getline(aStream, name);
  
  SGGeod pos(SGGeod::fromDegFt(lon, lat, elev_ft));
  name = simgear::strutils::strip(name);
  
  if ((rawType >= 7) && (rawType <= 9)) {
    // marker beacons use a different run-time class now
     FGMarkerBeacon::create(rawType, name, pos);
     return NULL; // not a nav-record, but that's okay
  }
  
  FGPositioned::Type type = mapRobinTypeToFGPType(rawType);
  if (type == FGPositioned::INVALID) {
    return NULL;
  }
  
  // silently multiply adf frequencies by 100 so that adf
  // vs. nav/loc frequency lookups can use the same code.
  if (type == FGPositioned::NDB) {
    freq *= 100;
  }
  
  return new FGNavRecord(type, ident, name, pos,
    freq, range, multiuse);
}

// load and initialize the navigational databases
bool fgNavDBInit( FGNavList *navlist, FGNavList *loclist, FGNavList *gslist,
                  FGNavList *dmelist, 
                  FGNavList *tacanlist, FGNavList *carrierlist,
                  FGTACANList *channellist)
{
    SG_LOG(SG_GENERAL, SG_INFO, "Loading Navaid Databases");

    SGPath path( globals->get_fg_root() );
    path.append( "Navaids/nav.dat" );

    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }

    // skip first two lines
    in >> skipeol;
    in >> skipeol;

    while (!in.eof()) {
      FGNavRecord *r = createNavFromStream(in);
      if (!r) {
        continue;
      }
      
      switch (r->type()) {
      case FGPositioned::NDB:
      case FGPositioned::VOR:
        navlist->add(r);
        break;
        
      case FGPositioned::ILS:
      case FGPositioned::LOC:
        loclist->add(r);
        break;
        
      case FGPositioned::GS:
        gslist->add(r);
        break;
      
      case FGPositioned::DME:
      {
        dmelist->add(r);
        string::size_type loc1= r->name().find( "TACAN", 0 );
        string::size_type loc2 = r->name().find( "VORTAC", 0 );
                       
        if( loc1 != string::npos || loc2 != string::npos) {
          tacanlist->add(r);
        }

        break;
      }
      
      default:
        throw sg_range_exception("got unsupported NavRecord type", "fgNavDBInit");
      }

      in >> skipcomment;
    } // of stream data loop

// load the carrier navaids file
    
    string file, name;
    path = globals->get_fg_root() ;
    path.append( "Navaids/carrier_nav.dat" );
    
    file = path.str();
    SG_LOG( SG_GENERAL, SG_INFO, "opening file: " << path.str() );
    
    sg_gzifstream incarrier( path.str() );
    
    if ( !incarrier.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }
    
    // skip first two lines
    //incarrier >> skipeol;
    //incarrier >> skipeol;
    
    while ( ! incarrier.eof() ) {
      FGNavRecord *r = createNavFromStream(incarrier);
      if (!r) {
        continue;
      }
      
      carrierlist->add (r);
    } // end while

// end loading the carrier navaids file

// load the channel/freqency file
    string channel, freq;
    path="";
    path = globals->get_fg_root();
    path.append( "Navaids/TACAN_freq.dat" );
    
    SG_LOG( SG_GENERAL, SG_INFO, "opening file: " << path.str() );
        
    sg_gzifstream inchannel( path.str() );
    
    if ( !inchannel.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }
    
    // skip first line
    inchannel >> skipeol;
    while ( ! inchannel.eof() ) {
        FGTACANRecord *r = new FGTACANRecord;
        inchannel >> (*r);
        channellist->add ( r );
       	//cout << "channel = " << r->get_channel() ;
       	//cout << " freq = " << r->get_freq() << endl;
 	
    } // end while

 
 // end ReadChanFile


    return true;
}

FGRunway* getRunwayFromName(const std::string& aName)
{
  vector<string> parts = simgear::strutils::split(aName);
  if (parts.size() < 2) {
    SG_LOG(SG_GENERAL, SG_WARN, "getRunwayFromName: malformed name:" << aName);
    return NULL;
  }
  
  const FGAirport* apt = fgFindAirportID(parts[0]);
  if (!apt) {
    SG_LOG(SG_GENERAL, SG_WARN, "navaid " << aName << " associated with bogus airport ID:" << parts[0]);
    return NULL;
  }
  
  FGRunway* runway = apt->getRunwayByIdent(parts[1]);
  if (!runway) {
    SG_LOG(SG_GENERAL, SG_WARN, "navaid " << aName << " associated with bogus runway ID:" << parts[1]);
    return NULL;
  }

  return runway;
}
