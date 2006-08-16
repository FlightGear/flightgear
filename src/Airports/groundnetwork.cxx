// groundnet.cxx - Implimentation of the FlightGear airport ground handling code
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <math.h>
#include <algorithm>

//#include <plib/sg.h>
//#include <plib/ul.h>

//#include <Environment/environment_mgr.hxx>
//#include <Environment/environment.hxx>
//#include <simgear/misc/sg_path.hxx>
//#include <simgear/props/props.hxx>
//#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/route/waypoint.hxx>
//#include <Main/globals.hxx>
//#include <Main/fg_props.hxx>
//#include <Airports/runways.hxx>

//#include STL_STRING

#include "groundnetwork.hxx"

SG_USING_STD(sort);

/**************************************************************************
 * FGTaxiNode
 *************************************************************************/
FGTaxiNode::FGTaxiNode()
{
}

/***************************************************************************
 * FGTaxiSegment
 **************************************************************************/
FGTaxiSegment::FGTaxiSegment()
{
}

void FGTaxiSegment::setStart(FGTaxiNodeVector *nodes)
{
  FGTaxiNodeVectorIterator i = nodes->begin();
  while (i != nodes->end())
    {
      if (i->getIndex() == startNode)
	{
	  start = i->getAddress();
	  i->addSegment(this);
	  return;
	}
      i++;
    }
}

void FGTaxiSegment::setEnd(FGTaxiNodeVector *nodes)
{
  FGTaxiNodeVectorIterator i = nodes->begin();
  while (i != nodes->end())
    {
      if (i->getIndex() == endNode)
	{
	  end = i->getAddress();
	  return;
	}
      i++;
    }
}

// There is probably a computationally cheaper way of 
// doing this.
void FGTaxiSegment::setTrackDistance()
{
  double course;
  SGWayPoint first  (start->getLongitude(),
		     start->getLatitude(),
		     0);
  SGWayPoint second (end->getLongitude(),
		     end->getLatitude(),
		     0);
  first.CourseAndDistance(second, &course, &length);
}

bool FGTaxiRoute::next(int *nde) 
{ 
  //for (intVecIterator i = nodes.begin(); i != nodes.end(); i++)
  //  cerr << "FGTaxiRoute contains : " << *(i) << endl;
  //cerr << "Offset from end: " << nodes.end() - currNode << endl;
  //if (currNode != nodes.end())
  //  cerr << "true" << endl;
  //else
  //  cerr << "false" << endl;
  //if (nodes.size() != (routes.size()) +1)
  //  cerr << "ALERT: Misconfigured TaxiRoute : " << nodes.size() << " " << routes.size() << endl;
      
  if (currNode == nodes.end())
    return false;
  *nde = *(currNode); 
  currNode++;
  currRoute++;
  return true;
};

bool FGTaxiRoute::next(int *nde, int *rte) 
{ 
  //for (intVecIterator i = nodes.begin(); i != nodes.end(); i++)
  //  cerr << "FGTaxiRoute contains : " << *(i) << endl;
  //cerr << "Offset from end: " << nodes.end() - currNode << endl;
  //if (currNode != nodes.end())
  //  cerr << "true" << endl;
  //else
  //  cerr << "false" << endl;
  if (nodes.size() != (routes.size()) +1) {
    SG_LOG(SG_GENERAL, SG_ALERT, "ALERT: Misconfigured TaxiRoute : " << nodes.size() << " " << routes.size());
    exit(1);
  }
  if (currNode == nodes.end())
    return false;
  *nde = *(currNode); 
  *rte = *(currRoute);
  currNode++;
  currRoute++;
  return true;
};
/***************************************************************************
 * FGGroundNetwork()
 **************************************************************************/

FGGroundNetwork::FGGroundNetwork()
{
  hasNetwork = false;
  foundRoute = false;
  totalDistance = 0;
  maxDistance = 0;
}

void FGGroundNetwork::addSegment(const FGTaxiSegment &seg)
{
  segments.push_back(seg);
}

void FGGroundNetwork::addNode(const FGTaxiNode &node)
{
  nodes.push_back(node);
}

void FGGroundNetwork::addNodes(FGParkingVec *parkings)
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



void FGGroundNetwork::init()
{
  hasNetwork = true;
  int index = 0;
  FGTaxiSegmentVectorIterator i = segments.begin();
  while(i != segments.end()) {
    //cerr << "initializing node " << i->getIndex() << endl;
    i->setStart(&nodes);
    i->setEnd  (&nodes);
    i->setTrackDistance();
    i->setIndex(index++);
    //cerr << "Track distance = " << i->getLength() << endl;
    //cerr << "Track ends at"      << i->getEnd()->getIndex() << endl;
    i++;
  }
  //exit(1);
}

int FGGroundNetwork::findNearestNode(double lat, double lon)
{
  double minDist = HUGE_VAL;
  double course, dist;
  int index;
  SGWayPoint first  (lon,
		     lat,
		     0);
  
  for (FGTaxiNodeVectorIterator 
	 itr = nodes.begin();
       itr != nodes.end(); itr++)
    {
      double course;
      SGWayPoint second (itr->getLongitude(),
			 itr->getLatitude(),
			 0);
      first.CourseAndDistance(second, &course, &dist);
      if (dist < minDist)
	{
	  minDist = dist;
	  index = itr->getIndex();
	  //cerr << "Minimum distance of " << minDist << " for index " << index << endl;
	}
    }
  return index;
}

FGTaxiNode *FGGroundNetwork::findNode(int idx)
{
  for (FGTaxiNodeVectorIterator 
	 itr = nodes.begin();
       itr != nodes.end(); itr++)
    {
      if (itr->getIndex() == idx)
	return itr->getAddress();
    }
  return 0;
}

FGTaxiSegment *FGGroundNetwork::findSegment(int idx)
{
  for (FGTaxiSegmentVectorIterator 
	 itr = segments.begin();
       itr != segments.end(); itr++)
    {
      if (itr->getIndex() == idx)
	return itr->getAddress();
    }
  return 0;
}

FGTaxiRoute FGGroundNetwork::findShortestRoute(int start, int end) 
{
  foundRoute = false;
  totalDistance = 0;
  FGTaxiNode *firstNode = findNode(start);
  FGTaxiNode *lastNode  = findNode(end);
  //prevNode = prevPrevNode = -1;
  //prevNode = start;
  routes.clear();
  nodesStack.clear();
  routesStack.clear();

  trace(firstNode, end, 0, 0);
  FGTaxiRoute empty;
  
  if (!foundRoute)
    {
      SG_LOG( SG_GENERAL, SG_INFO, "Failed to find route from waypoint " << start << " to " << end );
      exit(1);
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


void FGGroundNetwork::trace(FGTaxiNode *currNode, int end, int depth, double distance)
{
  // Just check some preconditions of the trace algorithm
  if (nodesStack.size() != routesStack.size()) 
    {
      SG_LOG(SG_GENERAL, SG_ALERT, "size of nodesStack and routesStack is not equal. NodesStack :" 
	     << nodesStack.size() << ". RoutesStack : " << routesStack.size());
    }
  nodesStack.push_back(currNode->getIndex());
  totalDistance += distance;
  //cerr << "Starting trace " << depth << " total distance: " << totalDistance<< endl;
  //<< currNode->getIndex() << endl;

  // If the current route matches the required end point we found a valid route
  // So we can add this to the routing table
  if (currNode->getIndex() == end)
    {
      //cerr << "Found route : " <<  totalDistance << "" << " " << *(nodesStack.end()-1) << endl;
      routes.push_back(FGTaxiRoute(nodesStack,routesStack,totalDistance));
      if (nodesStack.empty() || routesStack.empty())
	{
	  printRoutingError(string("while finishing route"));
	}
      nodesStack.pop_back();
      routesStack.pop_back();
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
  // i should point to nodesStack.end()-1; and we can continue
  // if i is not nodesStack.end, the previous node was found, 
  // and we should return. 
  // This only works at trace levels of 1 or higher though
  if (depth > 0) {
    intVecIterator i = nodesStack.begin();
    while ((*i) != currNode->getIndex()) {
      //cerr << "Route so far : " << (*i) << endl;
      i++;
    }
    if (i != nodesStack.end()-1) {
      if (nodesStack.empty() || routesStack.empty())
	{
	  printRoutingError(string("while returning from an already encountered node"));
	}
      nodesStack.pop_back();
      routesStack.pop_back();
      totalDistance -= distance;
      return;
    }
    // If the total distance from start to the current waypoint
    // is longer than that of a route we can also stop this trace 
    // and go back one level. 
    if ((totalDistance > maxDistance) && foundRoute)
      {
	//cerr << "Stopping rediculously long trace: " << totalDistance << endl;
	if (nodesStack.empty() || routesStack.empty())
	{
	  printRoutingError(string("while returning from finding a rediculously long route"));
	}
	nodesStack.pop_back();
	routesStack.pop_back();
	totalDistance -= distance;
	return;
      }
  }
  
  //cerr << "2" << endl;
  if (currNode->getBeginRoute() != currNode->getEndRoute())
    {
      //cerr << "3" << endl;
      for (FGTaxiSegmentPointerVectorIterator 
	     i = currNode->getBeginRoute();
	   i != currNode->getEndRoute();
	   i++)
	{
	  //cerr << (*i)->getLength() << endl;
	  //cerr << (*i)->getIndex() << endl;
	  int idx = (*i)->getIndex();
	  routesStack.push_back((*i)->getIndex());
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
    }
  if (nodesStack.empty())
    {
      printRoutingError(string("while finishing trace"));
    }
  nodesStack.pop_back();
  // Make sure not to dump the level-zero routesStack entry, because that was never created.
  if (depth)
    {
      routesStack.pop_back();
      //cerr << "leaving trace " << routesStack.size() << endl;
    }
  totalDistance -= distance;
  return;
}

void FGGroundNetwork::printRoutingError(string mess)
{
  SG_LOG(SG_GENERAL, SG_ALERT,  "Error in ground network trace algorithm " << mess);
  if (nodesStack.empty())
    {
      SG_LOG(SG_GENERAL, SG_ALERT, " nodesStack is empty. Dumping routesStack");
      for (intVecIterator i = routesStack.begin() ; i != routesStack.end(); i++)
	SG_LOG(SG_GENERAL, SG_ALERT, "Route " << (*i));
    }
  if (routesStack.empty())
    {
      SG_LOG(SG_GENERAL, SG_ALERT, " routesStack is empty. Dumping nodesStack"); 
      for (intVecIterator i = nodesStack.begin() ; i != nodesStack.end(); i++)
	SG_LOG(SG_GENERAL, SG_ALERT, "Node " << (*i));
    }
  //exit(1);
}
