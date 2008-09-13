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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <math.h>
#include <algorithm>

#include <simgear/compiler.h>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/debug/logstream.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>
#include <Airports/dynamics.hxx>

#include <string>

#include "simple.hxx"
#include "xmlloader.hxx"

using std::sort;
using std::random_shuffle;




/***************************************************************************
 * FGAirport
 ***************************************************************************/

FGAirport::FGAirport(const string &id, const SGGeod& location, const SGGeod& tower_location,
        const string &name, bool has_metar, Type aType) :
    FGPositioned(aType, id, location),
    _tower_location(tower_location),
    _name(name),
    _has_metar(has_metar),
    _dynamics(0)
{
}


FGAirport::~FGAirport()
{
    delete _dynamics;
}

bool FGAirport::isAirport() const
{
  return type() == AIRPORT;
}

bool FGAirport::isSeaport() const
{
  return type() == SEAPORT;
}

bool FGAirport::isHeliport() const
{
  return type() == HELIPORT;
}

FGAirportDynamics * FGAirport::getDynamics()
{
    if (_dynamics != 0) {
        return _dynamics;
    } else {
        //cerr << "Trying to load dynamics for " << _id << endl;
        _dynamics = new FGAirportDynamics(this);
        XMLLoader::load(_dynamics);

        FGRunwayPreference rwyPrefs(this);
        XMLLoader::load(&rwyPrefs);
        _dynamics->setRwyUse(rwyPrefs);
   }
    return _dynamics;
}

unsigned int FGAirport::numRunways() const
{
  return mRunways.size();
}

FGRunway* FGAirport::getRunwayByIndex(unsigned int aIndex) const
{
  assert(aIndex >= 0 && aIndex < mRunways.size());
  return mRunways[aIndex];
}

bool FGAirport::hasRunwayWithIdent(const string& aIdent) const
{
  return (getIteratorForRunwayIdent(aIdent) != mRunways.end());
}

FGRunway* FGAirport::getRunwayByIdent(const string& aIdent) const
{
  Runway_iterator it = getIteratorForRunwayIdent(aIdent);
  if (it == mRunways.end()) {
    SG_LOG(SG_GENERAL, SG_ALERT, "no such runway '" << aIdent << "' at airport " << ident());
    throw sg_range_exception("unknown runway " + aIdent + " at airport:" + ident(), "FGAirport::getRunwayByIdent");
  }
  
  return *it;
}

FGAirport::Runway_iterator
FGAirport::getIteratorForRunwayIdent(const string& aIdent) const
{
  string ident(aIdent);
  if ((aIdent.size() == 1) || !isdigit(aIdent[1])) {
    ident = "0" + aIdent;
  }

  Runway_iterator it = mRunways.begin();
  for (; it != mRunways.end(); ++it) {
    if ((*it)->ident() == ident) {
      return it;
    }
  }

  return it; // end()
}

static double normaliseBearing(double aBearing)
{
  while (aBearing < 0.0) {
    aBearing += 360.0;
  }
  
  while (aBearing > 360.0) {
    aBearing -= 360.0;
  }
  
  return aBearing;
}

FGRunway* FGAirport::findBestRunwayForHeading(double aHeading) const
{
  Runway_iterator it = mRunways.begin();
  FGRunway* result = NULL;
  double currentBestQuality = 0.0;
  
  SGPropertyNode *param = fgGetNode("/sim/airport/runways/search", true);
  double lengthWeight = param->getDoubleValue("length-weight", 0.01);
  double widthWeight = param->getDoubleValue("width-weight", 0.01);
  double surfaceWeight = param->getDoubleValue("surface-weight", 10);
  double deviationWeight = param->getDoubleValue("deviation-weight", 1);
    
  for (; it != mRunways.end(); ++it) {
    double good = (*it)->score(lengthWeight, widthWeight, surfaceWeight);
    
    double dev = normaliseBearing(aHeading - (*it)->headingDeg());
    double bad = fabs(deviationWeight * dev) + 1e-20;
    double quality = good / bad;
    
    if (quality > currentBestQuality) {
      currentBestQuality = quality;
      result = *it;
    }
  }

  return result;
}

unsigned int FGAirport::numTaxiways() const
{
  return mTaxiways.size();
}

FGRunway* FGAirport::getTaxiwayByIndex(unsigned int aIndex) const
{
  assert(aIndex >= 0 && aIndex < mTaxiways.size());
  return mTaxiways[aIndex];
}

void FGAirport::addRunway(FGRunway* aRunway)
{
  aRunway->setAirport(this);
  
  if (aRunway->isTaxiway()) {
    mTaxiways.push_back(aRunway);
  } else {
    mRunways.push_back(aRunway);
  }
}

FGRunway* FGAirport::getActiveRunwayForUsage() const
{
  static FGEnvironmentMgr* envMgr = NULL;
  if (!envMgr) {
    envMgr = (FGEnvironmentMgr *) globals->get_subsystem("environment");
  }
  
  FGEnvironment stationWeather(envMgr->getEnvironment(mPosition));
  
  double windSpeed = stationWeather.get_wind_speed_kt();
  double hdg = stationWeather.get_wind_from_heading_deg();
  if (windSpeed <= 0.0) {
    hdg = 270;	// This forces West-facing rwys to be used in no-wind situations
    // which is consistent with Flightgear's initial setup.
  }
  
  return findBestRunwayForHeading(hdg);
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


FGAirportList::~FGAirportList( void )
{
    for (unsigned int i = 0; i < airports_array.size(); ++i) {
        delete airports_array[i];
    }
}


// add an entry to the list
FGAirport* FGAirportList::add( const string &id, const SGGeod& location, const SGGeod& tower_location,
                         const string &name, bool has_metar, FGPositioned::Type aType)
{
    FGAirport* a = new FGAirport(id, location, tower_location, name, has_metar, aType);
    airports_by_id[a->getId()] = a;
    // try and read in an auxilary file

    airports_array.push_back( a );
    SG_LOG( SG_GENERAL, SG_BULK, "Adding " << id << " pos = " << location.getLongitudeDeg()
            << ", " << location.getLatitudeDeg() << " elev = " << location.getElevationFt() );
            
    return a;
}


// search for the specified id
FGAirport* FGAirportList::search( const string& id)
{
    airport_map_iterator itr = airports_by_id.find(id);
    return (itr == airports_by_id.end() ? NULL : itr->second);
}

// wrap an FGIdentOrdering in an STL-compatible functor. not the most
// efficent / pretty thing in the world, but avoids template nastiness in the 
// headers, and we're only doing O(log(N)) comparisoms per search
class orderingFunctor
{
public:
  orderingFunctor(FGIdentOrdering* aOrder) :
    mOrdering(aOrder)
  { assert(aOrder); }
  
  bool operator()(const airport_map::value_type& aA, const std::string& aB) const
  {
    return mOrdering->compare(aA.first,aB);
  }

  bool operator()(const std::string& aA, const airport_map::value_type& aB) const
  {
    return mOrdering->compare(aA, aB.first);
  }

  bool operator()(const airport_map::value_type& aA, const airport_map::value_type& aB) const
  {
    return mOrdering->compare(aA.first, aB.first);
  }
  
private:
  FGIdentOrdering* mOrdering;
};

const FGAirport* FGAirportList::findFirstById(const std::string& aIdent, FGIdentOrdering* aOrder)
{
  airport_map_iterator itr;
  if (aOrder) {
    orderingFunctor func(aOrder);
    itr = std::lower_bound(airports_by_id.begin(),airports_by_id.end(), aIdent, func);
  } else {
    itr = airports_by_id.lower_bound(aIdent);
  }
  
  if (itr == airports_by_id.end()) {
    return NULL;
  }
  
  return itr->second;
}

// search for the airport nearest the specified position
FGAirport* FGAirportList::search(double lon_deg, double lat_deg, double max_range)
{
    static FGAirportSearchFilter accept_any;
    return search(lon_deg, lat_deg, max_range, accept_any);
}


// search for the airport nearest the specified position and
// passing the filter
FGAirport* FGAirportList::search(double lon_deg, double lat_deg,
        double max_range,
        FGAirportSearchFilter& filter)
{
    double min_dist = max_range;

    airport_list_iterator it = airports_array.begin();
    airport_list_iterator end = airports_array.end();
    airport_list_iterator closest = end;
    for (; it != end; ++it) {
        if (!filter.pass(*it))
            continue;

        // crude manhatten distance based on lat/lon difference
        double d = fabs(lon_deg - (*it)->getLongitude())
                + fabs(lat_deg - (*it)->getLatitude());
        if (d < min_dist) {
            closest = it;
            min_dist = d;
        }
    }
    return closest != end ? *closest : 0;
}


int
FGAirportList::size () const
{
    return airports_array.size();
}


const FGAirport *FGAirportList::getAirport( unsigned int index ) const
{
    if (index < airports_array.size()) {
        return(airports_array[index]);
    } else {
        return(NULL);
    }
}


/**
 * Mark the specified airport record as not having metar
 */
void FGAirportList::no_metar( const string &id )
{
    if(airports_by_id.find(id) != airports_by_id.end()) {
        airports_by_id[id]->setMetar(false);
    }
}


/**
 * Mark the specified airport record as (yes) having metar
 */
void FGAirportList::has_metar( const string &id )
{
    if(airports_by_id.find(id) != airports_by_id.end()) {
        airports_by_id[id]->setMetar(true);
    }
}


// find basic airport location info from airport database
const FGAirport *fgFindAirportID( const string& id)
{
    const FGAirport* result = NULL;
    if ( id.length() ) {
        SG_LOG( SG_GENERAL, SG_BULK, "Searching for airport code = " << id );

        result = globals->get_airports()->search( id );

        if ( result == NULL ) {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Failed to find " << id << " in apt.dat.gz" );
            return NULL;
        }
    } else {
        return NULL;
    }
    SG_LOG( SG_GENERAL, SG_BULK,
            "Position for " << id << " is ("
            << result->getLongitude() << ", "
            << result->getLatitude() << ")" );

    return result;
}


// get airport elevation
double fgGetAirportElev( const string& id )
{
    SG_LOG( SG_GENERAL, SG_BULK,
            "Finding elevation for airport: " << id );

    const FGAirport *a=fgFindAirportID( id);
    if (a) {
        return a->getElevation();
    } else {
        return -9999.0;
    }
}


// get airport position
Point3D fgGetAirportPos( const string& id )
{
    SG_LOG( SG_ATC, SG_BULK,
            "Finding position for airport: " << id );

    const FGAirport *a = fgFindAirportID( id);

    if (a) {
        return Point3D(a->getLongitude(), a->getLatitude(), a->getElevation());
    } else {
        return Point3D(0.0, 0.0, -9999.0);
    }
}
