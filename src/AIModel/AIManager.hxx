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

#include <list>

#include <simgear/structure/subsystem_mgr.hxx>

#include <Main/fg_props.hxx>

#include <AIModel/AIBase.hxx>
#include <AIModel/AIScenario.hxx>
#include <AIModel/AIFlightPlan.hxx>

SG_USING_STD(list);
class FGAIThermal;


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

    FGAIManager();
    ~FGAIManager();

    void init();
    void bind();
    void unbind();
    void update(double dt);

    void* createBallistic( FGAIModelEntity *entity );
    void* createAircraft( FGAIModelEntity *entity );
    void* createThermal( FGAIModelEntity *entity );
    void* createStorm( FGAIModelEntity *entity );
    void* createShip( FGAIModelEntity *entity );

    void destroyObject( void* ID );

    inline double get_user_latitude() { return user_latitude; }
    inline double get_user_longitude() { return user_longitude; }
    inline double get_user_altitude() { return user_altitude; }
    inline double get_user_heading() { return user_heading; }
    inline double get_user_pitch() { return user_pitch; }
    inline double get_user_yaw() { return user_yaw; }
    inline double get_user_speed() {return user_speed; }

    inline int getNum( FGAIBase::object_type ot ) {
      return (0 < ot && ot < FGAIBase::MAX_OBJECTS) ? numObjects[ot] : numObjects[0];
    }

    void processScenario( string &filename );

private:

    bool initDone;
    bool enabled;
    int numObjects[FGAIBase::MAX_OBJECTS];
    SGPropertyNode* root;
    SGPropertyNode* wind_from_down_node;
    string scenario_filename;

    double user_latitude;
    double user_longitude;
    double user_altitude;
    double user_heading;
    double user_pitch;
    double user_yaw;
    double user_speed;
    double _dt;
    int dt_count;
    void fetchUserState( void );

    // used by thermals
    double range_nearest;
    double strength;
    void processThermal( FGAIThermal* thermal ); 

};

#endif  // _FG_AIMANAGER_HXX
