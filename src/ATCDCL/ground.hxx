// FGGround - a class to provide ground control at larger airports.
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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

#ifndef _FG_GROUND_HXX
#define _FG_GROUND_HXX

#include <map>
#include <vector>
#include <list>

#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/props/props.hxx>

#include "ATC.hxx"
#include "ATCProjection.hxx"

#include <string>

class FGATCMgr;

//////////////////////////////////////////////////////
// Types for the logical network data structure
enum arc_type {
	RUNWAY,
	TAXIWAY
};

enum node_type {
	GATE,
	APRON,
	HOLD,
	JUNCTION,
	TJUNCTION
};

enum GateType {
	TRANSPORT_PASSENGER,
	TRANSPORT_PASSENGER_NARROWBODY,
	TRANSPORT_PASSENGER_WIDEBODY,
	TRANSPORT_CARGO,
	GA_LOCAL,
	GA_LOCAL_SINGLE,
	GA_LOCAL_TWIN,
	GA_TRANSIENT,
	GA_TRANSIENT_SINGLE,
	GA_TRANSIENT_TWIN,
	OTHER	// ie. anything goes!!
};

enum network_element_type {
	NODE,
	ARC
};

struct ground_network_element {
	network_element_type struct_type;
};

struct arc : public ground_network_element {
	int distance;
	std::string name;
	arc_type type;
	bool directed;	//false if 2-way, true if 1-way.  
	//This is a can of worms since arcs might be one way in different directions under different circumstances
	unsigned int n1;	// The nodeID of the first node
	unsigned int n2;	// The nodeID of the second node
	// If the arc is directed then flow is normally from n1 to n2.  See the above can of worms comment though.
};

typedef std::vector <arc*> arc_array_type;	// This was and may become again a list instead of vector
typedef arc_array_type::iterator arc_array_iterator;
typedef arc_array_type::const_iterator arc_array_const_iterator; 

struct node : public ground_network_element {
	node();
	~node();
	
	unsigned int nodeID;	//each node in an airport needs a unique ID number - this is ZERO-BASED to match array position
	SGGeod pos;
	SGVec3d orthoPos;
	std::string name;
	node_type type;
	arc_array_type arcs;
	double max_turn_radius;
};

typedef std::vector <node*> node_array_type;
typedef node_array_type::iterator node_array_iterator;
typedef node_array_type::const_iterator node_array_const_iterator;

struct Gate : public node {
	GateType gateType;
	int max_weight;	//units??
	//airline_code airline;	//For the future - we don't have any airline codes ATM
	int id;	// The gate number in the logical scheme of things
	std::string name;	// The real-world gate letter/number
	//node* pNode;
	bool used;
	double heading;	// The direction the parked-up plane should point in degrees
};

typedef std::vector < Gate* > gate_vec_type;
typedef gate_vec_type::iterator gate_vec_iterator;
typedef gate_vec_type::const_iterator gate_vec_const_iterator;

// A map of gate vs. the logical (internal FGFS) gate ID
typedef std::map < int, Gate* > gate_map_type;
typedef gate_map_type::iterator gate_map_iterator;
typedef gate_map_type::const_iterator gate_map_const_iterator;

// Runways - all the runway stuff is likely to change in the future
struct Rwy {
	int id;	//note this is a very simplified scheme for now - R & L are not differentiated
	//It should work for simple one rwy airports
	node_array_type exits;	//Array of available exits from runway
	// should probably add an FGRunway structure here as well eventually
	// Eventually we will also want some encoding of real-life preferred runways
	// This will get us up and running for single runway airports though.
};

typedef std::vector < Rwy > runway_array_type;
typedef runway_array_type::iterator runway_array_iterator;
typedef runway_array_type::const_iterator runway_array_const_iterator;

// end logical network types
///////////////////////////////////////////////////////

///////////////////////////////////////////////////////
// Structures to use the network

// A path through the network 
typedef std::vector < ground_network_element* > ground_network_path_type;
typedef ground_network_path_type::iterator ground_network_path_iterator;
typedef ground_network_path_type::const_iterator ground_network_path_const_iterator;

//////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////
//
// Stuff for the shortest-path algorithms
struct a_path {
	a_path();
	
	ground_network_path_type path;
	int cost;
};

// Paths mapped by nodeID reached so-far
typedef std::map < unsigned int, a_path* > shortest_path_map_type;
typedef shortest_path_map_type::iterator shortest_path_map_iterator;

// Nodes mapped by their ID
//typedef map < unsigned int, node* > node_map_type;
//typedef node_map_type::iterator node_map_iterator;
////////////////////////////////////////////////

// Planes active within the ground network.

// A more specialist plane rec to include ground information
struct GroundRec {	
    PlaneRec plane;
    node* destination;
    node* last_clearance;
    bool taxiRequestOutstanding;	// Plane has requested taxi and we haven't responded yet
    double clearanceCounter;		// Hack for communication timing - counter since clearance requested in seconds 
	
    bool cleared;  // set true when the plane has been cleared to somewhere
    bool incoming; //true for arrivals, false for departures
    // status?
    // Almost certainly need to add more here
};

typedef std::list < GroundRec* > ground_rec_list;
typedef ground_rec_list::iterator ground_rec_list_itr;
typedef ground_rec_list::const_iterator ground_rec_list_const_itr;

///////////////////////////////////////////////////////////////////////////////
//
// FGGround
//
///////////////////////////////////////////////////////////////////////////////
class FGGround : public FGATC {

public:
	FGGround();
	FGGround(const std::string& id);
	~FGGround();
    void Init();

    void Update(double dt);
	
	inline const std::string& get_trans_ident() { return trans_ident; }
	
	// Randomly fill some of the available gates and GA parking spots with planes
	void PopulateGates();
	
	// Return a suitable gate (maybe this should be a list of suitable gates so the plane or controller can choose the closest one)
	void ReturnGate(Gate &gate, GateType type);
	
	// Return a pointer to an unused gate
	Gate* GetGateNode();
	
	// Return a pointer to a hold short node
	node* GetHoldShortNode(const std::string& rwyID);
	
	// Runway stuff - this might change in the future.
	// Get a list of exits from a given runway
	// It is up to the calling function to check for non-zero size of returned array before use
	node_array_type GetExits(const std::string& rwyID);
	
	// Get a path from one node to another
	ground_network_path_type GetPath(node* A, node* B);
	
	// Get a path from a node to a runway threshold
	ground_network_path_type GetPath(node* A, const std::string& rwyID);
	
	// Get a path from a node to a runway hold short point
	// Bit of a hack this at the moment!
	ground_network_path_type GetPathToHoldShort(node* A, const std::string& rwyID);

private:
	FGATCMgr* ATCmgr;	
	// This is purely for synactic convienience to avoid writing globals->get_ATC_mgr()-> all through the code!
	
    // Need a data structure to hold details of the various active planes
    // Need a data structure to hold details of the logical network
    // including which gates are filled - or possibly another data structure
    // with the positions of the inactive planes.
    // Need a data structure to hold outstanding communications from aircraft.
    // Possibly need a data structure to hold outstanding communications to aircraft.

	// The logical network
	// NODES WILL BE STORED IN THE NETWORK IN ORDER OF nodeID NUMBER
	// ie. NODE 5 WILL BE AT network[5]
    node_array_type network;
	
	// A map of all the gates indexed against internal (FGFS) ID
	gate_map_type gates;
	gate_map_iterator gatesItr;

	FGATCAlignedProjection ortho;
	
	// Planes currently active
    //ground_rec_list ground_traffic;

    // Find the shortest route through the logical network between two points.
    //FindShortestRoute(point a, point b);

    // Assign a gate or parking location to a new arrival
    //AssignGate(ground_rec &g);

    // Generate the next clearance for an airplane
    //NextClearance(ground_rec &g);
	
	// environment - need to make sure we're getting the surface winds and not winds aloft.
	SGPropertyNode_ptr wind_from_hdg;	//degrees
	SGPropertyNode_ptr wind_speed_knots;		//knots
	
	// for failure modeling
	std::string trans_ident;		// transmitted ident
	bool ground_failed;		// ground failed?
	bool networkLoadOK;		// Indicates whether LoadNetwork returned true or false at last attempt
	
	// Tower control
	bool untowered;		// True if this is an untowered airport (we still need the ground class for shortest path implementation etc
	//FGATC* tower;		// Pointer to the tower control

	// Logical runway details - this might change in the future.
	//runway_array_type runways;	// STL way
	Rwy runways[37];	// quick hack!
	
	// Physical runway details
	double aptElev;		// Airport elevation
	std::string activeRwy;	// Active runway number - For now we'll disregard multiple / alternate runway operation.
	RunwayDetails rwy;	// Assumed to be the active one for now.// Figure out which runways are active.
	
	// For now we'll just be simple and do one active runway - eventually this will get much more complex
	// Copied from FGTower - TODO - it would be better to implement this just once, and have ground call tower
	// for runway operation details, but at the moment we can't guarantee that tower control at a given airport
	// will be initialised before ground so we can't do that.
	void DoRwyDetails();	
	
	// Load the logical ground network for this airport from file.
	// Return true if successfull.
	bool LoadNetwork();
	
	// Parse a runway exit string and push the supplied node pointer onto the runway exit list
	void ParseRwyExits(node* np, char* es);
	
	// Return a random gate ID of an unused gate.
	// Two error values may be returned and must be checked for by the calling function:
	// -2 signifies that no gates exist at this airport.
	// -1 signifies that all gates are currently full.
	// TODO - modify to return a suitable gate based on aircraft size/weight.
	int GetRandomGateID();
	
	// Return a pointer to the node at a runway threshold
	// Returns NULL if unsuccessful.
	node* GetThresholdNode(const std::string& rwyID);
	
	// A shortest path algorithm from memory (I can't find the bl&*dy book again!)
	ground_network_path_type GetShortestPath(node* A, node* B); 
	
	// Planes
	ground_rec_list ground_traffic;
	ground_rec_list_itr ground_traffic_itr;
};

#endif	// _FG_GROUND_HXX

