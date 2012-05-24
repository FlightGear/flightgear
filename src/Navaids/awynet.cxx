// awynet.cxx
// by Durk Talsma, started June 2005.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <math.h>
#include <algorithm>
#include <iostream>

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/route/waypoint.hxx>
#include <simgear/misc/sg_path.hxx>

#include "awynet.hxx"

using std::sort;
using std::string;
using std::cerr;
using std::endl;

/**************************************************************************
 * FGNode
 *************************************************************************/
FGNode::FGNode()
{
}

FGNode::FGNode(const SGGeod& aPos, int idx, std::string id) :
  ident(id),
  geod(aPos),
  index(idx)
{
  cart = SGVec3d::fromGeod(geod);
}

bool FGNode::matches(std::string id, const SGGeod& aPos)
{
  if ((ident == id) &&
      (fabs(aPos.getLatitudeDeg() - geod.getLatitudeDeg()) < 1.0) &&
      (fabs(aPos.getLongitudeDeg() - geod.getLongitudeDeg()) < 1.0))
    return true;
  else
    return false;
}

/***************************************************************************
 * FGAirway
 **************************************************************************/
FGAirway::FGAirway()
{
  length = 0;
}

void FGAirway::setStart(node_map *nodes)
{
  node_map_iterator itr = nodes->find(startNode);
  if (itr == nodes->end()) {
    cerr << "Couldn't find node: " << startNode << endl;
  }
  else {
    start = itr->second->getAddress();
    itr->second->addAirway(this);
  }
}

void FGAirway::setEnd(node_map *nodes)
{
 node_map_iterator itr = nodes->find(endNode);
  if (itr == nodes->end()) {
    cerr << "Couldn't find node: " << endNode << endl;
  }
  else {
    end = itr->second->getAddress();
  }
}

// There is probably a computationally cheaper way of
// doing this.
void FGAirway::setTrackDistance()
{
  length = SGGeodesy::distanceM(start->getPosition(), end->getPosition());
}

/***************************************************************************
 * FGAirRoute()
 **************************************************************************/


bool FGAirRoute::next(int *val)
{
  //for (intVecIterator i = nodes.begin(); i != nodes.end(); i++)
  //  cerr << "FGTaxiRoute contains : " << *(i) << endl;
  //cerr << "Offset from end: " << nodes.end() - currNode << endl;
  //if (currNode != nodes.end())
  //  cerr << "true" << endl;
  //else
  //  cerr << "false" << endl;

  if (currNode == nodes.end())
    return false;
  *val = *(currNode);
  currNode++;
  return true;
};

void FGAirRoute::add(const FGAirRoute &other) {
  for (constIntVecIterator i = other.nodes.begin() ;
       i != other.nodes.end(); i++)
    {
      nodes.push_back((*i));
    }
  distance += other.distance;
}
/***************************************************************************
 * FGAirwayNetwork()
 **************************************************************************/

FGAirwayNetwork::FGAirwayNetwork()
{
  hasNetwork = false;
  foundRoute = false;
  totalDistance = 0;
  maxDistance = 0;
}

FGAirwayNetwork::~FGAirwayNetwork()
{
    for (unsigned int it = 0; it < nodes.size(); it++) {
	delete nodes[ it];
    }
}
void FGAirwayNetwork::addAirway(const FGAirway &seg)
{
  segments.push_back(seg);
}

//void FGAirwayNetwork::addNode(const FGNode &node)
//{
//  nodes.push_back(node);
//}

/*
  void FGAirwayNetwork::addNodes(FGParkingVec *parkings)
  {
  FGTaxiNode n;
  FGParkingVecIterator i = parkings->begin();
  while (i != parkings->end())
  {
  n.setIndex(i->getIndex());
  n.setLatitude(i->getLatitude());
  n.setLongitude(i->getLongitude());
  nodes.push_back(n);

  i++;
  }
  }
*/


void FGAirwayNetwork::init()
{
  hasNetwork = true;
  FGAirwayVectorIterator i = segments.begin();
  while(i != segments.end()) {
    //cerr << "initializing Airway " << i->getIndex() << endl;
    i->setStart(&nodesMap);
    i->setEnd  (&nodesMap);
    //i->setTrackDistance();
    //cerr << "Track distance = " << i->getLength() << endl;
    //cerr << "Track ends at"      << i->getEnd()->getIndex() << endl;
    i++;
  }
  //exit(1);
}


void FGAirwayNetwork::load(const SGPath& path)
{
  std::string identStart, identEnd, token, name;
  double latStart, lonStart, latEnd, lonEnd;
  int type, base, top;
  int airwayIndex = 0;
  FGNode *n;

  sg_gzifstream in( path.str() );
  if ( !in.is_open() ) {
    SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
    exit(-1);
  }
  // toss the first two lines of the file
  in >> skipeol;
  in >> skipeol;

  // read in each remaining line of the file
    while ( ! in.eof() ) {
      string token;
      in >> token;

      if ( token == "99" ) {
	return; //in >> skipeol;
      }
      // Read each line from the database
      identStart = token;
      in >> latStart >> lonStart >> identEnd >> latEnd >> lonEnd >> type >> base >> top >> name;
      in >> skipeol;
      /*out << identStart << " "
	<< latStart   << " "
	<< lonStart   << " "
	<< identEnd   << " "
	<< latEnd     << " "
	<< lonEnd     << " "
	<< type       << " "
	<< base       << " "
	<< top        << " "
	<< name       << " "
	<< endl;*/
      //first determine whether the start and end reference database already exist
      //if not we should create them
      int startIndex = 0, endIndex=0;
      //     FGNodeVectorIterator i = nodes.begin();
  //     while (i != nodes.end() && (!(i->matches(identStart,latStart, lonStart))))
// 	{
// 	  i++;
// 	  startIndex++;
// 	}
//       if (i == nodes.end())
// 	{
// 	  nodes.push_back(FGNode(latStart, lonStart, startIndex, identStart));
// 	//cout << "Adding node: " << identStart << endl;
// 	}

//       i = nodes.begin();
//       while (i != nodes.end() && (!(i->matches(identEnd,latEnd, lonEnd)))) {
// 	i++;
// 	endIndex++;
//       }
//       if (i == nodes.end()) {
// 	nodes.push_back(FGNode(latEnd, lonEnd, endIndex, identEnd));
// 	//cout << "Adding node: " << identEnd << endl;
//       }
      // generate unique IDs for the nodes, consisting of a combination
      // of the Navaid identifier + the integer value of the lat/lon position.
      // identifier alone will not suffice, because they might not be globally unique.
      char buffer[32];
      string startNode, endNode;
      // Start
      buffer[sizeof(buffer)-1] = 0;
      snprintf(buffer, sizeof(buffer)-1, "%s%d%d", identStart.c_str(), (int) latStart, (int) lonStart);
      startNode = buffer;

      node_map_iterator itr = nodesMap.find(string(buffer));
      if (itr == nodesMap.end()) {
	startIndex = nodes.size();
  SGGeod startPos(SGGeod::fromDeg(lonStart, latStart));
	n = new FGNode(startPos, startIndex, identStart);
	nodesMap[string(buffer)] = n;
	nodes.push_back(n);
	//cout << "Adding node: " << identStart << endl;
      }
      else {
	startIndex = itr->second->getIndex();
      }
      // Start
      snprintf(buffer, 32, "%s%d%d", identEnd.c_str(), (int) latEnd, (int) lonEnd);
      endNode = buffer;

      itr = nodesMap.find(string(buffer));
      if (itr == nodesMap.end()) {
	endIndex = nodes.size();
  SGGeod endPos(SGGeod::fromDeg(lonEnd, latEnd));
	n = new FGNode(endPos, endIndex, identEnd);
	nodesMap[string(buffer)] = n;
	nodes.push_back(n);
	//cout << "Adding node: " << identEnd << endl;
      }
      else {
	endIndex = itr->second->getIndex();
      }


      FGAirway airway;
      airway.setIndex        ( airwayIndex++ );
      airway.setStartNodeRef ( startNode     );
      airway.setEndNodeRef   ( endNode       );
      airway.setType         ( type          );
      airway.setBase         ( base          );
      airway.setTop          ( top           );
      airway.setName         ( name          );
      segments.push_back(airway);
      //cout << "    Adding Airway: " << name << " " << startIndex << " " << endIndex << endl;
    }
  }

int FGAirwayNetwork::findNearestNode(const SGGeod& aPos)
{
  double minDist = HUGE_VAL;
  int index = -1;
  SGVec3d cart = SGVec3d::fromGeod(aPos);
  
  //cerr << "Lat " << lat << " lon " << lon << endl;
  for (FGNodeVectorIterator
	 itr = nodes.begin();
       itr != nodes.end(); itr++)
    {
      double d2 = distSqr(cart, (*itr)->getCart());
      if (d2 < minDist)
      {
        minDist = d2;
	  	  index = (*itr)->getIndex();
      }
      //cerr << (*itr)->getIndex() << endl;
    }
  //cerr << " returning  " << index << endl;
  return index;
}

FGNode *FGAirwayNetwork::findNode(int idx)
{
  for (FGNodeVectorIterator
	 itr = nodes.begin();
       itr != nodes.end(); itr++)
    {
      if ((*itr)->getIndex() == idx)
	return (*itr)->getAddress();
    }
  return 0;
}

FGAirRoute FGAirwayNetwork::findShortestRoute(int start, int end)
{
  foundRoute = false;
  totalDistance = 0;
  FGNode *firstNode = findNode(start);
  //FGNode *lastNode  = findNode(end);
  //prevNode = prevPrevNode = -1;
  //prevNode = start;
  routes.clear();
  traceStack.clear();
  trace(firstNode, end, 0, 0);
  FGAirRoute empty;

  if (!foundRoute)
    {
      SG_LOG( SG_GENERAL, SG_INFO, "Failed to find route from waypoint " << start << " to " << end );
      cerr << "Failed to find route from waypoint " << start << " to " << end << endl;
      //exit(1);
    }
  sort(routes.begin(), routes.end());
  //for (intVecIterator i = route.begin(); i != route.end(); i++)
  //  {
  //    rte->push_back(*i);
  //  }

  if (routes.begin() != routes.end())
    return *(routes.begin());
  else
    return empty;
}


void FGAirwayNetwork::trace(FGNode *currNode, int end, int depth, double distance)
{
  traceStack.push_back(currNode->getIndex());
  totalDistance += distance;
  //cerr << depth << " ";
  //cerr << "Starting trace " << depth << " total distance: " << totalDistance<< endl;
  //<< currNode->getIndex() << endl;

  // If the current route matches the required end point we found a valid route
  // So we can add this to the routing table
  if (currNode->getIndex() == end)
    {
      cerr << "Found route : " <<  totalDistance << "" << " " << *(traceStack.end()-1) << endl;
      routes.push_back(FGAirRoute(traceStack,totalDistance));
      traceStack.pop_back();
      if (!(foundRoute))
	maxDistance = totalDistance;
      else
	if (totalDistance < maxDistance)
	  maxDistance = totalDistance;
      foundRoute = true;
      totalDistance -= distance;
      return;
    }


  // search if the currentNode has been encountered before
  // if so, we should step back one level, because it is
  // rather rediculous to proceed further from here.
  // if the current node has not been encountered before,
  // i should point to traceStack.end()-1; and we can continue
  // if i is not traceStack.end, the previous node was found,
  // and we should return.
  // This only works at trace levels of 1 or higher though
  if (depth > 0) {
    intVecIterator i = traceStack.begin();
    while ((*i) != currNode->getIndex()) {
      //cerr << "Route so far : " << (*i) << endl;
      i++;
    }
    if (i != traceStack.end()-1) {
      traceStack.pop_back();
      totalDistance -= distance;
      return;
    }
    // If the total distance from start to the current waypoint
    // is longer than that of a route we can also stop this trace
    // and go back one level.
    if ((totalDistance > maxDistance) && foundRoute)
      {
	cerr << "Stopping rediculously long trace: " << totalDistance << endl;
	traceStack.pop_back();
	totalDistance -= distance;
	return;
      }
  }

  //cerr << "2" << endl;
  if (currNode->getBeginRoute() != currNode->getEndRoute())
    {
      //cerr << "l3l" << endl;
      for (FGAirwayPointerVectorIterator
	     i = currNode->getBeginRoute();
	   i != currNode->getEndRoute();
	   i++)
	{
	  //cerr << (*i)->getLenght() << endl;
	  trace((*i)->getEnd(), end, depth+1, (*i)->getLength());
	//  {
	//      // cerr << currNode -> getIndex() << " ";
	//      route.push_back(currNode->getIndex());
	//      return true;
	//    }
	}
    }
  else
    {
      //SG_LOG( SG_GENERAL, SG_DEBUG, "4" );
      //cerr << "4" << endl;
    }
  traceStack.pop_back();
  totalDistance -= distance;
  return;
}

