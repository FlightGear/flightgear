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

#ifndef _INST_ND_HXX
#define _INST_ND_HXX

#include <osg/ref_ptr>
#include <osg/Geode>
#include <osg/Texture2D>
#include <osgText/Text>

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <vector>
#include <string>

class FGODGauge;
class FGRouteMgr;
class FGNavRecord;

class NavDisplay : public SGSubsystem
{
public:

    NavDisplay(SGPropertyNode *node);
    virtual ~NavDisplay();

    virtual void init();
    virtual void update(double dt);

protected:
    string _name;
    int _num;
    double _time;
    double _updateInterval;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _Instrument;
    SGPropertyNode_ptr _radar_mode_control_node;

    SGPropertyNode_ptr _user_lat_node;
    SGPropertyNode_ptr _user_lon_node;
    SGPropertyNode_ptr _user_heading_node;
    SGPropertyNode_ptr _user_alt_node;

    FGODGauge *_odg;

    // Convenience function for creating a property node with a
    // default value
    template<typename DefaultType>
    SGPropertyNode *getInstrumentNode(const char *name, DefaultType value);

private:
    string _texture_path;

    typedef enum { ARC, MAP, PLAN, ROSE, BSCAN} DisplayMode;
    DisplayMode _display_mode;

    float _scale;   // factor to convert nm to display units
    float _view_heading;
    bool _drawData;
    
    SGPropertyNode_ptr _Radar_controls;

    SGPropertyNode_ptr _radar_weather_node;
    SGPropertyNode_ptr _radar_position_node;
    SGPropertyNode_ptr _radar_data_node;
    SGPropertyNode_ptr _radar_symbol_node;
    SGPropertyNode_ptr _radar_arpt_node;
    SGPropertyNode_ptr _radar_station_node;
    
    SGPropertyNode_ptr _radar_centre_node;
    SGPropertyNode_ptr _radar_coverage_node;
    SGPropertyNode_ptr _radar_ref_rng_node;
    SGPropertyNode_ptr _radar_hdg_marker_node;
    SGPropertyNode_ptr _radar_rotate_node;
    SGPropertyNode_ptr _radar_tcas_node;
    SGPropertyNode_ptr _radar_absalt_node;

    SGPropertyNode_ptr _draw_track_node;
    SGPropertyNode_ptr _draw_heading_node;
    SGPropertyNode_ptr _draw_north_node;
    SGPropertyNode_ptr _draw_fix_node;
    
    SGPropertyNode_ptr _font_node;
    SGPropertyNode_ptr _ai_enabled_node;
    SGPropertyNode_ptr _navRadio1Node;
    SGPropertyNode_ptr _navRadio2Node;
    
    osg::ref_ptr<osg::Texture2D> _resultTexture;
    osg::ref_ptr<osg::Texture2D> _symbols;
    osg::ref_ptr<osg::Geode> _radarGeode;
    osg::ref_ptr<osg::Geode> _textGeode;
  
    osg::Geometry *_geom;
  
    osg::DrawArrays* _symbolPrimSet;
    osg::Vec2Array *_vertices;
    osg::Vec2Array *_texCoords;
    osg::Vec3Array* _quadColors;
    
    osg::Geometry* _lineGeometry;
    osg::DrawArrays* _linePrimSet;
    osg::Vec2Array* _lineVertices;
    osg::Vec3Array* _lineColors;
  
  
    osg::Matrixf _centerTrans;
    osg::Matrixf _projectMat;
    
    osg::ref_ptr<osgText::Font> _font;
    osg::Vec4 _font_color;
    osg::Vec4 _tcas_colors[4];
    float _font_size;
    float _font_spacing;

    FGRouteMgr* _route;
    SGGeod _pos;
    double _rangeNm;
    
    void update_route();
    void update_aircraft();
    void update_stations();
    void update_airports();
    void update_waypoints();
    
    bool update_tcas(const SGPropertyNode *model, const SGGeod& modelPos, 
                            double user_alt,double alt, bool absMode);
    void update_data(const SGPropertyNode *ac, double altitude, double heading,
                               double radius, double bearing, bool selected);
    void updateFont();
    
    osg::Matrixf project(const SGGeod& geod) const;
    osg::Vec2 projectBearingRange(double bearingDeg, double rangeNm) const;
    osg::Vec2 projectGeod(const SGGeod& geod) const;
    
    void addSymbol(const SGGeod& pos, int symbolIndex, 
        const std::string& data, const osg::Vec3& color);
  
    void addLine(osg::Vec2 a, osg::Vec2 b, const osg::Vec3& color);
    
    FGNavRecord* drawTunedNavaid(const SGPropertyNode_ptr& radio );    
};

#endif // _INST_ND_HXX
