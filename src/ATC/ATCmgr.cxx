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

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>

#include "ATCmgr.hxx"
#include "commlist.hxx"
//#include "atislist.hxx"
//#include "groundlist.hxx"
//#include "towerlist.hxx"
//#include "approachlist.hxx"
#include "ATCdisplay.hxx"
#include "ATCDialog.hxx"

/*
// periodic radio station search wrapper
static void fgATCSearch( void ) {
	globals->get_ATC_mgr()->Search();
}
*/ //This wouldn't compile - including Time/event.hxx breaks it :-(

FGATCMgr::FGATCMgr() {
	comm_ident[0] = "";
	comm_ident[1] = "";
	last_comm_ident[0] = "";
	last_comm_ident[1] = "";
	approach_ident = "";
	last_in_range = false;
	comm_type[0] = INVALID;
	comm_type[1] = INVALID;
	comm_atc_ptr[0] = NULL;
	comm_atc_ptr[1] = NULL;
}

FGATCMgr::~FGATCMgr() {
	delete v1;
}

void FGATCMgr::bind() {
}

void FGATCMgr::unbind() {
}

void FGATCMgr::init() {
	comm_node[0] = fgGetNode("/radios/comm[0]/frequencies/selected-mhz", true);
	comm_node[1] = fgGetNode("/radios/comm[1]/frequencies/selected-mhz", true);
	lon_node = fgGetNode("/position/longitude-deg", true);
	lat_node = fgGetNode("/position/latitude-deg", true);
	elev_node = fgGetNode("/position/altitude-ft", true);
	atc_list_itr = atc_list.begin();
	// Search for connected ATC stations once per 0.8 seconds or so
	// global_events.Register( "fgATCSearch()", fgATCSearch,
	//		    fgEVENT::FG_EVENT_READY, 800);
	// For some reason the above doesn't compile - including Time/event.hxx stops compilation.
	
	// Initialise the frequency search map
    current_commlist = new FGCommList;
    SGPath p_comm( globals->get_fg_root() );
    current_commlist->init( p_comm );
	
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

#ifdef ENABLE_AUDIO_SUPPORT	
	// Load all available voices.
	// For now we'll do one hardwired one
	
	v1 = new FGATCVoice;
	voiceOK = v1->LoadVoice("default");
	voice = true;
	
	/* I've loaded the voice even if /sim/sound/audible is false
	*  since I know no way of forcing load of the voice if the user
	*  subsequently switches /sim/sound/audible to true. */
#else
	voice = false;
#endif

	// Initialise the ATC Dialogs
	//cout << "Initing Transmissions..." << endl;
    SG_LOG(SG_GENERAL, SG_INFO, "  ATC Transmissions");
    current_transmissionlist = new FGTransmissionList;
    SGPath p_transmission( globals->get_fg_root() );
    p_transmission.append( "ATC/default.transmissions" );
    current_transmissionlist->init( p_transmission );
	//cout << "Done Transmissions" << endl;

    SG_LOG(SG_GENERAL, SG_INFO, "  ATC Dialog System");
    current_atcdialog = new FGATCDialog;
    current_atcdialog->Init();

	ATCDialogInit();
	
	// DCL - testing
	//current_atcdialog->add_entry( "EGNX", "Request vectoring for approach", "Request Vectors" );
	//current_atcdialog->add_entry( "EGNX", "Mayday, Mayday", "Declare Emergency" );
}

void FGATCMgr::update(double dt) {
	//cout << "Entering update..." << endl;
	//Traverse the list of active stations.
	//Only update one class per update step to avoid the whole ATC system having to calculate between frames.
	//Eventually we should only update every so many steps.
	//cout << "In FGATCMgr::update - atc_list.size = " << atc_list.size() << '\n';
	if(atc_list.size()) {
		if(atc_list_itr == atc_list.end()) {
			atc_list_itr = atc_list.begin();
		}
		//cout << "Updating " << (*atc_list_itr)->get_ident() << ' ' << (*atc_list_itr)->GetType() << '\n';
		//cout << "Freq = " << (*atc_list_itr)->get_freq() << '\n';
		(*atc_list_itr)->Update();
		//cout << "Done ATC update..." << endl;
		++atc_list_itr;
	}
	
	// Search the tuned frequencies every now and then - this should be done with the event scheduler
	static int i = 0;
	if(i == 7) {
		AreaSearch();
	}
	if(i == 15) {
		//cout << "About to search(1)" << endl;
		FreqSearch(1);
	}
	if(i == 30) {
		//cout << "About to search(2)" << endl;
		FreqSearch(2);
		i = 0;
	}
	++i;
	
	//cout << "comm1 type = " << comm_type[0] << '\n';
}

// Remove from list only if not needed by the AI system or the other comm channel
// TODO - implement me!!
void FGATCMgr::CommRemoveFromList(const char* id, atc_type tp, int chan) {
	/*AirportATC a;
	if(GetAirportATCDetails((string)id, &a)) {
		if(a.set_by_AI) {
			// Don't remove
			a.set_by_comm_search = false;
			airport_atc_map[(string)id] = a;
			return;
		} else {
			// remove
		}
	}*/
	
	// Hack - need to implement this properly
	RemoveFromList(id, tp);
}
    

// Remove from list - should only be called from above or similar
// This function *will* remove it from the list regardless of who else might want it.
void FGATCMgr::RemoveFromList(const char* id, atc_type tp) {
	//cout << "Requested type = " << tp << '\n';
	//cout << "id = " << id << '\n';
	atc_list_itr = atc_list.begin();
	while(atc_list_itr != atc_list.end()) {
		//cout << "type = " << (*atc_list_itr)->GetType() << '\n';
		//cout << "Ident = " << (*atc_list_itr)->get_ident() << '\n';
		if( (!strcmp((*atc_list_itr)->get_ident(), id))
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


// Find in list - return a currently active ATC pointer given ICAO code and type
// Return NULL if the given service is not in the list
// - *** THE CALLING FUNCTION MUST CHECK FOR THIS ***
FGATC* FGATCMgr::FindInList(const char* id, atc_type tp) {
	atc_list_itr = atc_list.begin();
	while(atc_list_itr != atc_list.end()) {
		if( (!strcmp((*atc_list_itr)->get_ident(), id))
		&& ((*atc_list_itr)->GetType() == tp) ) {
			return(*atc_list_itr);
		}	// Note that that can upset where we are in the list but that shouldn't really matter
		++atc_list_itr;
	}
	// If we get here it's not in the list
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
// FIXME - we really ought to take out the necessity for two function calls by simply returning
// a NULL pointer if the service doesn't exist and requiring the caller to check for it (NULL).
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
			ATCData data;
			if(current_commlist->FindByFreq(a->lon, a->lat, a->elev, a->tower_freq, &data, TOWER)) {
				t->SetData(&data);
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
		SG_LOG(SG_GENERAL, SG_ALERT, "ERROR - ATIS station should not be requested from FGATCMgr::GetATCPointer");
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
	
	SG_LOG(SG_GENERAL, SG_ALERT, "ERROR IN FGATCMgr - reached end of GetATCPointer");
	
	return(NULL);
}

// Return a pointer to an appropriate voice for a given type of ATC
// creating the voice if necessary - ie. make sure exactly one copy
// of every voice in use exists in memory.
//
// TODO - in the future this will get more complex and dole out country/airport
// specific voices, and possible make sure that the same voice doesn't get used
// at different airports in quick succession if a large enough selection are available.
FGATCVoice* FGATCMgr::GetVoicePointer(atc_type type) {
	// TODO - implement me better - maintain a list of loaded voices and other voices!!
	if(voice) {
		switch(type) {
		case ATIS:
			if(voiceOK) {
				return(v1);
			}
		case TOWER:
			return(NULL);
		case APPROACH:
			return(NULL);
		case GROUND:
			return(NULL);
		default:
			return(NULL);
		}
		return(NULL);
	} else {
		return(NULL);
	}
}

// Display a dialog box with options relevant to the currently tuned ATC service.
void FGATCMgr::doPopupDialog() {
	ATCDoDialog(comm_type[0]);	// FIXME - currently hardwired to comm1
}

// Search for ATC stations by frequency
void FGATCMgr::FreqSearch(int channel) {
	int chan = channel - 1;		// Convert to zero-based for the arrays

	ATCData data;	
	double freq = comm_node[chan]->getDoubleValue();
	lon = lon_node->getDoubleValue();
	lat = lat_node->getDoubleValue();
	elev = elev_node->getDoubleValue() * SG_FEET_TO_METER;
	
	// Query the data store and get the closest match if any
	if(current_commlist->FindByFreq(lon, lat, elev, freq, &data)) {
		// We have a match
		// What's the logic?
		// If this channel not previously valid then easy - add ATC to list
		// If this channel was valid then - Have we tuned to a different service?
		// If so - de-register one and add the other
		if(comm_valid[chan]) {
			if((comm_ident[chan] == data.ident) && (comm_type[chan] == data.type)) {
				// Then we're still tuned into the same service so do nought and return
				return;
			} else {
				// Something's changed - either the location or the service type
				// We need to feed the channel in so we're not removing it if we're also tuned in on the other channel
				CommRemoveFromList(comm_ident[chan], comm_type[chan], chan);
			}
		}
		// At this point we can assume that we need to add the service.
		comm_ident[chan] = (data.ident).c_str();
		comm_type[chan] = data.type;
		comm_x[chan] = (double)data.x;
		comm_y[chan] = (double)data.y;
		comm_z[chan] = (double)data.z;
		comm_lon[chan] = (double)data.lon;
		comm_lat[chan] = (double)data.lat;
		comm_elev[chan] = (double)data.elev;
		comm_valid[chan] = true;
		
		// This was a switch-case statement but the compiler didn't like the new variable creation with it. 
		if(comm_type[chan] == ATIS) {
			FGATIS* a = new FGATIS;
			a->SetData(&data);
			comm_atc_ptr[chan] = a;
			a->SetDisplay();
			//a->set_refname((chan) ? "atis2" : "atis1");		// FIXME - that line is limited to 2 channels
			atc_list.push_back(a);
		} else if (comm_type[chan] == TOWER) {
			FGATC* app = FindInList(comm_ident[chan], TOWER);
			if(app != NULL) {
				// The station is already in the ATC list
				app->SetDisplay();
			} else {
				// Generate the station and put in the ATC list
				FGTower* t = new FGTower;
				t->SetData(&data);
				comm_atc_ptr[chan] = t;
				t->SetDisplay();
				atc_list.push_back(t);
			}
		} else if (comm_type[chan] == APPROACH) {
			// We have to be a bit more carefull here since approaches are also searched by area
			FGATC* app = FindInList(comm_ident[chan], APPROACH);
			if(app != NULL) {
				// The station is already in the ATC list
				app->AddPlane("Player");
				app->SetDisplay();
				comm_atc_ptr[chan] = app;
			} else {
				// Generate the station and put in the ATC list
				FGApproach* a = new FGApproach;
				a->SetData(&data);
				a->AddPlane("Player");
				atc_list.push_back(a);
				comm_atc_ptr[chan] = a;
			}			
		}
	} else {
		if(comm_valid[chan]) {
			if(comm_type[chan] != APPROACH) {
				// Currently approaches are removed by Alexander's out-of-range mechanism
				CommRemoveFromList(comm_ident[chan], comm_type[chan], chan);
			}
			// Note that we *don't* call SetNoDisplay() here because the other comm channel
			// might be tuned into the same station - this is handled by CommRemoveFromList(...)
			comm_type[chan] = INVALID;
			comm_atc_ptr[chan] = NULL;
			comm_valid[chan] = false;
		}
	}
}


// Search ATC stations by area in order that we appear 'on the radar'
void FGATCMgr::AreaSearch() {
	// Search for Approach stations
	comm_list_type approaches;
	comm_list_iterator app_itr;
	
	lon = lon_node->getDoubleValue();
	lat = lat_node->getDoubleValue();
	elev = elev_node->getDoubleValue() * SG_FEET_TO_METER;
	
	// search stations in range
	int num_app = current_commlist->FindByPos(lon, lat, elev, &approaches, APPROACH);
	if (num_app != 0) {
		//cout << num_app << " approaches found in radiostack search !!!!" << endl;
		
		for(app_itr = approaches.begin(); app_itr != approaches.end(); app_itr++) {
			
			FGATC* app = FindInList((app_itr->ident).c_str(), app_itr->type);
			if(app != NULL) {
				// The station is already in the ATC list
				app->AddPlane("Player");
				//app->Update();
			} else {
				// Generate the station and put in the ATC list
				FGApproach* a = new FGApproach;
				a->SetData(&(*app_itr));
				a->AddPlane("Player");
				//a->Update();
				atc_list.push_back(a);
			}
		}
	}
	
	// remove planes which are out of range
	// TODO - I'm not entirely sure that this belongs here.
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
