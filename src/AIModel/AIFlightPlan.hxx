// FGAIFlightPlan - class for loading and storing  AI flight plans
// Written by David Culp, started May 2004
// - davidculp2@comcast.net
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

#ifndef _FG_AIFLIGHTPLAN_HXX
#define _FG_AIFLIGHTPLAN_HXX

#include <simgear/compiler.h>
#include <vector>
#include <string>
SG_USING_STD(vector);
SG_USING_STD(string);


class FGAIFlightPlan {

public:

  typedef struct {
   string name;
   double latitude;
   double longitude;
   double altitude;
   double speed;
   double crossat;
   bool gear_down;
   bool flaps_down;
  } waypoint;

   FGAIFlightPlan(string filename);
   ~FGAIFlightPlan();

   waypoint* getPreviousWaypoint( void );
   waypoint* getCurrentWaypoint( void );
   waypoint* getNextWaypoint( void );
   void IncrementWaypoint( void );

   double getDistanceToGo(double lat, double lon, waypoint* wp);
   void setLeadDistance(double speed, double bearing, waypoint* current, waypoint* next);
   void setLeadDistance(double distance_ft);
   double getLeadDistance( void ) const {return lead_distance;}
   double getBearing(waypoint* previous, waypoint* next);
   double getBearing(double lat, double lon, waypoint* next);

private:

    typedef vector <waypoint*> wpt_vector_type;
    typedef wpt_vector_type::iterator wpt_vector_iterator;

    wpt_vector_type       waypoints;
    wpt_vector_iterator   wpt_iterator;

    double distance_to_go;
    double lead_distance;

};    



#endif  // _FG_AIFLIGHTPLAN_HXX

