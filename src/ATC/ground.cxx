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

#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/constants.h>
#include <Main/globals.hxx>

#include <stdlib.h>
#include STL_FSTREAM

#include "ground.hxx"

FGGround::FGGround() {
	display = false;
	networkLoadOK = false;
}

FGGround::~FGGround() {
}

void FGGround::ParseRwyExits(node* np, char* es) {
	char* token;
	char estr[20];
	strcpy(estr, es);
	const char delimiters[] = "-";
	token = strtok(estr, delimiters);
	while(token != NULL) {
		int i = atoi(token);
		//cout << "token = " << token << endl;
		//cout << "rwy number = " << i << endl;
		//runways[(atoi(token))].exits.push_back(np);
		runways[i].exits.push_back(np);
		//cout << "token = " << token << '\n';
		token = strtok(NULL, delimiters);
	}
}
	

// Load the ground logical network of the current instances airport
// Return true if successfull.
// TODO - currently the file is assumed to reside in the base/ATC directory.
// This might change to something more thought out in the future.
bool FGGround::LoadNetwork() {
	node* np;
	arc* ap;
	Gate* gp;
	
	ifstream fin;
	SGPath path = globals->get_fg_root();
	//string taxiPath = "ATC/" + ident + ".taxi";
	string taxiPath = "ATC/KEMT.taxi";	// FIXME - HARDWIRED FOR TESTING
	path.append(taxiPath);
	
	SG_LOG(SG_GENERAL, SG_INFO, "Trying to read taxiway data for " << ident << "...");
	//cout << "Trying to read taxiway data for " << ident << "..." << endl;
	fin.open(path.c_str(), ios::in);
	if(!fin) {
		SG_LOG(SG_GENERAL, SG_ALERT, "Unable to open taxiway data input file " << path.c_str());
		//cout << "Unable to open taxiway data input file " << path.c_str() << endl;
		return(false);
	}
	
	char ch;
	char buf[30];
	while(!fin.eof()) {
		fin >> buf;
		// Node, arc, or [End]?
		//cout << "Read in ground network element type = " << buf << endl;
		if(!strcmp(buf, "[End]")) {		// TODO - maybe make this more robust to spelling errors by just looking for '['
			cout << "Done reading " << path.c_str() << endl;
			break;
		} else if(!strcmp(buf, "N")) {
			// Node
			np = new node;
			np->struct_type = NODE;
			fin >> buf;
			np->nodeID = atoi(buf);
			fin >> buf;
			np->pos.setlon(atof(buf));
			fin >> buf;
			np->pos.setlat(atof(buf));
			fin >> buf;
			np->pos.setelev(atof(buf));
			fin >> buf;		// node type
			if(!strcmp(buf, "J")) {
				np->type = JUNCTION;
			} else if(!strcmp(buf, "T")) {
				np->type = TJUNCTION;
			} else if(!strcmp(buf, "H")) {
				np->type = HOLD;
			} else {
				cout << "**** ERROR ***** Unknown node type in taxi network...\n";
				delete np;
				return(false);
			}
			fin >> buf;		// rwy exit information - gets parsed later - FRAGILE - will break if buf is reused.
			// Now the name
			np->name = "";
			while(1) {
				fin.unsetf(ios::skipws);
				fin >> ch;
				np->name += ch;
				if((ch == '"') || (ch == 0x0A)) {
					break;
				}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
			}
			fin.setf(ios::skipws);
			network.push_back(np);
			// FIXME - fragile - replies on buf not getting modified from exits read to here
			// see if we also need to push it onto the runway exit list
			cout << "strlen(buf) = " << strlen(buf) << endl;
			if(strlen(buf) > 2) {
				cout << "Calling ParseRwyExits for " << buf << endl;
				ParseRwyExits(np, buf);
			}
		} else if(!strcmp(buf, "A")) {
			ap = new arc;
			ap->struct_type = ARC;
			fin >> buf;
			ap->n1 = atoi(buf);
			fin >> buf;
			ap->n2 = atoi(buf);
			fin >> buf;
			if(!strcmp(buf, "R")) {
				ap->type = RUNWAY;
			} else if(!strcmp(buf, "T")) {
				ap->type = TAXIWAY;
			} else {
				cout << "**** ERROR ***** Unknown arc type in taxi network...\n";
				delete ap;
				return(false);
			}
			// directed?
			fin >> buf;
			if(!strcmp(buf, "Y")) {
				ap->directed = true;
			} else if(!strcmp(buf, "N")) {
				ap->directed = false;
			} else {
				cout << "**** ERROR ***** Unknown arc directed value in taxi network - should be Y/N !!!\n";
				delete ap;
				return(false);
			}			
			// Now the name
			ap->name = "";
			while(1) {
				fin.unsetf(ios::skipws);
				fin >> ch;
				ap->name += ch;
				if((ch == '"') || (ch == 0x0A)) {
					break;
				}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
			}
			fin.setf(ios::skipws);
			network[ap->n1]->arcs.push_back(ap);
			network[ap->n2]->arcs.push_back(ap);
		} else if(!strcmp(buf, "G")) {
			gp = new Gate;
			gp->struct_type = NODE;
			gp->type = GATE;
			fin >> buf;
			gp->nodeID = atoi(buf);
			fin >> buf;
			gp->pos.setlon(atof(buf));
			fin >> buf;
			gp->pos.setlat(atof(buf));
			fin >> buf;
			gp->pos.setelev(atof(buf));
			fin >> buf;		// gate type - ignore this for now
			fin >> buf;		// gate heading
			gp->heading = atoi(buf);
			// Now the name
			gp->name = "";
			while(1) {
				fin.unsetf(ios::skipws);
				fin >> ch;
				gp->name += ch;
				if((ch == '"') || (ch == 0x0A)) {
					break;
				}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
			}
			fin.setf(ios::skipws);
			network.push_back(gp);
		} else {
			// Something has gone seriously pear-shaped
			cout << "********* ERROR - unknown ground network element type... aborting read of " << path.c_str() << '\n';
			return(false);
		}
		
		fin >> skipeol;		
	}
	return(true);
}

void FGGround::Init() {
	display = false;
	
	// For now we'll hardwire the threshold end
	Point3D P010(-118.037483, 34.081358, 296 * SG_FEET_TO_METER);
	double hdg = 25.32;
	ortho.Init(P010, hdg);
	
	networkLoadOK = LoadNetwork();
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
// It is up to the calling function to check for non-zero size of returned array before use
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