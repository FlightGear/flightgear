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

#include <cassert>
#include <algorithm>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/sg_inlines.h>

#include "navlist.hxx"

#include <Airports/runways.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>

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

// discount navids if they conflict with another on the same frequency
// this only applies to navids associated with opposite ends of a runway,
// with matching frequencies.
bool navidUsable(FGNavRecord* aNav, const SGGeod &aircraft)
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

} // of anonymous namespace

// FGNavList ------------------------------------------------------------------


//------------------------------------------------------------------------------
FGNavList::TypeFilter::TypeFilter(const FGPositioned::Type type)
{
  if (type == FGPositioned::INVALID) {
    _mintype = FGPositioned::NDB;
    _maxtype = FGPositioned::GS;
  } else {
    _mintype = _maxtype = type;
  }
}

//------------------------------------------------------------------------------
FGNavList::TypeFilter::TypeFilter( const FGPositioned::Type minType,
                                   const FGPositioned::Type maxType ):
  _mintype(minType),
  _maxtype(maxType)
{
}

//------------------------------------------------------------------------------
bool FGNavList::TypeFilter::fromTypeString(const std::string& type)
{
  FGPositioned::Type t;
  if(      type == "any"  ) t = FGPositioned::INVALID;
  else if( type == "fix"  ) t = FGPositioned::FIX;
  else if( type == "vor"  ) t = FGPositioned::VOR;
  else if( type == "ndb"  ) t = FGPositioned::NDB;
  else if( type == "ils"  ) t = FGPositioned::ILS;
  else if( type == "dme"  ) t = FGPositioned::DME;
  else if( type == "tacan") t = FGPositioned::TACAN;
  else                      return false;

  _mintype = _maxtype = t;

  return true;
}

/**
 * Filter returning Tacan stations. Checks for both pure TACAN stations
 * but also co-located VORTACs. This is done by searching for DMEs whose
 * name indicates they are a TACAN or VORTAC; not a great solution.
 */
class TacanFilter : public FGNavList::TypeFilter
{
public:
  TacanFilter() :
    TypeFilter(FGPositioned::DME, FGPositioned::TACAN)
  {
  }

  virtual bool pass(FGPositioned* pos) const
  {
    if (pos->type() == FGPositioned::TACAN) {
      return true;
    }

    assert(pos->type() == FGPositioned::DME);
    string::size_type loc1 = pos->name().find( "TACAN" );
    string::size_type loc2 = pos->name().find( "VORTAC" );
    return (loc1 != string::npos) || (loc2 != string::npos);
  }
};

FGNavList::TypeFilter* FGNavList::locFilter()
{
  static TypeFilter tf(FGPositioned::ILS, FGPositioned::LOC);
  return &tf;
}

FGNavList::TypeFilter* FGNavList::ndbFilter()
{
  static TypeFilter tf(FGPositioned::NDB);
  return &tf;
}

FGNavList::TypeFilter* FGNavList::navFilter()
{
  static TypeFilter tf(FGPositioned::VOR, FGPositioned::LOC);
  return &tf;
}

FGNavList::TypeFilter* FGNavList::tacanFilter()
{
  static TacanFilter tf;
  return &tf;
}

FGNavList::TypeFilter* FGNavList::mobileTacanFilter()
{
  static TypeFilter tf(FGPositioned::MOBILE_TACAN);
  return &tf;
}

FGNavRecordRef FGNavList::findByFreq( double freq,
                                      const SGGeod& position,
                                      TypeFilter* filter )
{
  flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
  int freqKhz = static_cast<int>(freq * 100 + 0.5);
  PositionedIDVec stations(cache->findNavaidsByFreq(freqKhz, position, filter));
  if (stations.empty()) {
    return NULL;
  }

// now walk the (sorted) results list to find a usable, in-range navaid
  SGVec3d acCart(SGVec3d::fromGeod(position));
  double min_dist
    = FG_NAV_MAX_RANGE*SG_NM_TO_METER*FG_NAV_MAX_RANGE*SG_NM_TO_METER;

  for (auto id : stations) {
    FGNavRecordRef station = FGPositioned::loadById<FGNavRecord>(id);
    if (filter && !filter->pass(station)) {
      continue;
    }

    double d2 = distSqr(station->cart(), acCart);
    if (d2 > min_dist) {
    // since results are sorted by proximity, as soon as we pass the
    // distance cutoff we're done - fall out and return NULL
      break;
    }

    if (navidUsable(station, position)) {
      return station;
    }
  }

// fell out of the loop, no usable match
  return NULL;
}

FGNavRecordRef FGNavList::findByFreq(double freq, TypeFilter* filter)
{
  flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
  int freqKhz = static_cast<int>(freq * 100 + 0.5);
  PositionedIDVec stations(cache->findNavaidsByFreq(freqKhz, filter));
  if (stations.empty()) {
    return NULL;
  }

  for (auto id : stations) {
    FGNavRecordRef station = FGPositioned::loadById<FGNavRecord>(id);
    if (filter->pass(station)) {
      return station;
    }
  }

  return NULL;
}

nav_list_type FGNavList::findAllByFreq( double freq, const SGGeod& position,
                                       TypeFilter* filter)
{
  nav_list_type stations;

  flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
    // note this frequency is passed in 'database units', which depend on the
    // type of navaid being requested
  int f = static_cast<int>(freq * 100 + 0.5);
  PositionedIDVec ids(cache->findNavaidsByFreq(f, position, filter));

  for (PositionedID id : ids) {
    FGNavRecordRef station = FGPositioned::loadById<FGNavRecord>(id);
    if (!filter->pass(station)) {
      continue;
    }

    stations.push_back(station);
  }

  return stations;
}

nav_list_type FGNavList::findByIdentAndFreq(const string& ident, const double freq,
                                            TypeFilter* filter)
{
  nav_list_type reply;
  int f = (int)(freq*100.0 + 0.5);

  FGPositionedList stations = FGPositioned::findAllWithIdent(ident, filter);
  for (auto ref : stations) {
    FGNavRecord* nav = static_cast<FGNavRecord*>(ref.ptr());
    if ( f <= 0.0 || nav->get_freq() == f) {
      reply.push_back( nav );
    }
  }

  return reply;
}

// Given an Ident and optional frequency and type ,
// return a list of matching stations sorted by distance to the given position
nav_list_type FGNavList::findByIdentAndFreq( const SGGeod & position,
        const std::string& ident, const double freq,
                                            TypeFilter* filter)
{
    nav_list_type reply = findByIdentAndFreq( ident, freq, filter );
    NavRecordDistanceSortPredicate sortPredicate( position );
    std::sort( reply.begin(), reply.end(), sortPredicate );

    return reply;
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

/*
 * ref: 070031b3c01f64a44433e8cce742373f4c76b7ac
 * The ground reply frequencies are used to keep channels 1-16 and 60-69 out of the VOR frequency range as those channels
 * don't have VORs associated with them.
 * - this method will therefore only be able to locate channels 17 to 59.
 */
FGTACANRecord *FGTACANList::findByFrequency(int frequency_kHz)
{
    //029Y    10925 (encoded) = 109.25 = 109250khz - so we divide the input by 10
    int tfreq = frequency_kHz / 10;
    for (tacan_ident_map_type::iterator it = ident_channels.begin(); it != ident_channels.end(); it++)
    {
        for (tacan_list_type::iterator lit = it->second.begin(); lit != it->second.end(); lit++)
            if ((*lit)->get_freq() == tfreq)
                return (*lit);
    }
    return nullptr;
}
// Given a TACAN Channel return the first matching frequency
FGTACANRecord *FGTACANList::findByChannel( const string& channel )
{
    const tacan_list_type& stations = ident_channels[channel];
    SG_LOG( SG_NAVAID, SG_DEBUG, "findByChannel " << channel<< " size " << stations.size() );

    if (!stations.empty()) {
        return stations[0];
    }
    return NULL;
}
