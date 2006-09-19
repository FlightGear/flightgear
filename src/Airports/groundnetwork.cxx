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

#include <AIModel/AIFlightPlan.hxx>

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
  oppositeDirection = 0;
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
/***************************************************************************
 * FGTaxiRoute
 **************************************************************************/
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
  if (currNode != nodes.begin()) // make sure route corresponds to the end node
    currRoute++;
  currNode++;
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
  //*rte = *(currRoute);
  if (currNode != nodes.begin()) // Make sure route corresponds to the end node
    {
      *rte = *(currRoute);
      currRoute++;
    }
  else
    {
      // If currNode points to the first node, this means the aircraft is not on the taxi node
      // yet. Make sure to return a unique identifyer in this situation though, because otherwise
      // the speed adjust AI code may be unable to resolve whether two aircraft are on the same 
      // taxi route or not. the negative of the preceding route seems a logical choice, as it is 
      // unique for any starting location. 
      // Note that this is probably just a temporary fix until I get Parking / tower control working.
      *rte = -1 * *(currRoute); 
    }
  currNode++;
  return true;
};

void FGTaxiRoute::rewind(int route)
{
  int currPoint;
  int currRoute;
  first();
  do {
    if (!(next(&currPoint, &currRoute))) { 
      SG_LOG(SG_GENERAL,SG_ALERT, "Error in rewinding TaxiRoute: current" << currRoute 
	     << " goal " << route);
    }
  } while (currRoute != route);
}

/***************************************************************************
 * FGTrafficRecord
 **************************************************************************/
void FGTrafficRecord::setPositionAndIntentions(int pos, FGAIFlightPlan *route)
{
 
  currentPos = pos;
  if (intentions.size()) {
    if (*intentions.begin() != pos) {
      SG_LOG(SG_GENERAL, SG_ALERT, "Error in FGTrafficRecord::setPositionAndIntentions");
      cerr << "Pos : " << pos << " Curr " << *(intentions.begin())  << endl;
      for (intVecIterator i = intentions.begin(); i != intentions.end() ; i++) {
	cerr << (*i) << " ";
      }
      cerr << endl;
    }
    intentions.erase(intentions.begin());
  } else {
    //int legNr, routeNr;
    //FGAIFlightPlan::waypoint* const wpt= route->getCurrentWaypoint();
    int size = route->getNrOfWayPoints();
    cerr << "Setting pos" << pos << " ";
    cerr << "setting intentions ";
    for (int i = 0; i < size; i++) {
      int val = route->getRouteIndex(i);
     
      if ((val) && (val != pos))
	{
	  intentions.push_back(val); 
	  cerr << val<< " ";
	}
    }
    cerr << endl;
    //while (route->next(&legNr, &routeNr)) {
    //intentions.push_back(routeNr);
    //}
    //route->rewind(currentPos);
  }
  //exit(1);
}

bool FGTrafficRecord::checkPositionAndIntentions(FGTrafficRecord &other)
{
  bool result = false;
  //cerr << "Start check 1" << endl;
  if (currentPos == other.currentPos) 
    {
      //cerr << "Check Position and intentions: current matches" << endl;
      result = true;
    }
 //  else if (other.intentions.size()) 
//     {
//       cerr << "Start check 2" << endl;
//       intVecIterator i = other.intentions.begin(); 
//       while (!((i == other.intentions.end()) || ((*i) == currentPos)))
// 	i++;
//       if (i != other.intentions.end()) {
// 	cerr << "Check Position and intentions: current matches other.intentions" << endl;
// 	result = true;
//       }
  else if (intentions.size()) {
    //cerr << "Start check 3" << endl;
    intVecIterator i = intentions.begin(); 
    while (!((i == intentions.end()) || ((*i) == other.currentPos)))
      i++;
    if (i != intentions.end()) {
      //cerr << "Check Position and intentions: .other.current matches" << endl;
      result = true;
    }
  }
  //cerr << "Done !!" << endl;
  return result;
}

void FGTrafficRecord::setPositionAndHeading(double lat, double lon, double hdg, 
					    double spd, double alt)
{
  latitude = lat;
  longitude = lon;
  heading = hdg;
  speed = spd;
  altitude = alt;
}

int FGTrafficRecord::crosses(FGGroundNetwork *net, FGTrafficRecord &other)
{
  if (checkPositionAndIntentions(other) || (other.checkPositionAndIntentions(*this)))
    return -1;
  intVecIterator i, j;
  int currentTargetNode = 0, otherTargetNode = 0;
  if (currentPos > 0)
    currentTargetNode = net->findSegment(currentPos      )->getEnd()->getIndex(); // OKAY,... 
  if (other.currentPos > 0)
    otherTargetNode   = net->findSegment(other.currentPos)->getEnd()->getIndex(); // OKAY,...
  if ((currentTargetNode == otherTargetNode) && currentTargetNode > 0)
    return currentTargetNode;
  if (intentions.size())
    {
      for (i = intentions.begin(); i != intentions.end(); i++)
	{
	  if (currentTargetNode == net->findSegment(*i)->getEnd()->getIndex())
	    {
	      cerr << "Current crosses at " << currentTargetNode <<endl;
	      return currentTargetNode;
	    }
	  
	}
    }
  if (other.intentions.size())
    {
      for (i = other.intentions.begin(); i != other.intentions.end(); i++)
	{
	  if (otherTargetNode == net->findSegment(*i)->getEnd()->getIndex())
	    {
	      cerr << "Other crosses at " << currentTargetNode <<endl;
	      return otherTargetNode;
	    }
	}
    }
  if (intentions.size() && other.intentions.size())
    {
      for (i = intentions.begin(); i != intentions.end(); i++) 
	{
	  for (j = other.intentions.begin(); j != other.intentions.end(); j++)
	    {
	      //cerr << "finding segment " << *i << " and " << *j << endl;
	      currentTargetNode = net->findSegment(*i)->getEnd()->getIndex();
	      otherTargetNode   = net->findSegment(*j)->getEnd()->getIndex();
	      if (currentTargetNode == otherTargetNode) 
		{
		  //cerr << "Routes will cross at " << currentTargetNode << endl;
		  return currentTargetNode;
		}
	    }
	}
    }
  return -1;
}

bool FGTrafficRecord::isOpposing (FGGroundNetwork *net, FGTrafficRecord &other, int node)
{
  // Check if current segment is the reverse segment for the other aircraft
  FGTaxiSegment *opp;
  //cerr << "Current segment " << currentPos << endl;
  if ((currentPos > 0) && (other.currentPos > 0))
    {
      opp = net->findSegment(currentPos)->opposite();
      if (opp) {
	if (opp->getIndex() == other.currentPos)
	  return true;
      }
      
      for (intVecIterator i = intentions.begin(); i != intentions.end(); i++)
	{
	  for (intVecIterator j = intentions.begin(); j != intentions.end(); j++)
	    {  
	      // cerr << "Current segment 1 " << (*i) << endl;
	      if (opp = net->findSegment(*i)->opposite())
		{
		  if (opp->getIndex() == 
		      net->findSegment(*j)->getIndex())
		    {
		      cerr << "Nodes " << net->findSegment(*i)->getIndex()
			   << " and  " << net->findSegment(*j)->getIndex()
			   << " are opposites " << endl;
		      if (net->findSegment(*i)->getStart()->getIndex() == node) {
			{
			  cerr << "Found the node" << endl;
			  return true;
			}
		      }
		    }
		}
	    }
	}
    }
  return false;
}


/***************************************************************************
 * FGGroundNetwork()
 **************************************************************************/

FGGroundNetwork::FGGroundNetwork()
{
  hasNetwork = false;
  foundRoute = false;
  totalDistance = 0;
  maxDistance = 0;
  currTraffic = activeTraffic.begin();

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
  int index = 1;
  sort(nodes.begin(), nodes.end());
  sort(segments.begin(), segments.end());
  FGTaxiSegmentVectorIterator i = segments.begin();
  while(i != segments.end()) {
    //cerr << "initializing node " << i->getIndex() << endl;
    i->setStart(&nodes);
    i->setEnd  (&nodes);
    i->setTrackDistance();
    i->setIndex(index);
    //cerr << "Track distance = " << i->getLength() << endl;
    //cerr << "Track ends at"      << i->getEnd()->getIndex() << endl;
    i++;
    index++;
  }
  i = segments.begin();
  while(i != segments.end()) {
    FGTaxiSegmentPointerVectorIterator j = i->getEnd()->getBeginRoute(); 
    while (j != i->getEnd()->getEndRoute())
      {
	if ((*j)->getEnd()->getIndex() == i->getStart()->getIndex())
	  {
	    int start1 = i->getStart()->getIndex();
	    int end1   = i->getEnd()  ->getIndex();
	    int start2 = (*j)->getStart()->getIndex();
	    int end2   = (*j)->getEnd()->getIndex();
	    int oppIndex = (*j)->getIndex();
	    cerr << "Oppossite of  " << i->getIndex() << " (" << start1 << "," << end1 << ") "
		 << "happens to be " << oppIndex      << " (" << start2 << "," << end2 << ") " << endl;
	    break;
	  }
	  j++;
      }
    i++;
  }
  //exit(1);
}

int FGGroundNetwork::findNearestNode(double lat, double lon)
{
  double minDist = HUGE_VAL;
  double dist;
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
{ /*
    for (FGTaxiNodeVectorIterator 
    itr = nodes.begin();
    itr != nodes.end(); itr++)
    {
    if (itr->getIndex() == idx)
    return itr->getAddress();
    }*/
  
  if ((idx >= 0) && (idx < nodes.size())) 
    return nodes[idx].getAddress();
  else
    return 0;
}

FGTaxiSegment *FGGroundNetwork::findSegment(int idx)
{/*
  for (FGTaxiSegmentVectorIterator 
	 itr = segments.begin();
       itr != segments.end(); itr++)
    {
      if (itr->getIndex() == idx)
	return itr->getAddress();
    } 
 */
  if ((idx > 0) && (idx <= segments.size()))
    return segments[idx-1].getAddress();
  else
    {
      cerr << "Alert: trying to find invalid segment " << idx << endl;
      return 0;
    }
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


void FGGroundNetwork::announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentPosition,
				       double lat, double lon, double heading, 
				       double speed, double alt, double radius)
{
  TrafficVectorIterator i = activeTraffic.begin();
  // Search search if the current id alread has an entry
  // This might be faster using a map instead of a vector, but let's start by taking a safe route
  if (activeTraffic.size()) {
    while ((i->getId() != id) && i != activeTraffic.end()) {
      i++;
    }
  }
  // Add a new TrafficRecord if no one exsists for this aircraft.
  if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
    FGTrafficRecord rec;
    rec.setId(id);
    rec.setPositionAndIntentions(currentPosition, intendedRoute);
    rec.setPositionAndHeading(lat, lon, heading, speed, alt);
    rec.setRadius(radius); // only need to do this when creating the record.
    activeTraffic.push_back(rec);
  } else {
    i->setPositionAndIntentions(currentPosition, intendedRoute); 
    i->setPositionAndHeading(lat, lon, heading, speed, alt);
  }
}

void FGGroundNetwork::signOff(int id) {
 TrafficVectorIterator i = activeTraffic.begin();
  // Search search if the current id alread has an entry
  // This might be faster using a map instead of a vector, but let's start by taking a safe route
  if (activeTraffic.size()) {
    while ((i->getId() != id) && i != activeTraffic.end()) {
      i++;
    }
  }
  if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
    SG_LOG(SG_GENERAL, SG_ALERT, "AI error: Aircraft without traffic record is signing off");
  } else {
      i = activeTraffic.erase(i);
  }
}

void FGGroundNetwork::update(int id, double lat, double lon, double heading, double speed, double alt, 
			     double dt) {
  TrafficVectorIterator i = activeTraffic.begin();
  // Search search if the current id has an entry
  // This might be faster using a map instead of a vector, but let's start by taking a safe route
  TrafficVectorIterator current, closest;
  if (activeTraffic.size()) {
    while ((i->getId() != id) && i != activeTraffic.end()) {
      i++;
    }
  }
  // update position of the current aircraft
  if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
    SG_LOG(SG_GENERAL, SG_ALERT, "AI error: updating aircraft without traffic record");
  } else {
    i->setPositionAndHeading(lat, lon, heading, speed, alt);
    current = i;
  }
  
  setDt(getDt() + dt);
  
  // Update every three secs, but add some randomness
  // to prevent all IA objects doing this in synchrony
  //if (getDt() < (3.0) + (rand() % 10))
  //  return;
  //else
  //  setDt(0);
  checkSpeedAdjustment(id, lat, lon, heading, speed, alt);
  checkHoldPosition   (id, lat, lon, heading, speed, alt);
}

void FGGroundNetwork::checkSpeedAdjustment(int id, double lat, 
					   double lon, double heading, 
					   double speed, double alt)
{
  
  // Scan for a speed adjustment change. Find the nearest aircraft that is in front
  // and adjust speed when we get too close. Only do this when current position and/or
  // intentions of the current aircraft match current taxiroute position of the proximate
  // aircraft. For traffic that is on other routes we need to issue a "HOLD Position"
  // instruction. See below for the hold position instruction.
  TrafficVectorIterator current, closest;
  TrafficVectorIterator i = activeTraffic.begin();
  if (activeTraffic.size()) 
    {
      while ((i->getId() != id) && (i != activeTraffic.end()))
	{
	  i++;
	}
    }
  else
    {
      return;
    }
  current = i;
  double mindist = HUGE;
  if (activeTraffic.size()) 
    {
      double course, dist, bearing, minbearing;
      
      //TrafficVector iterator closest;
      for (TrafficVectorIterator i = activeTraffic.begin(); 
	   i != activeTraffic.end(); i++)
	{
	  if (i != current) {
	    SGWayPoint curr  (lon,
			      lat,
			      alt);
	    SGWayPoint other    (i->getLongitude  (),
				 i->getLatitude (),
				 i->getAltitude  ());
	    other.CourseAndDistance(curr, &course, &dist);
	    bearing = fabs(heading-course);
	    if (bearing > 180)
	      bearing = 360-bearing;
	    if ((dist < mindist) && (bearing < 60.0))
	      {
		mindist = dist;
		closest = i;
		minbearing = bearing;
	      }
	  }
	}
      //cerr << "Distance : " << dist << " bearing : " << bearing << " heading : " << heading 
      //   << " course : " << course << endl;
      current->clearSpeedAdjustment();
      // Only clear the heading adjustment at positive speeds, otherwise the waypoint following
      // code wreaks havoc
      if (speed > 0.2)
	current->clearHeadingAdjustment();
      // All clear
      if (mindist > 100)
	{
	  //current->clearSpeedAdjustment();
	  //current->clearHeadingAdjustment();
	} 
      else
	{
	  if (current->getId() == closest->getWaitsForId())
	    return;
	  else 
	    current->setWaitsForId(closest->getId());
	  
	  // Getting close: Slow down to a bit less than the other aircraft
	  double maxAllowableDistance = (1.1*current->getRadius()) + (1.1*closest->getRadius());
	  if (mindist > maxAllowableDistance)
	    {
	      if (current->checkPositionAndIntentions(*closest)) 
		{
		  // Adjust speed, but don't let it drop to below 1 knots
		  //if (fabs(speed) > 1)
		  if (!(current->hasHeadingAdjustment())) 
		    {
		      current->setSpeedAdjustment(closest->getSpeed()* (mindist/100));
		      //cerr << "Adjusting speed to " << closest->getSpeed() * (mindist / 100) << " " 
		      //	 << "Bearing = " << minbearing << " Distance = " << mindist
		      //	 << " Latitude = " <<lat << " longitude = " << lon << endl;
		      //<< " Latitude = " <<closest->getLatitude() 
		      //<< " longitude = " << closest->getLongitude() 
		      //  << endl;
		    }
		  else
		    {
		      double newSpeed = (maxAllowableDistance-mindist);
		      current->setSpeedAdjustment(newSpeed);
		    } 
		}
	    }
	  else
	    { 
	      if (!(current->hasHeadingAdjustment())) 
		{
		  double newSpeed;
		  if (mindist > 10) {
		    newSpeed = 0.01;
		    current->setSpeedAdjustment(newSpeed);
		  } else {
		    newSpeed = -1 * (maxAllowableDistance-mindist);
		    current->setSpeedAdjustment(newSpeed);
		    current->setHeadingAdjustment(heading);
		  }
		}
	    }
	}
    }
}

void FGGroundNetwork::checkHoldPosition(int id, double lat, 
					double lon, double heading, 
					double speed, double alt)
{
  // Check for "Hold position instruction".
  // The hold position should be issued under the following conditions:
  // 1) For aircraft entering or crossing a runway with active traffic on it, or landing aircraft near it
  // 2) For taxiing aircraft that use one taxiway in opposite directions
  // 3) For crossing or merging taxiroutes.

  TrafficVectorIterator current, closest;
  TrafficVectorIterator i = activeTraffic.begin();
  if (activeTraffic.size()) 
    {
      while ((i->getId() != id) && i != activeTraffic.end()) 
	{
	  i++;
	}
    }
  else
    {
      return ;
    }
  current = i;
  current->setHoldPosition(false);
  double course, dist, bearing, minbearing;
  for (i = activeTraffic.begin(); 
       i != activeTraffic.end(); i++)
    {
      if (i != current) 
	{
	  int node = current->crosses(this, *i);
	  if (node != -1)
	    {
	      // Determine whether it's save to continue or not. 
	      // If we have a crossing route, there are two possibilities:
	      // 1) This is an interestion
	      // 2) This is oncoming two-way traffic, using the same taxiway.
	      //cerr << "Hold check 1 : " << id << " has common node " << node << endl;
	      SGWayPoint nodePos(findNode(node)->getLongitude  (),
				 findNode(node)->getLatitude   (),
				 alt);
	      SGWayPoint curr  (lon,
				lat,
				alt);
	      
	      SGWayPoint other    (i->getLongitude  (),
				   i->getLatitude (),
				   i->getAltitude  ());
	      //other.CourseAndDistance(curr, &course, &dist);
	      bool needsToWait;
	      if (current->isOpposing(this, *i, node))
		{
		  needsToWait = true;
		  //cerr << "Hold check 2 : " << id << "  has opposing segment " << endl;
		  // issue a "Hold Position" as soon as we're close to the offending node
		  // For now, I'm doing this as long as the other aircraft doesn't
		  // have a hold instruction as soon as we're within a reasonable 
		  // distance from the offending node.
		  // This may be a bit of a conservative estimate though, as it may
		  // be well possible that both aircraft can both continue to taxi 
		  // without crashing into each other.
		} 
	      else 
		{
		  other.CourseAndDistance(nodePos, &course, &dist);
		  if (dist > 2.0*i->getRadius())
		    {
		      needsToWait = false; 
		      //cerr << "Hold check 3 : " << id <<"  Other aircraft approaching node is still far away. (" << dist << " nm). Can safely continue " 
		      //	   << endl;
		    }
		  else 
		    {
		      needsToWait = true;
		      //cerr << "Hold check 4: " << id << "  Would need to wait for other aircraft : distance = " << dist << " nm" << endl;
		    }
		}
	      curr.CourseAndDistance(nodePos, &course, &dist);
	      if (!(i->hasHoldPosition()))
		{
		  
		  if ((dist < 2.5*current->getRadius()) && 
		      (needsToWait) && 
		      (!(current->getId() == i->getWaitsForId())) &&
		      (!(current->getSpeedAdjustment())))
		    
		    {
		      current->setHoldPosition(true);
		      //cerr << "Hold check 5: " << id <<"  Setting Hold Position: distance to node : " << dist << " nm"<< endl;
		    }
		  else
		    {
		      //cerr << "Hold check 6: " << id << "  No need to hold yet: Distance to node : " << dist << " nm"<< endl;
		    }
		}
	    }
	}
    }
}


bool FGGroundNetwork::hasInstruction(int id)
{
   TrafficVectorIterator i = activeTraffic.begin();
  // Search search if the current id has an entry
  // This might be faster using a map instead of a vector, but let's start by taking a safe route
  if (activeTraffic.size()) 
    {
      while ((i->getId() != id) && i != activeTraffic.end()) {
      i++;
    }
  }
  if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
    SG_LOG(SG_GENERAL, SG_ALERT, "AI error: checking ATC instruction for aircraft without traffic record");
  } else {
    return i->hasInstruction();
  }
  return false;
}

FGATCInstruction FGGroundNetwork::getInstruction(int id)
{
  TrafficVectorIterator i = activeTraffic.begin();
  // Search search if the current id has an entry
  // This might be faster using a map instead of a vector, but let's start by taking a safe route
  if (activeTraffic.size()) {
    while ((i->getId() != id) && i != activeTraffic.end()) {
      i++;
    }
  }
  if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
    SG_LOG(SG_GENERAL, SG_ALERT, "AI error: requesting ATC instruction for aircraft without traffic record");
  } else {
    return i->getInstruction();
  }
  return FGATCInstruction();
}




/***************************************************************************
 * FGATCInstruction
 *
 * This class is really out of place here, and should be combined with
 * FGATC controller and go into it's own file / directory
 * I'm leaving it for now though, because I'm testing this stuff quite
 * heavily.
 **************************************************************************/
FGATCInstruction::FGATCInstruction()
{
  holdPattern    = false; 
  holdPosition   = false;
  changeSpeed    = false;
  changeHeading  = false;
  changeAltitude = false;

  double speed   = 0;
  double heading = 0;
  double alt     = 0;
}

bool FGATCInstruction::hasInstruction()
{
  return (holdPattern || holdPosition || changeSpeed || changeHeading || changeAltitude);
}


