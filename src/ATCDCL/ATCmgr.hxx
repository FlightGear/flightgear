// ATCMgr.hxx - definition of FGATCMgr 
// - a global management class for FlightGear generated ATC
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

#ifndef _FG_ATCMGR_HXX
#define _FG_ATCMGR_HXX

#include <simgear/structure/subsystem_mgr.hxx>

#include <Main/fg_props.hxx>
#include <GUI/gui.h>

#include <string>
#include <list>
#include <map>

#include "ATC.hxx"

using std::string;
using std::list;
using std::map;

// Structure for holding details of the ATC frequencies at a given airport, and whether they are in the active list or not.
// These can then be cross referenced with the commlists which are stored by frequency or bucket.
// Non-available services are denoted by a frequency of zero.
// These structures are only intended to be created for in-use airports, and removed when no longer needed. 
struct AirportATC {
	AirportATC();
	
    SGGeod geod;
    float atis_freq;
    bool atis_active;
};

class FGATCMgr : public SGSubsystem
{

private:

    bool initDone;	// Hack - guard against update getting called before init

    // A map of airport ID vs frequencies and ATC provision
    typedef map < string, AirportATC* > airport_atc_map_type;
    typedef airport_atc_map_type::iterator airport_atc_map_iterator;
    typedef airport_atc_map_type::const_iterator airport_atc_map_const_iterator;

    airport_atc_map_type airport_atc_map;
    airport_atc_map_iterator airport_atc_map_itr;

    // A list of pointers to all currently active ATC classes
    typedef map<string,FGATC*> atc_list_type;
    typedef atc_list_type::iterator atc_list_iterator;
    typedef atc_list_type::const_iterator atc_list_const_iterator;

    // Everything put in this list should be created dynamically
    // on the heap and ***DELETED WHEN REMOVED!!!!!***
    atc_list_type* atc_list;
    atc_list_iterator atc_list_itr;
    // Any member function of FGATCMgr is permitted to leave this iterator pointing
    // at any point in or at the end of the list.
    // Hence any new access must explicitly first check for atc_list.end() before dereferencing.

    // Position of the Users Aircraft
    SGGeod _aircraftPos;

    // Pointers to current users position
    SGPropertyNode_ptr lon_node;
    SGPropertyNode_ptr lat_node;
    SGPropertyNode_ptr elev_node;

    //FGATIS atis;
	
    // Voice related stuff
    bool voice;			// Flag - true if we are using voice
#ifdef ENABLE_AUDIO_SUPPORT
    bool voiceOK;		// Flag - true if at least one voice has loaded OK
    FGATCVoice* v1;
#endif

public:

    FGATCMgr();
    ~FGATCMgr();

    void init();

    void bind();

    void unbind();

    void update(double dt);

    // Returns true if the airport is found in the map
    bool GetAirportATCDetails(const string& icao, AirportATC* a);
	
    // Return a pointer to an appropriate voice for a given type of ATC
    // creating the voice if necessary - ie. make sure exactly one copy
    // of every voice in use exists in memory.
    //
    // TODO - in the future this will get more complex and dole out country/airport
    // specific voices, and possible make sure that the same voice doesn't get used
    // at different airports in quick succession if a large enough selection are available.
    FGATCVoice* GetVoicePointer(const atc_type& type);
    
    atc_type GetComm1ATCType() { return(INVALID/* kludge */); }
    FGATC* GetComm1ATCPointer() { return(0/* kludge */); }
    atc_type GetComm2ATCType() { return(INVALID); }
    FGATC* GetComm2ATCPointer() { return(0/* kludge */); }
    
    // Get the frequency of a given service at a given airport
    // Returns zero if not found
    unsigned short int GetFrequency(const string& ident, const atc_type& tp);
    
    // Register the fact that the comm radio is tuned to an airport
    bool CommRegisterAirport(const string& ident, int chan, const atc_type& tp);
	
private:

    // Remove a class from the atc_list and delete it from memory
	// *if* no other comm channel or AI plane is using it.
    void ZapOtherService(const string ncunit, const string svc_name);

    // Return a pointer to a class in the list given ICAO code and type
    // Return NULL if the given service is not in the list
    // - *** THE CALLING FUNCTION MUST CHECK FOR THIS ***
    FGATC* FindInList(const string& id, const atc_type& tp);

    // Search the specified radio for stations on the same frequency and in range.
    void FreqSearch(const string navcomm, const int unit);
};

#endif  // _FG_ATCMGR_HXX
