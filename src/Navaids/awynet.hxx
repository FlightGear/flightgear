// airwaynet.hxx - A number of classes to handle taxiway
// assignments by the AI code
//
// Written by Durk Talsma. Based upon the ground netword code, started June 2005.
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#ifndef _AIRWAYNETWORK_HXX_
#define _AIRWAYNETWORK_HXX_

#include STL_STRING
#include <fstream>
#include <set>
#include <map>
#include <vector>

SG_USING_STD(string);
SG_USING_STD(map);
SG_USING_STD(set);
SG_USING_STD(vector);
SG_USING_STD(fstream);

#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>


//#include "parking.hxx"

class FGAirway; // forward reference

typedef vector<FGAirway>  FGAirwayVector;
typedef vector<FGAirway *> FGAirwayPointerVector;
typedef vector<FGAirway>::iterator FGAirwayVectorIterator;
typedef vector<FGAirway*>::iterator FGAirwayPointerVectorIterator;

/**************************************************************************************
 * class FGNode
 *************************************************************************************/
class FGNode
{
private:
  string ident;
  double lat;
  double lon;
  int index;
  FGAirwayPointerVector next; // a vector to all the segments leaving from this node

public:
  FGNode();
  FGNode(double lt, double ln, int idx, string id) { lat = lt; lon = ln; index = idx; ident = id;};

  void setIndex(int idx)                  { index = idx;};
  void setLatitude (double val)           { lat = val;};
  void setLongitude(double val)           { lon = val;};
  //void setLatitude (const string& val)           { lat = processPosition(val);  };
  //void setLongitude(const string& val)           { lon = processPosition(val);  };
  void addAirway(FGAirway *segment) { next.push_back(segment); };

  double getLatitude() { return lat;};
  double getLongitude(){ return lon;};

  int getIndex() { return index; };
  string getIdent() { return ident; };
  FGNode *getAddress() { return this;};
  FGAirwayPointerVectorIterator getBeginRoute() { return next.begin(); };
  FGAirwayPointerVectorIterator getEndRoute()   { return next.end();   };

  bool matches(string ident, double lat, double lon);
};

typedef vector<FGNode *> FGNodeVector;
typedef vector<FGNode *>::iterator FGNodeVectorIterator;


typedef map < string, FGNode *> node_map;
typedef node_map::iterator node_map_iterator;
typedef node_map::const_iterator const_node_map_iterator;


/***************************************************************************************
 * class FGAirway
 **************************************************************************************/
class FGAirway
{
private:
  string startNode;
  string endNode;
  double length;
  FGNode *start;
  FGNode *end;
  int index;
  int type; // 1=low altitude; 2=high altitude airway
  int base; // base altitude
  int top;  // top altitude
  string name;

public:
  FGAirway();
  FGAirway(FGNode *, FGNode *, int);

  void setIndex        (int val) { index     = val; };
  void setStartNodeRef (string val) { startNode = val; };
  void setEndNodeRef   (string val) { endNode   = val; };

  void setStart(node_map *nodes);
  void setEnd  (node_map *nodes);
  void setType (int tp) { type = tp;};
  void setBase (int val) { base = val;};
  void setTop  (int val) { top  = val;};
  void setName (string val) { name = val;};

  void setTrackDistance();

  FGNode * getEnd() { return end;};
  double getLength() { if (length == 0) setTrackDistance(); return length; };
  int getIndex() { return index; };
  string getName() { return name; };


};


typedef vector<int> intVec;
typedef vector<int>::iterator intVecIterator;
typedef vector<int>::const_iterator constIntVecIterator;

class FGAirRoute
{
private:
  intVec nodes;
  double distance;
  intVecIterator currNode;

public:
  FGAirRoute() { distance = 0; currNode = nodes.begin(); };
  FGAirRoute(intVec nds, double dist) { nodes = nds; distance = dist; currNode = nodes.begin();};
  bool operator< (const FGAirRoute &other) const {return distance < other.distance; };
  bool empty () { return nodes.begin() == nodes.end(); };
  bool next(int *val);

  void first() { currNode = nodes.begin(); };
  void add(const FGAirRoute &other);
  void add(int node) {nodes.push_back(node);};

  friend istream& operator >> (istream& in, FGAirRoute& r);
};

inline istream& operator >> ( istream& in, FGAirRoute& r )
{
  int node;
  in >> node;
  r.nodes.push_back(node);
  //getline( in, n.name );
  return in;
}

typedef vector<FGAirRoute> AirRouteVector;
typedef vector<FGAirRoute>::iterator AirRouteVectorIterator;

/**************************************************************************************
 * class FGAirwayNetwork
 *************************************************************************************/
class FGAirwayNetwork
{
private:
  bool hasNetwork;
  node_map        nodesMap;
  FGNodeVector    nodes;
  FGAirwayVector segments;
  //intVec route;
  intVec traceStack;
  AirRouteVector routes;

  bool foundRoute;
  double totalDistance, maxDistance;

public:
  FGAirwayNetwork();

  //void addNode   (const FGNode& node);
  //void addNodes  (FGParkingVec *parkings);
  void addAirway(const FGAirway& seg);

  void init();
  bool exists() { return hasNetwork; };
  int findNearestNode(double lat, double lon);
  FGNode *findNode(int idx);
  FGAirRoute findShortestRoute(int start, int end);
  void trace(FGNode *, int, int, double dist);

  void load(SGPath path);
};

#endif
