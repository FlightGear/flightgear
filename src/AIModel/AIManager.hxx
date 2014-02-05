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
#include <map>

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>

#include <AIModel/AIBase.hxx>
#include <AIModel/AIFlightPlan.hxx>

#include <Traffic/SchedFlight.hxx>
#include <Traffic/Schedule.hxx>

class FGAIThermal;

typedef SGSharedPtr<FGAIBase> FGAIBasePtr;

class FGAIManager : public SGSubsystem
{

public:
    FGAIManager();
    virtual ~FGAIManager();

    void init();
    virtual void shutdown();
    void postinit();
    void reinit();
    void bind();
    void unbind();
    void update(double dt);
    void updateLOD(SGPropertyNode* node);
    void attach(FGAIBase *model);

    const FGAIBase *calcCollision(double alt, double lat, double lon, double fuse_range);

    inline double get_user_heading() const { return user_heading; }
    inline double get_user_pitch() const { return user_pitch; }
    inline double get_user_speed() const {return user_speed; }
    inline double get_wind_from_east() const {return wind_from_east; }
    inline double get_wind_from_north() const {return wind_from_north; }
    inline double get_user_roll() const { return user_roll; }
    inline double get_user_agl() const { return user_altitude_agl; }

    bool loadScenario( const std::string &filename );

    static SGPropertyNode_ptr loadScenarioFile(const std::string& filename);

    static bool getStartPosition(const std::string& id, const std::string& pid,
        SGGeod& geodPos, double& hdng, SGVec3d& uvw);

    FGAIBasePtr addObject(const SGPropertyNode* definition);
    bool isVisible(const SGGeod& pos) const;
    
    /**
     * @brief given a reference to an /ai/models/<foo>[n] node, return the
     * corresponding AIObject implementation, or NULL.
     */
    FGAIBasePtr getObjectFromProperty(const SGPropertyNode* aProp) const;
private:
    // FGSubmodelMgr is a friend for access to the AI_list
    friend class FGSubmodelMgr;
    
    // A list of pointers to AI objects
    typedef std::list <FGAIBasePtr> ai_list_type;
    typedef ai_list_type::iterator ai_list_iterator;
    typedef ai_list_type::const_iterator ai_list_const_iterator;
    
    const ai_list_type& get_ai_list() const {
        return ai_list;
    }

    int getNumAiObjects() const;
    
    void removeDeadItem(FGAIBase* base);
  
    double calcRangeFt(const SGVec3d& aCartPos, FGAIBase* aObject) const;

    bool loadScenarioCommand(const SGPropertyNode* args);
    bool unloadScenarioCommand(const SGPropertyNode* args);
    bool addObjectCommand(const SGPropertyNode* definition);
    
    bool removeObject(const SGPropertyNode* args);
    bool unloadScenario( const std::string &filename );
    void unloadAllScenarios();
    
    SGPropertyNode_ptr root;
    SGPropertyNode_ptr enabled;
    SGPropertyNode_ptr thermal_lift_node;
    SGPropertyNode_ptr user_altitude_agl_node;
    SGPropertyNode_ptr user_speed_node;
    SGPropertyNode_ptr wind_from_east_node;
    SGPropertyNode_ptr wind_from_north_node;
    SGPropertyNode_ptr _environmentVisiblity;


    ai_list_type ai_list;
    
    double user_altitude_agl;
    double user_heading;
    double user_pitch;
    double user_roll;
    double user_speed;
    double wind_from_east;
    double wind_from_north;

    void fetchUserState( void );

    // used by thermals
    double range_nearest;
    double strength;
    void processThermal( double dt, FGAIThermal* thermal );

    SGPropertyChangeCallback<FGAIManager> cb_ai_bare;
    SGPropertyChangeCallback<FGAIManager> cb_ai_detailed;
    
    class Scenario;
    typedef std::map<std::string, Scenario*> ScenarioDict;
    ScenarioDict _scenarios;
};

#endif  // _FG_AIMANAGER_HXX
