/**
 * routePath.hxx - convert a route to straight line segments, for graphical
 * output or display.
 */
 
// Written by James Turner, started 2010.
//
// Copyright (C) 2010  Curtis L. Olson
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

#ifndef FG_ROUTE_PATH_HXX
#define FG_ROUTE_PATH_HXX

#include <Navaids/route.hxx>

namespace flightgear
{
  class Hold;
  class FlightPlan;
}

typedef std::vector<SGGeod> SGGeodVec;

class RoutePath
{
public:
  RoutePath(const flightgear::FlightPlan* fp);
  
  SGGeodVec pathForIndex(int index) const;
  
  SGGeod positionForIndex(int index) const;

  SGGeod positionForDistanceFrom(int index, double distanceM) const;

  double trackForIndex(int index) const;
  
  double distanceForIndex(int index) const;
  
  double distanceBetweenIndices(int from, int to) const;

private:
  class RoutePathPrivate;
  
  void commonInit();
  
  double computeDistanceForIndex(int index) const;
    
  SGGeodVec pathForHold(flightgear::Hold* hold) const;
  
  
  void interpolateGreatCircle(const SGGeod& aFrom, const SGGeod& aTo, SGGeodVec& r) const;
  
  
  RoutePathPrivate* d;
};

#endif
