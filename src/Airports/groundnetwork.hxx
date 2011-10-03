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

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Shape>


#include <simgear/compiler.h>
#include <simgear/route/waypoint.hxx>

#include <string>
#include <vector>

using std::string;
using std::vector;

#include "gnnode.hxx"
#include "parking.hxx"
#include <ATC/trafficcontrol.hxx>


class FGTaxiSegment; // forward reference
class FGAIFlightPlan; // forward reference
class FGAirport;      // forward reference

typedef vector<FGTaxiSegment*>  FGTaxiSegmentVector;
typedef vector<FGTaxiSegment*>::iterator FGTaxiSegmentVectorIterator;

//typedef vector<FGTaxiSegment*> FGTaxiSegmentPointerVector;
//typedef vector<FGTaxiSegment*>::iterator FGTaxiSegmentPointerVectorIterator;

/***************************************************************************************
 * class FGTaxiSegment
 **************************************************************************************/
class FGTaxiSegment
{
private:
    int startNode;
    int endNode;
    double length;
    double heading;
    SGGeod center;
    bool isActive;
    bool isPushBackRoute;
    bool isBlocked;
    FGTaxiNode *start;
    FGTaxiNode *end;
    int index;
    FGTaxiSegment *oppositeDirection;



public:
    FGTaxiSegment() :
            startNode(0),
            endNode(0),
            length(0),
            heading(0),
            isActive(0),
            isPushBackRoute(0),
            isBlocked(0),
            start(0),
            end(0),
            index(0),
            oppositeDirection(0)
    {
    };

    FGTaxiSegment         (const FGTaxiSegment &other) :
            startNode         (other.startNode),
            endNode           (other.endNode),
            length            (other.length),
            heading           (other.heading),
            center            (other.center),
            isActive          (other.isActive),
            isPushBackRoute   (other.isPushBackRoute),
            isBlocked         (other.isBlocked),
            start             (other.start),
            end               (other.end),
            index             (other.index),
            oppositeDirection (other.oppositeDirection)
    {
    };

    FGTaxiSegment& operator=(const FGTaxiSegment &other)
    {
        startNode          = other.startNode;
        endNode            = other.endNode;
        length             = other.length;
        heading            = other.heading;
        center             = other.center;
        isActive           = other.isActive;
        isPushBackRoute    = other.isPushBackRoute;
        isBlocked          = other.isBlocked;
        start              = other.start;
        end                = other.end;
        index              = other.index;
        oppositeDirection  = other.oppositeDirection;
        return *this;
    };

    void setIndex        (int val) {
        index     = val;
    };
    void setStartNodeRef (int val) {
        startNode = val;
    };
    void setEndNodeRef   (int val) {
        endNode   = val;
    };

    void setOpposite(FGTaxiSegment *opp) {
        oppositeDirection = opp;
    };

    void setStart(FGTaxiNodeVector *nodes);
    void setEnd  (FGTaxiNodeVector *nodes);
    void setPushBackType(bool val) {
        isPushBackRoute = val;
    };
    void setDimensions(double elevation);
    void block() {
        isBlocked = true;
    }
    void unblock() {
        isBlocked = false;
    };
    bool hasBlock() {
        return isBlocked;
    };

    FGTaxiNode * getEnd() {
        return end;
    };
    FGTaxiNode * getStart() {
        return start;
    };
    double getLength() {
        return length;
    };
    int getIndex() {
        return index;
    };
    double getLatitude()  {
        return center.getLatitudeDeg();
    };
    double getLongitude() {
        return center.getLongitudeDeg();
    };
    double getHeading()   {
        return heading;
    };
    bool isPushBack() {
        return isPushBackRoute;
    };

    int getPenalty(int nGates);

    FGTaxiSegment *getAddress() {
        return this;
    };

    bool operator<(const FGTaxiSegment &other) const {
        return index < other.index;
    };
    //bool hasSmallerHeadingDiff (const FGTaxiSegment &other) const { return headingDiff < other.headingDiff; };
    FGTaxiSegment *opposite() {
        return oppositeDirection;
    };
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
//  int depth;
    intVecIterator currNode;
    intVecIterator currRoute;

public:
    FGTaxiRoute() {
        distance = 0;
        currNode = nodes.begin();
        currRoute = routes.begin();
    };
    FGTaxiRoute(intVec nds, intVec rts, double dist, int dpth) {
        nodes = nds;
        routes = rts;
        distance = dist;
        currNode = nodes.begin();
        currRoute = routes.begin();
//    depth = dpth;
    };

    FGTaxiRoute& operator= (const FGTaxiRoute &other) {
        nodes = other.nodes;
        routes = other.routes;
        distance = other.distance;
//    depth = other.depth;
        currNode = nodes.begin();
        currRoute = routes.begin();
        return *this;
    };

    FGTaxiRoute(const FGTaxiRoute& copy) :
            nodes(copy.nodes),
            routes(copy.routes),
            distance(copy.distance),
//    depth(copy.depth),
            currNode(nodes.begin()),
            currRoute(routes.begin())
    {};

    bool operator< (const FGTaxiRoute &other) const {
        return distance < other.distance;
    };
    bool empty () {
        return nodes.begin() == nodes.end();
    };
    bool next(int *nde);
    bool next(int *nde, int *rte);
    void rewind(int legNr);

    void first() {
        currNode = nodes.begin();
        currRoute = routes.begin();
    };
    int size() {
        return nodes.size();
    };
    int nodesLeft() {
        return nodes.end() - currNode;
    };

//  int getDepth() { return depth; };
};

typedef vector<FGTaxiRoute> TaxiRouteVector;
typedef vector<FGTaxiRoute>::iterator TaxiRouteVectorIterator;

/**************************************************************************************
 * class FGGroundNetWork
 *************************************************************************************/
class FGGroundNetwork : public FGATCController
{
private:
    bool hasNetwork;
    bool networkInitialized;
    time_t nextSave;
    //int maxDepth;
    int count;
    FGTaxiNodeVector    nodes;
    FGTaxiNodeVector    pushBackNodes;
    FGTaxiSegmentVector segments;
    //intVec route;
    //intVec nodesStack;
    //intVec routesStack;
    TaxiRouteVector routes;
    TrafficVector activeTraffic;
    TrafficVectorIterator currTraffic;

    bool foundRoute;
    double totalDistance, maxDistance;
    FGTowerController *towerController;
    FGAirport *parent;


    //void printRoutingError(string);

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
    bool exists() {
        return hasNetwork;
    };
    void setTowerController(FGTowerController *twrCtrlr) {
        towerController = twrCtrlr;
    };

    int findNearestNode(double lat, double lon);
    int findNearestNode(const SGGeod& aGeod);

    FGTaxiNode *findNode(unsigned idx);
    FGTaxiSegment *findSegment(unsigned idx);
    FGTaxiRoute findShortestRoute(int start, int end, bool fullSearch=true);
    //void trace(FGTaxiNode *, int, int, double dist);

    int getNrOfNodes() {
        return nodes.size();
    };

    void setParent(FGAirport *par) {
        parent = par;
    };

    virtual void announcePosition(int id, FGAIFlightPlan *intendedRoute, int currentRoute,
                                  double lat, double lon, double hdg, double spd, double alt,
                                  double radius, int leg, FGAIAircraft *aircraft);
    virtual void signOff(int id);
    virtual void updateAircraftInformation(int id, double lat, double lon, double heading, double speed, double alt, double dt);
    virtual bool hasInstruction(int id);
    virtual FGATCInstruction getInstruction(int id);

    bool checkTransmissionState(int minState, int MaxState, TrafficVectorIterator i, time_t now, AtcMsgId msgId,
                                AtcMsgDir msgDir);
    bool checkForCircularWaits(int id);
    virtual void render(bool);
    virtual string getName();
    virtual void update(double dt);

    void saveElevationCache();
};


#endif
