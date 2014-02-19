// runways.cxx -- a simple class to manage airport runway info
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

#include <cstdio>              // sprintf()
#include <cstdlib>             // atoi()
#include <cassert>

#include <simgear/compiler.h>

#include <simgear/props/props.hxx>

#include <string>

#include "runways.hxx"

#include <Airports/airport.hxx>
#include <Navaids/procedure.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/NavDataCache.hxx>

using std::string;

FGRunway::FGRunway(PositionedID aGuid,
                   PositionedID aAirport, const string& aIdent,
                        const SGGeod& aGeod,
                        const double heading, const double length,
                        const double width,
                        const double displ_thresh,
                        const double stopway,
                        const int surface_code) :
  FGRunwayBase(aGuid, RUNWAY, aIdent, aGeod,
               heading, length, width, surface_code),
  _airport(aAirport),
  _reciprocal(0),
  _displ_thresh(displ_thresh),
  _stopway(stopway),
  _ils(0)
{
}

string FGRunway::reverseIdent(const string& aRunwayIdent)
{
  // Helipads don't have a seperate number per end
  if (aRunwayIdent.size() && (aRunwayIdent[0] == 'H' || aRunwayIdent[0] == 'h' || aRunwayIdent[0] == 'x')) {
    return aRunwayIdent;
  }

  std::string ident(aRunwayIdent);
    
  char buf[4];
  int rn = atoi(ident.substr(0,2).c_str());
  rn += 18;
  while(rn > 36) {
	  rn -= 36;
  }
  
  sprintf(buf, "%02i", rn);
  if(ident.size() == 3) {
    char suffix = toupper(ident[2]);
    if(suffix == 'L') {
      buf[2] = 'R';
    } else if (suffix == 'R') {
      buf[2] = 'L';
    } else {
      // something else, just copy
      buf[2] = suffix;
    }
    
    buf[3] = 0;
  }
  
  return buf;
}

double FGRunway::score(double aLengthWt, double aWidthWt, double aSurfaceWt, double aIlsWt) const
{
  int surface = 1;
  if (_surface_code == 12 || _surface_code == 5) // dry lakebed & gravel
    surface = 2;
  else if (_surface_code == 1 || _surface_code == 2) // asphalt & concrete
    surface = 3;

  int ils = (_ils != 0);
    
  return _length * aLengthWt + _width * aWidthWt + surface * aSurfaceWt + ils * aIlsWt + 1e-20;
}

SGGeod FGRunway::begin() const
{
  return pointOnCenterline(0.0);
}

SGGeod FGRunway::end() const
{
  return pointOnCenterline(lengthM());
}

SGGeod FGRunway::threshold() const
{
  return pointOnCenterline(_displ_thresh);
}

void FGRunway::setReciprocalRunway(PositionedID other)
{
  assert(_reciprocal==0);  
  _reciprocal = other;
}

FGAirport* FGRunway::airport() const
{
  return loadById<FGAirport>(_airport);
}

FGRunway* FGRunway::reciprocalRunway() const
{
  return loadById<FGRunway>(_reciprocal);
}

FGNavRecord* FGRunway::ILS() const
{
  if (_ils == 0) {
    return NULL;
  }
  
  return loadById<FGNavRecord>(_ils);
}

FGNavRecord* FGRunway::glideslope() const
{
  flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
  PositionedID gsId = cache->findNavaidForRunway(guid(), FGPositioned::GS);
  if (gsId == 0) {
    return NULL;
  }
  
  return loadById<FGNavRecord>(gsId);
}

flightgear::SIDList FGRunway::getSIDs() const
{
  FGAirport* apt = airport();
  flightgear::SIDList result;
  for (unsigned int i=0; i<apt->numSIDs(); ++i) {
    flightgear::SID* s = apt->getSIDByIndex(i);
    if (s->isForRunway(this)) {
      result.push_back(s);
    }
  } // of SIDs at the airport iteration
  
  return result;
}

flightgear::STARList FGRunway::getSTARs() const
{
  FGAirport* apt = airport();
  flightgear::STARList result;
  for (unsigned int i=0; i<apt->numSTARs(); ++i) {
    flightgear::STAR* s = apt->getSTARByIndex(i);
    if (s->isForRunway(this)) {
      result.push_back(s);
    }
  } // of STARs at the airport iteration
  
  return result;
}

flightgear::ApproachList
FGRunway::getApproaches(flightgear::ProcedureType type) const
{
  FGAirport* apt = airport();
  flightgear::ApproachList result;
  for (unsigned int i=0; i<apt->numApproaches(); ++i)
  {
    flightgear::Approach* s = apt->getApproachByIndex(i);
    if(    s->runway() == this
        && (type == flightgear::PROCEDURE_INVALID || type == s->type()) )
    {
      result.push_back(s);
    }
  } // of approaches at the airport iteration
  
  return result;
}

void FGRunway::updateThreshold(const SGGeod& newThreshold, double newHeading,
                     double newDisplacedThreshold,
                     double newStopway)
{
    modifyPosition(newThreshold);
    _heading = newHeading;
    _stopway = newStopway;
    _displ_thresh = newDisplacedThreshold;
}

FGHelipad::FGHelipad(PositionedID aGuid,
                        PositionedID aAirport, const string& aIdent,
                        const SGGeod& aGeod,
                        const double heading, const double length,
                        const double width,
                        const int surface_code) :
  FGRunwayBase(aGuid, RUNWAY, aIdent, aGeod,
               heading, length, width, surface_code)
{
}
