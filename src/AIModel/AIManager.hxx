// AIManager.hxx - David Culp - based on:
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

#include <Traffic/SchedFlight.hxx>
#include <Traffic/Schedule.hxx>

SG_USING_STD(list);
SG_USING_STD(vector);

class FGModelID
{
private:
  ssgBranch * model;
  string path;
public:
  FGModelID(const string& pth, ssgBranch * mdl) { path =pth; model=mdl;};
  ssgBranch * const getModelId() const { return model;};
  const string & getPath() const { return path;};
};

typedef vector<FGModelID> ModelVec;
typedef vector<FGModelID>::const_iterator ModelVecIterator;

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
  ModelVec loadedModels;

public:

    FGAIManager();
    ~FGAIManager();

    void init();
    void reinit();
    void bind();
    void unbind();
    void update(double dt);

    void* createBallistic( FGAIModelEntity *entity );
    void* createAircraft( FGAIModelEntity *entity,   FGAISchedule *ref=0 );
    void* createThermal( FGAIModelEntity *entity );
    void* createStorm( FGAIModelEntity *entity );
    void* createShip( FGAIModelEntity *entity );
    void* createCarrier( FGAIModelEntity *entity );
    void* createStatic( FGAIModelEntity *entity );

    void destroyObject( int ID );

    inline double get_user_latitude() const { return user_latitude; }
    inline double get_user_longitude() const { return user_longitude; }
    inline double get_user_altitude() const { return user_altitude; }
    inline double get_user_heading() const { return user_heading; }
    inline double get_user_pitch() const { return user_pitch; }
    inline double get_user_yaw() const { return user_yaw; }
    inline double get_user_speed() const {return user_speed; }
    inline double get_wind_from_east() const {return wind_from_east; }
    inline double get_wind_from_north() const {return wind_from_north; }

    inline int getNum( FGAIBase::object_type ot ) const {
      return (0 < ot && ot < FGAIBase::MAX_OBJECTS) ? numObjects[ot] : numObjects[0];
    }

    void processScenario( const string &filename );

  ssgBranch * getModel(const string& path) const;
  void setModel(const string& path, ssgBranch *model);

  static bool getStartPosition(const string& id, const string& pid,
                               Point3D& geodPos, double& hdng, sgdVec3 uvw);

private:

    bool initDone;
    bool enabled;
    int numObjects[FGAIBase::MAX_OBJECTS];
    SGPropertyNode* root;
    SGPropertyNode* wind_from_down_node;
    SGPropertyNode* user_latitude_node;
    SGPropertyNode* user_longitude_node;
    SGPropertyNode* user_altitude_node;
    SGPropertyNode* user_heading_node;
    SGPropertyNode* user_pitch_node;
    SGPropertyNode* user_yaw_node;
    SGPropertyNode* user_speed_node;
    SGPropertyNode* wind_from_east_node ;
    SGPropertyNode* wind_from_north_node ;

    string scenario_filename;

    double user_latitude;
    double user_longitude;
    double user_altitude;
    double user_heading;
    double user_pitch;
    double user_yaw;
    double user_speed;
    double wind_from_east;
    double wind_from_north;
    double _dt;
    int dt_count;
    void fetchUserState( void );

    // used by thermals
    double range_nearest;
    double strength;
    void processThermal( FGAIThermal* thermal ); 

};

#endif  // _FG_AIMANAGER_HXX
