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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef _FG_GROUND_HXX
#define _FG_GROUND_HXX

#include STL_IOSTREAM
#include STL_STRING

SG_USING_STD(string);
SG_USING_STD(ios);

#include <map>
#include <vector>
#include <list>
#include <simgear/math/point3d.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "ATC.hxx"
#include "ATCProjection.hxx"

SG_USING_STD(map);
SG_USING_STD(vector);
SG_USING_STD(list);

//////////////////////////////////////////////////////
// Types for the logical network data structure
typedef enum arc_type {
	RUNWAY,
	TAXIWAY
};

typedef enum node_type {
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

typedef enum network_element_type {
	NODE,
	ARC
};

struct ground_network_element {
	network_element_type struct_type;
};

struct arc : public ground_network_element {
	int distance;
	string name;
	arc_type type;
	bool directed;	//false if 2-way, true if 1-way.  
	//This is a can of worms since arcs might be one way in different directions under different circumstances
	unsigned int n1;	// The nodeID of the first node
	unsigned int n2;	// The nodeID of the second node
	// If the arc is directed then flow is normally from n1 to n2.  See the above can of worms comment though.
};

typedef vector <arc*> arc_array_type;	// This was and may become again a list instead of vector
typedef arc_array_type::iterator arc_array_iterator;
typedef arc_array_type::const_iterator arc_array_const_iterator; 

struct node : public ground_network_element {
	node();
	~node();
	
	unsigned int nodeID;	//each node in an airport needs a unique ID number - this is ZERO-BASED to match array position
	Point3D pos;
	Point3D orthoPos;
	char* name;
	node_type type;
	arc_array_type arcs;
	double max_turn_radius;
};

typedef vector <node*> node_array_type;
typedef node_array_type::iterator node_array_iterator;
typedef node_array_type::const_iterator node_array_const_iterator;

struct Gate : public node {
	GateType gateType;
	int max_weight;	//units??
	//airline_code airline;	//For the future - we don't have any airline codes ATM
	int id;	// The gate number in the logical scheme of things
	string sid;	// The real-world gate letter/number
	//node* pNode;
	bool used;
	double heading;	// The direction the parked-up plane should point in degrees
};

typedef vector < Gate* > gate_vec_type;
typedef gate_vec_type::iterator gate_vec_iterator;
typedef gate_vec_type::const_iterator gate_vec_const_iterator;

// A map of gate vs. the logical (internal FGFS) gate ID
typedef map < int, Gate* > gate_map_type;
typedef gate_map_type::iterator gate_map_iterator;
typedef gate_map_type::const_iterator gate_map_const_iterator;

// Runways - all the runway stuff is likely to change in the future
typedef struct Rwy {
	int id;	//note this is a very simplified scheme for now - R & L are not differentiated
	//It should work for simple one rwy airports
	node_array_type exits;	//Array of available exits from runway
	// should probably add an FGRunway structure here as well eventually
	// Eventually we will also want some encoding of real-life preferred runways
	// This will get us up and running for single runway airports though.
};
typedef vector < Rwy > runway_array_type;
typedef runway_array_type::iterator runway_array_iterator;
typedef runway_array_type::const_iterator runway_array_const_iterator;

// end logical network types
///////////////////////////////////////////////////////

///////////////////////////////////////////////////////
// Structures to use the network

// A path through the network 
typedef vector < ground_network_element* > ground_network_path_type;
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
typedef map < unsigned int, a_path* > shortest_path_map_type;
typedef shortest_path_map_type::iterator shortest_path_map_iterator;

// Nodes mapped by their ID
//typedef map < unsigned int, node* > node_map_type;
//typedef node_map_type::iterator node_map_iterator;
////////////////////////////////////////////////

// Planes active within the ground network.
// somewhere in the ATC/AI system we are going to have defined something like
// typedef struct plane_rec
// list <PlaneRec> plane_rec_list_type
/*
// A more specialist plane rec to include ground information
typedef struct ground_rec {
    plane_rec plane;
    point current_pos;
    node destination;
    node last_clearance;
    bool cleared;  // set true when the plane has been cleared to somewhere
    bool incoming; //true for arrivals, false for departures
    // status?
    // Almost certainly need to add more here
};

typedef list<ground_rec*> ground_rec_list;
typedef ground_rec_list::iterator ground_rec_list_itr;
typedef ground_rec_list::const_iterator ground_rec_list_const_itr;
*/
//////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// FGGround
//
///////////////////////////////////////////////////////////////////////////////
class FGGround : public FGATC {

public:
	FGGround();
	~FGGround();
    void Init();

    void Update();
	
	inline string get_trans_ident() { return trans_ident; }
	inline atc_type GetType() { return GROUND; }
    inline void SetDisplay() {display = true;}
    inline void SetNoDisplay() {display = false;}

    // Its possible that NewArrival and NewDeparture should simply be rolled into Request.

    // Contact ground control on arrival, assumed to request any gate
    //void NewArrival(plane_rec plane);

    // Contact ground control on departure, assumed to request currently active runway.
    //void NewDeparture(plane_rec plane);

    // Contact ground control when the calling routine doesn't know if arrival
    // or departure is appropriate.
    //void NewContact(plane_rec plane);

    // Make a request of ground control.
    //void Request(ground_request request);
	
	// Randomly fill some of the available gates and GA parking spots with planes
	void PopulateGates();
	
	// Return a suitable gate (maybe this should be a list of suitable gates so the plane or controller can choose the closest one)
	void ReturnGate(Gate &gate, GateType type);
	
	// Return a pointer to an unused gate
	Gate* GetGateNode();
	
	// Runway stuff - this might change in the future.
	// Get a list of exits from a given runway
	// It is up to the calling function to check for non-zero size of returned array before use
	node_array_type GetExits(int rwyID);
	
	// Get a path from one node to another
	ground_network_path_type GetPath(node* A, node* B); 

private:

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
	
	// Runway stuff - this might change in the future.
	//runway_array_type runways;	// STL way
	Rwy runways[36];	// quick hack!

	FGATCAlignedProjection ortho;
	
	// Planes currently active
    //ground_rec_list ground_traffic;

    // Find the shortest route through the logical network between two points.
    //FindShortestRoute(point a, point b);

    // Project a point in WGS84 lat/lon onto the local gnomonic.
    //ConvertWGS84ToXY(sgVec3 wgs84, point xy);

    // Assign a gate or parking location to a new arrival
    //AssignGate(ground_rec &g);

    // Generate the next clearance for an airplane
    //NextClearance(ground_rec &g);
	
	bool display;		// Flag to indicate whether we should be outputting to the ATC display.
	bool displaying;		// Flag to indicate whether we are outputting to the ATC display.
	// for failure modeling
	string trans_ident;		// transmitted ident
	bool ground_failed;		// ground failed?
	bool networkLoadOK;		// Indicates whether LoadNetwork returned true or false at last attempt
	
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
	
	// A shortest path algorithm sort of from memory (I can't find the bl&*dy book again!)
	ground_network_path_type GetShortestPath(node* A, node* B); 
};

#endif	// _FG_GROUND_HXX

