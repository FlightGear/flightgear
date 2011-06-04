// navigation display texture
//
// Written by James Turner, forked from wxradar code
//
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "NavDisplay.hxx"

#include <osg/Array>
#include <osg/Geometry>
#include <osg/Matrixf>
#include <osg/PrimitiveSet>
#include <osg/StateSet>
#include <osg/LineWidth>

#include <osg/Version>
#include <osgDB/ReaderWriter>
#include <osgDB/WriteFile>

#include <simgear/constants.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <sstream>
#include <iomanip>
#include <iostream>             // for cout, endl

using std::stringstream;
using std::endl;
using std::setprecision;
using std::fixed;
using std::setw;
using std::setfill;
using std::cout;
using std::endl;

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Cockpit/panel.hxx>
#include <Navaids/routePath.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/fix.hxx>
#include <Airports/simple.hxx>
#include <Airports/runways.hxx>

#include "instrument_mgr.hxx"
#include "od_gauge.hxx"

static const float UNIT = 1.0f / 8.0f;  // 8 symbols in a row/column in the texture
static const char *DEFAULT_FONT = "typewriter.txf";

NavDisplay::NavDisplay(SGPropertyNode *node) :
    _name(node->getStringValue("name", "nd")),
    _num(node->getIntValue("number", 0)),
    _time(0.0),
    _interval(node->getDoubleValue("update-interval-sec", 1.0)),
    _elapsed_time(0),
    _persistance(0),
    _sim_init_done(false),
    _odg(0),
    _scale(0),
    _angle_offset(0),
    _view_heading(0),
    _x_offset(0),
    _y_offset(0),
    _radar_ref_rng(0),
    _lat(0),
    _lon(0),
    _resultTexture(0),
    _font_size(0),
    _font_spacing(0),
    _rangeNm(0)
{
    _Instrument = fgGetNode(string("/instrumentation/" + _name).c_str(), _num, true);
    _font_node = _Instrument->getNode("font", true);

#define INITFONT(p, val, type) if (!_font_node->hasValue(p)) _font_node->set##type##Value(p, val)
    INITFONT("name", DEFAULT_FONT, String);
    INITFONT("size", 8, Float);
    INITFONT("line-spacing", 0.25, Float);
    INITFONT("color/red", 0, Float);
    INITFONT("color/green", 0.8, Float);
    INITFONT("color/blue", 0, Float);
    INITFONT("color/alpha", 1, Float);
#undef INITFONT

}


NavDisplay::~NavDisplay()
{
}


void
NavDisplay::init ()
{
    _serviceable_node = _Instrument->getNode("serviceable", true);

    // texture name to use in 2D and 3D instruments
    _texture_path = _Instrument->getStringValue("radar-texture-path",
        "Aircraft/Instruments/Textures/od_wxradar.rgb");
    _resultTexture = FGTextureManager::createTexture(_texture_path.c_str(), false);

    string path = _Instrument->getStringValue("symbol-texture-path",
        "Aircraft/Instruments/Textures/nd-symbols.png");
    SGPath tpath = globals->resolve_aircraft_path(path);

    // no mipmap or else alpha will mix with pixels on the border of shapes, ruining the effect
    _symbols = SGLoadTexture2D(tpath, NULL, false, false);

    FGInstrumentMgr *imgr = (FGInstrumentMgr *)globals->get_subsystem("instrumentation");
    _odg = (FGODGauge *)imgr->get_subsystem("od_gauge");
    _odg->setSize(_Instrument->getIntValue("texture-size", 512));


    _user_lat_node = fgGetNode("/position/latitude-deg", true);
    _user_lon_node = fgGetNode("/position/longitude-deg", true);
    _user_alt_node = fgGetNode("/position/altitude-ft", true);

    SGPropertyNode *n = _Instrument->getNode("display-controls", true);
    _radar_weather_node     = n->getNode("WX", true);
    _radar_position_node    = n->getNode("pos", true);
    _radar_data_node        = n->getNode("data", true);
    _radar_symbol_node      = n->getNode("symbol", true);
    _radar_centre_node      = n->getNode("centre", true);
    _radar_tcas_node        = n->getNode("tcas", true);
    _radar_absalt_node      = n->getNode("abs-altitude", true);
  
    _ai_enabled_node = fgGetNode("/sim/ai/enabled", true);
    _route = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
    
// OSG geometry setup
    _radarGeode = new osg::Geode;
    osg::StateSet *stateSet = _radarGeode->getOrCreateStateSet();
    stateSet->setTextureAttributeAndModes(0, _symbols.get());
    
    osg::LineWidth *lw = new osg::LineWidth();
    lw->setWidth(2.0);
    stateSet->setAttribute(lw);
    
    _geom = new osg::Geometry;
    _geom->setUseDisplayList(false);
    // Initially allocate space for 128 quads
    _vertices = new osg::Vec2Array;
    _vertices->setDataVariance(osg::Object::DYNAMIC);
    _vertices->reserve(128 * 4);
    _geom->setVertexArray(_vertices);
    _texCoords = new osg::Vec2Array;
    _texCoords->setDataVariance(osg::Object::DYNAMIC);
    _texCoords->reserve(128 * 4);
    _geom->setTexCoordArray(0, _texCoords);
    
    osg::Vec3Array *colors = new osg::Vec3Array;
    colors->push_back(osg::Vec3(1.0f, 1.0f, 1.0f)); // color of echos
    _geom->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
    _geom->setColorArray(colors);
    
    _symbolPrimSet = new osg::DrawArrays(osg::PrimitiveSet::QUADS);
    _symbolPrimSet->setDataVariance(osg::Object::DYNAMIC);
    _geom->addPrimitiveSet(_symbolPrimSet);
    
    _geom->setInitialBound(osg::BoundingBox(osg::Vec3f(-256.0f, -256.0f, 0.0f),
        osg::Vec3f(256.0f, 256.0f, 0.0f)));
    _radarGeode->addDrawable(_geom);
    _odg->allocRT();
    // Texture in the 2D panel system
    FGTextureManager::addTexture(_texture_path.c_str(), _odg->getTexture());

    _lineGeometry = new osg::Geometry;
    _lineGeometry->setUseDisplayList(false);
    
    _lineVertices = new osg::Vec2Array;
    _lineVertices->setDataVariance(osg::Object::DYNAMIC);
    _lineVertices->reserve(128 * 4);
    _lineGeometry->setVertexArray(_vertices);
    
                  
    _lineColors = new osg::Vec3Array;
    _lineColors->setDataVariance(osg::Object::DYNAMIC);
    _lineGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    _lineGeometry->setColorArray(colors);
    
    _linePrimSet = new osg::DrawArrays(osg::PrimitiveSet::LINES);
    _linePrimSet->setDataVariance(osg::Object::DYNAMIC);
    _lineGeometry->addPrimitiveSet(_linePrimSet);
    
    _lineGeometry->setInitialBound(osg::BoundingBox(osg::Vec3f(-256.0f, -256.0f, 0.0f),
                                            osg::Vec3f(256.0f, 256.0f, 0.0f)));

    _radarGeode->addDrawable(_lineGeometry);              
                  
    _textGeode = new osg::Geode;

    osg::Camera *camera = _odg->getCamera();
    camera->addChild(_radarGeode.get());
    camera->addChild(_textGeode.get());

    updateFont();
}


// Local coordinates for each echo
const osg::Vec3f symbolCoords[4] = {
    osg::Vec3f(-.7f, -.7f, 0.0f), osg::Vec3f(.7f, -.7f, 0.0f),
    osg::Vec3f(.7f, .7f, 0.0f), osg::Vec3f(-.7f, .7f, 0.0f)
};


const osg::Vec2f symbolTexCoords[4] = {
    osg::Vec2f(0.0f, 0.0f), osg::Vec2f(UNIT, 0.0f),
    osg::Vec2f(UNIT, UNIT), osg::Vec2f(0.0f, UNIT)
};


// helper
static void
addQuad(osg::Vec2Array *vertices, osg::Vec2Array *texCoords,
        const osg::Matrixf& transform, const osg::Vec2f& texBase)
{
    for (int i = 0; i < 4; i++) {
        const osg::Vec3f coords = transform.preMult(symbolCoords[i]);
        texCoords->push_back(texBase + symbolTexCoords[i]);
        vertices->push_back(osg::Vec2f(coords.x(), coords.y()));
    }
}


// Rotate by a heading value
static inline
osg::Matrixf wxRotate(float angle)
{
    return osg::Matrixf::rotate(angle, 0.0f, 0.0f, -1.0f);
}


void
NavDisplay::update (double delta_time_sec)
{
  if (!fgGetBool("sim/sceneryloaded", false)) {
    return;
  }

  if (!_odg || !_serviceable_node->getBoolValue()) {
    _Instrument->setStringValue("status", "");
    return;
  }
  
  _time += delta_time_sec;
  if (_time < _interval){
    return;
  }
  _time -= _interval;

  string mode = _Instrument->getStringValue("display-mode", "arc");
  _rangeNm = _Instrument->getFloatValue("range", 40.0);
  _view_heading = fgGetDouble("/orientation/heading-deg") * SG_DEGREES_TO_RADIANS;
  _scale = 200.0;
    
  bool centre = _radar_centre_node->getBoolValue();
    if (centre) {
        _centerTrans = osg::Matrixf::identity();
    } else {
        _centerTrans = osg::Matrixf::identity();
    }
    
    _drawData = _radar_data_node->getBoolValue();
    _pos = SGGeod::fromDegFt(_user_lon_node->getDoubleValue(),
                                      _user_lat_node->getDoubleValue(),
                                      _user_alt_node->getDoubleValue());
    
  _vertices->clear();
  _lineVertices->clear();
  _lineColors->clear();
  _texCoords->clear();
  _textGeode->removeDrawables(0, _textGeode->getNumDrawables());
  
  update_aircraft();
  update_route();
  
  
  _symbolPrimSet->set(osg::PrimitiveSet::QUADS, 0, _vertices->size());
  _symbolPrimSet->dirty();
  _linePrimSet->set(osg::PrimitiveSet::LINES, 0, _lineVertices->size());
  _linePrimSet->dirty();
}

osg::Matrixf NavDisplay::project(const SGGeod& geod) const
{
    double rangeM, bearing, az2;
    SGGeodesy::inverse(_pos, geod, bearing, az2, rangeM);
    
    double radius = ((rangeM * SG_METER_TO_NM) / _rangeNm) * _scale;
    bearing *= SG_DEGREES_TO_RADIANS;
    
    return osg::Matrixf(wxRotate(_view_heading - bearing)
                          * osg::Matrixf::translate(0.0f, radius, 0.0f)
                          * wxRotate(bearing) * _centerTrans);

}

void
NavDisplay::addSymbol(const SGGeod& pos, int symbolIndex, const std::string& data)
{
    int symbolRow = symbolIndex >> 4;
    int symbolColumn = symbolIndex & 0x0f;
    
    const osg::Vec2f texBase(UNIT * symbolColumn, UNIT * symbolRow);
    float size = 600 * UNIT;
    
    osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
                   * project(pos));
    addQuad(_vertices, _texCoords, m, texBase);
    
    if (!_drawData) {
        return;
    }

// add data drawable
    osgText::Text* text = new osgText::Text;
    text->setFont(_font.get());
    text->setFontResolution(12, 12);
    text->setCharacterSize(_font_size);

    osg::Vec3 dataPos = m.preMult(osg::Vec3(16, 16, 0));
    text->setLineSpacing(_font_spacing);
    
    text->setAlignment(osgText::Text::LEFT_CENTER);
    text->setText(data);
    text->setPosition(osg::Vec3((int) dataPos.x(), (int)dataPos.y(), 0));
    _textGeode->addDrawable(text);
}

void
NavDisplay::update_route()
{
    if (_route->numWaypts() < 2) {
        return;
    }
    
    RoutePath path(_route->waypts());
    for (int w=0; w<_route->numWaypts(); ++w) {
        bool isPast = w < _route->currentIndex();
        SGGeodVec gv(path.pathForIndex(w));
        if (!gv.empty()) {
            osg::Vec3 color(1.0, 0.0, 1.0);
            if (isPast) {
                color = osg::Vec3(0.5, 0.5, 0.5);
            }
            
            osg::Vec3 pr = project(gv[0]).preMult(osg::Vec3(0.0, 0.0, 0.0));
            for (unsigned int i=1; i<gv.size(); ++i) {
                _lineVertices->push_back(osg::Vec2(pr.x(), pr.y()));
                pr = project(gv[i]).preMult(osg::Vec3(0.0, 0.0, 0.0));
               _lineVertices->push_back(osg::Vec2(pr.x(), pr.y()));
                
               _lineColors->push_back(color);
               _lineColors->push_back(color);
            }
        } // of line drawing
        
        flightgear::WayptRef wpt(_route->wayptAtIndex(w));
        SGGeod g = path.positionForIndex(w);
        int symbolIndex = isPast ? 1 : 0;
        if (!(g == SGGeod())) {
            std::string data = wpt->ident();
            addSymbol(g, symbolIndex, data);
        }
    } // of waypoints iteration
}

void
NavDisplay::update_aircraft()
{
    if (!_ai_enabled_node->getBoolValue()) {
        return;
    }

    bool draw_tcas     = _radar_tcas_node->getBoolValue();
    bool draw_absolute = _radar_absalt_node->getBoolValue();
    bool draw_echoes   = _radar_position_node->getBoolValue();
    bool draw_symbols  = _radar_symbol_node->getBoolValue();
    bool draw_data     = _radar_data_node->getBoolValue();
    if (!draw_echoes && !draw_symbols && !draw_data)
        return;
    
    const SGPropertyNode *ai = fgGetNode("/ai/models", true);
    for (int i = ai->nChildren() - 1; i >= 0; i--) {
        const SGPropertyNode *model = ai->getChild(i);
        if (!model->nChildren()) {
            continue;
        }

        double echo_radius, sigma;
        const string name = model->getName();

        if (name == "aircraft" || name == "tanker")
            echo_radius = 1, sigma = 1;
        else if (name == "multiplayer" || name == "wingman" || name == "static")
            echo_radius = 1.5, sigma = 1;
        else if (name == "ship" || name == "carrier" || name == "escort" ||name == "storm")
            echo_radius = 1.5, sigma = 100;
        else if (name == "thermal")
            echo_radius = 2, sigma = 100;
        else if (name == "rocket")
            echo_radius = 0.1, sigma = 0.1;
        else if (name == "ballistic")
            echo_radius = 0.001, sigma = 0.001;
        else
            continue;
      
        SGGeod aiModelPos = SGGeod::fromDegFt(model->getDoubleValue("position/longitude-deg"), 
                                            model->getDoubleValue("position/latitude-deg"), 
                                            model->getDoubleValue("position/altitude-ft"));
      
        double heading = model->getDoubleValue("orientation/true-heading-deg");
        double rangeM, bearing, az2;
        SGGeodesy::inverse(_pos, aiModelPos, bearing, az2, rangeM);

     //   if (!inRadarRange(sigma, rangeM))
     //       continue;

        bearing *= SG_DEGREES_TO_RADIANS;
        heading *= SG_DEGREES_TO_RADIANS;

        float radius = rangeM * _scale;
     //   float angle = relativeBearing(bearing, _view_heading);

        bool is_tcas_contact = false;
        if (draw_tcas)
        {
            is_tcas_contact = update_tcas(model,rangeM, 
                                          _pos.getElevationFt(),
                                          aiModelPos.getElevationFt(),
                                          bearing,radius,draw_absolute);
        }

        // data mode
        if (draw_symbols && (!draw_tcas)) {
            const osg::Vec2f texBase(0, 3 * UNIT);
            float size = 600 * UNIT;
            osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
                * wxRotate(heading - bearing)
                * osg::Matrixf::translate(0.0f, radius, 0.0f)
                * wxRotate(bearing) * _centerTrans);
            addQuad(_vertices, _texCoords, m, texBase);
        }

        if (draw_data || is_tcas_contact) {
            update_data(model, aiModelPos.getElevationFt(), heading, radius, bearing, false);
        }
    } // of ai models iteration
}

/** Update TCAS display.
 * Return true when processed as TCAS contact, false otherwise. */
bool
NavDisplay::update_tcas(const SGPropertyNode *model,double range,double user_alt,double alt,
                       double bearing,double radius,bool absMode)
{
    int threatLevel=0;
    {
        // update TCAS symbol
        osg::Vec2f texBase;
        threatLevel = model->getIntValue("tcas/threat-level",-1);
        if (threatLevel == -1)
        {
            // no TCAS information (i.e. no transponder) => not visible to TCAS
            return false;
        }
        int row = 7 - threatLevel;
        int col = 4;
        double vspeed = model->getDoubleValue("velocities/vertical-speed-fps");
        if (vspeed < -3.0) // descending
            col+=1;
        else
        if (vspeed > 3.0) // climbing
            col+=2;
        texBase = osg::Vec2f(col*UNIT,row * UNIT);
        float size = 200 * UNIT;
            osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
                * wxRotate(-bearing)
                * osg::Matrixf::translate(0.0f, radius, 0.0f)
                * wxRotate(bearing) * _centerTrans);
            addQuad(_vertices, _texCoords, m, texBase);
    }

    {
        // update TCAS data
        osgText::Text *altStr = new osgText::Text;
        altStr->setFont(_font.get());
        altStr->setFontResolution(12, 12);
        altStr->setCharacterSize(_font_size);
        altStr->setColor(_tcas_colors[threatLevel]);
        osg::Matrixf m(wxRotate(-bearing)
            * osg::Matrixf::translate(0.0f, radius, 0.0f)
            * wxRotate(bearing) * _centerTrans);
    
        osg::Vec3 pos = m.preMult(osg::Vec3(16, 16, 0));
        // cast to int's, otherwise text comes out ugly
        altStr->setLineSpacing(_font_spacing);
    
        stringstream text;
        altStr->setAlignment(osgText::Text::LEFT_CENTER);
        int altDif = (alt-user_alt+50)/100;
        char sign = 0;
        int dy=0;
        if (altDif>=0)
        {
            sign='+';
            dy=2;
        }
        else
        if (altDif<0)
        {
            sign='-';
            altDif = -altDif;
            dy=-30;
        }
        altStr->setPosition(osg::Vec3((int)pos.x()-30, (int)pos.y()+dy, 0));
        if (absMode)
        {
            // absolute altitude display
            text << setprecision(0) << fixed
                 << setw(3) << setfill('0') << alt/100 << endl;
        }
        else // relative altitude display
        if (sign)
        {
            text << sign
                 << setprecision(0) << fixed
                 << setw(2) << setfill('0') << altDif << endl;
        }
    
        altStr->setText(text.str());
        _textGeode->addDrawable(altStr);
    }

    return true;
}

void NavDisplay::update_data(const SGPropertyNode *ac, double altitude, double heading,
                       double radius, double bearing, bool selected)
{
  osgText::Text *callsign = new osgText::Text;
  callsign->setFont(_font.get());
  callsign->setFontResolution(12, 12);
  callsign->setCharacterSize(_font_size);
  callsign->setColor(selected ? osg::Vec4(1, 1, 1, 1) : _font_color);
  osg::Matrixf m(wxRotate(-bearing)
                 * osg::Matrixf::translate(0.0f, radius, 0.0f)
                 * wxRotate(bearing) * _centerTrans);
  
  osg::Vec3 pos = m.preMult(osg::Vec3(16, 16, 0));
  // cast to int's, otherwise text comes out ugly
  callsign->setPosition(osg::Vec3((int)pos.x(), (int)pos.y(), 0));
  callsign->setAlignment(osgText::Text::LEFT_BOTTOM_BASE_LINE);
  callsign->setLineSpacing(_font_spacing);
  
  const char *identity = ac->getStringValue("transponder-id");
  if (!identity[0])
    identity = ac->getStringValue("callsign");
  
  stringstream text;
  text << identity << endl
  << setprecision(0) << fixed
  << setw(3) << setfill('0') << heading * SG_RADIANS_TO_DEGREES << "\xB0 "
  << setw(0) << altitude << "ft" << endl
  << ac->getDoubleValue("velocities/true-airspeed-kt") << "kts";
  
  callsign->setText(text.str());
  _textGeode->addDrawable(callsign);
}

void
NavDisplay::updateFont()
{
    float red = _font_node->getFloatValue("color/red");
    float green = _font_node->getFloatValue("color/green");
    float blue = _font_node->getFloatValue("color/blue");
    float alpha = _font_node->getFloatValue("color/alpha");
    _font_color.set(red, green, blue, alpha);

    _font_size = _font_node->getFloatValue("size");
    _font_spacing = _font_size * _font_node->getFloatValue("line-spacing");
    string path = _font_node->getStringValue("name", DEFAULT_FONT);

    SGPath tpath;
    if (path[0] != '/') {
        tpath = globals->get_fg_root();
        tpath.append("Fonts");
        tpath.append(path);
    } else {
        tpath = path;
    }

#if (FG_OSG_VERSION >= 21000)
    osg::ref_ptr<osgDB::ReaderWriter::Options> fontOptions = new osgDB::ReaderWriter::Options("monochrome");
    osg::ref_ptr<osgText::Font> font = osgText::readFontFile(tpath.c_str(), fontOptions.get());
#else
    osg::ref_ptr<osgText::Font> font = osgText::readFontFile(tpath.c_str());
#endif

    if (font != 0) {
        _font = font;
        _font->setMinFilterHint(osg::Texture::NEAREST);
        _font->setMagFilterHint(osg::Texture::NEAREST);
        _font->setGlyphImageMargin(0);
        _font->setGlyphImageMarginRatio(0);
    }

    for (int i=0;i<4;i++)
    {
        const float defaultColors[4][3] = {{0,1,1},{0,1,1},{1,0.5,0},{1,0,0}};
        SGPropertyNode_ptr color_node = _font_node->getNode("tcas/color",i,true);
        float red   = color_node->getFloatValue("red",defaultColors[i][0]);
        float green = color_node->getFloatValue("green",defaultColors[i][1]);
        float blue  = color_node->getFloatValue("blue",defaultColors[i][2]);
        float alpha = color_node->getFloatValue("alpha",1);
        _tcas_colors[i]=osg::Vec4(red, green, blue, alpha);
    }
}


