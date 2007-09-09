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

#include <osg/ref_ptr>
#include <osg/Geode>
#include <osg/Texture2D>
#include <osgText/Text>

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

class wxRadarBg : public SGSubsystem, public SGPropertyChangeListener {
public:

    wxRadarBg ( SGPropertyNode *node );
    wxRadarBg ();
    virtual ~wxRadarBg ();

    virtual void init ();
    virtual void update (double dt);
    virtual void valueChanged(SGPropertyNode*);

private:
    string _name;
    int _num;
    double _interval;
    double _time;
    string _texture_path;

    typedef enum { ARC, MAP, PLAN, ROSE } DisplayMode;
    DisplayMode _display_mode;

    string _last_switchKnob;
    bool _sim_init_done;

    float _range_nm;
    float _scale;   // factor to convert nm to display units
    double _radar_ref_rng;
    float _angle_offset;
    float _view_heading;
    double _lat, _lon;
    float _x_offset, _y_offset;

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
    SGPropertyNode_ptr _radar_symbol_node;
    SGPropertyNode_ptr _radar_mode_control_node;
    SGPropertyNode_ptr _radar_centre_node;
    SGPropertyNode_ptr _radar_coverage_node;
    SGPropertyNode_ptr _radar_ref_rng_node;
    SGPropertyNode_ptr _radar_hdg_marker_node;
    SGPropertyNode_ptr _radar_rotate_node;
    SGPropertyNode_ptr _radar_font_node;

    SGPropertyNode_ptr _ai_enabled_node;

    osg::ref_ptr<osg::Texture2D> _resultTexture;
    osg::ref_ptr<osg::Texture2D> _wxEcho;
    osg::ref_ptr<osg::Geode> _radarGeode;
    osg::ref_ptr<osg::Geode> _textGeode;
    osg::Geometry *_geom;
    osg::Vec2Array *_vertices;
    osg::Vec2Array *_texCoords;
    osg::Matrixf _centerTrans;
    osg::ref_ptr<osgText::Font> _font;

    list_of_SGWxRadarEcho _radarEchoBuffer;

    FGODGauge *_odg;
    FGAIManager* _ai;

    void update_weather();
    void update_aircraft();
    void update_tacan();
    void update_heading_marker();
    void update_data(FGAIBase* ac, double radius, double bearing, bool selected);

    void center_map();
    void apply_map_offset();
    bool withinRadarHorizon(double user_alt, double alt, double range);
    bool inRadarRange(int type, double range);
    float calcRelBearing(float bearing, float heading);
    void calcRangeBearing(double lat, double lon, double lat2, double lon2,
            double &range, double &bearing) const;
    void updateFont();
};

#endif // _INST_WXRADAR_HXX
