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
#include <Airports/runways.hxx>

#include "AIBase.hxx"

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
   bool finished;
   bool gear_down;
   bool flaps_down;
   bool on_ground;
  } waypoint;

   FGAIFlightPlan(string filename);
  FGAIFlightPlan(FGAIModelEntity *entity,
		 double course,
		 time_t start,
		 FGAirport *dep,
		 FGAirport *arr,
		 bool firstLeg,
		 double radius,
		 string fltType,
		 string acType,
		 string airline);
   ~FGAIFlightPlan();

   waypoint* getPreviousWaypoint( void );
   waypoint* getCurrentWaypoint( void );
   waypoint* getNextWaypoint( void );
   void IncrementWaypoint( bool erase );

   double getDistanceToGo(double lat, double lon, waypoint* wp);
   int getLeg () { return leg;};
   void setLeadDistance(double speed, double bearing, waypoint* current, waypoint* next);
   void setLeadDistance(double distance_ft);
   double getLeadDistance( void ) const {return lead_distance;}
   double getBearing(waypoint* previous, waypoint* next);
   double getBearing(double lat, double lon, waypoint* next);
  time_t getStartTime() { return start_time; }; 

  void    create(FGAirport *dep, FGAirport *arr, int leg, double alt, double speed, double lat, double lon,
		 bool firstLeg, double radius, string fltType, string aircraftType, string airline);

  void setLeg(int val) { leg = val;};
  void setTime(time_t st) { start_time = st; };
  int getGate() { return gateId; };
  double getLeadInAngle() { return leadInAngle; };
  string getRunway() { return rwy._rwy_no; };
  string getRunwayId() { return rwy._id; };
private:
  FGRunway rwy;
    typedef vector <waypoint*> wpt_vector_type;
    typedef wpt_vector_type::iterator wpt_vector_iterator;

    wpt_vector_type       waypoints;
    wpt_vector_iterator   wpt_iterator;

    double distance_to_go;
    double lead_distance;
  double leadInAngle;
    time_t start_time;
  int leg;
  int gateId;

  void createPushBack(bool, FGAirport*, double, double, double, string, string, string);
  void createTaxi(bool, int, FGAirport *, double, string, string, string);
  void createTakeOff(bool, FGAirport *, double);
  void createClimb(bool, FGAirport *, double, double);
  void createCruise(bool, FGAirport*, FGAirport*, double, double, double, double);
  void createDecent(FGAirport *);
  void createLanding(FGAirport *);
  void createParking(FGAirport *);
  void deleteWaypoints(); 
  void resetWaypoints();
};    



#endif  // _FG_AIFLIGHTPLAN_HXX

