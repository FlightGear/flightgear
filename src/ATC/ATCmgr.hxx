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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef _FG_ATCMGR_HXX
#define _FG_ATCMGR_HXX

#include <Main/fgfs.hxx>
#include <Main/fg_props.hxx>

#include <string>
#include <list>
#include <map>

#include "atis.hxx"
#include "tower.hxx"
#include "approach.hxx"
//#include "ground.hxx"
#include "ATC.hxx"

SG_USING_STD(string);
SG_USING_STD(list);
SG_USING_STD(map);

const int max_app = 20;

// Structure for holding details of the ATC frequencies at a given airport, and whether they are in the active list or not.
// These can then be cross referenced with the [atis][tower][etc]lists which are stored by frequency.
// Non-available services are denoted by a frequency of zero.
// Eventually the whole ATC data structures may have to be rethought if we turn out to be massive memory hogs!!
struct AirportATC {
    float lon;
    float lat;
    float elev;
    float atis_freq;
    bool atis_active;
    float tower_freq;
    bool tower_active;
    float ground_freq;
    bool ground_active;
    //float approach_freq;
    //bool approach_active;
    //float departure_freq;
    //bool departure_active;

    // Flags to ensure the stations don't get wrongly deactivated
    bool set_by_AI;	// true when the AI manager has activated this station
    bool set_by_comm_search;	// true when the comm_search has activated this station
};

class FGATCMgr : public FGSubsystem
{

private:

    // A map of airport ID vs frequencies and ATC provision
    typedef map < string, AirportATC* > airport_atc_map_type;
    typedef airport_atc_map_type::iterator airport_atc_map_iterator;
    typedef airport_atc_map_type::const_iterator airport_atc_map_const_iterator;

    airport_atc_map_type airport_atc_map;
    airport_atc_map_iterator airport_atc_map_itr;

    // A list of pointers to all currently active ATC classes
    typedef list <FGATC*> atc_list_type;
    typedef atc_list_type::iterator atc_list_iterator;
    typedef atc_list_type::const_iterator atc_list_const_iterator;

    // Everything put in this list should be created dynamically
    // on the heap and ***DELETED WHEN REMOVED!!!!!***
    atc_list_type atc_list;
    atc_list_iterator atc_list_itr;
    // Any member function of FGATCMgr is permitted to leave this iterator pointing
    // at any point in or at the end of the list.
    // Hence any new access must explicitly first check for atc_list.end() before dereferencing.

    // Position of the Users Aircraft
    double lon;
    double lat;
    double elev;

    // Type of ATC control that the user's radios are tuned to.
    atc_type comm1_type;
    atc_type comm2_type;

    double comm1_freq;
    double comm2_freq;

    // Pointers to users current communication frequencies.
    SGPropertyNode *comm1_node;
    SGPropertyNode *comm2_node;

    // Pointers to current users position
    SGPropertyNode *lon_node;
    SGPropertyNode *lat_node;
    SGPropertyNode *elev_node;

    // Position of the ATC that the comm radios are tuned to in order to decide whether transmission
    // will be received
    double comm1_x, comm1_y, comm1_z, comm1_elev;
    double comm1_range, comm1_effective_range;
    bool comm1_valid; 
    bool comm1_atis_valid;
    bool comm1_tower_valid;
    bool comm1_approach_valid;
    const char* comm1_ident;
    const char* comm1_atis_ident;
    const char* comm1_tower_ident;
    const char* comm1_approach_ident;
    const char* last_comm1_ident;
    const char* last_comm1_atis_ident;
    const char* last_comm1_tower_ident;
    const char* last_comm1_approach_ident;
    const char* approach_ident;
    bool last_in_range;
    double comm2_x, comm2_y, comm2_z, comm2_elev;
    double comm2_range, comm2_effective_range;
    bool comm2_valid;
    const char* comm2_ident;
    const char* last_comm2_ident;

    FGATIS atis;
    //FGGround ground;
    FGTower tower;
    FGApproach approaches[max_app];
    FGApproach approach;
    //FGDeparture departure;

public:

    FGATCMgr();
    ~FGATCMgr();

    void init();

    void bind();

    void unbind();

    void update(int dt);

    // Returns true if the airport is found in the map
    bool GetAirportATCDetails(string icao, AirportATC* a);

    // Return a pointer to a given sort of ATC at a given airport and activate if necessary
    FGATC* GetATCPointer(string icao, atc_type type);

private:

    // Remove a class from the atc_list and delete it from memory
    void RemoveFromList(const char* id, atc_type tp);

    // Search a specified freq for matching stations
    void Search();

};

#endif  // _FG_ATCMGR_HXX
