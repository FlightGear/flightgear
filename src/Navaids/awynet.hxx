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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifndef _AIRWAYNETWORK_HXX_
#define _AIRWAYNETWORK_HXX_

#include <string>
#include <istream>
#include <set>
#include <map>
#include <vector>

#include <simgear/misc/sgstream.hxx>


//#include "parking.hxx"

class FGAirway; // forward reference
class SGPath;
class SGGeod;

typedef std::vector<FGAirway>  FGAirwayVector;
typedef std::vector<FGAirway *> FGAirwayPointerVector;
typedef std::vector<FGAirway>::iterator FGAirwayVectorIterator;
typedef std::vector<FGAirway*>::iterator FGAirwayPointerVectorIterator;

/**************************************************************************************
 * class FGNode
 *************************************************************************************/
class FGNode
{
private:
  std::string ident;
  SGGeod geod;
  SGVec3d cart; // cached cartesian position
  int index;
  FGAirwayPointerVector next; // a vector to all the segments leaving from this node

public:
  FGNode();
  FGNode(const SGGeod& aPos, int idx, std::string id);

  void setIndex(int idx)                  { index = idx;};
  void addAirway(FGAirway *segment) { next.push_back(segment); };

  const SGGeod& getPosition() {return geod;}
  const SGVec3d& getCart() { return cart; }
  int getIndex() { return index; };
  std::string getIdent() { return ident; };
  FGNode *getAddress() { return this;};
  FGAirwayPointerVectorIterator getBeginRoute() { return next.begin(); };
  FGAirwayPointerVectorIterator getEndRoute()   { return next.end();   };

  bool matches(std::string ident, const SGGeod& aPos);
};

typedef std::vector<FGNode *> FGNodeVector;
typedef std::vector<FGNode *>::iterator FGNodeVectorIterator;


typedef std::map < std::string, FGNode *> node_map;
typedef node_map::iterator node_map_iterator;
typedef node_map::const_iterator const_node_map_iterator;


/***************************************************************************************
 * class FGAirway
 **************************************************************************************/
class FGAirway
{
private:
  std::string startNode;
  std::string endNode;
  double length;
  FGNode *start;
  FGNode *end;
  int index;
  int type; // 1=low altitude; 2=high altitude airway
  int base; // base altitude
  int top;  // top altitude
  std::string name;

public:
  FGAirway();
  FGAirway(FGNode *, FGNode *, int);

  void setIndex        (int val) { index     = val; };
  void setStartNodeRef (std::string val) { startNode = val; };
  void setEndNodeRef   (std::string val) { endNode   = val; };

  void setStart(node_map *nodes);
  void setEnd  (node_map *nodes);
  void setType (int tp) { type = tp;};
  void setBase (int val) { base = val;};
  void setTop  (int val) { top  = val;};
  void setName (std::string val) { name = val;};

  void setTrackDistance();

  FGNode * getEnd() { return end;};
  double getLength() { if (length == 0) setTrackDistance(); return length; };
  int getIndex() { return index; };
  std::string getName() { return name; };


};


typedef std::vector<int> intVec;
typedef std::vector<int>::iterator intVecIterator;
typedef std::vector<int>::const_iterator constIntVecIterator;

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

  friend std::istream& operator >> (std::istream& in, FGAirRoute& r);
};

inline std::istream& operator >> ( std::istream& in, FGAirRoute& r )
{
  int node;
  in >> node;
  r.nodes.push_back(node);
  //getline( in, n.name );
  return in;
}

typedef std::vector<FGAirRoute> AirRouteVector;
typedef std::vector<FGAirRoute>::iterator AirRouteVectorIterator;

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
  ~FGAirwayNetwork();

  //void addNode   (const FGNode& node);
  //void addNodes  (FGParkingVec *parkings);
  void addAirway(const FGAirway& seg);

  void init();
  bool exists() { return hasNetwork; };
  int findNearestNode(const SGGeod& aPos);
  FGNode *findNode(int idx);
  FGAirRoute findShortestRoute(int start, int end);
  void trace(FGNode *, int, int, double dist);

  void load(const SGPath& path);
};

#endif
