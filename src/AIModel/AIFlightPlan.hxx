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

#include <Airports/simple.hxx>
#include <Navaids/awynet.hxx>

#include "AIBase.hxx"

using std::vector;
using std::string;

class FGTaxiRoute;
class FGRunway;

class FGAIFlightPlan {

public:

  typedef struct {
   string name;
   double latitude;
   double longitude;
   double altitude;
   double speed;
   double crossat;
   bool finished;
   bool gear_down;
   bool flaps_down;
   bool on_ground;
    int routeIndex;  // For AI/ATC purposes;
   double time_sec;
   string time;

  } waypoint;

  FGAIFlightPlan(const string& filename);
  FGAIFlightPlan(const std::string& p,
		 double course,
		 time_t start,
		 FGAirport *dep,
		 FGAirport *arr,
		 bool firstLeg,
		 double radius,
                 double alt,
                 double lat,
                 double lon,
                 double speed,
		 const string& fltType,
		 const string& acType,
		 const string& airline);
   ~FGAIFlightPlan();

   waypoint* const getPreviousWaypoint( void ) const;
   waypoint* const getCurrentWaypoint( void ) const;
   waypoint* const getNextWaypoint( void ) const;
   void IncrementWaypoint( bool erase );

   double getDistanceToGo(double lat, double lon, waypoint* wp) const;
   int getLeg () const { return leg;};
   void setLeadDistance(double speed, double bearing, waypoint* current, waypoint* next);
   void setLeadDistance(double distance_ft);
   double getLeadDistance( void ) const {return lead_distance;}
   double getBearing(waypoint* previous, waypoint* next) const;
   double getBearing(double lat, double lon, waypoint* next) const;
  time_t getStartTime() const { return start_time; }

  void    create(FGAirport *dep, FGAirport *arr, int leg, double alt, double speed, double lat, double lon,
		 bool firstLeg, double radius, const string& fltType, const string& aircraftType, const string& airline);

  void setLeg(int val) { leg = val;}
  void setTime(time_t st) { start_time = st; }
  int getGate() const { return gateId; }
  double getLeadInAngle() const { return leadInAngle; }
  const string& getRunway() const;
  
  void setRepeat(bool r) { repeat = r; }
  bool getRepeat(void) const { return repeat; }
  void restart(void);
  int getNrOfWayPoints() { return waypoints.size(); }
  int getRouteIndex(int i); // returns the AI related index of this current routes. 
  FGTaxiRoute *getTaxiRoute() { return taxiRoute; }
  void deleteTaxiRoute();
  string getRunway() { return activeRunway; }
  bool isActive(time_t time) {return time >= this->getStartTime();}

private:
  FGRunway* rwy;
  typedef vector <waypoint*> wpt_vector_type;
  typedef wpt_vector_type::const_iterator wpt_vector_iterator;

  wpt_vector_type       waypoints;
  wpt_vector_iterator   wpt_iterator;

  bool repeat;
  double distance_to_go;
  double lead_distance;
  double leadInAngle;
  time_t start_time;
  int leg;
  int gateId, lastNodeVisited;
  string activeRunway;
  FGAirRoute airRoute;
  FGTaxiRoute *taxiRoute;

  void createPushBack(bool, FGAirport*, double, double, double, const string&, const string&, const string&);
  void createPushBackFallBack(bool, FGAirport*, double, double, double, const string&, const string&, const string&);
  void createTaxi(bool, int, FGAirport *, double, double, double, const string&, const string&, const string&);
  void createTakeOff(bool, FGAirport *, double, const string&);
  void createClimb(bool, FGAirport *, double, double, const string&);
  void createCruise(bool, FGAirport*, FGAirport*, double, double, double, double, const string&);
  void createDecent(FGAirport *, const string&);
  void createLanding(FGAirport *);
  void createParking(FGAirport *, double radius);
  void deleteWaypoints(); 
  void resetWaypoints();

  string getRunwayClassFromTrafficType(string fltType);

  //void createCruiseFallback(bool, FGAirport*, FGAirport*, double, double, double, double);
 void evaluateRoutePart(double deplat, double deplon, double arrlat, double arrlon);
};    

#endif  // _FG_AIFLIGHTPLAN_HXX
