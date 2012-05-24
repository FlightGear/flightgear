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
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>

#include <AIModel/AIBase.hxx>
#include <AIModel/AIFlightPlan.hxx>

#include <Traffic/SchedFlight.hxx>
#include <Traffic/Schedule.hxx>

using std::list;

class FGAIThermal;

class FGAIManager : public SGSubsystem
{

public:

    // A list of pointers to AI objects
    typedef list <SGSharedPtr<FGAIBase> > ai_list_type;
    typedef ai_list_type::iterator ai_list_iterator;
    typedef ai_list_type::const_iterator ai_list_const_iterator;

    ai_list_type ai_list;

    inline const ai_list_type& get_ai_list() const {
        SG_LOG(SG_AI, SG_DEBUG, "AI Manager: AI model return list size " << ai_list.size());
        return ai_list;
    }

    FGAIManager();
    ~FGAIManager();

    void init();
    void postinit();
    void reinit();
    void bind();
    void unbind();
    void update(double dt);
    void updateLOD(SGPropertyNode* node);
    void attach(FGAIBase *model);

    void destroyObject( int ID );
    const FGAIBase *calcCollision(double alt, double lat, double lon, double fuse_range);

    inline double get_user_latitude() const { return user_latitude; }
    inline double get_user_longitude() const { return user_longitude; }
    inline double get_user_altitude() const { return user_altitude; }
    inline double get_user_heading() const { return user_heading; }
    inline double get_user_pitch() const { return user_pitch; }
    inline double get_user_yaw() const { return user_yaw; }
    inline double get_user_speed() const {return user_speed; }
    inline double get_wind_from_east() const {return wind_from_east; }
    inline double get_wind_from_north() const {return wind_from_north; }
    inline double get_user_roll() const { return user_roll; }
    inline double get_user_agl() const { return user_altitude_agl; }

    int getNumAiObjects(void) const;

    void processScenario( const string &filename );

    static SGPropertyNode_ptr loadScenarioFile(const std::string& filename);

    static bool getStartPosition(const string& id, const string& pid,
        SGGeod& geodPos, double& hdng, SGVec3d& uvw);

private:

    int mNumAiTypeModels[FGAIBase::MAX_OBJECTS];
    int mNumAiModels;

    double calcRange(double lat, double lon, double lat2, double lon2)const;

    SGPropertyNode_ptr root;
    SGPropertyNode_ptr enabled;
    SGPropertyNode_ptr thermal_lift_node;
    SGPropertyNode_ptr user_latitude_node;
    SGPropertyNode_ptr user_longitude_node;
    SGPropertyNode_ptr user_altitude_node;
    SGPropertyNode_ptr user_altitude_agl_node;
    SGPropertyNode_ptr user_heading_node;
    SGPropertyNode_ptr user_pitch_node;
    SGPropertyNode_ptr user_yaw_node;
    SGPropertyNode_ptr user_roll_node;
    SGPropertyNode_ptr user_speed_node;
    SGPropertyNode_ptr wind_from_east_node;
    SGPropertyNode_ptr wind_from_north_node;

    double user_latitude;
    double user_longitude;
    double user_altitude;
    double user_altitude_agl;
    double user_heading;
    double user_pitch;
    double user_yaw;
    double user_roll;
    double user_speed;
    double user_agl;
    double wind_from_east;
    double wind_from_north;
    double _dt;

    void fetchUserState( void );

    // used by thermals
    double range_nearest;
    double strength;
    void processThermal( FGAIThermal* thermal ); 

    SGPropertyChangeCallback<FGAIManager> cb_ai_bare;
    SGPropertyChangeCallback<FGAIManager> cb_ai_detailed;
};

#endif  // _FG_AIMANAGER_HXX
