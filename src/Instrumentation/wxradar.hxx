// Wx Radar background texture
//
// Written by Harald JOHNSEN, started May 2005.
// With major amendments by Vivian MEAZZA May 2007
// Ported to OSG by Tim MOORE Jun 2007
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
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
//
//

#ifndef _INST_WXRADAR_HXX
#define _INST_WXRADAR_HXX

#include <plib/ssg.h>

#include <osg/ref_ptr>
#include <osg/Geode>
#include <osg/Texture2D>

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/environment/visual_enviro.hxx>

#include <AIModel/AIBase.hxx>
#include <AIModel/AIManager.hxx>
#include <vector>
#include <string>
SG_USING_STD(vector);
SG_USING_STD(string);

class FGAIBase;

class FGODGauge;

class wxRadarBg : public SGSubsystem {
public:

    wxRadarBg ( SGPropertyNode *node );
    wxRadarBg ();
    virtual ~wxRadarBg ();

    virtual void init ();
    virtual void update (double dt);

private:

    string _name;
    int _num;

    string _last_switchKnob;
    bool _sim_init_done;

    double _user_speed_east_fps;
    double _user_speed_north_fps;

    float _x_displacement;
    float _y_displacement;
    float _x_sym_displacement;
    float _y_sym_displacement;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _Instrument;
    SGPropertyNode_ptr _Tacan;
    SGPropertyNode_ptr _Radar_controls;

    SGPropertyNode_ptr _user_lat_node;
    SGPropertyNode_ptr _user_lon_node;
    SGPropertyNode_ptr _user_heading_node;
    SGPropertyNode_ptr _user_alt_node;
    SGPropertyNode_ptr _user_speed_east_fps_node;
    SGPropertyNode_ptr _user_speed_north_fps_node;

    SGPropertyNode_ptr _tacan_serviceable_node;
    SGPropertyNode_ptr _tacan_distance_node;
    SGPropertyNode_ptr _tacan_name_node;
    SGPropertyNode_ptr _tacan_bearing_node;
    SGPropertyNode_ptr _tacan_in_range_node;

    SGPropertyNode_ptr _radar_weather_node;
    SGPropertyNode_ptr _radar_position_node;
    SGPropertyNode_ptr _radar_data_node;
    SGPropertyNode_ptr _radar_mode_control_node;
    SGPropertyNode_ptr _radar_centre_node;
    SGPropertyNode_ptr _radar_coverage_node;
    SGPropertyNode_ptr _radar_ref_rng_node;

    SGPropertyNode_ptr _ai_enabled_node;

    osg::ref_ptr<osg::Texture2D> resultTexture;
    osg::ref_ptr<osg::Texture2D> wxEcho;
    osg::ref_ptr<osg::Geode> radarGeode;

    list_of_SGWxRadarEcho _radarEchoBuffer;

    FGODGauge *_odg;
    FGAIManager* _ai;

    bool calcRadarHorizon(double user_alt, double alt, double range);
    bool calcMaxRange(int type, double range);

    void calcRngBrg(double lat, double lon, double lat2, double lon2,
            double &range, double &bearing) const;

    void updateRadar();

    float calcRelBearing(float bearing, float heading);

    // A list of pointers to AI objects
    typedef list <SGSharedPtr<FGAIBase> > radar_list_type;
    typedef radar_list_type::iterator radar_list_iterator;
    typedef radar_list_type::const_iterator radar_list_const_iterator;

    radar_list_type _radar_list;
};

#endif // _INST_WXRADAR_HXX
