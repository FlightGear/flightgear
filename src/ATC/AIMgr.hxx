// AIMgr.hxx - definition of FGAIMgr 
// - a global management class for FlightGear generated AI traffic
//
// Written by David Luff, started March 2002.
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

/*****************************************************************
*
* WARNING - Curt has some ideas about AI traffic so anything in here
* may get rewritten or scrapped.  Contact Curt curt@flightgear.org 
* before spending any time or effort on this code!!!
*
******************************************************************/

#ifndef _FG_AIMGR_HXX
#define _FG_AIMGR_HXX

#include <Main/fgfs.hxx>
#include <Main/fg_props.hxx>

#include <list>

#include "AIEntity.hxx"

SG_USING_STD(list);

class FGAIMgr : public FGSubsystem
{

private:

    // A list of pointers to all currently active AI stuff
    typedef list <FGAIEntity*> ai_list_type;
    typedef ai_list_type::iterator ai_list_iterator;
    typedef ai_list_type::const_iterator ai_list_const_iterator;

    // Everything put in this list should be created dynamically
    // on the heap and ***DELETED WHEN REMOVED!!!!!***
    ai_list_type ai_list;
    ai_list_iterator ai_list_itr;
    // Any member function of FGATCMgr is permitted to leave this iterator pointing
    // at any point in or at the end of the list.
    // Hence any new access must explicitly first check for atc_list.end() before dereferencing.

    // Position of the Users Aircraft
    // (This may be needed to calculate the distance from the user when deciding which 3D model to render)
    double current_lon;
    double current_lat;
    double current_elev;
    // Pointers to current users position
    SGPropertyNode *current_lon_node;
    SGPropertyNode *current_lat_node;
    SGPropertyNode *current_elev_node;

    //FGATIS atis;
    //FGGround ground;
    //FGTower tower;
    //FGApproach approach;
    //FGDeparture departure;

public:

    FGAIMgr();
    ~FGAIMgr();

    void init();

    void bind();

    void unbind();

    void update(int dt);

private:

    // Remove a class from the ai_list and delete it from memory
    //void RemoveFromList(const char* id, atc_type tp);

};

#endif  // _FG_AIMGR_HXX
