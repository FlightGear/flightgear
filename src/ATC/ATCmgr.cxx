// ATCmgr.cxx - Implementation of FGATCMgr - a global Flightgear ATC manager.
//
// Written by David Luff, started February 2002.
//
// Copyright (C) 2002  David C Luff - david.luff@nottingham.ac.uk
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

//#include <Time/event.hxx>

#include "ATCmgr.hxx"
#include "atislist.hxx"
//#include "groundlist.hxx"
#include "towerlist.hxx"
#include "approachlist.hxx"

/*
// periodic radio station search wrapper
static void fgATCSearch( void ) {
    globals->get_ATC_mgr()->Search();
}
*/ //This wouldn't compile - including Time/event.hxx breaks it :-(

FGATCMgr::FGATCMgr() {
    comm1_ident = "";
    comm1_atis_ident = "";
    comm1_tower_ident = "";
    comm1_approach_ident = "";
    last_comm1_ident = "";
    last_comm1_atis_ident = "";
    last_comm1_tower_ident = "";
    last_comm1_approach_ident = "";
    approach_ident = "";
    last_in_range = false;
    comm1_atis_valid = false;
    comm1_tower_valid = false;
    comm1_approach_valid = false;
}

FGATCMgr::~FGATCMgr() {
}

void FGATCMgr::bind() {
}

void FGATCMgr::unbind() {
}

void FGATCMgr::init() {
    comm1_node = fgGetNode("/radios/comm[0]/frequencies/selected-mhz", true);
    comm2_node = fgGetNode("/radios/comm[1]/frequencies/selected-mhz", true);
    lon_node = fgGetNode("/position/longitude-deg", true);
    lat_node = fgGetNode("/position/latitude-deg", true);
    elev_node = fgGetNode("/position/altitude-ft", true);
    atc_list_itr = atc_list.begin();
    // Search for connected ATC stations once per 0.8 seconds or so
    // global_events.Register( "fgATCSearch()", fgATCSearch,
    //		    fgEVENT::FG_EVENT_READY, 800);
    // For some reason the above doesn't compile - including Time/event.hxx stops compilation.

    // Initialise the airport_atc_map - we'll cheat for now and just hardcode KEMT and any others that may be needed for development
    AirportATC *a = new AirportATC;
    a->lon = -118.034719;
    a->lat = 34.086114;
    a->elev = 296.0;
    a->atis_freq = 118.75;
    a->atis_active = false;
    a->tower_freq = 121.2;
    a->tower_active = false;
    a->ground_freq = 125.9;
    a->ground_active = false;

    //a->set_by_AI = true;
    //a->set_by_comm_search = false;

    airport_atc_map[(string)"KEMT"] = a;
}

void FGATCMgr::update(double dt) {
    //Traverse the list of active stations.
    //Only update one class per update step to avoid the whole ATC system having to calculate between frames.
    //Eventually we should only update every so many steps.
    //cout << "In FGATCMgr::update - atc_list.size = " << atc_list.size() << '\n';
    if(atc_list.size()) {
	if(atc_list_itr == atc_list.end()) {
	    atc_list_itr = atc_list.begin();
	}
	(*atc_list_itr)->Update();
	++atc_list_itr;
    }

    // Search the tuned frequencies every now and then - this should be done with the event scheduler
    static int i = 0;
    if(i == 30) {
	Search();
	i = 0;
    }
    ++i;
}
/*
// Remove from list only if not needed by the AI system
void FGATCMgr::CommRemoveFromList(const char* id, atc_type tp) {
    AirportATC a;
    if(GetAirportATCDetails((string)id, &a)) {
	if(a.set_by_AI) {
	    // Don't remove
	    a.set_by_comm_search = false;
	    airport_atc_map[(string)id] = a;
	    return;
	} else {
	    // remove
	    
*/    

// Remove from list - should only be called from above or similar
void FGATCMgr::RemoveFromList(const char* id, atc_type tp) {
    //cout << "Requested type = " << tp << '\n';
    //cout << "id = " << id << '\n';
    atc_list_itr = atc_list.begin();
    while(atc_list_itr != atc_list.end()) {
	//cout << "type = " << (*atc_list_itr)->GetType() << '\n';
	//cout << "Ident = " << (*atc_list_itr)->GetIdent() << '\n';
        if( (!strcmp((*atc_list_itr)->GetIdent(), id))
	    && ((*atc_list_itr)->GetType() == tp) ) {
	    //Before removing it stop it transmitting!!
	    //cout << "OBLITERATING FROM LIST!!!\n";
	    (*atc_list_itr)->SetNoDisplay();
	    (*atc_list_itr)->Update();
	    delete (*atc_list_itr);
	    atc_list_itr = atc_list.erase(atc_list_itr);
	    break;
	}  // Note that that can upset where we are in the list but that doesn't really matter
	++atc_list_itr;
    }
}

//DCL - this routine untested so far.
// Find in list - return a currently active ATC pointer given ICAO code and type
FGATC* FGATCMgr::FindInList(const char* id, atc_type tp) {
    atc_list_itr = atc_list.begin();
    while(atc_list_itr != atc_list.end()) {
	if( (!strcmp((*atc_list_itr)->GetIdent(), id))
	    && ((*atc_list_itr)->GetType() == tp) ) {
	    return(*atc_list_itr);
	}	// Note that that can upset where we are in the list but that shouldn't really matter
	++atc_list_itr;
    }
    // We need a fallback position
    cout << "*** Failed to find FGATC* in FGATCMgr::FindInList - this should not happen!" << endl;
    return(NULL);
}

// Returns true if the airport is found in the map
bool FGATCMgr::GetAirportATCDetails(string icao, AirportATC* a) {
    if(airport_atc_map.find(icao) != airport_atc_map.end()) {
        *a = *airport_atc_map[icao];
	return(true);
    } else {
	return(false);
    }
}


// Return a pointer to a given sort of ATC at a given airport and activate if necessary
// ONLY CALL THIS FUNCTION AFTER FIRST CHECKING THE SERVICE EXISTS BY CALLING GetAirportATCDetails
FGATC* FGATCMgr::GetATCPointer(string icao, atc_type type) {
    AirportATC *a = airport_atc_map[icao];
    //cout << "a->lon = " << a->lon << '\n';
    //cout << "a->elev = " << a->elev << '\n';
    //cout << "a->tower_freq = " << a->tower_freq << '\n';
    switch(type) {
    case TOWER:
	if(a->tower_active) {
	    // Get the pointer from the list
	    return(FindInList(icao.c_str(), type));	// DCL - this untested so far.
	} else {
	    FGTower* t = new FGTower;
	    if(current_towerlist->query(a->lon, a->lat, a->elev, a->tower_freq, &tower)) {
		*t = tower;
		atc_list.push_back(t);
		a->tower_active = true;
		airport_atc_map[icao] = a;
		return(t);
	    } else {
		cout << "ERROR - tower that should exist in FGATCMgr::GetATCPointer for airport " << icao << " not found\n";
	    }
	}
	break;
    // Lets add the rest to get rid of the compiler warnings even though we don't need them yet.
    case APPROACH:
	break;
    case ATIS:
	cout << "ERROR - ATIS station should not be requested from FGATCMgr::GetATCPointer" << endl;
	break;
    case GROUND:
	break;
    case INVALID:
	break;
    case ENROUTE:
	break;
    case DEPARTURE:
	break;
    }

    cout << "ERROR IN FGATCMgr - reached end of GetATCPointer\n";

    return(NULL);
}



void FGATCMgr::Search() {

    ////////////////////////////////////////////////////////////////////////
    // Comm1.
    ////////////////////////////////////////////////////////////////////////
    //cout << "In FGATCMgr::Search() - atc_list.size = " << atc_list.size() << '\n';

    comm1_freq = comm1_node->getDoubleValue();
    //cout << "************* comm1_freq = " << comm1_freq << '\n';
    double lon = lon_node->getDoubleValue();
    double lat = lat_node->getDoubleValue();
    double elev = elev_node->getDoubleValue() * SG_FEET_TO_METER;

    // Store the comm1_type
    //atc_type old_comm1_type = comm1_type;

// We must be able to generalise some of the repetetive searching below!

    //Search for ATIS first
    if(current_atislist->query(lon, lat, elev, comm1_freq, &atis)) {
	//cout << "atis found in radiostack search !!!!" << endl;
	//cout << "last_comm1_atis_ident = " << last_comm1_atis_ident << '\n';
	//cout << "comm1_type " << comm1_type << '\n';
	comm1_atis_ident = atis.GetIdent();
	comm1_atis_valid = true;
	if(last_comm1_atis_ident != comm1_atis_ident) {
	    if(last_comm1_atis_ident != "") {
		RemoveFromList(last_comm1_atis_ident, ATIS);
	    }
	    last_comm1_atis_ident = comm1_atis_ident;
	    //cout << "last_comm1_atis_ident = " << last_comm1_atis_ident << '\n';
	    comm1_type = ATIS;
	    comm1_elev = atis.get_elev();
	    comm1_range = FG_ATIS_DEFAULT_RANGE;
	    comm1_effective_range = comm1_range;
	    comm1_x = atis.get_x();
	    comm1_y = atis.get_y();
	    comm1_z = atis.get_z();
	    FGATIS* a = new FGATIS;
	    *a = atis;
	    a->SetDisplay();
	    atc_list.push_back(a);
	    //cout << "Found a new atis station in range" << endl;
	    //cout << " id = " << atis.GetIdent() << endl;
	    return;  //This rather assumes that we never have more than one type of station in range.
	}
    } else {
	if(comm1_atis_valid) {
	    //cout << "Removing ATIS " << comm1_atis_ident << " from list\n";
	    RemoveFromList(comm1_atis_ident, ATIS);
	    comm1_atis_valid = false;
	    if(comm1_type == ATIS) {
		comm1_type = INVALID;
	    }
	    comm1_atis_ident = "";
	    //comm1_trans_ident = "";
	    last_comm1_atis_ident = "";
	}
	//cout << "not picking up atis" << endl;
    }

    //Next search for tower
    //cout << "comm1_freq = " << comm1_freq << '\n';
    if(current_towerlist->query(lon, lat, elev, comm1_freq, &tower)) {
	//cout << "tower found in radiostack search !!!!" << endl;
	comm1_tower_ident = tower.GetIdent();
	//cout << "comm1_tower_ident = " << comm1_tower_ident << '\n';
	comm1_tower_valid = true;
	if(last_comm1_tower_ident != comm1_tower_ident) {
	    if(last_comm1_tower_ident != "") {
		RemoveFromList(last_comm1_tower_ident, TOWER);
	    }
	    last_comm1_tower_ident = comm1_tower_ident;
	    comm1_type = TOWER;
	    comm1_elev = tower.get_elev();
	    comm1_range = FG_TOWER_DEFAULT_RANGE;
	    comm1_effective_range = comm1_range;
	    comm1_x = tower.get_x();
	    comm1_y = tower.get_y();
	    comm1_z = tower.get_z();
	    FGTower* t = new FGTower;
	    *t = tower;
	    t->SetDisplay();
	    atc_list.push_back(t);
	    //cout << "Found a new tower station in range" << endl;
	    //cout << " id = " << tower.GetIdent() << endl;
	    return;  //This rather assumes that we never have more than one type of station in range.
	}
    } else {
	if(comm1_tower_valid) {
	    //cout << "removing tower\n";
	    RemoveFromList(comm1_tower_ident, TOWER);
	    //comm1_valid = false;
	    if(comm1_type == TOWER) {
		comm1_type = INVALID;	// Only invalidate if we haven't switched it to something else
	    }
	    comm1_tower_valid = false;
	    comm1_tower_ident = "";
	    last_comm1_tower_ident = "";
	    //comm1_ident = "";
	    //comm1_trans_ident = "";
	    //last_comm1_ident = "";
	}
	//cout << "not picking up tower" << endl;
    }
/*
    //Next search for Ground control
    if(current_groundlist->query(lon, lat, elev, comm1_freq, &ground)) {
	//cout << "Ground Control found in radiostack search !!!!" << endl;
	comm1_ident = ground.GetIdent();
	comm1_valid = true;
	if((last_comm1_ident != comm1_ident) || (comm1_type != GROUND)) {
	    if(last_comm1_ident != "") {
		RemoveFromList(last_comm1_ident, GROUND);
	    }
	    last_comm1_ident = comm1_ident;
	    comm1_type = GROUND;
	    comm1_elev = ground.get_elev();
	    comm1_range = FG_GROUND_DEFAULT_RANGE;
	    comm1_effective_range = comm1_range;
	    comm1_x = ground.get_x();
	    comm1_y = ground.get_y();
	    comm1_z = ground.get_z();
	    FGGround* g = new FGGround;
	    *g = ground;
	    g->SetDisplay();
	    atc_list.push_back(g);
	    // For now we will automatically make contact with ground when the radio is tuned.
	    // This rather assumes that the user tunes the radio at the appropriate place
	    // (ie. having just turned off the runway) and only uses ground control on arrival
	    // but its a start!
 	    g->NewArrival(current_plane);
	    //cout << "Found a new ground station in range" << endl;
	    //cout << " id = " << ground.GetIdent() << endl;
	    return;  //This rather assumes that we never have more than one type of station in range.
	}
    } else {
	if((comm1_valid) && (comm1_type == GROUND)) {
	    RemoveFromList(comm1_ident, GROUND);
	    comm1_valid = false;
	    comm1_type = INVALID;
	    comm1_ident = "";
	    //comm1_trans_ident = "";
	    last_comm1_ident = "";
	}
	//cout << "not picking up ground control" << endl;
    }
*/
// ================================================================================
// Search for Approach stations
// ================================================================================
    // init number of approach stations reachable by plane
    int  num_app = 0;

    // search stations in range
    current_approachlist->query_bck(lon, lat, elev, approaches, max_app, num_app);
    if (num_app != 0) {
      //cout << num_app << " approaches found in radiostack search !!!!" << endl;

      for ( int i=0; i<num_app; i++ ) {
	bool new_app = true;
	approach_ident = approaches[i].GetIdent();

	// check if station already exists on ATC stack
	atc_list_itr = atc_list.begin();
	while(atc_list_itr != atc_list.end()) {
	  //cout << "ATC list: " << (*atc_list_itr)->GetIdent() << endl;
	  if((!strcmp((*atc_list_itr)->GetIdent(), approach_ident))
	     && ((*atc_list_itr)->GetType() == APPROACH) ) {
	    new_app = false;
	    string pid = "Player";
	    (*atc_list_itr)->AddPlane(pid);
	    (*atc_list_itr)->Update();
	    break;
	  }
	  ++atc_list_itr;
	}
	// generate new Approach on ATC stack
	if (new_app) {
	  FGApproach* a = new FGApproach;
	  *a = approaches[i];
	  string pid = "Player";
	  a->AddPlane(pid);
	  a->Update();
	  a->SetDisplay();
	  atc_list.push_back(a);
	  //cout << "Found a new approach station in range: Id = " 
	  //     << approaches[i].GetIdent() << endl;
	}
      }
    }

    // remove planes which are out of range
    atc_list_itr = atc_list.begin();
    while(atc_list_itr != atc_list.end()) {
      if((*atc_list_itr)->GetType() == APPROACH ) {
	int np = (*atc_list_itr)->RemovePlane();
	// if approach has no planes left remove it from ATC list
	if ( np == 0) {
	  (*atc_list_itr)->SetNoDisplay();
	  (*atc_list_itr)->Update();
	  delete (*atc_list_itr);
	  atc_list_itr = atc_list.erase(atc_list_itr);
	  break;     // the other stations will be checked next time
	}
      }
      ++atc_list_itr;
    }
}
