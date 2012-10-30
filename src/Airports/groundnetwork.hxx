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

#include <string>
#include <vector>
#include <list>
#include <map>

#include "gnnode.hxx"
#include "parking.hxx"
#include <ATC/trafficcontrol.hxx>


class Block;
class FGRunway;
class FGTaxiSegment; // forward reference
class FGAIFlightPlan; // forward reference
class FGAirport;      // forward reference

typedef std::vector<FGTaxiSegment*>  FGTaxiSegmentVector;
typedef FGTaxiSegmentVector::iterator FGTaxiSegmentVectorIterator;

typedef std::map<int, FGTaxiNode_ptr> IndexTaxiNodeMap;

class Block
{
private:
    int id;
    time_t blocktime;
    time_t touch;
public:
    Block(int i, time_t bt, time_t curr) { id = i; blocktime= bt; touch = curr; };
    ~Block() {};
    int getId() { return id; };
    void updateTimeStamps(time_t bt, time_t now) { blocktime = (bt < blocktime) ? bt : blocktime; touch = now; };
    const time_t getBlockTime() const { return blocktime; };
    time_t getTimeStamp() { return touch; };
    bool operator< (const Block &other) const { return blocktime < other.blocktime; };
};

typedef std::vector<Block> BlockList;
typedef BlockList::iterator BlockListIterator;

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
    bool isActive;
    bool isPushBackRoute;
    BlockList blockTimes;
    FGTaxiNode *start;
    FGTaxiNode *end;
    int index;
    FGTaxiSegment *oppositeDirection;

public:
  FGTaxiSegment(int start, int end, bool isPushBack);
  
    void setIndex        (int val) {
        index     = val;
    };

    void setOpposite(FGTaxiSegment *opp) {
        oppositeDirection = opp;
    };

    bool bindToNodes(const IndexTaxiNodeMap& nodes);
  
    void setDimensions(double elevation);
    void block(int id, time_t blockTime, time_t now);
    void unblock(time_t now); 
    bool hasBlock(time_t now);

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
  
    // compute the center of the arc
    SGGeod getCenter() const;
  
    double getHeading()   {
        return heading;
    };
    bool isPushBack() {
        return isPushBackRoute;
    };

    int getPenalty(int nGates);

    bool operator<(const FGTaxiSegment &other) const {
        return index < other.index;
    };
    //bool hasSmallerHeadingDiff (const FGTaxiSegment &other) const { return headingDiff < other.headingDiff; };
    FGTaxiSegment *opposite() {
        return oppositeDirection;
    };
};




typedef std::vector<int> intVec;
typedef std::vector<int>::iterator intVecIterator;



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

typedef std::vector<FGTaxiRoute> TaxiRouteVector;
typedef std::vector<FGTaxiRoute>::iterator TaxiRouteVectorIterator;

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
    int version;
  
    IndexTaxiNodeMap nodes;
    FGTaxiNodeVector pushBackNodes;
  
    FGTaxiSegmentVector segments;

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


    void parseCache();
public:
    FGGroundNetwork();
    ~FGGroundNetwork();

    void addNode   (FGTaxiNode* node);
    void addNodes  (FGParkingVec *parkings);
    void addSegment(FGTaxiSegment* seg);
    void setVersion (int v) { version = v;};
    
    int getVersion() { return version; };

    void init();
    bool exists() {
        return hasNetwork;
    };
    void setTowerController(FGTowerController *twrCtrlr) {
        towerController = twrCtrlr;
    };

    int findNearestNode(const SGGeod& aGeod);
    int findNearestNodeOnRunway(const SGGeod& aGeod, FGRunway* aRunway = NULL);

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
    virtual std::string getName();
    virtual void update(double dt);

    void saveElevationCache();
    void addVersion(int v) {version = v; };
};


#endif
