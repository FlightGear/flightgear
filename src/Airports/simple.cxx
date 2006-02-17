//
// simple.cxx -- a really simplistic class to manage airport ID,
//               lat, lon of the center of one of it's runways, and 
//               elevation in feet.
//
// Written by Curtis Olson, started April 1998.
// Updated by Durk Talsma, started December, 2004.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <math.h>
#include <algorithm>

#include <simgear/compiler.h>

#include <plib/sg.h>
#include <plib/ul.h>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
//#include <simgear/route/waypoint.hxx>
#include <simgear/debug/logstream.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>

#include STL_STRING

#include "simple.hxx"

SG_USING_STD(sort);
SG_USING_STD(random_shuffle);






/***************************************************************************
 * FGAirport
 ***************************************************************************/
FGAirport::FGAirport() : _longitude(0), _latitude(0), _elevation(0)
{
  dynamics = 0;
}


FGAirport::FGAirport(const string &id, double lon, double lat, double elev, const string &name, bool has_metar)
{
  _id = id;
  _longitude = lon;
  _latitude  = lat;
  _elevation = elev;
  _name      = name;
  _has_metar = has_metar;
  dynamics   = 0;
}

FGAirport::~FGAirport()
{
    delete dynamics;
}


FGAirportDynamics * FGAirport::getDynamics()
{
  
  if (dynamics != 0)
    return dynamics;
  else
    {
      FGRunwayPreference rwyPrefs;
      //cerr << "Trying to load dynamics for " << _id << endl;
      dynamics = new FGAirportDynamics(_latitude, _longitude, _elevation, _id);

      SGPath parkpath( globals->get_fg_root() );
      parkpath.append( "/Airports/AI/" );
      parkpath.append(_id);
      parkpath.append("parking.xml"); 
      
      SGPath rwyPrefPath( globals->get_fg_root() );
      rwyPrefPath.append( "/Airports/AI/" );
      rwyPrefPath.append(_id);
      rwyPrefPath.append("rwyuse.xml");
      //if (ai_dirs.find(id.c_str()) != ai_dirs.end()
      //  && parkpath.exists()) 
      if (parkpath.exists())
	{
	  try {
	    readXML(parkpath.str(),*dynamics);
 	    dynamics->init();
	  } 
	  catch  (const sg_exception &e) {
	    //cerr << "unable to read " << parkpath.str() << endl;
	  }
	}
      //if (ai_dirs.find(id.c_str()) != ai_dirs.end()
      //  && rwyPrefPath.exists()) 
      if (rwyPrefPath.exists())
	{
	  try {
	    readXML(rwyPrefPath.str(), rwyPrefs);
	    dynamics->setRwyUse(rwyPrefs);
	  }
	  catch  (const sg_exception &e) {
	    //cerr << "unable to read " << rwyPrefPath.str() << endl;
	    //exit(1);
	  }
	}
      //exit(1);
    }
  return dynamics;
}





/******************************************************************************
 * FGAirportList
 *****************************************************************************/

// Populates a list of subdirectories of $FG_ROOT/Airports/AI so that
// the add() method doesn't have to try opening 2 XML files in each of
// thousands of non-existent directories.  FIXME: should probably add
// code to free this list after parsing of apt.dat is finished;
// non-issue at the moment, however, as there are no AI subdirectories
// in the base package.
//
// Note: 2005/12/23: This is probably not necessary anymore, because I'm
// Switching to runtime airport dynamics loading (DT). 
FGAirportList::FGAirportList()
{
//     ulDir* d;
//     ulDirEnt* dent;
//     SGPath aid( globals->get_fg_root() );
//     aid.append( "/Airports/AI" );
//     if((d = ulOpenDir(aid.c_str())) == NULL)
//         return;
//     while((dent = ulReadDir(d)) != NULL) {
//         SG_LOG( SG_GENERAL, SG_DEBUG, "Dent: " << dent->d_name );
//         ai_dirs.insert(dent->d_name);
//     }
//     ulCloseDir(d);
}


FGAirportList::~FGAirportList( void ) {
    for(unsigned int i = 0; i < airports_array.size(); ++i) {
        delete airports_array[i];
    }
}


// add an entry to the list
void FGAirportList::add( const string &id, const double longitude,
                         const double latitude, const double elevation,
                         const string &name, const bool has_metar )
{
    FGRunwayPreference rwyPrefs;
    FGAirport* a = new FGAirport(id, longitude, latitude, elevation, name, has_metar);

    
    airports_by_id[a->getId()] = a;
    // try and read in an auxilary file
    
    airports_array.push_back( a );
    SG_LOG( SG_GENERAL, SG_BULK, "Adding " << id << " pos = " << longitude
            << ", " << latitude << " elev = " << elevation );
}


// search for the specified id
FGAirport* FGAirportList::search( const string& id) {
    airport_map_iterator itr = airports_by_id.find(id); 
    return(itr == airports_by_id.end() ? NULL : itr->second);
}


// search for first subsequent alphabetically to supplied id
const FGAirport* FGAirportList::findFirstById( const string& id, bool exact ) {
    airport_map_iterator itr;
    if(exact) {
        itr = airports_by_id.find(id);
    } else {
        itr = airports_by_id.lower_bound(id);
    }
    if(itr == airports_by_id.end()) {
        return(NULL);
    } else {
        return(itr->second);
    }
}


// search for the airport nearest the specified position
FGAirport* FGAirportList::search( double lon_deg, double lat_deg,
                                 bool with_metar ) {
    int closest = -1;
    double min_dist = 360.0;
    unsigned int i;
    for ( i = 0; i < airports_array.size(); ++i ) {
        // crude manhatten distance based on lat/lon difference
        double d = fabs(lon_deg - airports_array[i]->getLongitude())
            + fabs(lat_deg - airports_array[i]->getLatitude());
        if ( d < min_dist ) {
            if ( !with_metar || (with_metar&&airports_array[i]->getMetar()) ) {
                closest = i;
                min_dist = d;
            }
        }
    }

    return ( closest > -1 ? airports_array[closest] : NULL );
}


int
FGAirportList::size () const
{
    return airports_array.size();
}

const FGAirport *FGAirportList::getAirport( unsigned int index ) const
{
    if(index < airports_array.size()) {
        return(airports_array[index]);
    } else {
        return(NULL);
    }
}


/**
 * Mark the specified airport record as not having metar
 */
void FGAirportList::no_metar( const string &id ) {
    if(airports_by_id.find(id) != airports_by_id.end()) { 
        airports_by_id[id]->setMetar(false);
    }
}


/**
 * Mark the specified airport record as (yes) having metar
 */
void FGAirportList::has_metar( const string &id ) {
    if(airports_by_id.find(id) != airports_by_id.end()) { 
        airports_by_id[id]->setMetar(true);
    }
}

// find basic airport location info from airport database
const FGAirport *fgFindAirportID( const string& id) {
    const FGAirport* result = NULL;
    if ( id.length() ) {
        SG_LOG( SG_GENERAL, SG_INFO, "Searching for airport code = " << id );

        result = globals->get_airports()->search( id );

        if ( result == NULL ) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Failed to find " << id << " in apt.dat.gz" );
            return NULL;
        }
    } else {
        return NULL;
    }
    SG_LOG( SG_GENERAL, SG_INFO,
            "Position for " << id << " is ("
            << result->getLongitude() << ", "
            << result->getLatitude() << ")" );

    return result;
}


// get airport elevation
double fgGetAirportElev( const string& id ) {
    
    // double lon, lat;

    SG_LOG( SG_GENERAL, SG_INFO,
            "Finding elevation for airport: " << id );

    const FGAirport *a=fgFindAirportID( id);
    if (a) {
        return a->getElevation();
    } else {
        return -9999.0;
    }
}

// get airport position
Point3D fgGetAirportPos( const string& id ) {
    // double lon, lat;

    SG_LOG( SG_ATC, SG_INFO,
            "Finding position for airport: " << id );

    const FGAirport *a = fgFindAirportID( id);
    
    if (a) {
        return Point3D(a->getLongitude(), a->getLatitude(), a->getElevation());
    } else {
        return Point3D(0.0, 0.0, -9999.0);
    }
}
