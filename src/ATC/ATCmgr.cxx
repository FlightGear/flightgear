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

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <Airports/simple.hxx>
#include "ATCmgr.hxx"
#include "commlist.hxx"
#include "ATCdisplay.hxx"
#include "ATCDialog.hxx"
#include "ATCutils.hxx"

/*
// periodic radio station search wrapper
static void fgATCSearch( void ) {
	globals->get_ATC_mgr()->Search();
}
*/ //This wouldn't compile - including Time/event.hxx breaks it :-(
   // Is this still true?? -EMH-

AirportATC::AirportATC() :
    lon(0.0),
    lat(0.0),
    elev(0.0),
    atis_freq(0.0),
    atis_active(false),
    tower_freq(0.0),
    tower_active(false),
    ground_freq(0.0),
    ground_active(false),
    set_by_AI(false),
	numAI(0)
	//airport_atc_map.clear();
{
	for(int i=0; i<ATC_NUM_TYPES; ++i) {
		set_by_comm[0][i] = false;
		set_by_comm[1][i] = false;
	}
}

FGATCMgr::FGATCMgr() {
	comm_ident[0] = "";
	comm_ident[1] = "";
	//last_comm_ident[0] = "";
	//last_comm_ident[1] = "";
	//approach_ident = "";
	last_in_range = false;
	comm_type[0] = INVALID;
	comm_type[1] = INVALID;
	comm_atc_ptr[0] = NULL;
	comm_atc_ptr[1] = NULL;
	comm_valid[0] = false;
	comm_valid[1] = false;
	
	initDone = false;
}

FGATCMgr::~FGATCMgr() {
	delete v1;
}

void FGATCMgr::bind() {
}

void FGATCMgr::unbind() {
}

void FGATCMgr::init() {
	//cout << "ATCMgr::init called..." << endl;
	
	comm_node[0] = fgGetNode("/instrumentation/comm[0]/frequencies/selected-mhz", true);
	comm_node[1] = fgGetNode("/instrumentation/comm[1]/frequencies/selected-mhz", true);
	lon_node = fgGetNode("/position/longitude-deg", true);
	lat_node = fgGetNode("/position/latitude-deg", true);
	elev_node = fgGetNode("/position/altitude-ft", true);
	atc_list_itr = atc_list.begin();
	
	// Search for connected ATC stations once per 0.8 seconds or so
	// globals->get_event_mgr()->add( "fgATCSearch()", fgATCSearch,
        //                                 FGEvent::FG_EVENT_READY, 800);
        //  
	// For some reason the above doesn't compile - including Time/event.hxx stops compilation.
        // Is this still true after the reorganization of the event managar??
        // -EMH-
	
	// Initialise the frequency search map
    current_commlist = new FGCommList;
    SGPath p_comm( globals->get_fg_root() );
    current_commlist->init( p_comm );
	
#ifdef ENABLE_AUDIO_SUPPORT	
	// Load all available voices.
	// For now we'll do one hardwired one
	
	v1 = new FGATCVoice;
	voiceOK = v1->LoadVoice("default");
	voice = true;
	
	/* I've loaded the voice even if /sim/sound/pause is true
	*  since I know no way of forcing load of the voice if the user
	*  subsequently switches /sim/sound/audible to true.
        *  (which is the right thing to do -- CLO) :-) */
#else
	voice = false;
#endif

	// Initialise the ATC Dialogs
	//cout << "Initing Transmissions..." << endl;
    SG_LOG(SG_ATC, SG_INFO, "  ATC Transmissions");
    current_transmissionlist = new FGTransmissionList;
    SGPath p_transmission( globals->get_fg_root() );
    p_transmission.append( "ATC/default.transmissions" );
    current_transmissionlist->init( p_transmission );
	//cout << "Done Transmissions" << endl;

    SG_LOG(SG_ATC, SG_INFO, "  ATC Dialog System");
    current_atcdialog = new FGATCDialog;
    current_atcdialog->Init();

	initDone = true;
	//cout << "ATCmgr::init done!" << endl;
}

void FGATCMgr::update(double dt) {
	if(!initDone) {
		init();
		SG_LOG(SG_ATC, SG_WARN, "Warning - ATCMgr::update(...) called before ATCMgr::init()");
	}
	
	current_atcdialog->Update(dt);
	
	//cout << "Entering update..." << endl;
	//Traverse the list of active stations.
	//Only update one class per update step to avoid the whole ATC system having to calculate between frames.
	//Eventually we should only update every so many steps.
	//cout << "In FGATCMgr::update - atc_list.size = " << atc_list.size() << endl;
	if(atc_list.size()) {
		if(atc_list_itr == atc_list.end()) {
			atc_list_itr = atc_list.begin();
		}
		//cout << "Updating " << (*atc_list_itr)->get_ident() << ' ' << (*atc_list_itr)->GetType() << '\n';
		//cout << "Freq = " << (*atc_list_itr)->get_freq() << '\n';
		(*atc_list_itr)->Update(dt * atc_list.size());
		//cout << "Done ATC update..." << endl;
		++atc_list_itr;
	}
	
	/*
	cout << "ATC_LIST: " << atc_list.size() << ' ';
	for(atc_list_iterator it = atc_list.begin(); it != atc_list.end(); it++) {
		cout << (*it)->get_ident() << ' ';
	}
	cout << '\n';
	*/
	
	// Search the tuned frequencies every now and then - this should be done with the event scheduler
	static int i = 0;	// Very ugly - but there should only ever be one instance of FGATCMgr.
	/*
	if(i == 7) {
		//cout << "About to AreaSearch()" << endl;
		AreaSearch();
	}
	*/
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
	//cout << "Leaving update..." << endl;
}


// Returns frequency in KHz - should I alter this to return in MHz?
unsigned short int FGATCMgr::GetFrequency(const string& ident, const atc_type& tp) {
	ATCData test;
	bool ok = current_commlist->FindByCode(ident, test, tp);
	return(ok ? test.freq : 0);
}	


// Register the fact that the AI system wants to activate an airport
// Might need more sophistication in this in the future - eg registration by aircraft call-sign.
bool FGATCMgr::AIRegisterAirport(const string& ident) {
	SG_LOG(SG_ATC, SG_BULK, "AI registered airport " << ident << " with the ATC system");
	//cout << "AI registered airport " << ident << " with the ATC system" << '\n';
	if(airport_atc_map.find(ident) != airport_atc_map.end()) {
		airport_atc_map[ident]->set_by_AI = true;
		airport_atc_map[ident]->numAI++;
		return(true);
	} else {
		const FGAirport *ap = fgFindAirportID(ident);
		if (ap) {
			//cout << "ident = " << ident << '\n';
			AirportATC *a = new AirportATC;
			// I'm not entirely sure that this AirportATC structure business is actually needed - it just duplicates what we can find out anyway!
			a->lon = ap->getLongitude();
			a->lat = ap->getLatitude();
			a->elev = ap->getElevation();
			a->atis_freq = GetFrequency(ident, ATIS);
			//cout << "ATIS freq = " << a->atis_freq << '\n';
			a->atis_active = false;
			a->tower_freq = GetFrequency(ident, TOWER);
			//cout << "Tower freq = " << a->tower_freq << '\n';
			a->tower_active = false;
			a->ground_freq = GetFrequency(ident, GROUND);
			//cout << "Ground freq = " << a->ground_freq << '\n';
			a->ground_active = false;
			// TODO - some airports will have a tower/ground frequency but be inactive overnight.
			a->set_by_AI = true;
			a->numAI = 1;
			airport_atc_map[ident] = a;
			return(true);
		} else {
			SG_LOG(SG_ATC, SG_ALERT, "ERROR - can't find airport " << ident << " in AIRegisterAirport(...)");
		}
	}
	return(false);
}


// Register the fact that the comm radio is tuned to an airport
// Channel is zero based
bool FGATCMgr::CommRegisterAirport(const string& ident, int chan, const atc_type& tp) {
	SG_LOG(SG_ATC, SG_BULK, "Comm channel " << chan << " registered airport " << ident);
	//cout << "Comm channel " << chan << " registered airport " << ident << ' ' << tp << '\n';
	if(airport_atc_map.find(ident) != airport_atc_map.end()) {
		//cout << "IN MAP - flagging set by comm..." << endl;
		airport_atc_map[ident]->set_by_comm[chan][tp] = true;
		if(tp == ATIS) {
			airport_atc_map[ident]->atis_active = true;
		} else if(tp == TOWER) {
			airport_atc_map[ident]->tower_active = true;
		} else if(tp == GROUND) {
			airport_atc_map[ident]->ground_active = true;
		} else if(tp == APPROACH) {
			//a->approach_active = true;
		}	// TODO - there *must* be a better way to do this!!!
		return(true);
	} else {
		//cout << "NOT IN MAP - creating new..." << endl;
		const FGAirport *ap = fgFindAirportID(ident);
		if (ap) {
			AirportATC *a = new AirportATC;
			// I'm not entirely sure that this AirportATC structure business is actually needed - it just duplicates what we can find out anyway!
			a->lon = ap->getLongitude();
			a->lat = ap->getLatitude();
			a->elev = ap->getElevation();
			a->atis_freq = GetFrequency(ident, ATIS);
			a->atis_active = false;
			a->tower_freq = GetFrequency(ident, TOWER);
			a->tower_active = false;
			a->ground_freq = GetFrequency(ident, GROUND);
			a->ground_active = false;
			if(tp == ATIS) {
				a->atis_active = true;
			} else if(tp == TOWER) {
				a->tower_active = true;
			} else if(tp == GROUND) {
				a->ground_active = true;
			} else if(tp == APPROACH) {
				//a->approach_active = true;
			}	// TODO - there *must* be a better way to do this!!!
			// TODO - some airports will have a tower/ground frequency but be inactive overnight.
			a->set_by_AI = false;
			a->numAI = 0;
			a->set_by_comm[chan][tp] = true;
			airport_atc_map[ident] = a;
			return(true);
		}
	}
	return(false);
}


// Remove from list only if not needed by the AI system or the other comm channel
// Note that chan is zero based.
void FGATCMgr::CommRemoveFromList(const string& id, const atc_type& tp, int chan) {
	SG_LOG(SG_ATC, SG_BULK, "CommRemoveFromList called for airport " << id << " " << tp << " by channel " << chan);
	//cout << "CommRemoveFromList called for airport " << id << " " << tp << " by channel " << chan << '\n';
	if(airport_atc_map.find(id) != airport_atc_map.end()) {
		AirportATC* a = airport_atc_map[id];
		//cout << "In CommRemoveFromList, a->ground_freq = " << a->ground_freq << endl;
		if(a->set_by_AI && tp != ATIS) {
			// Set by AI, so don't remove simply because user isn't tuned in any more - just stop displaying
			SG_LOG(SG_ATC, SG_BULK, "In CommRemoveFromList, service was set by AI\n");
			FGATC* aptr = GetATCPointer(id, tp);
			switch(chan) {
			case 0:
				//cout << "chan 1\n";
				a->set_by_comm[0][tp] = false;
				if(!a->set_by_comm[1][tp]) {
					//cout << "not set by comm2\n";
					if(aptr != NULL) {
						//cout << "Got pointer\n";
						aptr->SetNoDisplay();
						//cout << "Setting no display...\n";
					} else {
						//cout << "Not got pointer\n";
					}
				}
				break;
			case 1:
				a->set_by_comm[1][tp] = false;
				if(!a->set_by_comm[0][tp]) {
					if(aptr != NULL) {
						aptr->SetNoDisplay();
						//cout << "Setting no display...\n";
					}
				}
				break;
			}
			//airport_atc_map[id] = a;
			return;
		} else {
			switch(chan) {
			case 0:
				a->set_by_comm[0][tp] = false;
				// Remove only if not also set by the other comm channel
				if(!a->set_by_comm[1][tp]) {
					a->tower_active = false;
					a->ground_active = false;
					RemoveFromList(id, tp);
				}
				break;
			case 1:
				a->set_by_comm[1][tp] = false;
				if(!a->set_by_comm[0][tp]) {
					a->tower_active = false;
					a->ground_active = false;
					RemoveFromList(id, tp);
				}
				break;
			}
		}
	}
}
    

// Remove from list - should only be called from above or similar
// This function *will* remove it from the list regardless of who else might want it.
void FGATCMgr::RemoveFromList(const string& id, const atc_type& tp) {
	//cout << "FGATCMgr::RemoveFromList called..." << endl;
	//cout << "Requested type = " << tp << endl;
	//cout << "id = " << id << endl;
	atc_list_iterator it = atc_list.begin();
	while(it != atc_list.end()) {
		//cout << "type = " << (*it)->GetType() << '\n';
		//cout << "Ident = " << (*it)->get_ident() << '\n';
		if( ((*it)->get_ident() == id)
			&& ((*it)->GetType() == tp) ) {
			//Before removing it stop it transmitting!!
			//cout << "OBLITERATING FROM LIST!!!\n";
			(*it)->SetNoDisplay();
			(*it)->Update(0.00833);
			delete (*it);
			atc_list.erase(it);
			atc_list_itr = atc_list.begin();	// Reset the persistent itr incase we've left it off the end.
			break;
		}
		++it;
	}
}


// Find in list - return a currently active ATC pointer given ICAO code and type
// Return NULL if the given service is not in the list
// - *** THE CALLING FUNCTION MUST CHECK FOR THIS ***
FGATC* FGATCMgr::FindInList(const string& id, const atc_type& tp) {
	//cout << "Entering FindInList for " << id << ' ' << tp << endl;
	atc_list_iterator it = atc_list.begin();
	while(it != atc_list.end()) {
		if( ((*it)->get_ident() == id)
		    && ((*it)->GetType() == tp) ) {
			return(*it);
		}
		++it;
	}
	// If we get here it's not in the list
	//cout << "Couldn't find it in the list though :-(" << endl;
	return(NULL);
}

// Returns true if the airport is found in the map
bool FGATCMgr::GetAirportATCDetails(const string& icao, AirportATC* a) {
	if(airport_atc_map.find(icao) != airport_atc_map.end()) {
		*a = *airport_atc_map[icao];
		return(true);
	} else {
		return(false);
	}
}


// Return a pointer to a given sort of ATC at a given airport and activate if necessary
// Returns NULL if service doesn't exist - calling function should check for this.
// We really ought to make this private and call it from the CommRegisterAirport / AIRegisterAirport functions
// - at the moment all these GetATC... functions exposed are just too complicated.
FGATC* FGATCMgr::GetATCPointer(const string& icao, const atc_type& type) {
	if(airport_atc_map.find(icao) == airport_atc_map.end()) {
		//cout << "Unable to find " << icao << ' ' << type << " in the airport_atc_map" << endl;
		return NULL;
	}
	//cout << "In GetATCPointer, found " << icao << ' ' << type << endl;
	AirportATC *a = airport_atc_map[icao];
	//cout << "a->lon = " << a->lon << '\n';
	//cout << "a->elev = " << a->elev << '\n';
	//cout << "a->tower_freq = " << a->tower_freq << '\n';
	switch(type) {
	case TOWER:
		if(a->tower_active) {
			// Get the pointer from the list
			return(FindInList(icao, type));
		} else {
			ATCData data;
			if(current_commlist->FindByFreq(a->lon, a->lat, a->elev, a->tower_freq, &data, TOWER)) {
				FGTower* t = new FGTower;
				t->SetData(&data);
				atc_list.push_back(t);
				a->tower_active = true;
				airport_atc_map[icao] = a;
				//cout << "Initing tower " << icao << " in GetATCPointer()\n";
				t->Init();
				return(t);
			} else {
				SG_LOG(SG_ATC, SG_ALERT, "ERROR - tower that should exist in FGATCMgr::GetATCPointer for airport " << icao << " not found");
			}
		}
		break;
	case APPROACH:
		break;
	case ATIS:
		SG_LOG(SG_ATC, SG_ALERT, "ERROR - ATIS station should not be requested from FGATCMgr::GetATCPointer");
		break;
	case GROUND:
		//cout << "IN CASE GROUND" << endl;
		if(a->ground_active) {
			// Get the pointer from the list
			return(FindInList(icao, type));
		} else {
			ATCData data;
			if(current_commlist->FindByFreq(a->lon, a->lat, a->elev, a->ground_freq, &data, GROUND)) {
				FGGround* g = new FGGround;
				g->SetData(&data);
				atc_list.push_back(g);
				a->ground_active = true;
				airport_atc_map[icao] = a;
				g->Init();
				return(g);
			} else {
				SG_LOG(SG_ATC, SG_ALERT, "ERROR - ground control that should exist in FGATCMgr::GetATCPointer for airport " << icao << " not found");
			}
		}
		break;
		case INVALID:
		break;
		case ENROUTE:
		break;
		case DEPARTURE:
		break;
	}
	
	SG_LOG(SG_ATC, SG_ALERT, "ERROR IN FGATCMgr - reached end of GetATCPointer");
	//cout << "ERROR IN FGATCMgr - reached end of GetATCPointer" << endl;
	
	return(NULL);
}

// Return a pointer to an appropriate voice for a given type of ATC
// creating the voice if necessary - ie. make sure exactly one copy
// of every voice in use exists in memory.
//
// TODO - in the future this will get more complex and dole out country/airport
// specific voices, and possible make sure that the same voice doesn't get used
// at different airports in quick succession if a large enough selection are available.
FGATCVoice* FGATCMgr::GetVoicePointer(const atc_type& type) {
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
		comm_ident[chan] = data.ident;
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
			CommRegisterAirport(comm_ident[chan], chan, ATIS);
			FGATC* app = FindInList(comm_ident[chan], ATIS);
			if(app != NULL) {
				// The station is already in the ATC list
				//cout << "In list - flagging SetDisplay..." << endl;
				comm_atc_ptr[chan] = app;
				app->SetDisplay();
			} else {
				// Generate the station and put in the ATC list
				//cout << "Not in list - generating..." << endl;
				FGATIS* a = new FGATIS;
				a->SetData(&data);
				comm_atc_ptr[chan] = a;
				a->SetDisplay();
				//a->Init();
				atc_list.push_back(a);
			}
		} else if (comm_type[chan] == TOWER) {
			//cout << "TOWER TOWER TOWER\n";
			CommRegisterAirport(comm_ident[chan], chan, TOWER);
			//cout << "Done (TOWER)" << endl;
			FGATC* app = FindInList(comm_ident[chan], TOWER);
			if(app != NULL) {
				// The station is already in the ATC list
				SG_LOG(SG_GENERAL, SG_DEBUG, comm_ident[chan] << " is in list - flagging SetDisplay...");
				//cout << comm_ident[chan] << " is in list - flagging SetDisplay...\n";
				comm_atc_ptr[chan] = app;
				app->SetDisplay();
			} else {
				// Generate the station and put in the ATC list
				SG_LOG(SG_GENERAL, SG_DEBUG, comm_ident[chan] << " is not in list - generating...");
				//cout << comm_ident[chan] << " is not in list - generating...\n";
				FGTower* t = new FGTower;
				t->SetData(&data);
				comm_atc_ptr[chan] = t;
				//cout << "Initing tower in FreqSearch()\n";
				t->Init();
				t->SetDisplay();
				atc_list.push_back(t);
			}
		}  else if (comm_type[chan] == GROUND) {
			CommRegisterAirport(comm_ident[chan], chan, GROUND);
			//cout << "Done (GROUND)" << endl;
			FGATC* app = FindInList(comm_ident[chan], GROUND);
			if(app != NULL) {
				// The station is already in the ATC list
				comm_atc_ptr[chan] = app;
				app->SetDisplay();
			} else {
				// Generate the station and put in the ATC list
				FGGround* g = new FGGround;
				g->SetData(&data);
				comm_atc_ptr[chan] = g;
				g->Init();
				g->SetDisplay();
				atc_list.push_back(g);
			}
		} else if (comm_type[chan] == APPROACH) {
			// We have to be a bit more carefull here since approaches are also searched by area
			CommRegisterAirport(comm_ident[chan], chan, APPROACH);
			//cout << "Done (APPROACH)" << endl;
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
				comm_atc_ptr[chan] = a;
				a->Init();
				a->SetDisplay();
				a->AddPlane("Player");
				atc_list.push_back(a);
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
	int num_app = current_commlist->FindByPos(lon, lat, elev, 100.0, &approaches, APPROACH);
	if (num_app != 0) {
		//cout << num_app << " approaches found in radiostack search !!!!" << endl;
		
		for(app_itr = approaches.begin(); app_itr != approaches.end(); app_itr++) {
			
			FGATC* app = FindInList(app_itr->ident, app_itr->type);
			if(app != NULL) {
				// The station is already in the ATC list
				//cout << "In list adding player\n";
				app->AddPlane("Player");
				//app->Update();
			} else {
				// Generate the station and put in the ATC list
				FGApproach* a = new FGApproach;
				a->SetData(&(*app_itr));
				//cout << "Adding player\n";
				a->AddPlane("Player");
				//a->Update();
				atc_list.push_back(a);
			}
		}
	}
	
	// remove planes which are out of range
	// TODO - I'm not entirely sure that this belongs here.
	atc_list_iterator it = atc_list.begin();
	while(it != atc_list.end()) {
		if((*it)->GetType() == APPROACH ) {
			int np = (*it)->RemovePlane();
			// if approach has no planes left remove it from ATC list
			if ( np == 0) {
				//cout << "REMOVING AN APPROACH STATION WITH NO PLANES..." << endl;
				(*it)->SetNoDisplay();
				(*it)->Update(0.00833);
				delete (*it);
				atc_list.erase(it);
				atc_list_itr = atc_list.begin();	// Reset the persistent itr incase we've left it off the end.
				break;     // the other stations will be checked next time
			}
		}
		++it;
	}
}
