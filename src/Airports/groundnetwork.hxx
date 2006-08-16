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

#include STL_STRING
#include <vector>

SG_USING_STD(string);
SG_USING_STD(vector);

#include "parking.hxx"

class FGTaxiSegment; // forward reference

typedef vector<FGTaxiSegment>  FGTaxiSegmentVector;
typedef vector<FGTaxiSegment*> FGTaxiSegmentPointerVector;
typedef vector<FGTaxiSegment>::iterator FGTaxiSegmentVectorIterator;
typedef vector<FGTaxiSegment*>::iterator FGTaxiSegmentPointerVectorIterator;

/**************************************************************************************
 * class FGTaxiNode
 *************************************************************************************/
class FGTaxiNode 
{
private:
  double lat;
  double lon;
  int index;
  FGTaxiSegmentPointerVector next; // a vector to all the segments leaving from this node
  
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
  FGTaxiSegmentPointerVectorIterator getBeginRoute() { return next.begin(); };
  FGTaxiSegmentPointerVectorIterator getEndRoute()   { return next.end();   }; 
};

typedef vector<FGTaxiNode> FGTaxiNodeVector;
typedef vector<FGTaxiNode>::iterator FGTaxiNodeVectorIterator;

/***************************************************************************************
 * class FGTaxiSegment
 **************************************************************************************/
class FGTaxiSegment
{
private:
  int startNode;
  int endNode;
  double length;
  FGTaxiNode *start;
  FGTaxiNode *end;
  int index;

public:
  FGTaxiSegment();
  FGTaxiSegment(FGTaxiNode *, FGTaxiNode *, int);

  void setIndex        (int val) { index     = val; };
  void setStartNodeRef (int val) { startNode = val; };
  void setEndNodeRef   (int val) { endNode   = val; };

  void setStart(FGTaxiNodeVector *nodes);
  void setEnd  (FGTaxiNodeVector *nodes);
  void setTrackDistance();

  FGTaxiNode * getEnd() { return end;};
  double getLength() { return length; };
  int getIndex() { return index; };

 FGTaxiSegment *getAddress() { return this;};

  
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
  intVecIterator currNode;
  intVecIterator currRoute;

public:
  FGTaxiRoute() { distance = 0; currNode = nodes.begin(); currRoute = routes.begin();};
  FGTaxiRoute(intVec nds, intVec rts, double dist) { 
    nodes = nds; 
    routes = rts;
    distance = dist; 
    currNode = nodes.begin();
  };
  bool operator< (const FGTaxiRoute &other) const {return distance < other.distance; };
  bool empty () { return nodes.begin() == nodes.end(); };
  bool next(int *nde); 
  bool next(int *nde, int *rte);
  
  void first() { currNode = nodes.begin(); currRoute = routes.begin(); };
  int size() { return nodes.size(); };
};

typedef vector<FGTaxiRoute> TaxiRouteVector;
typedef vector<FGTaxiRoute>::iterator TaxiRouteVectorIterator;

/**************************************************************************************
 * class FGGroundNetWork
 *************************************************************************************/
class FGGroundNetwork
{
private:
  bool hasNetwork;
  FGTaxiNodeVector    nodes;
  FGTaxiSegmentVector segments;
  //intVec route;
  intVec nodesStack;
  intVec routesStack;
  TaxiRouteVector routes;
  
  bool foundRoute;
  double totalDistance, maxDistance;

  void printRoutingError(string);
  
public:
  FGGroundNetwork();

  void addNode   (const FGTaxiNode& node);
  void addNodes  (FGParkingVec *parkings);
  void addSegment(const FGTaxiSegment& seg); 

  void init();
  bool exists() { return hasNetwork; };
  int findNearestNode(double lat, double lon);
  FGTaxiNode *findNode(int idx);
  FGTaxiSegment *findSegment(int idx);
  FGTaxiRoute findShortestRoute(int start, int end);
  void trace(FGTaxiNode *, int, int, double dist);
 
};

#endif
