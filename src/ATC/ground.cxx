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

#include <simgear/math/sg_random.h>
#include <simgear/constants.h>

#include "ground.hxx"

FGGround::FGGround() {
	display = false;
}

FGGround::~FGGround() {
}

void FGGround::Init() {
	display = false;
	
	// Build hardwired (very simplified) logical network for KEMT for now
	// Once it works we'll progress to reading KEMT data from file,
	// and finally to reading any airport with specified taxiway data from file.

	node* np;
	arc* ap;
	Gate* gp;
	
	// For now we'll hardwire the threshold end
	Point3D P010(-118.037483, 34.081358, 296 * SG_FEET_TO_METER);
	double hdg = 25.32;
	ortho.Init(P010, hdg);

	// HARDWIRED FOR TESTING - for now we'll only allow exit at each end of runway

	///////////////////////////////////////////////////////
	// NODES
	///////////////////////////////////////////////////////

	// node - runway01 threshold
	Point3D p1(-118.0372167, 34.08178333, 0.0);
	np = new node;
	np->struct_type = NODE;
	np->pos = p1;
	np->orthoPos = ortho.ConvertToLocal(p1);
	np->name = "rwy 01";
	np->nodeID = 0;
	np->type = JUNCTION;
	runways[1].exits.push_back(np);
	runways[19].exits.push_back(np);
	//np->max_turn_radius = ...
	network.push_back(np);

	// node - runway19 threshold
	Point3D p2(-118.0321833, 34.09066667, 0.0);
	np = new node;
	np->struct_type = NODE;
	np->pos = p2;
	np->orthoPos = ortho.ConvertToLocal(p2);
	np->name = "rwy 19";
	np->nodeID = 1;
	np->type = JUNCTION;
	runways[1].exits.push_back(np);
	runways[19].exits.push_back(np);
	//np->max_turn_radius = ...
	network.push_back(np);

	// node - AlphaSouth
	Point3D p3(-118.0369167, 34.08166667, 0.0);
	np = new node;
	np->struct_type = NODE;
	np->pos = p3;
	np->orthoPos = ortho.ConvertToLocal(p3);
	np->name = "";
	np->nodeID = 2;
	np->type = HOLD;
	//np->max_turn_radius = ...
	network.push_back(np);

	// node - AlphaNorth
	Point3D p4(-118.03185, 34.0906, 0.0);
	np = new node;
	np->struct_type = NODE;
	np->pos = p4;
	np->orthoPos = ortho.ConvertToLocal(p4);
	np->name = "";
	np->nodeID = 3;
	np->type = HOLD;
	//np->max_turn_radius = ...
	network.push_back(np);

	// node - southern turnoff to parking
	Point3D p5(-118.03515, 34.0848, 0.0);
	np = new node;
	np->struct_type = NODE;
	np->pos = p5;
	np->orthoPos = ortho.ConvertToLocal(p5);
	np->name = "";
	np->nodeID = 4;
	np->type = TJUNCTION;
	//np->max_turn_radius = ...
	network.push_back(np);

	// node - northern turnoff to parking
	Point3D p6(-118.0349667, 34.08511667, 0.0);
	np = new node;
	np->struct_type = NODE;
	np->pos = p6;
	np->orthoPos = ortho.ConvertToLocal(p6);
	np->name = "";
	np->nodeID = 5;
	np->type = TJUNCTION;
	//np->max_turn_radius = ...
	network.push_back(np);

	// GATES

	// node - Turn into gate 1 (Western-most gate, ie. nearest rwy)
	Point3D p7(-118.0348333, 34.08466667, 0.0);
	np = new node;
	np->struct_type = NODE;
	np->pos = p7;
	np->orthoPos = ortho.ConvertToLocal(p7);
	np->name = "";
	np->nodeID = 6;
	np->type = TJUNCTION;
	//np->max_turn_radius = ...
	network.push_back(np);

	// node - Gate 1
	Point3D p8(-118.0347333, 34.08483333, 0.0);
	gp = new Gate;
	gp->struct_type = NODE;
	gp->pos = p8;
	gp->orthoPos = ortho.ConvertToLocal(p8);
	gp->name = "";
	gp->nodeID = 7;
	gp->type = GATE;
	gp->heading = 10;
	//np->max_turn_radius = ...
	network.push_back(gp);
	gates[1] = *gp;

	// node - Turn out of gate 1
	Point3D p9(-118.03465, 34.08498333, 0.0);
	np = new node;
	np->struct_type = NODE;
	np->pos = p9;
	np->orthoPos = ortho.ConvertToLocal(p9);
	np->name = "";
	np->nodeID = 8;
	np->type = TJUNCTION;
	//np->max_turn_radius = ...
	network.push_back(np);

	// node - Turn into gate 2
	Point3D p10(-118.0346, 34.08456667, 0.0);
	np = new node;
	np->struct_type = NODE;
	np->pos = p10;
	np->orthoPos = ortho.ConvertToLocal(p10);
	np->name = "";
	np->nodeID = 9;
	np->type = TJUNCTION;
	//np->max_turn_radius = ...
	network.push_back(np);
	gates[2] = *gp;

	// node - Gate 2
	Point3D p11(-118.0345167, 34.08473333, 0.0);
	gp = new Gate;
	gp->struct_type = NODE;
	gp->pos = p11;
	gp->orthoPos = ortho.ConvertToLocal(p11);
	gp->name = "";
	gp->nodeID = 10;
	gp->type = GATE;
	gp->heading = 10;
	//np->max_turn_radius = ...
	network.push_back(gp);

	// node - Turn out of gate 2	
	Point3D p12(-118.0344167, 34.0849, 0.0);
	np = new node;
	np->struct_type = NODE;
	np->pos = p12;
	np->orthoPos = ortho.ConvertToLocal(p12);
	np->name = "";
	np->nodeID = 11;
	np->type = TJUNCTION;
	//np->max_turn_radius = ...
	network.push_back(np);

	/////////////////////////////////////////////////////////
	// ARCS
	/////////////////////////////////////////////////////////

	// Each arc connects two nodes
	// Eventually the nodeID of the nodes that the arc connects will be read from file
	// For now we just 'know' them !!

	// arc - the runway - connects nodes 0 and 1
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = RUNWAY;
	ap->directed = false;
	ap->n1 = 0;
	ap->n2 = 1;
	network[0]->arcs.push_back(ap);
	network[1]->arcs.push_back(ap);

	// arc - the exit from 01 threshold to alpha - connects nodes 0 and 2
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = false;
	ap->n1 = 0;
	ap->n2 = 2;
	network[0]->arcs.push_back(ap);
	network[2]->arcs.push_back(ap);

	// arc - the exit from 19 threshold to alpha - connects nodes 1 and 3
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = false;
	ap->n1 = 1;
	ap->n2 = 3;
	network[1]->arcs.push_back(ap);
	network[3]->arcs.push_back(ap);

	// arc - Alpha south - connects nodes 2 and 4
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = false;
	ap->n1 = 2;
	ap->n2 = 4;
	network[2]->arcs.push_back(ap);
	network[4]->arcs.push_back(ap);

	// arc - Alpha middle - connects nodes 4 and 5
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = false;
	ap->n1 = 4;
	ap->n2 = 5;
	network[4]->arcs.push_back(ap);
	network[5]->arcs.push_back(ap);

	// arc - Alpha North - connects nodes 3 and 5
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = false;
	ap->n1 = 3;
	ap->n2 = 5;
	network[3]->arcs.push_back(ap);
	network[5]->arcs.push_back(ap);

	// arc - connects nodes 4 and 6
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = true;
	ap->n1 = 4;
	ap->n2 = 6;
	network[4]->arcs.push_back(ap);
	network[6]->arcs.push_back(ap);

	// arc - connects nodes 6 and 9
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = true;
	ap->n1 = 6;
	ap->n2 = 9;
	network[6]->arcs.push_back(ap);
	network[9]->arcs.push_back(ap);

	// arc - connects nodes 5 and 8
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = true;
	ap->n1 = 5;
	ap->n2 = 8;	
	network[5]->arcs.push_back(ap);
	network[8]->arcs.push_back(ap);

	// arc - connects nodes 8 and 11
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = true;
	ap->n1 = 8;
	ap->n2 = 11;
	network[8]->arcs.push_back(ap);
	network[11]->arcs.push_back(ap);

	// arc - connects nodes 6 and 7
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = true;
	ap->n1 = 6;
	ap->n2 = 7;
	network[6]->arcs.push_back(ap);
	network[7]->arcs.push_back(ap);

	// arc - connects nodes 7 and 8
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = true;
	ap->n1 = 7;
	ap->n2 = 8;
	network[7]->arcs.push_back(ap);
	network[8]->arcs.push_back(ap);

	// arc - connects nodes 9 and 10
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = true;
	ap->n1 = 9;
	ap->n2 = 10;
	network[9]->arcs.push_back(ap);
	network[10]->arcs.push_back(ap);

	// arc - connects nodes 10 and 11
	ap = new arc;
	ap->struct_type = ARC;
	ap->name = "";
	ap->type = TAXIWAY;
	ap->directed = true;
	ap->n1 = 10;
	ap->n2 = 11;
	network[10]->arcs.push_back(ap);
	network[11]->arcs.push_back(ap);
}

void FGGround::Update() {
	// Each time step, what do we need to do?
	// We need to go through the list of outstanding requests and acknowedgements
	// and process at least one of them.
	// We need to go through the list of planes under our control and check if
	// any need to be addressed.
	// We need to check for planes not under our control coming within our 
	// control area and address if necessary.
	
	// Lets take the example of a plane which has just contacted ground
	// following landing - presumably requesting where to go?
	// First we need to establish the position of the plane within the logical network.
	// Next we need to decide where its going. 
}

// FIXME - at the moment this assumes there is at least one gate and crashes if none
// FIXME - In fact, at the moment this routine doesn't work at all and hence is munged to always return Gate 1 !!!!
int FGGround::GetRandomGateID() {
	//cout << "GetRandomGateID called" << endl;
	return(1);

	gate_vec_type gateVec;
	//gate_vec_iterator gateVecItr;
	int num = 0;
	int thenum;
	int ID;
	
	gatesItr = gates.begin();
	while(gatesItr != gates.end()) {
		if(gatesItr->second.used == false) {
			gateVec.push_back(gatesItr->second);
			num++;
		}
		++gatesItr;
	}
	
	// Randomly select one from the list
	thenum = (int)(sg_random() * gateVec.size());
	ID = gateVec[thenum].id;
	//cout << "Returning gate ID " << ID << " from GetRandomGateID" << endl;
	return(ID);
}

// Return a pointer to a gate node based on the gate ID
Gate* FGGround::GetGateNode(int gateID) {
	//TODO - ought to add some sanity checking here - ie does a gate of this ID exist?!
	return(&(gates[gateID]));
}

// Get a path from a point on a runway to a gate

// Get a path from a node to another node
// Eventually we will need complex algorithms for this taking other traffic,
// shortest path and suitable paths into accout.  For now we're going to hardwire for KEMT!!!!
ground_network_path_type FGGround::GetPath(node* A, node* B) {
	ground_network_path_type path;
	//arc_array_iterator arcItr;
	//bool found;

	// VERY HARDWIRED - this hardwires a path from the far end of R01 to Gate 1.
	// In fact in real life the area between R01/19 and Taxiway Alpha at KEMT is tarmaced and planes
	// are supposed to exit the rwy asap.
	// OK - for now very hardwire this for testing
	path.push_back(network[1]);
	path.push_back(network[1]->arcs[1]);	// ONLY BECAUSE WE KNOW THIS IS THE ONE !!!!!
	path.push_back(network[3]);
	path.push_back(network[3]->arcs[1]);
	path.push_back(network[5]);
	path.push_back(network[5]->arcs[0]);
	path.push_back(network[4]);
	path.push_back(network[4]->arcs[2]);
	path.push_back(network[6]);
	path.push_back(network[6]->arcs[2]);
	path.push_back(network[7]);				// THE GATE!!  Note that for now we're not even looking at the requested exit and gate passed in !!!!!

#if 0	
	// In this hardwired scheme there are two possibilities - taxiing from rwy to gate or gate to rwy.
	if(B->type == GATE) {
		//return an inward path
		path.push_back(A);
		// In this hardwired scheme we know A is a rwy exit and should have one taxiway arc only
		// THIS WILL NOT HOLD TRUE IN THE GENERAL CASE
		arcItr = A->arcs.begin();
		found = false;
		while(arcItr != A->arcs.end()) {
			if(arcItr->type == TAXIWAY) {
				path.push_back(&(*arcItr));
				found = true;
				break;
			}
		}
		if(found == false) {
			//cout << "AI/ATC SUBSYSTEM ERROR - no taxiway from runway exit in airport.cxx" << endl;
		}
		// Then push back the start of taxiway node
		// Then push back the taxiway arc
		arcItr = A->arcs.begin();
		found = false;
		while(arcItr != A->arcs.end()) {
			if(arcItr->type == TAXIWAY) { // FIXME - OOPS - two taxiways go off this node
				// How are we going to differentiate, apart from one called Alpha.
				// I suppose eventually the traversal algorithms will select.
				path.push_back(&(*arcItr));
				found = true;
				break;
	    	}
		}
		if(found == false) {
	    	//cout << "AI/ATC SUBSYSTEM ERROR - no taxiway from runway exit in airport.cxx" << endl;
		}
		// Then push back the junction node
		// Planes always face one way in the parking, so depending on which parking exit we have either take it or push back another taxiway node
		// Repeat if necessary
		// Then push back the gate B
		path.push_back(B);
	} else {
		//return an outward path
	}	

	// WARNING TODO FIXME - this is VERY FRAGILE - eg taxi to apron!!! but should be enough to
	// see an AI plane physically taxi.
#endif	// 0
	
	return(path);
};


// Randomly or otherwise populate some of the gates with parked planes
// (might eventually be done by the AIMgr if and when lots of AI traffic is generated)

// Return a list of exits from a given runway
node_array_type FGGround::GetExits(int rwyID) {
	return(runways[rwyID].exits);
}
#if 0
void FGGround::NewArrival(plane_rec plane) {
	// What are we going to do here?
	// We need to start a new ground_rec and add the plane_rec to it
	// We need to decide what gate we are going to clear it to.
	// Then we need to add clearing it to that gate to the pending transmissions queue? - or simply transmit?
	// Probably simply transmit for now and think about a transmission queue later if we need one.
	// We might need one though in order to add a little delay for response time.
	ground_rec* g = new ground_rec;
	g->plane_rec = plane;
	g->current_pos = ConvertWGS84ToXY(plane.pos);
	g->node = GetNode(g->current_pos);  // TODO - might need to sort out node/arc here
	AssignGate(g);
	g->cleared = false;
	ground_traffic.push_back(g);
	NextClearance(g);
}

void FGGround::NewContact(plane_rec plane) {
	// This is a bit of a convienience function at the moment and is likely to change.
	if(at a gate or apron)
		NewDeparture(plane);
	else
		NewArrival(plane);
}

void FGGround::NextClearance(ground_rec &g) {
	// Need to work out where we can clear g to.
	// Assume the pilot doesn't need progressive instructions
	// We *should* already have a gate or holding point assigned by the time we get here
	// but it wouldn't do any harm to check.
	
	// For now though we will hardwire it to clear to the final destination.
}

void FGGround::AssignGate(ground_rec &g) {
	// We'll cheat for now - since we only have the user's aircraft and a couple of airports implemented
	// we'll hardwire the gate!
	// In the long run the logic of which gate or area to send the plane to could be somewhat non-trivial.
}
#endif //0