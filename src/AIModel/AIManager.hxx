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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _FG_AIMANAGER_HXX
#define _FG_AIMANAGER_HXX

#include <list>

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/ssgSharedPtr.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include <Main/fg_props.hxx>

#include <AIModel/AIBase.hxx>
#include <AIModel/AIFlightPlan.hxx>

#include <Traffic/SchedFlight.hxx>
#include <Traffic/Schedule.hxx>

SG_USING_STD(list);
SG_USING_STD(vector);

class FGModelID
{
private:
  ssgSharedPtr<ssgBranch> model;
  string path;
public:
  FGModelID(const string& pth, ssgBranch * mdl) { path =pth; model=mdl;};
  ssgBranch * const getModelId() const { return model;};
  const string & getPath() const { return path;};
  int getNumRefs() const { return model.getNumRefs(); };
};

typedef vector<FGModelID> ModelVec;
typedef vector<FGModelID>::iterator ModelVecIterator;

class FGAIThermal;


class FGAIManager : public SGSubsystem
{

public:

    // A list of pointers to AI objects
    typedef list <SGSharedPtr<FGAIBase> > ai_list_type;
    typedef ai_list_type::iterator ai_list_iterator;
    typedef ai_list_type::const_iterator ai_list_const_iterator;

    ai_list_type ai_list;
  ModelVec loadedModels;

    inline const list <SGSharedPtr<FGAIBase> >& get_ai_list() const { return ai_list; }

    FGAIManager();
    ~FGAIManager();

    void init();
    void reinit();
    void bind();
    void unbind();
    void update(double dt);

    void attach(SGSharedPtr<FGAIBase> model);

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

    int getNumAiObjects(void) const;

    void processScenario( const string &filename );

  ssgBranch * getModel(const string& path);
  void setModel(const string& path, ssgBranch *model);

  static SGPropertyNode_ptr loadScenarioFile(const std::string& filename);

  static bool getStartPosition(const string& id, const string& pid,
                               SGGeod& geodPos, double& hdng, SGVec3d& uvw);

private:

    bool enabled;
    int mNumAiTypeModels[FGAIBase::MAX_OBJECTS];
    int mNumAiModels;

    SGPropertyNode_ptr root;
    SGPropertyNode_ptr wind_from_down_node;
    SGPropertyNode_ptr user_latitude_node;
    SGPropertyNode_ptr user_longitude_node;
    SGPropertyNode_ptr user_altitude_node;
    SGPropertyNode_ptr user_heading_node;
    SGPropertyNode_ptr user_pitch_node;
    SGPropertyNode_ptr user_yaw_node;
    SGPropertyNode_ptr user_speed_node;
    SGPropertyNode_ptr wind_from_east_node ;
    SGPropertyNode_ptr wind_from_north_node ;

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
    void fetchUserState( void );

    // used by thermals
    double range_nearest;
    double strength;
    void processThermal( FGAIThermal* thermal ); 

};

#endif  // _FG_AIMANAGER_HXX
