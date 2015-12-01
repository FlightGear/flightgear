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

#include "gnnode.hxx"
#include "parking.hxx"
#include <ATC/trafficcontrol.hxx>

class FGAirportDynamicsXMLLoader;

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

/***************************************************************************************
 * class FGTaxiSegment
 **************************************************************************************/
class FGTaxiSegment
{
private:
    // weak (non-owning) pointers deliberately here:
    // the ground-network owns the nodes
    const FGTaxiNode* startNode;
    const FGTaxiNode* endNode;
    
    bool isActive;
    BlockList blockTimes;

    int index;
    FGTaxiSegment *oppositeDirection; // also deliberatley weak

    friend class FGGroundNetwork;
public:
    FGTaxiSegment(FGTaxiNode* start, FGTaxiNode* end);
  
    void setIndex        (int val) {
        index     = val;
    };
  
    void setDimensions(double elevation);
    void block(int id, time_t blockTime, time_t now);
    void unblock(time_t now); 
    bool hasBlock(time_t now);

    FGTaxiNodeRef getEnd() const;
    FGTaxiNodeRef getStart() const;
  
    double getLength() const;
  
    // compute the center of the arc
    SGGeod getCenter() const;
  
    double getHeading() const;
    
    int getIndex() {
      return index;
    };

    int getPenalty(int nGates);

    bool operator<(const FGTaxiSegment &other) const {
        return index < other.index;
    };

    FGTaxiSegment *opposite() {
        return oppositeDirection;
    };
};

/***************************************************************************************
 * class FGTaxiRoute
 **************************************************************************************/
class FGTaxiRoute
{
private:
    FGTaxiNodeVector nodes;
    intVec routes;
    double distance;
    FGTaxiNodeVector::iterator currNode;
    intVec::iterator currRoute;

public:
    FGTaxiRoute() {
        distance = 0;
        currNode = nodes.begin();
        currRoute = routes.begin();
    };
  
    FGTaxiRoute(const FGTaxiNodeVector& nds, intVec rts,  double dist, int dpth) {
        nodes = nds;
        routes = rts;
        distance = dist;
        currNode = nodes.begin();
        currRoute = routes.begin();
    };

    FGTaxiRoute& operator= (const FGTaxiRoute &other) {
        nodes = other.nodes;
        routes = other.routes;
        distance = other.distance;
        currNode = nodes.begin();
        currRoute = routes.begin();
        return *this;
    };

    FGTaxiRoute(const FGTaxiRoute& copy) :
            nodes(copy.nodes),
            routes(copy.routes),
            distance(copy.distance),
            currNode(nodes.begin()),
            currRoute(routes.begin())
    {};

    bool operator< (const FGTaxiRoute &other) const {
        return distance < other.distance;
    };
    bool empty () {
        return nodes.empty();
    };
    bool next(FGTaxiNodeRef& nde, int *rte);
  
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
};

/**************************************************************************************
 * class FGGroundNetWork
 *************************************************************************************/
class FGGroundNetwork : public FGATCController
{
private:
    friend class FGAirportDynamicsXMLLoader;

    bool hasNetwork;
    bool networkInitialized;
    time_t nextSave;
    //int maxDepth;
    int count;
    int version;
  
    FGTaxiSegmentVector segments;

    TrafficVector activeTraffic;
    TrafficVectorIterator currTraffic;

    double totalDistance, maxDistance;
    FGTowerController *towerController;
    FGAirport *parent;

    FGParkingList m_parkings;
    FGTaxiNodeVector m_nodes;

    FGTaxiNodeRef findNodeByIndex(int index) const;

    //void printRoutingError(string);

    void checkSpeedAdjustment(int id, double lat, double lon,
                              double heading, double speed, double alt);
    void checkHoldPosition(int id, double lat, double lon,
                           double heading, double speed, double alt);


    void addSegment(const FGTaxiNodeRef& from, const FGTaxiNodeRef& to);
    void addParking(const FGParkingRef& park);

    FGTaxiNodeVector segmentsFrom(const FGTaxiNodeRef& from) const;

public:
    FGGroundNetwork();
    ~FGGroundNetwork();
    
    void setVersion (int v) { version = v;};
    int getVersion() { return version; };

    void init(FGAirport* pr);
    bool exists() {
        return hasNetwork;
    };
    void setTowerController(FGTowerController *twrCtrlr) {
        towerController = twrCtrlr;
    };

    FGTaxiNodeRef findNearestNode(const SGGeod& aGeod) const;
    FGTaxiNodeRef findNearestNodeOnRunway(const SGGeod& aGeod, FGRunway* aRunway = NULL) const;

    FGTaxiSegment *findSegment(unsigned int idx) const;

    const FGParkingList& allParkings() const;

    /**
     * Find the taxiway segment joining two (ground-net) nodes. Returns
     * NULL if no such segment exists.
     * It is permitted to pass HULL for the 'to' indicating that any
     * segment originating at 'from' is acceptable.
     */
    FGTaxiSegment *findSegment(const FGTaxiNode* from, const FGTaxiNode* to) const;
  
    FGTaxiRoute findShortestRoute(FGTaxiNode* start, FGTaxiNode* end, bool fullSearch=true);

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

    void addVersion(int v) {version = v; };
};


#endif
