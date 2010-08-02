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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>

#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/constants.h>
#include <Main/globals.hxx>

#include <stdlib.h>
#include <fstream>

#include "ground.hxx"
#include "ATCutils.hxx"
#include "ATCmgr.hxx"

using std::ifstream;
using std::cout;

node::node() {
}

node::~node() {
	for(unsigned int i=0; i < arcs.size(); ++i) {
		delete arcs[i];
	}
}

// Make sure that a_path.cost += distance is safe from the moment it's created.
a_path::a_path() {
	cost = 0;
}

FGGround::FGGround() {
	ATCmgr = globals->get_ATC_mgr();
	_type = GROUND;
	networkLoadOK = false;
	ground_traffic.erase(ground_traffic.begin(), ground_traffic.end());
	ground_traffic_itr = ground_traffic.begin();
	
	// Init the property nodes - TODO - need to make sure we're getting surface winds.
	wind_from_hdg = fgGetNode("/environment/wind-from-heading-deg", true);
	wind_speed_knots = fgGetNode("/environment/wind-speed-kt", true);
	
	// TODO - get the actual airport elevation
	aptElev = 0.0;
}

FGGround::FGGround(const string& id) {
	ATCmgr = globals->get_ATC_mgr();
	networkLoadOK = false;
	ground_traffic.erase(ground_traffic.begin(), ground_traffic.end());
	ground_traffic_itr = ground_traffic.begin();
	ident = id;
	
	// Init the property nodes - TODO - need to make sure we're getting surface winds.
	wind_from_hdg = fgGetNode("/environment/wind-from-heading-deg", true);
	wind_speed_knots = fgGetNode("/environment/wind-speed-kt", true);
	
	// TODO - get the actual airport elevation
	aptElev = 0.0;
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
// NOTE - currently it is assumed that all nodes are loaded before any arcs.
// It won't work ATM if this doesn't hold true.
bool FGGround::LoadNetwork() {
	node* np;
	arc* ap;
	Gate* gp;
	
	int gateCount = 0;	// This is used to allocate gateID's from zero upwards
	// This may well change in the future - probably to reading in the real-world
	// gate numbers from file.
	
	ifstream fin;
	SGPath path = globals->get_fg_root();
	//string taxiPath = "ATC/" + ident + ".taxi";
	string taxiPath = "ATC/KEMT.taxi";	// FIXME - HARDWIRED FOR TESTING
	path.append(taxiPath);
	
	SG_LOG(SG_ATC, SG_INFO, "Trying to read taxiway data for " << ident << "...");
	//cout << "Trying to read taxiway data for " << ident << "..." << endl;
	fin.open(path.c_str(), ios::in);
	if(!fin) {
		SG_LOG(SG_ATC, SG_ALERT, "Unable to open taxiway data input file " << path.c_str());
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
			SG_LOG(SG_ATC, SG_INFO, "Done reading " << path.c_str() << endl);
			break;
		} else if(!strcmp(buf, "N")) {
			// Node
			np = new node;
			np->struct_type = NODE;
			fin >> buf;
			np->nodeID = atoi(buf);
			fin >> buf;
			np->pos.setLongitudeDeg(atof(buf));
			fin >> buf;
			np->pos.setLatitudeDeg(atof(buf));
			fin >> buf;
			np->pos.setElevationM(atof(buf));
			fin >> buf;		// node type
			if(!strcmp(buf, "J")) {
				np->type = JUNCTION;
			} else if(!strcmp(buf, "T")) {
				np->type = TJUNCTION;
			} else if(!strcmp(buf, "H")) {
				np->type = HOLD;
			} else {
				SG_LOG(SG_ATC, SG_ALERT, "**** ERROR ***** Unknown node type in taxi network...\n");
				delete np;
				return(false);
			}
			fin >> buf;		// rwy exit information - gets parsed later - FRAGILE - will break if buf is reused.
			// Now the name
			fin >> ch;	// strip the leading " off
			np->name = "";
			while(1) {
				fin.unsetf(ios::skipws);
				fin >> ch;
				if((ch == '"') || (ch == 0x0A)) {
					break;
				}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
				np->name += ch;
			}
			fin.setf(ios::skipws);
			network.push_back(np);
			// FIXME - fragile - replies on buf not getting modified from exits read to here
			// see if we also need to push it onto the runway exit list
			//cout << "strlen(buf) = " << strlen(buf) << endl;
			if(strlen(buf) > 2) {
				//cout << "Calling ParseRwyExits for " << buf << endl;
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
				SG_LOG(SG_ATC, SG_ALERT, "**** ERROR ***** Unknown arc type in taxi network...\n");
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
				SG_LOG(SG_ATC, SG_ALERT, "**** ERROR ***** Unknown arc directed value in taxi network - should be Y/N !!!\n");
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
			ap->distance = (int)dclGetHorizontalSeparation(network[ap->n1]->pos, network[ap->n2]->pos);
			//cout << "Distance  = " << ap->distance << '\n';
			network[ap->n1]->arcs.push_back(ap);
			network[ap->n2]->arcs.push_back(ap);
		} else if(!strcmp(buf, "G")) {
			gp = new Gate;
			gp->struct_type = NODE;
			gp->type = GATE;
			fin >> buf;
			gp->nodeID = atoi(buf);
			fin >> buf;
			gp->pos.setLongitudeDeg(atof(buf));
			fin >> buf;
			gp->pos.setLatitudeDeg(atof(buf));
			fin >> buf;
			gp->pos.setElevationM(atof(buf));
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
			gp->id = gateCount;		// Warning - this will likely change in the future.
			gp->used = false;
			network.push_back(gp);
			gates[gateCount] = gp;
			gateCount++;
		} else {
			// Something has gone seriously pear-shaped
			SG_LOG(SG_ATC, SG_ALERT, "********* ERROR - unknown ground network element type... aborting read of " << path.c_str() << '\n');
			return(false);
		}
		
		fin >> skipeol;		
	}
	return(true);
}

void FGGround::Init() {
	untowered = false;
	
	// Figure out which is the active runway - TODO - it would be better to have ground call tower
	// for runway operation details, but at the moment we can't guarantee that tower control at a given airport
	// will be initialised before ground so we can't do that.
	DoRwyDetails();
	//cout << "In FGGround::Init, active rwy is " << activeRwy << '\n';
	ortho.Init(rwy.threshold_pos, rwy.hdg);

	networkLoadOK = LoadNetwork();
}

void FGGround::Update(double dt) {
	
	// Call the base class update for the response time handling.
	FGATC::Update(dt);
}

// Figure out which runways are active.
// For now we'll just be simple and do one active runway - eventually this will get much more complex
// Copied from FGTower - TODO - it would be better to implement this just once, and have ground call tower
// for runway operation details, but at the moment we can't guarantee that tower control at a given airport
// will be initialised before ground so we can't do that.
void FGGround::DoRwyDetails() {
	//cout << "GetRwyDetails called" << endl;
		
  const FGAirport* apt = fgFindAirportID(ident);
  if (!apt) {
    SG_LOG(SG_ATC, SG_WARN, "FGGround::DoRwyDetails: unknown ICAO:" << ident);
    return;
  }
  
	FGRunway* runway = apt->getActiveRunwayForUsage();

  activeRwy = runway->ident();
  rwy.rwyID = runway->ident();
  SG_LOG(SG_ATC, SG_INFO, "In FGGround, active runway for airport " << ident << " is " << activeRwy);
  
  // Get the threshold position
  double other_way = runway->headingDeg() - 180.0;
  while(other_way <= 0.0) {
    other_way += 360.0;
  }
    // move to the +l end/center of the runway
  //cout << "Runway center is at " << runway._lon << ", " << runway._lat << '\n';
  double tshlon = 0.0, tshlat = 0.0, tshr = 0.0;
  double tolon = 0.0, tolat = 0.0, tor = 0.0;
  rwy.length = runway->lengthM();
  geo_direct_wgs_84 ( aptElev, runway->latitude(), runway->longitude(), other_way, 
                      rwy.length / 2.0 - 25.0, &tshlat, &tshlon, &tshr );
  geo_direct_wgs_84 ( aptElev, runway->latitude(), runway->longitude(), runway->headingDeg(), 
                      rwy.length / 2.0 - 25.0, &tolat, &tolon, &tor );
  // Note - 25 meters in from the runway end is a bit of a hack to put the plane ahead of the user.
  // now copy what we need out of runway into rwy
  rwy.threshold_pos = SGGeod::fromDegM(tshlon, tshlat, aptElev);
  SGGeod takeoff_end = SGGeod::fromDegM(tolon, tolat, aptElev);
  //cout << "Threshold position = " << tshlon << ", " << tshlat << ", " << aptElev << '\n';
  //cout << "Takeoff position = " << tolon << ", " << tolat << ", " << aptElev << '\n';
  rwy.hdg = runway->headingDeg();
  // Set the projection for the local area based on this active runway
  ortho.Init(rwy.threshold_pos, rwy.hdg);	
  rwy.end1ortho = ortho.ConvertToLocal(rwy.threshold_pos);	// should come out as zero
  rwy.end2ortho = ortho.ConvertToLocal(takeoff_end);
}

// Return a random gate ID of an unused gate.
// Two error values may be returned and must be checked for by the calling function:
// -2 signifies that no gates exist at this airport.
// -1 signifies that all gates are currently full.
int FGGround::GetRandomGateID() {
	// Check that this airport actually has some gates!!
	if(!gates.size()) {
		return(-2);
	}

	gate_vec_type gateVec;
	int num = 0;
	int thenum;
	int ID;
	
	gatesItr = gates.begin();
	while(gatesItr != gates.end()) {
		if((gatesItr->second)->used == false) {
			gateVec.push_back(gatesItr->second);
			num++;
		}
		++gatesItr;
	}

	// Check that there are some unused gates!
	if(!gateVec.size()) {
		return(-1);
	}
	
	// Randomly select one from the list
	sg_srandom_time();
	thenum = (int)(sg_random() * gateVec.size());
	ID = gateVec[thenum]->id;
	
	return(ID);
}

// Return a pointer to an unused gate node
Gate* FGGround::GetGateNode() {
	int id = GetRandomGateID();
	if(id < 0) {
		return(NULL);
	} else {
		return(gates[id]);
	}
}


node* FGGround::GetHoldShortNode(const string& rwyID) {
	return(NULL);	// TODO - either implement me or remove me!!!
}


// WARNING - This is hardwired to my prototype logical network format
// and will almost certainly change when Bernie's stuff comes on-line.
// Returns NULL if it can't find a valid node.
node* FGGround::GetThresholdNode(const string& rwyID) {
	// For now go through all the nodes and parse their names
	// Maybe in the future we'll map threshold nodes by ID
	//cout << "Size of network is " << network.size() << '\n';
	for(unsigned int i=0; i<network.size(); ++i) {
		//cout << "Name = " << network[i]->name << '\n';
		if(network[i]->name.size()) {
			string s = network[i]->name;
			// Warning - the next bit is fragile and dependent on my current naming scheme
			//cout << "substr = " << s.substr(0,3) << '\n';
			//cout << "size of s = " << s.size() << '\n'; 
			if(s.substr(0,3) == "rwy") {
				//cout << "subsubstr = " << s.substr(4, s.size() - 4) << '\n';
				if(s.substr(4, s.size() - 4) == rwyID) {
					return network[i];
				}
			}
		}
	}
	return NULL;
}


// Get a path from a point on a runway to a gate
// TODO !!

// Get a path from a node to another node
// Eventually we will need complex algorithms for this taking other traffic,
// shortest path and suitable paths into accout.
// For now we'll just call the shortest path algorithm.
ground_network_path_type FGGround::GetPath(node* A, node* B) {	
	return(GetShortestPath(A, B));
};

// Get a path from a node to a runway threshold
ground_network_path_type FGGround::GetPath(node* A, const string& rwyID) {
	node* b = GetThresholdNode(rwyID);
	if(b == NULL) {
		SG_LOG(SG_ATC, SG_ALERT, "ERROR - unable to find path to runway theshold in ground.cxx for airport " << ident << '\n');
		ground_network_path_type emptyPath;
		emptyPath.erase(emptyPath.begin(), emptyPath.end());
		return(emptyPath);
	}
	return GetShortestPath(A, b);
}

// Get a path from a node to a runway hold short point
// Bit of a hack this at the moment!
ground_network_path_type FGGround::GetPathToHoldShort(node* A, const string& rwyID) {
	ground_network_path_type path = GetPath(A, rwyID);
	path.pop_back();	// That should be the threshold stripped of 
	path.pop_back();	// and that should be the arc from hold short to threshold
	// This isn't robust though - TODO - implement properly!
	return(path);
}

// A shortest path algorithm from memory (ie. I can't find the bl&*dy book again!)
// I'm sure there must be enchancements that we can make to this, such as biasing the
// order in which the nodes are searched out from in favour of those geographically
// closer to the destination.
// Note that we are working with the master set of nodes and arcs so we mustn't change
// or delete them -  we only delete the paths that we create during the algorithm.
ground_network_path_type FGGround::GetShortestPath(node* A, node* B) {
	a_path* pathPtr;
	shortest_path_map_type pathMap;
	node_array_type nodesLeft;
	
	// Debugging check
	int pathsCreated = 0;
	
	// Initialise the algorithm
	nodesLeft.push_back(A);
	pathPtr = new a_path;
	pathsCreated++;
	pathPtr->path.push_back(A);
	pathPtr->cost = 0;
	pathMap[A->nodeID] = pathPtr;
	bool solution_found = false;	// Flag to indicate that at least one candidate path has been found
	int solution_cost = -1;			// Cost of current best cost solution.  -1 indicates no solution found yet.
	a_path solution_path;		
											
	node* nPtr;	// nPtr is used to point to the node we are currently working with
	
	while(nodesLeft.size()) {
		//cout << "\n*****nodesLeft*****\n";
		//for(unsigned int i=0; i<nodesLeft.size(); ++i) {
			//cout << nodesLeft[i]->nodeID << '\n';
		//}
		//cout << "*******************\n\n";
		nPtr = *nodesLeft.begin();		// Thought - definate optimization possibilities here in the choice of which nodes we process first.
		nodesLeft.erase(nodesLeft.begin());
		//cout << "Walking out from node " << nPtr->nodeID << '\n';
		for(unsigned int i=0; i<nPtr->arcs.size(); ++i) {
			//cout << "ARC TO " << ((nPtr->arcs[i]->n1 == nPtr->nodeID) ? nPtr->arcs[i]->n2 : nPtr->arcs[i]->n1) << '\n';
		}
		if((solution_found) && (solution_cost <= pathMap[nPtr->nodeID]->cost)) {
			// Do nothing - we've already found a solution and this partial path is already more expensive
		} else {
			// This path could still be better than the current solution - check it out
			for(unsigned int i=0; i<(nPtr->arcs.size()); i++) {
				// Map the new path against the end node, ie. *not* the one we just started with.
				unsigned int end_nodeID = ((nPtr->arcs[i]->n1 == nPtr->nodeID) ? nPtr->arcs[i]->n2 : nPtr->arcs[i]->n1);
				//cout << "end_nodeID = " << end_nodeID << '\n';
				//cout << "pathMap size is " << pathMap.size() << '\n';
				if(end_nodeID == nPtr->nodeID) {
					//cout << "Circular arc!\n";
					// Then its a circular arc - don't bother!!
					//nPtr->arcs.erase(nPtr->arcs.begin() + i);
				} else {
					// see if the end node is already in the map or not
					if(pathMap.find(end_nodeID) == pathMap.end()) {
						//cout << "Not in the map" << endl;;
						// Not in the map - easy!
						pathPtr = new a_path;
						pathsCreated++;
						*pathPtr = *pathMap[nPtr->nodeID];	// *copy* the path
						pathPtr->path.push_back(nPtr->arcs[i]);
						pathPtr->path.push_back(network[end_nodeID]);
						pathPtr->cost += nPtr->arcs[i]->distance;
						pathMap[end_nodeID] = pathPtr;
						nodesLeft.push_back(network[end_nodeID]);	// By definition this can't be in the list already, or
						// it would also have been in the map and hence OR'd with this one.
						if(end_nodeID == B->nodeID) {
							//cout << "Solution found!!!" << endl;
							// Since this node wasn't in the map this is by definition the first solution
							solution_cost = pathPtr->cost;
							solution_path = *pathPtr;
							solution_found = true;
						}
					} else {
						//cout << "Already in the map" << endl;
						// In the map - not so easy - need to get rid of an arc from the higher cost one.
						//cout << "Current cost of node " << end_nodeID << " is " << pathMap[end_nodeID]->cost << endl;
						int newCost = pathMap[nPtr->nodeID]->cost + nPtr->arcs[i]->distance;
						//cout << "New cost is of node " << nPtr->nodeID << " is " << newCost << endl;
						if(newCost >= pathMap[end_nodeID]->cost) {
							// No need to do anything.
							//cout << "Not doing anything!" << endl;
						} else {
							delete pathMap[end_nodeID];
							pathsCreated--;
							
							pathPtr = new a_path;
							pathsCreated++;
							*pathPtr = *pathMap[nPtr->nodeID];	// *copy* the path
							pathPtr->path.push_back(nPtr->arcs[i]);
							pathPtr->path.push_back(network[end_nodeID]);
							pathPtr->cost += nPtr->arcs[i]->distance;
							pathMap[end_nodeID] = pathPtr;
							
							// We need to add this node to the list-to-do again to force a recalculation 
							// onwards from this node with the new lower cost to node cost.
							nodesLeft.push_back(network[end_nodeID]);
							
							if(end_nodeID == B->nodeID) {
								//cout << "Solution found!!!" << endl;
								// Need to check if there is a previous better solution
								if((solution_cost < 0) || (pathPtr->cost < solution_cost)) {
									solution_cost = pathPtr->cost;
									solution_path = *pathPtr;
									solution_found = true;
								}
							}
						}
					}
				}
			}
		}
	}
	
	// delete all the paths before returning
	shortest_path_map_iterator spItr = pathMap.begin();
	while(spItr != pathMap.end()) {
		if(spItr->second != NULL) {
			delete spItr->second;
			--pathsCreated;
		}
		++spItr;
	}
	
	//cout << "pathsCreated = " << pathsCreated << '\n';
	if(pathsCreated > 0) {
		SG_LOG(SG_ATC, SG_ALERT, "WARNING - Possible memory leak in FGGround::GetShortestPath\n\
									  Please report to flightgear-devel@flightgear.org\n");
	}
	
	//cout << (solution_found ? "Result: solution found\n" : "Result: no solution found\n");
	return(solution_path.path);		// TODO - we really ought to have a fallback position incase a solution isn't found.
}
		


// Randomly or otherwise populate some of the gates with parked planes
// (might eventually be done by the AIMgr if and when lots of AI traffic is generated)

// Return a list of exits from a given runway
// It is up to the calling function to check for non-zero size of returned array before use
node_array_type FGGround::GetExits(const string& rwyID) {
	// FIXME - get a 07L or similar in here and we're stuffed!!!
	return(runways[atoi(rwyID.c_str())].exits);
}
