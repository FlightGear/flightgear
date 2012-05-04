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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.

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

class FGAIWaypoint {
private:
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
   double trackLength; // distance from previous FGAIWaypoint (for AI purposes);
   std::string time;

public:
    FGAIWaypoint();
    ~FGAIWaypoint() {};
    void setName        (std::string nam) { name        = nam; };
    void setLatitude    (double lat) { latitude    = lat; };
    void setLongitude   (double lon) { longitude   = lon; };
    void setAltitude    (double alt) { altitude    = alt; };
    void setSpeed       (double spd) { speed       = spd; };
    void setCrossat     (double val) { crossat     = val; };
    void setFinished    (bool   fin) { finished    = fin; };
    void setGear_down   (bool   grd) { gear_down   = grd; };
    void setFlaps_down  (bool   fld) { flaps_down  = fld; };
    void setOn_ground   (bool   grn) { on_ground   = grn; };
    void setRouteIndex  (int    rte) { routeIndex  = rte; };
    void setTime_sec    (double ts ) { time_sec    = ts;  };
    void setTrackLength (double tl ) { trackLength = tl;  };
    void setTime        (std::string tme) { time        = tme; };

    bool contains(std::string name);

    std::string getName  () { return name;        };
    double getLatitude   () { return latitude;    };
    double getLongitude  () { return longitude;   };
    double getAltitude   () { return altitude;    };
    double getSpeed      () { return speed;       };

    double getCrossat    () { return crossat;     };
    bool   getGear_down  () { return gear_down;   };
    bool   getFlaps_down () { return flaps_down;  };
    bool   getOn_ground  () { return on_ground;   };
    int    getRouteIndex () { return routeIndex;  };
    bool   isFinished    () { return finished;    };
    double getTime_sec   () { return time_sec;    };
    double getTrackLength() { return trackLength; };
    std::string getTime  () { return time;        };

  };


class FGAIFlightPlan {

public:

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

   FGAIWaypoint* const getPreviousWaypoint( void ) const;
   FGAIWaypoint* const getCurrentWaypoint( void ) const;
   FGAIWaypoint* const getNextWaypoint( void ) const;
   void IncrementWaypoint( bool erase );
   void DecrementWaypoint( bool erase );

   double getDistanceToGo(double lat, double lon, FGAIWaypoint* wp) const;
   int getLeg () const { return leg;};
   
   void setLeadDistance(double speed, double bearing, FGAIWaypoint* current, FGAIWaypoint* next);
   void setLeadDistance(double distance_ft);
   double getLeadDistance( void ) const {return lead_distance;}
   double getBearing(FGAIWaypoint* previous, FGAIWaypoint* next) const;
   double getBearing(double lat, double lon, FGAIWaypoint* next) const;
   double checkTrackLength(std::string wptName);
  time_t getStartTime() const { return start_time; }
   time_t getArrivalTime() const { return arrivalTime; }

  bool    create(FGAIAircraft *, FGAirport *dep, FGAirport *arr, int leg, double alt, double speed, double lat, double lon,
		 bool firstLeg, double radius, const std::string& fltType, const std::string& aircraftType, const std::string& airline, double distance);
  bool createPushBack(FGAIAircraft *, bool, FGAirport*, double, double, double, const std::string&, const std::string&, const std::string&);
  bool createTakeOff(FGAIAircraft *, bool, FGAirport *, double, const std::string&);

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

  void incrementLeg() { leg++;};

  void setRunway(std::string rwy) { activeRunway = rwy; };
  std::string getRunwayClassFromTrafficType(std::string fltType);

  void addWaypoint(FGAIWaypoint* wpt) { waypoints.push_back(wpt); };

  void setName(std::string n) { name = n; };
  std::string getName() { return name; };

  void setSID(FGAIFlightPlan* fp) { sid = fp;};
  FGAIFlightPlan* getSID() { return sid; };
  FGAIWaypoint *getWayPoint(int i) { return waypoints[i]; };
  FGAIWaypoint *getLastWaypoint() { return waypoints.back(); };
  
  void shortenToFirst(unsigned int number, std::string name);

private:
  FGAIFlightPlan *sid;
  typedef std::vector <FGAIWaypoint*> wpt_vector_type;
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
  bool isValid;

  void createPushBackFallBack(FGAIAircraft *, bool, FGAirport*, double, double, double, const std::string&, const std::string&, const std::string&);
  bool createClimb(FGAIAircraft *, bool, FGAirport *, double, double, const std::string&);
  bool createCruise(FGAIAircraft *, bool, FGAirport*, FGAirport*, double, double, double, double, const std::string&);
  bool createDescent(FGAIAircraft *, FGAirport *,  double latitude, double longitude, double speed, double alt,const std::string&, double distance);
  bool createLanding(FGAIAircraft *, FGAirport *, const std::string&);
  bool createParking(FGAIAircraft *, FGAirport *, double radius);
  void deleteWaypoints(); 
  void resetWaypoints();
  void eraseLastWaypoint();
  void pushBackWaypoint(FGAIWaypoint *wpt);

  bool createLandingTaxi(FGAIAircraft *, FGAirport *apt, double radius, const std::string& fltType, const std::string& acType, const std::string& airline);
  void createDefaultLandingTaxi(FGAIAircraft *, FGAirport* aAirport);
  void createDefaultTakeoffTaxi(FGAIAircraft *, FGAirport* aAirport, FGRunway* aRunway);
  bool createTakeoffTaxi(FGAIAircraft *, bool firstFlight, FGAirport *apt, double radius, const std::string& fltType, const std::string& acType, const std::string& airline);

  double getTurnRadius(double, bool);
        
  FGAIWaypoint* createOnGround(FGAIAircraft *, const std::string& aName, const SGGeod& aPos, double aElev, double aSpeed);
  FGAIWaypoint* createInAir(FGAIAircraft *, const std::string& aName, const SGGeod& aPos, double aElev, double aSpeed);
  FGAIWaypoint* cloneWithPos(FGAIAircraft *, FGAIWaypoint* aWpt, const std::string& aName, const SGGeod& aPos);
  FGAIWaypoint* clone(FGAIWaypoint* aWpt);
    

  //void createCruiseFallback(bool, FGAirport*, FGAirport*, double, double, double, double);
 void evaluateRoutePart(double deplat, double deplon, double arrlat, double arrlon);
 public:
  wpt_vector_iterator getFirstWayPoint() { return waypoints.begin(); };
  wpt_vector_iterator getLastWayPoint()  { return waypoints.end(); };
    bool isValidPlan() { return isValid; };
};

#endif  // _FG_AIFLIGHTPLAN_HXX
