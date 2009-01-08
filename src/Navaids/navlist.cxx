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

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/sg_inlines.h>

#include "navlist.hxx"

using std::string;

// FGNavList ------------------------------------------------------------------

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

class VORNDBFilter : public FGPositioned::Filter
{
public:
  virtual FGPositioned::Type minType() const {
    return FGPositioned::VOR;
  }

  virtual FGPositioned::Type maxType()  const {
    return FGPositioned::NDB;
  }
};

// Given an Ident and optional freqency, return the first matching
// station.
FGNavRecord *FGNavList::findByIdentAndFreq(const string& ident, const double freq )
{
  FGPositionedRef cur;
  VORNDBFilter filter;
  cur = FGPositioned::findNextWithPartialId(cur, ident, &filter);
  
  if (freq <= 0.0) {
    return static_cast<FGNavRecord*>(cur.ptr()); // might be null
  }
  
  int f = (int)(freq*100.0 + 0.5);
  while (cur) {
    FGNavRecord* nav = static_cast<FGNavRecord*>(cur.ptr());
    if (nav->get_freq() == f) {
      return nav;
    }
    
    cur = FGPositioned::findNextWithPartialId(cur, ident, &filter);
  }

  return NULL;
}

// LOC, ILS, GS, and DME antenna's could potentially be
// installed at the opposite end of the runway.  So it's not
// enough to simply find the closest antenna with the right
// frequency.  We need the closest antenna with the right
// frequency that is most oriented towards us.  (We penalize
// stations that are facing away from us by adding 5000 meters
// which is further than matching stations would ever be
// placed from each other.  (Do the expensive check only for
// directional atennas and only when there is a chance it is
// the closest station.)

static bool penaltyForNav(FGNavRecord* aNav, const SGGeod &aGeod)
{
  switch (aNav->type()) {
  case FGPositioned::ILS:
  case FGPositioned::LOC:
  case FGPositioned::GS:
// FIXME
//  case FGPositioned::DME: we can't get the heading for a DME transmitter, oops
    break;
  default:
    return false;
  }
  
  double hdg_deg = 0.0;
  if (aNav->type() == FGPositioned::GS) {
    int tmp = (int)(aNav->get_multiuse() / 1000.0);
    hdg_deg = aNav->get_multiuse() - (tmp * 1000);
  } else {    
    hdg_deg = aNav->get_multiuse();
  }
  
  double az1, az2, s;
  SGGeodesy::inverse(aGeod, aNav->geod(), az1, az2, s);
  az1 = az1 - hdg_deg;
  SG_NORMALIZE_RANGE(az1, -180.0, 180.0);
  return fabs(az1) > 90.0;
}

// Given a point and a list of stations, return the closest one to the
// specified point.
FGNavRecord *FGNavList::findNavFromList( const SGGeod &aircraft,
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
        // cout << "testing " << current->get_ident() << endl;
        d2 = distSqr(station->cart(), aircraftCart);
        if ( d2 < min_dist && penaltyForNav(station, aircraft))
        {
          double dist = sqrt(d2);
          d2 = (dist + 5000) * (dist + 5000);
        }
        
        if ( d2 < min_dist ) {
            min_dist = d2;
            nav = station;
        }
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


