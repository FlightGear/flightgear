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

#include <list>

#include "atis.hxx"
#include "ATC.hxx"

SG_USING_STD(list);

class FGATCMgr : public FGSubsystem
{

private:

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
    const char* comm1_ident;
    const char* last_comm1_ident;
    double comm2_x, comm2_y, comm2_z, comm2_elev;
    double comm2_range, comm2_effective_range;
    bool comm2_valid;
    const char* comm2_ident;
    const char* last_comm2_ident;

    FGATIS atis;
    //FGTower tower;
    //FGGround ground;
    //FGApproach approach;
    //FGDeparture departure;

public:

    FGATCMgr();
    ~FGATCMgr();

    void init();

    void bind();

    void unbind();

    void update(int dt);

private:

    // Remove a class from the atc_list and delete it from memory
    void RemoveFromList(const char* id, atc_type tp);

    // Search a specified freq for matching stations
    void Search();

};

#endif  // _FG_ATCMGR_HXX
