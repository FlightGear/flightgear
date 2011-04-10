// // FGAIFlightPlan - class for loading and storing  AI flight plans
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


class FGTaxiRoute;
class FGRunway;
class FGAIAircraft;
class FGAirport;
class SGGeod;

class FGAIFlightPlan {

public:

  typedef struct {
   std::string name;
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
   double trackLength; // distance from previous waypoint (for AI purposes);
   std::string time;

  } waypoint;
  FGAIFlightPlan();
  FGAIFlightPlan(const std::string& filename);
  FGAIFlightPlan(FGAIAircraft *,
                 const std::string& p,
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
		 const std::string& fltType,
		 const std::string& acType,
		 const std::string& airline);
   ~FGAIFlightPlan();

   waypoint* const getPreviousWaypoint( void ) const;
   waypoint* const getCurrentWaypoint( void ) const;
   waypoint* const getNextWaypoint( void ) const;
   void IncrementWaypoint( bool erase );
   void DecrementWaypoint( bool erase );

   double getDistanceToGo(double lat, double lon, waypoint* wp) const;
   int getLeg () const { return leg;};
   void setLeadDistance(double speed, double bearing, waypoint* current, waypoint* next);
   void setLeadDistance(double distance_ft);
   double getLeadDistance( void ) const {return lead_distance;}
   double getBearing(waypoint* previous, waypoint* next) const;
   double getBearing(double lat, double lon, waypoint* next) const;
   double checkTrackLength(std::string wptName);
  time_t getStartTime() const { return start_time; }
   time_t getArrivalTime() const { return arrivalTime; }

  void    create(FGAIAircraft *, FGAirport *dep, FGAirport *arr, int leg, double alt, double speed, double lat, double lon,
		 bool firstLeg, double radius, const std::string& fltType, const std::string& aircraftType, const std::string& airline, double distance);
  void createPushBack(FGAIAircraft *, bool, FGAirport*, double, double, double, const std::string&, const std::string&, const std::string&);
  void createTakeOff(FGAIAircraft *, bool, FGAirport *, double, const std::string&);

  void setLeg(int val) { leg = val;}
  void setTime(time_t st) { start_time = st; }
  int getGate() const { return gateId; }
  void setGate(int id) { gateId = id; };

  double getLeadInAngle() const { return leadInAngle; }
  const std::string& getRunway() const;
  
  void setRepeat(bool r) { repeat = r; }
  bool getRepeat(void) const { return repeat; }
  void restart(void);
  int getNrOfWayPoints() { return waypoints.size(); }
  int getRouteIndex(int i); // returns the AI related index of this current routes. 
  FGTaxiRoute *getTaxiRoute() { return taxiRoute; }
  void deleteTaxiRoute();
  std::string getRunway() { return activeRunway; }
  bool isActive(time_t time) {return time >= this->getStartTime();}

  void setRunway(std::string rwy) { activeRunway = rwy; };
  std::string getRunwayClassFromTrafficType(std::string fltType);

  void addWaypoint(waypoint* wpt) { waypoints.push_back(wpt); };

  void setName(std::string n) { name = n; };
  std::string getName() { return name; };

  void setSID(FGAIFlightPlan* fp) { sid = fp;};
  FGAIFlightPlan* getSID() { return sid; };

private:
  FGRunway* rwy;
  FGAIFlightPlan *sid;
  typedef std::vector <waypoint*> wpt_vector_type;
  typedef wpt_vector_type::const_iterator wpt_vector_iterator;


  wpt_vector_type       waypoints;
  wpt_vector_iterator   wpt_iterator;

  bool repeat;
  double distance_to_go;
  double lead_distance;
  double leadInAngle;
  time_t start_time;
  time_t arrivalTime;       // For AI/ATC purposes.
  int leg;
  int gateId, lastNodeVisited;
  std::string activeRunway;
  FGTaxiRoute *taxiRoute;
  std::string name;

  void createPushBackFallBack(FGAIAircraft *, bool, FGAirport*, double, double, double, const std::string&, const std::string&, const std::string&);
  void createClimb(FGAIAircraft *, bool, FGAirport *, double, double, const std::string&);
  void createCruise(FGAIAircraft *, bool, FGAirport*, FGAirport*, double, double, double, double, const std::string&);
  void createDescent(FGAIAircraft *, FGAirport *,  double latitude, double longitude, double speed, double alt,const std::string&, double distance);
  void createLanding(FGAIAircraft *, FGAirport *, const std::string&);
  void createParking(FGAIAircraft *, FGAirport *, double radius);
  void deleteWaypoints(); 
  void resetWaypoints();

  void createLandingTaxi(FGAIAircraft *, FGAirport *apt, double radius, const std::string& fltType, const std::string& acType, const std::string& airline);
  void createDefaultLandingTaxi(FGAIAircraft *, FGAirport* aAirport);
  void createDefaultTakeoffTaxi(FGAIAircraft *, FGAirport* aAirport, FGRunway* aRunway);
  void createTakeoffTaxi(FGAIAircraft *, bool firstFlight, FGAirport *apt, double radius, const std::string& fltType, const std::string& acType, const std::string& airline);

  double getTurnRadius(double, bool);
        
  waypoint* createOnGround(FGAIAircraft *, const std::string& aName, const SGGeod& aPos, double aElev, double aSpeed);
  waypoint* createInAir(FGAIAircraft *, const std::string& aName, const SGGeod& aPos, double aElev, double aSpeed);
  waypoint* cloneWithPos(FGAIAircraft *, waypoint* aWpt, const std::string& aName, const SGGeod& aPos);
  waypoint* clone(waypoint* aWpt);
    

  //void createCruiseFallback(bool, FGAirport*, FGAirport*, double, double, double, double);
 void evaluateRoutePart(double deplat, double deplon, double arrlat, double arrlon);
 public:
  wpt_vector_iterator getFirstWayPoint() { return waypoints.begin(); };
  wpt_vector_iterator getLastWayPoint()  { return waypoints.end(); };

};    

#endif  // _FG_AIFLIGHTPLAN_HXX
