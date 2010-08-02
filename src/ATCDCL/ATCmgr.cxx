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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>

#include <Airports/simple.hxx>

#include "ATCmgr.hxx"
#include "commlist.hxx"
#include "ATCDialog.hxx"
#include "ATCutils.hxx"
#include "transmissionlist.hxx"
#include "ground.hxx"


/*
// periodic radio station search wrapper
static void fgATCSearch( void ) {
    globals->get_ATC_mgr()->Search();
}
*/ //This wouldn't compile - including Time/event.hxx breaks it :-(
   // Is this still true?? -EMH-

AirportATC::AirportATC() :
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
}

FGATCMgr::FGATCMgr() :
    initDone(false),
    atc_list(new atc_list_type),
#ifdef ENABLE_AUDIO_SUPPORT
    voiceOK(false),
    voice(true),
#else
    voice(false),
#endif
    last_in_range(false),
    v1(0)
{
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
    
    lon_node = fgGetNode("/position/longitude-deg", true);
    lat_node = fgGetNode("/position/latitude-deg", true);
    elev_node = fgGetNode("/position/altitude-ft", true);
    atc_list_itr = atc_list->begin();
    
    // Search for connected ATC stations once per 0.8 seconds or so
    // globals->get_event_mgr()->add( "fgATCSearch()", fgATCSearch,
        //                                 FGEvent::FG_EVENT_READY, 800);
        //  
    // For some reason the above doesn't compile - including Time/event.hxx stops compilation.
        // Is this still true after the reorganization of the event managar??
        // -EMH-
    
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
    //cout << "In FGATCMgr::update - atc_list.size = " << atc_list->size() << endl;
    if(atc_list->size()) {
        if(atc_list_itr == atc_list->end()) {
            atc_list_itr = atc_list->begin();
        }
        //cout << "Updating " << (*atc_list_itr)->get_ident() << ' ' << (*atc_list_itr)->GetType() << '\n';
        //cout << "Freq = " << (*atc_list_itr)->get_freq() << '\n';
        (*atc_list_itr).second->Update(dt * atc_list->size());
        //cout << "Done ATC update..." << endl;
        ++atc_list_itr;
    }
    
#ifdef ATC_TEST
    cout << "ATC_LIST: " << atc_list->size() << ' ';
    for(atc_list_iterator it = atc_list->begin(); it != atc_list->end(); it++) {
        cout << (*it)->get_ident() << ' ';
    }
    cout << '\n';
#endif
    
    // Search the tuned frequencies every now and then - this should be done with the event scheduler
    static int i = 0;   // Very ugly - but there should only ever be one instance of FGATCMgr.
    /*** Area search is defeated.  Why?
    if(i == 7) {
        //cout << "About to AreaSearch()" << endl;
        AreaSearch();
    }
    ***/
    if(i == 15) {
        //cout << "About to search navcomm1" << endl;
        FreqSearch("comm", 0);
        FreqSearch("nav", 0);
    }
    if(i == 30) {
        //cout << "About to search navcomm2" << endl;
        FreqSearch("comm", 1);
        FreqSearch("nav", 1);
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

// Register the fact that the comm radio is tuned to an airport
// Channel is zero based
bool FGATCMgr::CommRegisterAirport(const string& ident, int chan, const atc_type& tp) {
    SG_LOG(SG_ATC, SG_BULK, "Comm channel " << chan << " registered airport " << ident);
    //cout << "Comm channel " << chan << " registered airport " << ident << ' ' << tp << '\n';
    if(airport_atc_map.find(ident) != airport_atc_map.end()) {
        //cout << "IN MAP - flagging set by comm..." << endl;
//xx        airport_atc_map[ident]->set_by_comm[chan][tp] = true;
        if(tp == ATIS || tp == AWOS) {
            airport_atc_map[ident]->atis_active = true;
        } else if(tp == TOWER) {
            airport_atc_map[ident]->tower_active = true;
        } else if(tp == GROUND) {
            airport_atc_map[ident]->ground_active = true;
        } else if(tp == APPROACH) {
            //a->approach_active = true;
        }   // TODO - there *must* be a better way to do this!!!
        return(true);
    } else {
        //cout << "NOT IN MAP - creating new..." << endl;
        const FGAirport *ap = fgFindAirportID(ident);
        if (ap) {
            AirportATC *a = new AirportATC;
            // I'm not entirely sure that this AirportATC structure business is actually needed - it just duplicates what we can find out anyway!
            a->geod = ap->geod();
            a->atis_freq = GetFrequency(ident, ATIS)
                    || GetFrequency(ident, AWOS);
            a->atis_active = false;
            a->tower_freq = GetFrequency(ident, TOWER);
            a->tower_active = false;
            a->ground_freq = GetFrequency(ident, GROUND);
            a->ground_active = false;
            if(tp == ATIS || tp == AWOS) {
                a->atis_active = true;
            } else if(tp == TOWER) {
                a->tower_active = true;
            } else if(tp == GROUND) {
                a->ground_active = true;
            } else if(tp == APPROACH) {
                //a->approach_active = true;
            }   // TODO - there *must* be a better way to do this!!!
            // TODO - some airports will have a tower/ground frequency but be inactive overnight.
            a->set_by_AI = false;
            a->numAI = 0;
//xx            a->set_by_comm[chan][tp] = true;
            airport_atc_map[ident] = a;
            return(true);
        }
    }
    return(false);
}

typedef map<string,int> MSI;

void FGATCMgr::ZapOtherService(const string ncunit, const string svc_name){
  for (atc_list_iterator svc = atc_list->begin(); svc != atc_list->end(); svc++) {
   
    if (svc->first != svc_name) {
      MSI &actv = svc->second->active_on;
      // OK, we have found some OTHER service;
      // see if it is (was) active on our unit:
      if (!actv.count(ncunit)) continue;
      // cout << "Eradicating '" << svc->first << "' from: " << ncunit << endl;
      actv.erase(ncunit);
      if (!actv.size()) {
        //cout << "Eradicating service: '" << svc->first << "'" << endl;
    svc->second->SetNoDisplay();
    svc->second->Update(0);     // one last update
    SG_LOG(SG_GENERAL, SG_INFO, "would have erased ATC service:" << svc->second->get_name()<< "/" 
      << svc->second->get_ident());
   // delete svc->second;
    atc_list->erase(svc);
// ALL pointers into the ATC list are now invalid,
// so let's reset them:
    atc_list_itr = atc_list->begin();
      }
      break;        // cannot be duplicates in the active list
    }
  }
}


// Find in list - return a currently active ATC pointer given ICAO code and type
// Return NULL if the given service is not in the list
// - *** THE CALLING FUNCTION MUST CHECK FOR THIS ***
FGATC* FGATCMgr::FindInList(const string& id, const atc_type& tp) {
  string ndx = id + decimalNumeral(tp);
  if (!atc_list->count(ndx)) return 0;
  return (*atc_list)[ndx];
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
            if(current_commlist->FindByFreq(a->geod, a->tower_freq, &data, TOWER)) {
                FGTower* t = new FGTower;
                t->SetData(&data);
                (*atc_list)[icao+decimalNumeral(type)] = t;
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
    case ATIS: case AWOS:
        SG_LOG(SG_ATC, SG_ALERT, "ERROR - ATIS station should not be requested from FGATCMgr::GetATCPointer");
        break;
    case GROUND:
        //cout << "IN CASE GROUND" << endl;
        if(a->ground_active) {
            // Get the pointer from the list
            return(FindInList(icao, type));
        } else {
            ATCData data;
            if(current_commlist->FindByFreq(a->geod, a->ground_freq, &data, GROUND)) {
                FGGround* g = new FGGround;
                g->SetData(&data);
                (*atc_list)[icao+decimalNumeral(type)] = g;
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
        case ATIS: case AWOS:
#ifdef ENABLE_AUDIO_SUPPORT
            // Delayed loading fo all available voices, needed because the
            // soundmanager might not be initialized (at all) at this point.
            // For now we'll do one hardwired one

            /* I've loaded the voice even if /sim/sound/pause is true
             *  since I know no way of forcing load of the voice if the user
             *  subsequently switches /sim/sound/audible to true.
             *  (which is the right thing to do -- CLO) :-)
             */
            if (!voiceOK && fgGetBool("/sim/sound/working")) {
                v1 = new FGATCVoice;
                try {
                    voiceOK = v1->LoadVoice("default");
                    voice = voiceOK;
                } catch ( sg_io_exception & e) {
                    voiceOK  = false;
                    SG_LOG(SG_ATC, SG_ALERT, "Unable to load default voice : "
                                            << e.getFormattedMessage().c_str());
                    voice = false;
                    delete v1;
                    v1 = 0;
                }
            }
#endif
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
void FGATCMgr::FreqSearch(const string navcomm, const int unit) {


        string ncunit = navcomm + "[" + decimalNumeral(unit) + "]";
    string commbase = "/instrumentation/" + ncunit;
    string commfreq = commbase + "/frequencies/selected-mhz";
        SGPropertyNode_ptr comm_node = fgGetNode(commfreq.c_str(), false);

    //cout << "FreqSearch: " << ncunit
    //  << "  node: " << comm_node << endl;
    if (!comm_node) return; // no such radio unit

    ATCData data;   
    double freq = comm_node->getDoubleValue();
    _aircraftPos = SGGeod::fromDegFt(lon_node->getDoubleValue(),
      lat_node->getDoubleValue(), elev_node->getDoubleValue());
    
// Query the data store and get the closest match if any
    //cout << "Will FindByFreq: " << lat << " " << lon << " " << elev
    //      << "  freq: " << freq << endl;
    if(current_commlist->FindByFreq(_aircraftPos, freq, &data)) {
      //cout << "FoundByFreq: " << freq 
      //  << "  ident: " << data.ident 
      //  << "  type: " << data.type << " ***" << endl;
// We are in range of something.


// Get rid of any *other* service that was on this radio unit:
            string svc_name = data.ident+decimalNumeral(data.type);
        ZapOtherService(ncunit, svc_name);
// See if the service already exists, possibly connected to
// some other radio unit:
        if (atc_list->count(svc_name)) {
          // make sure the service knows it's tuned on this radio:
          FGATC* svc = (*atc_list)[svc_name];
          svc->active_on[ncunit] = 1;
          svc->SetDisplay();
          if (data.type == APPROACH) svc->AddPlane("Player");
          return;
        }

        CommRegisterAirport(data.ident, unit, data.type);

// This was a switch-case statement but the compiler didn't like 
// the new variable creation with it. 
        if      (data.type == ATIS
              || data.type == AWOS)     (*atc_list)[svc_name] = new FGATIS;
        else if (data.type == TOWER)    (*atc_list)[svc_name] = new FGTower;
        else if (data.type == GROUND)   (*atc_list)[svc_name] = new FGGround;
        else if (data.type == APPROACH) (*atc_list)[svc_name] = new FGApproach;
        FGATC* svc = (*atc_list)[svc_name];
        svc->SetData(&data);
        svc->active_on[ncunit] = 1;
        svc->SetDisplay();
        svc->Init();
        if (data.type == APPROACH) svc->AddPlane("Player");
    } else {
      // No services in range.  Zap any service on this unit.
      ZapOtherService(ncunit, "x x x");
    } 
}

#ifdef AREA_SEARCH
/* I don't think AreaSearch ever gets called */
// Search ATC stations by area in order that we appear 'on the radar'
void FGATCMgr::AreaSearch() {
  const string AREA("AREA");
  // Search for Approach stations
  comm_list_type approaches;
  comm_list_iterator app_itr;

  lon = lon_node->getDoubleValue();
  lat = lat_node->getDoubleValue();
  elev = elev_node->getDoubleValue() * SG_FEET_TO_METER;
  for (atc_list_iterator svc = atc_list->begin(); svc != atc_list->end(); svc++) {
    MSI &actv = svc->second->active_on;
    if (actv.count(AREA)) actv[AREA] = 0;  // Mark all as maybe not in range
  }

  // search stations in range
  int num_app = current_commlist->FindByPos(lon, lat, elev, 100.0, &approaches, APPROACH);
  if (num_app != 0) {
    //cout << num_app << " approaches found in area search !!!!" << endl;

    for(app_itr = approaches.begin(); app_itr != approaches.end(); app_itr++) {
      FGATC* app = FindInList(app_itr->ident, app_itr->type);
      string svc_name = app_itr->ident+decimalNumeral(app_itr->type);
      if(app != NULL) {
    // The station is already in the ATC list
    app->AddPlane("Player");
      } else {
    // Generate the station and put in the ATC list
    FGApproach* a = new FGApproach;
    a->SetData(&(*app_itr));
    a->AddPlane("Player");
    (*atc_list)[svc_name] = a;
    //cout << "New area service: " << svc_name << endl;
      }
      FGATC* svc = (*atc_list)[svc_name];
      svc->active_on[AREA] = 1;
    }
  }

  for (atc_list_iterator svc = atc_list->begin(); svc != atc_list->end(); svc++) {
    MSI &actv = svc->second->active_on;
    if (!actv.count(AREA)) continue;
    if (!actv[AREA]) actv.erase(AREA);
    if (!actv.size()) {     // this service no longer active at all
      cout << "Eradicating area service: " << svc->first << endl;
      svc->second->SetNoDisplay();
      svc->second->Update(0);
      delete (svc->second);
      atc_list->erase(svc);
// Reset the persistent iterator, since any erase() makes it invalid:
      atc_list_itr = atc_list->begin(); 
// Hope we only move out of one approach-area;
// others will not be noticed until next update:
      break;
    }
  }
}
#endif
