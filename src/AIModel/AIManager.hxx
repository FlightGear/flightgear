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
#include "AIAircraft.hxx"

SG_USING_STD(list);


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

    // array of already-assigned ID's
    typedef vector <int> id_vector_type;
    id_vector_type ids;                    
    id_vector_type::iterator id_itr;

public:

    enum object_type { otAircraft, otShip, otBallistic, otRocket };

    FGAIManager();
    ~FGAIManager();

    void init();
    void bind();
    void unbind();
    void update(double dt);

    int assignID();
    void freeID(int ID);

    int createAircraft( string model_class, // see FGAIAircraft.hxx for possible classes
                        string path,        // path to exterior model
                        double latitude,    // in degrees -90 to 90
                        double longitude,   // in degrees -180 to 180
                        double altitude,    // in feet
                        double heading,     // true heading in degrees
                        double speed,       // in knots true airspeed (KTAS)    
                        double pitch = 0,   // in degrees
                        double roll = 0 );  // in degrees

    int createShip(     string path,        // path to exterior model
                        double latitude,    // in degrees -90 to 90
                        double longitude,   // in degrees -180 to 180
                        double altitude,    // in feet  (ex. for a lake!)
                        double heading,     // true heading in degrees
                        double speed,       // in knots true
                        double rudder );    // in degrees (between 0 and 5 works best)


    int createBallistic( string path,       // path to exterior model
                         double latitude,   // in degrees -90 to 90
                         double longitude,  // in degrees -180 to 180
                         double altitude,   // in feet
                         double azimuth,    // in degrees (same as heading)
                         double elevation,  // in degrees (same as pitch)
                         double speed );    // in feet per second

    void destroyObject( int ID );

    inline double get_user_latitude() { return user_latitude; }
    inline double get_user_longitude() { return user_longitude; }
    inline double get_user_altitude() { return user_altitude; }
    inline double get_user_heading() { return user_heading; }
    inline double get_user_pitch() { return user_pitch; }
    inline double get_user_yaw() { return user_yaw; }
    inline double get_user_speed() {return user_speed; }

private:

    bool initDone;
    int numObjects;
    SGPropertyNode* root;

    double user_latitude;
    double user_longitude;
    double user_altitude;
    double user_heading;
    double user_pitch;
    double user_yaw;
    double user_speed;
    int dt_count;
    void fetchUserState( void );

};

#endif  // _FG_AIMANAGER_HXX
