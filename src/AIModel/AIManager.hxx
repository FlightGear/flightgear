// AIManager.hxx - experimental! - David Culp - based on:
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

#ifndef _FG_AIMANAGER_HXX
#define _FG_AIMANAGER_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <Main/fg_props.hxx>
#include <list>
#include "AIBase.hxx"

SG_USING_STD(list);

 struct PERF_STRUCT {
   double accel;
   double decel;
   double climb_rate;
   double descent_rate;
   double takeoff_speed;
   double climb_speed;
   double cruise_speed;
   double descent_speed;
   double land_speed;
  };


class FGAIManager : public SGSubsystem
{

private:

    // A list of pointers to AI objects
    typedef list <FGAIBase*> ai_list_type;
    typedef ai_list_type::iterator ai_list_iterator;
    typedef ai_list_type::const_iterator ai_list_const_iterator;

    // Everything put in this list should be created dynamically
    // on the heap and ***DELETED WHEN REMOVED!!!!!***
    ai_list_type ai_list;
    ai_list_iterator ai_list_itr;

public:

    enum object_type { otAircraft, otShip, otBallistic, otRocket };

    FGAIManager();
    ~FGAIManager();

    void init();
    void bind();
    void unbind();
    void update(double dt);


private:

    bool initDone;

};

#endif  // _FG_AIMANAGER_HXX
