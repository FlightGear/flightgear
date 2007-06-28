// groundnet.hxx - A number of classes to handle taxiway
// assignments by the AI code
//
// Written by Durk Talsma, started June 2005.
//
// Copyright (C) 2004 Durk Talsma.
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

#ifndef _GROUNDNETWORK_HXX_
#define _GROUNDNETWORK_HXX_

#include <simgear/compiler.h>
#include <simgear/route/waypoint.hxx>


#include STL_STRING
#include <vector>

SG_USING_STD(string);
SG_USING_STD(vector);

#include "parking.hxx"
#include "trafficcontrol.hxx"



class FGTaxiSegment; // forward reference
class FGAIFlightPlan; // forward reference
class FGAirport;      // forward reference

typedef vector<FGTaxiSegment*>  FGTaxiSegmentVector;
typedef vector<FGTaxiSegment*>::iterator FGTaxiSegmentVectorIterator;

//typedef vector<FGTaxiSegment*> FGTaxiSegmentPointerVector;
//typedef vector<FGTaxiSegment*>::iterator FGTaxiSegmentPointerVectorIterator;

/**************************************************************************************
 * class FGTaxiNode
 *************************************************************************************/
class FGTaxiNode 
{
private:
  double lat;
  double lon;
  int index;
  FGTaxiSegmentVector next; // a vector of pointers to all the segments leaving from this node
  
public:
  FGTaxiNode();
  FGTaxiNode(double, double, int);

  void setIndex(int idx)                  { index = idx;};
  void setLatitude (double val)           { lat = val;};
  void setLongitude(double val)           { lon = val;};
  void setLatitude (const string& val)           { lat = processPosition(val);  };
  void setLongitude(const string& val)           { lon = processPosition(val);  };
  void addSegment(FGTaxiSegment *segment) { next.push_back(segment); };
  
  double getLatitude() { return lat;};
  double getLongitude(){ return lon;};

  int getIndex() { return index; };
  FGTaxiNode *getAddress() { return this;};
  FGTaxiSegmentVectorIterator getBeginRoute() { return next.begin(); };
  FGTaxiSegmentVectorIterator getEndRoute()   { return next.end();   }; 
  bool operator<(const FGTaxiNode &other) const { return index < other.index; };

  void sortEndSegments(bool);

  // used in way finding
  double pathscore;
  FGTaxiNode* previousnode;
  FGTaxiSegment* previousseg;
  
};

typedef vector<FGTaxiNode*> FGTaxiNodeVector;
typedef vector<FGTaxiNode*>::iterator FGTaxiNodeVectorIterator;

/***************************************************************************************
 * class FGTaxiSegment
 **************************************************************************************/
class FGTaxiSegment
{
private:
  int startNode;
  int endNode;
  double length;
  double course;
  double headingDiff;
  bool isActive;
  FGTaxiNode *start;
  FGTaxiNode *end;
  int index;
  FGTaxiSegment *oppositeDirection;

 

public:
  FGTaxiSegment();
  //FGTaxiSegment(FGTaxiNode *, FGTaxiNode *, int);

  void setIndex        (int val) { index     = val; };
  void setStartNodeRef (int val) { startNode = val; };
  void setEndNodeRef   (int val) { endNode   = val; };

  void setOpposite(FGTaxiSegment *opp) { oppositeDirection = opp; };

  void setStart(FGTaxiNodeVector *nodes);
  void setEnd  (FGTaxiNodeVector *nodes);
  void setTrackDistance();

  FGTaxiNode * getEnd() { return end;};
  FGTaxiNode * getStart() { return start; };
  double getLength() { return length; };
  int getIndex() { return index; };

 FGTaxiSegment *getAddress() { return this;};

  bool operator<(const FGTaxiSegment &other) const { return index < other.index; };
  bool hasSmallerHeadingDiff (const FGTaxiSegment &other) const { return headingDiff < other.headingDiff; };
  FGTaxiSegment *opposite() { return oppositeDirection; };
  void setCourseDiff(double crse);


  
};




typedef vector<int> intVec;
typedef vector<int>::iterator intVecIterator;



/***************************************************************************************
 * class FGTaxiRoute
 **************************************************************************************/
class FGTaxiRoute
{
private:
  intVec nodes;
  intVec routes;
  double distance;
  int depth;
  intVecIterator currNode;
  intVecIterator currRoute;

public:
  FGTaxiRoute() { distance = 0; currNode = nodes.begin(); currRoute = routes.begin();};
  FGTaxiRoute(intVec nds, intVec rts, double dist, int dpth) { 
    nodes = nds; 
    routes = rts;
    distance = dist; 
    currNode = nodes.begin();
    depth = dpth;
  };
  bool operator< (const FGTaxiRoute &other) const {return distance < other.distance; };
  bool empty () { return nodes.begin() == nodes.end(); };
  bool next(int *nde); 
  bool next(int *nde, int *rte);
  void rewind(int legNr);
  
  void first() { currNode = nodes.begin(); currRoute = routes.begin(); };
  int size() { return nodes.size(); };
  int getDepth() { return depth; };
};

typedef vector<FGTaxiRoute> TaxiRouteVector;
typedef vector<FGTaxiRoute>::iterator TaxiRouteVectorIterator;

bool sortByHeadingDiff(FGTaxiSegment *a, FGTaxiSegment *b);
bool sortByLength     (FGTaxiSegment *a, FGTaxiSegment *b);


/**************************************************************************************
 * class FGGroundNetWork
 *************************************************************************************/
class FGGroundNetwork : public FGATCController
{
private:
  bool hasNetwork;
  //int maxDepth;
  int count;
  FGTaxiNodeVector    nodes;
  FGTaxiSegmentVector segments;
  //intVec route;
  intVec nodesStack;
  intVec routesStack;
  TaxiRouteVector routes;
  TrafficVector activeTraffic;
  TrafficVectorIterator currTraffic;
  SGWayPoint destination;
  
  bool foundRoute;
  double totalDistance, maxDistance;
  FGTowerController *towerController;
  FGAirport *parent;
  

  void printRoutingError(string);

  void checkSpeedAdjustment(int id, double lat, double lon, 
			    double heading, double speed, double alt);
  void checkHoldPosition(int id, double lat, double lon, 
			 double heading, double speed, double alt);
  
public:
  FGGroundNetwork();
  ~FGGroundNetwork();

  void addNode   (const FGTaxiNode& node);
  void addNodes  (FGParkingVec *parkings);
  void addSegment(const FGTaxiSegment& seg); 

  void init();
  bool exists() { return hasNetwork; };
  void setTowerController(FGTowerController *twrCtrlr) { towerController = twrCtrlr; };
  int findNearestNode(double lat, double lon);
  FGTaxiNode *findNode(int idx);
  FGTaxiSegment *findSegment(int idx);
  FGTaxiRoute findShortestRoute(int start, int end);
  //void trace(FGTaxiNode *, int, int, double dist);

  void setParent(FGAirport *par) { parent = par; };

  virtual void announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute, 
				double lat, double lon, double hdg, double spd, double alt, 
				double radius, int leg, string callsign);
  virtual void signOff(int id);
  virtual void update(int id, double lat, double lon, double heading, double speed, double alt, double dt);
  virtual bool hasInstruction(int id);
  virtual FGATCInstruction getInstruction(int id);

  bool checkForCircularWaits(int id);
};


#endif
