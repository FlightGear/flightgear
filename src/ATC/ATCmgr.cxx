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
#include <simgear/misc/commands.hxx>
#include <simgear/debug/logstream.hxx>

#include "ATCmgr.hxx"
#include "atislist.hxx"
//#include "groundlist.hxx"
#include "towerlist.hxx"
#include "approachlist.hxx"
#include "ATCdisplay.hxx"

/*
// periodic radio station search wrapper
static void fgATCSearch( void ) {
	globals->get_ATC_mgr()->Search();
}
*/ //This wouldn't compile - including Time/event.hxx breaks it :-(

static char* t0 = "Request landing clearance";
static char* t1 = "Request departure clearance";
static char* t2 = "Report Runway vacated";
static char** towerOptions = new char*[4];
static char* a0 = "Request vectors";
static char** approachOptions = new char*[2];

// For the ATC dialog - copied from the Autopilot new heading dialog code!
static puDialogBox*		atcDialog;
static puFrame*			atcDialogFrame;
static puText*			atcDialogMessage;
//static puInput*			atcDialogInput;
static puOneShot*		atcDialogOkButton;
static puOneShot*		atcDialogCancelButton;
static puButtonBox*		atcDialogCommunicationOptions;

static void ATCDialogCancel(puObject *)
{
    //ATCDialogInput->rejectInput();
    FG_POP_PUI_DIALOG( atcDialog );
}

static void ATCDialogOK (puObject *me)
{
	switch(globals->get_ATC_mgr()->GetCurrentATCType()) {
	case INVALID:
		break;
	case ATIS:
		break;
	case TOWER: {
		FGTower* twr = (FGTower*)globals->get_ATC_mgr()->GetCurrentATCPointer();
		switch(atcDialogCommunicationOptions->getValue()) {
		case 0:
			cout << "Option 0 chosen\n";
			twr->RequestLandingClearance("golf bravo echo");
			break;
		case 1:
			cout << "Option 1 chosen\n";
			twr->RequestDepartureClearance("golf bravo echo");
			break;
		case 2:
			cout << "Option 2 chosen\n";
			twr->ReportRunwayVacated("golf bravo echo");
			break;
		default:
			break;
		}
		break;
	}
	case GROUND:
		break;
	case APPROACH:
		break;
	default:
		break;
	}

    ATCDialogCancel(me);
    //if(error) mkDialog(s.c_str());
}

static void ATCDialog(puObject *cb)
{
    //ApHeadingDialogInput   ->    setValue ( heading );
    //ApHeadingDialogInput    -> acceptInput();
    FG_PUSH_PUI_DIALOG(atcDialog);
}

static void ATCDialogInit()
{
	char defaultATCLabel[] = "Enter desired option to communicate with ATC:";
	char *s;

	// Option lists hardwired per ATC type	
	towerOptions[0] = new char[strlen(t0)];
	strcpy(towerOptions[0], t0);
	towerOptions[1] = new char[strlen(t1)];
	strcpy(towerOptions[1], t1);
	towerOptions[2] = new char[strlen(t2)];
	strcpy(towerOptions[2], t2);
	towerOptions[3] = NULL;
	
	approachOptions[0] = new char[strlen(a0)];
	strcpy(approachOptions[0], a0);
	approachOptions[1] = NULL;

	atcDialog = new puDialogBox (150, 50);
	{
		atcDialogFrame   = new puFrame (0, 0, 500, 250);
		
		atcDialogMessage = new puText          (250, 220);
		atcDialogMessage    -> setDefaultValue (defaultATCLabel);
		atcDialogMessage    -> getDefaultValue (&s);
		atcDialogMessage    -> setLabel        (s);
		atcDialogMessage    -> setLabelPlace   (PUPLACE_TOP_CENTERED);

		atcDialogCommunicationOptions = new puButtonBox (50, 50, 450, 210, NULL, true);
		
		atcDialogOkButton     =  new puOneShot         (50, 10, 110, 50);
		atcDialogOkButton     ->     setLegend         (gui_msg_OK);
		atcDialogOkButton     ->     makeReturnDefault (TRUE);
		atcDialogOkButton     ->     setCallback       (ATCDialogOK);
		
		atcDialogCancelButton =  new puOneShot         (140, 10, 210, 50);
		atcDialogCancelButton ->     setLegend         (gui_msg_CANCEL);
		atcDialogCancelButton ->     setCallback       (ATCDialogCancel);
		
	}
	FG_FINALIZE_PUI_DIALOG(atcDialog);
}

// For the command manager - maybe eventually this should go in the built in command list
static bool do_ATC_dialog(const SGPropertyNode* arg) {
	globals->get_ATC_mgr()->doStandardDialog();
	return(true);
}


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
	comm1_type = INVALID;
	tuned_atc_ptr = NULL;
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

#ifdef ENABLE_AUDIO_SUPPORT	
	// Load all available voices.
	// For now we'll do one hardwired one
	voiceOK = v1.LoadVoice("default");
	/* I've loaded the voice even if /sim/sound/audible is false
	*  since I know no way of forcing load of the voice if the user
	*  subsequently switches /sim/sound/audible to true. */
#else
	voice = false;
#endif

	// Initialise the ATC Dialogs
	ATCDialogInit();
	
	// Add ATC-dialog to the command list
	globals->get_commands()->addCommand("ATC-dialog", do_ATC_dialog);
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
		}
	}
}
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
	SG_LOG(SG_GENERAL, SG_ALERT, "*** Failed to find FGATC* in FGATCMgr::FindInList - this should not happen!");
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


// Render a transmission
// Outputs the transmission either on screen or as audio depending on user preference
// The repeating flag indicates whether the message should be repeated continuously or played once.
void FGATCMgr::Render(string msg, bool repeating) {
#ifdef ENABLE_AUDIO_SUPPORT
	voice = voiceOK && fgGetBool("/sim/sound/audible");
	if(voice) {
		int len;
		unsigned char* buf = v1.WriteMessage((char*)msg.c_str(), len, voice);
		if(voice) {
			FGSimpleSound* simple = new FGSimpleSound(buf, len);
			simple->set_volume(2.0);
			globals->get_soundmgr()->add(simple, refname);
			if(repeating) {
				globals->get_soundmgr()->play_looped(refname);
			} else {
				globals->get_soundmgr()->play_once(refname);
			}
		}
		delete[] buf;
	}
#endif	// ENABLE_AUDIO_SUPPORT
	if(!voice) {
		// first rip the underscores and the pause hints out of the string - these are for the convienience of the voice parser
		for(unsigned int i = 0; i < msg.length(); ++i) {
			if((msg.substr(i,1) == "_") || (msg.substr(i,1) == "/")) {
				msg[i] = ' ';
			}
		}
		globals->get_ATC_display()->RegisterRepeatingMessage(msg);
	}
	playing = true;	
}


// Cease rendering a transmission.
// At the moment this can handle one transmission active at a time only.
void FGATCMgr::NoRender() {
	if(playing) {
		if(voice) {
#ifdef ENABLE_AUDIO_SUPPORT		
			globals->get_soundmgr()->stop(refname);
			globals->get_soundmgr()->remove(refname);
#endif
		} else {
			globals->get_ATC_display()->CancelRepeatingMessage();
		}
		playing = false;
	}
}


// Display a dialog box with options relevant to the currently tuned ATC service.
void FGATCMgr::doStandardDialog() {
	/* DCL 2002/12/06 - This function currently in development 
	   and dosen't display anything usefull to the end-user */
	//cout << "FGATCMgr::doStandardDialog called..." << endl;
	
	// First - need to determine which ATC service (if any) the user is tuned to.
	//cout << "comm1_type = " << comm1_type << endl;
	
	// Second - customise the dialog box
	switch(comm1_type) {
	case INVALID:
		atcDialogCommunicationOptions->newList(NULL);
		atcDialogMessage->setLabel("Not tuned in to any ATC service.");
		break;
	case ATIS:
		atcDialogCommunicationOptions->newList(NULL);
		atcDialogMessage->setLabel("Tuned in to ATIS: no communication possible.");
		break;
	case TOWER: 
		atcDialogCommunicationOptions->newList(towerOptions);
		atcDialogMessage->setLabel("Tuned in to Tower - select communication to transmit:");
		break;
	case GROUND:
		atcDialogCommunicationOptions->newList(NULL);
		atcDialogMessage->setLabel("Tuned in to Ground - select communication to transmit:");
		break;
	case APPROACH:
		atcDialogCommunicationOptions->newList(approachOptions);
		atcDialogMessage->setLabel("Tuned in to Approach - select communication to transmit:");
		break;
	default:
		atcDialogCommunicationOptions->newList(NULL);
		atcDialogMessage->setLabel("Tuned in to unknown ATC service - enter transmission:");
		break;
	}
	
	// Third - display the dialog without pausing sim.
	ATCDialog(NULL);
	
	// Forth - need to direct input back from the dialog to the relevant ATC service.
	// This is in ATCDialogOK()
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
			tuned_atc_ptr = a;
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
			tuned_atc_ptr = NULL;
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
			tuned_atc_ptr = t;
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
			tuned_atc_ptr = NULL;
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
				tuned_atc_ptr = a;
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
