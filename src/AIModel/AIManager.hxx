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
#include "AIScenario.hxx"
#include "AIFlightPlan.hxx"

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

    // array of already-assigned ID's
    typedef vector <int> id_vector_type;
    id_vector_type ids;                    
    id_vector_type::iterator id_itr;

public:

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
                        double roll = 0 );  // in degrees

    int createAircraft( string model_class, // see FGAIAircraft.hxx for possible classes
                        string path,        // path to exterior model
                        FGAIFlightPlan *flightplan );

    int createShip(     string path,        // path to exterior model
                        double latitude,    // in degrees -90 to 90
                        double longitude,   // in degrees -180 to 180
                        double altitude,    // in feet  (ex. for a lake!)
                        double heading,     // true heading in degrees
                        double speed,       // in knots true
                        double rudder );    // in degrees (right is positive)(0 to 5 works best)

    int createShip(     string path,        // path to exterior model
                        FGAIFlightPlan *flightplan );

    int createBallistic( string path,       // path to exterior model
                         double latitude,   // in degrees -90 to 90
                         double longitude,  // in degrees -180 to 180
                         double altitude,   // in feet
                         double azimuth,    // in degrees (same as heading)
                         double elevation,  // in degrees (same as pitch)
                         double speed,      // in feet per second
                         double eda );      // equivalent drag area, ft2

    int createStorm( string path,        // path to exterior model
                     double latitude,    // in degrees -90 to 90
                     double longitude,   // in degrees -180 to 180
                     double altitude,    // in feet
                     double heading,     // true heading in degrees
                     double speed );     // in knots true airspeed (KTAS)    

    int createThermal( double latitude,    // in degrees -90 to 90
                       double longitude,   // in degrees -180 to 180
                       double strength,    // in feet per second
                       double diameter );  // in feet
                 
    void destroyObject( int ID );

    inline double get_user_latitude() { return user_latitude; }
    inline double get_user_longitude() { return user_longitude; }
    inline double get_user_altitude() { return user_altitude; }
    inline double get_user_heading() { return user_heading; }
    inline double get_user_pitch() { return user_pitch; }
    inline double get_user_yaw() { return user_yaw; }
    inline double get_user_speed() {return user_speed; }

    void processScenario( string filename );
    int getNum( FGAIBase::object_type ot);

private:

    bool initDone;
    bool enabled;
    int numObjects;
    SGPropertyNode* root;
    SGPropertyNode* wind_from_down;
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
