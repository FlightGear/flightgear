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
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sgstream.hxx>

#include "navrecord.hxx"
#include "navdb.hxx"
#include "Main/globals.hxx"

using std::string;


// load and initialize the navigational databases
bool fgNavDBInit( FGNavList *navlist, FGNavList *loclist, FGNavList *gslist,
                  FGNavList *dmelist, FGNavList *mkrlist, 
                  FGNavList *tacanlist, FGNavList *carrierlist,
                  FGTACANList *channellist)
{
    SG_LOG(SG_GENERAL, SG_INFO, "Loading Navaid Databases");
    // SG_LOG(SG_GENERAL, SG_INFO, "  VOR/NDB");
    // SGPath p_nav( globals->get_fg_root() );
    // p_nav.append( "Navaids/default.nav" );
    // navlist->init( p_nav );

    // SG_LOG(SG_GENERAL, SG_INFO, "  ILS and Marker Beacons");
    // beacons->init();
    // SGPath p_ils( globals->get_fg_root() );
    // p_ils.append( "Navaids/default.ils" );
    // ilslist->init( p_ils );


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
      FGNavRecord *r = FGNavRecord::createFromStream(in);
      if (!r) {
        break;
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
      
      case FGPositioned::OM:
      case FGPositioned::MM:
      case FGPositioned::IM:
        mkrlist->add(r);
        break;
      
      case FGPositioned::DME:
      {
        dmelist->add(r);
        string::size_type loc1= r->get_name().find( "TACAN", 0 );
        string::size_type loc2 = r->get_name().find( "VORTAC", 0 );
                       
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
      FGNavRecord *r = FGNavRecord::createFromStream(incarrier);
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
