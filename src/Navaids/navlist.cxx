// navlist.cxx -- navaids management class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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

#include <boost/foreach.hpp>
#include <algorithm>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/sg_inlines.h>

#include "navlist.hxx"

#include <Airports/runways.hxx>

using std::string;

namespace { // anonymous

class NavRecordDistanceSortPredicate
{
public:
  NavRecordDistanceSortPredicate( const SGGeod & position ) :
  _position(SGVec3d::fromGeod(position)) {}
  
  bool operator()( const nav_rec_ptr & n1, const nav_rec_ptr & n2 )
  {
    if( n1 == NULL || n2 == NULL ) return false;
    return distSqr(n1->cart(), _position) < distSqr(n2->cart(), _position);
  }
private:
  SGVec3d _position;
  
};

} // of anonymous namespace

// FGNavList ------------------------------------------------------------------


FGNavList::TypeFilter::TypeFilter(const FGPositioned::Type type)
{
  if (type == FGPositioned::INVALID) {
    _mintype = FGPositioned::VOR;
    _maxtype = FGPositioned::GS;
  } else {
    _mintype = _maxtype = type;
  }
}


FGNavList::FGNavList( void )
{
}


FGNavList::~FGNavList( void )
{
    nav_list_type navlist = navaids.begin()->second;
    navaids.erase( navaids.begin(), navaids.end() );
}


// load the navaids and build the map
bool FGNavList::init()
{
    // No need to delete the original navaid structures
    // since we're using an SGSharedPointer
    nav_list_type navlist = navaids.begin()->second;
    navaids.erase( navaids.begin(), navaids.end() );
    return true;
}

// add an entry to the lists
bool FGNavList::add( FGNavRecord *n )
{
    navaids[n->get_freq()].push_back(n);
    return true;
}

FGNavRecord *FGNavList::findByFreq( double freq, const SGGeod& position)
{
    const nav_list_type& stations = navaids[(int)(freq*100.0 + 0.5)];
    SG_LOG( SG_INSTR, SG_DEBUG, "findbyFreq " << freq << " size " << stations.size()  );
    return findNavFromList( position, stations );
}

nav_list_type FGNavList::findAllByFreq( double freq, const SGGeod& position, const FGPositioned::Type type)
{
  nav_list_type stations;
  TypeFilter filter(type);
  
  BOOST_FOREACH(nav_rec_ptr nav, navaids[(int)(freq*100.0 + 0.5)]) {
    if (filter.pass(nav.ptr())) {
      stations.push_back(nav);
    }
  }
  
  NavRecordDistanceSortPredicate sortPredicate( position );
  std::sort( stations.begin(), stations.end(), sortPredicate );
  return stations;
}


// Given an Ident and optional frequency, return the first matching
// station.
const nav_list_type FGNavList::findByIdentAndFreq(const string& ident, const double freq, const FGPositioned::Type type )
{
  FGPositionedRef cur;
  TypeFilter filter(type);
  nav_list_type reply;

  cur = FGPositioned::findNextWithPartialId(cur, ident, &filter);
  
  int f = (int)(freq*100.0 + 0.5);
  while (cur) {
    FGNavRecord* nav = static_cast<FGNavRecord*>(cur.ptr());
    if ( f <= 0.0 || nav->get_freq() == f) {
        reply.push_back( nav );
    }
    
    cur = FGPositioned::findNextWithPartialId(cur, ident, &filter);
  }

  return reply;
}

// Given an Ident and optional frequency and type ,
// return a list of matching stations sorted by distance to the given position
const nav_list_type FGNavList::findByIdentAndFreq( const SGGeod & position,
        const std::string& ident, const double freq, const FGPositioned::Type type )
{
    nav_list_type reply = findByIdentAndFreq( ident, freq, type );
    NavRecordDistanceSortPredicate sortPredicate( position );
    std::sort( reply.begin(), reply.end(), sortPredicate );

    return reply;
}

// discount navids if they conflict with another on the same frequency
// this only applies to navids associated with opposite ends of a runway,
// with matching frequencies.
static bool navidUsable(FGNavRecord* aNav, const SGGeod &aircraft)
{
  FGRunway* r(aNav->runway());
  if (!r || !r->reciprocalRunway()) {
    return true;
  }
  
// check if the runway frequency is paired
  FGNavRecord* locA = r->ILS();
  FGNavRecord* locB = r->reciprocalRunway()->ILS();
  
  if (!locA || !locB || (locA->get_freq() != locB->get_freq())) {
    return true; // not paired, ok
  }
  
// okay, both ends have an ILS, and they're paired. We need to select based on
// aircraft position. What we're going to use is *runway* (not navid) position,
// ie whichever runway end we are closer too. This makes back-course / missed
// approach behaviour incorrect, but that's the price we accept.
  double crs = SGGeodesy::courseDeg(aircraft, r->geod());
  double hdgDiff = crs - r->headingDeg();
  SG_NORMALIZE_RANGE(hdgDiff, -180.0, 180.0);  
  return (fabs(hdgDiff) < 90.0);
}

// Given a point and a list of stations, return the closest one to
// the specified point.
FGNavRecord* FGNavList::findNavFromList( const SGGeod &aircraft,
                                  const nav_list_type &stations )
{
    FGNavRecord *nav = NULL;
    double d2;                  // in meters squared
    double min_dist
        = FG_NAV_MAX_RANGE*SG_NM_TO_METER*FG_NAV_MAX_RANGE*SG_NM_TO_METER;
    SGVec3d aircraftCart = SGVec3d::fromGeod(aircraft);
    
    nav_list_const_iterator it;
    nav_list_const_iterator end = stations.end();
    // find the closest station within a sensible range (FG_NAV_MAX_RANGE)
    for ( it = stations.begin(); it != end; ++it ) {
        FGNavRecord *station = *it;
        d2 = distSqr(station->cart(), aircraftCart);
        if ( d2 > min_dist || !navidUsable(station, aircraft)) {
          continue;
        }
        
        min_dist = d2;
        nav = station;
    }

    return nav;
}

// Given a frequency, return the first matching station.
FGNavRecord *FGNavList::findStationByFreq( double freq )
{
    const nav_list_type& stations = navaids[(int)(freq*100.0 + 0.5)];

    SG_LOG( SG_INSTR, SG_DEBUG, "findStationByFreq " << freq << " size " << stations.size()  );

    if (!stations.empty()) {
        return stations[0];
    }
    return NULL;
}



// FGTACANList ----------------------------------------------------------------

FGTACANList::FGTACANList( void )
{
}


FGTACANList::~FGTACANList( void )
{
}


bool FGTACANList::init()
{
    return true;
}


// add an entry to the lists
bool FGTACANList::add( FGTACANRecord *c )
{
    ident_channels[c->get_channel()].push_back(c);
    return true;
}


// Given a TACAN Channel return the first matching frequency
FGTACANRecord *FGTACANList::findByChannel( const string& channel )
{
    const tacan_list_type& stations = ident_channels[channel];
    SG_LOG( SG_INSTR, SG_DEBUG, "findByChannel " << channel<< " size " << stations.size() );

    if (!stations.empty()) {
        return stations[0];
    }
    return NULL;
}


