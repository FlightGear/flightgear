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

/*
// periodic radio station search wrapper
static void fgATCSearch( void ) {
    globals->get_ATC_mgr()->Search();
}
*/ //This wouldn't compile - including Time/event.hxx breaks it :-(

FGATCMgr::FGATCMgr() {
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
}

void FGATCMgr::update(int dt) {
    //Traverse the list of active stations.
    //Only update one class per update step to avoid the whole ATC system having to calculate between frames.
    //Eventually we should only update every so many steps.
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

void FGATCMgr::RemoveFromList(const char* id, atc_type tp) {
    atc_list_itr = atc_list.begin();
    while(atc_list_itr != atc_list.end()) {
        if( (!strcmp((*atc_list_itr)->GetIdent(), id))
	    && ((*atc_list_itr)->GetType() == tp) ) {
	    //Before removing it stop it transmitting!!
	    (*atc_list_itr)->SetNoDisplay();
	    (*atc_list_itr)->Update();
	    delete (*atc_list_itr);
	    atc_list_itr = atc_list.erase(atc_list_itr);
	    break;
	}  // Note that that can upset where we are in the list but that doesn't really matter
	++atc_list_itr;
    }
}

void FGATCMgr::Search() {

    ////////////////////////////////////////////////////////////////////////
    // Comm1.
    ////////////////////////////////////////////////////////////////////////

    comm1_freq = comm1_node->getDoubleValue();
    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = elev_node->getDoubleValue() * SG_FEET_TO_METER;

    //Search for ATIS first
    if(current_atislist->query(lon, lat, elev, comm1_freq, &atis)) {
	//cout << "atis found in radiostack search !!!!" << endl;
	comm1_ident = atis.GetIdent();
	comm1_valid = true;
	if(last_comm1_ident != comm1_ident) {
	    if(last_comm1_ident != "") {
		RemoveFromList(last_comm1_ident, ATIS);
	    }
	    last_comm1_ident = comm1_ident;
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
	}
    } else {
	if(comm1_valid) {
	    RemoveFromList(comm1_ident, ATIS);
	    comm1_valid = false;
	    comm1_ident = "";
	    //comm1_trans_ident = "";
	    last_comm1_ident = "";
	}
	//cout << "not picking up atis" << endl;
    }
}