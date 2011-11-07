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

#include <cassert>
#include <boost/foreach.hpp>
#include <algorithm>

#include <osg/Array>
#include <osg/Geometry>
#include <osg/Matrixf>
#include <osg/PrimitiveSet>
#include <osg/StateSet>
#include <osg/LineWidth>
#include <osg/Version>

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

static const char *DEFAULT_FONT = "typewriter.txf";

static
osg::Matrixf degRotation(float angle)
{
    return osg::Matrixf::rotate(angle * SG_DEGREES_TO_RADIANS, 0.0f, 0.0f, -1.0f);
}

static osg::Vec4 readColor(SGPropertyNode* colorNode, const osg::Vec4& c)
{
    osg::Vec4 result;
    result.r() = colorNode->getDoubleValue("red",   c.r());
    result.g() = colorNode->getDoubleValue("green", c.g());
    result.b() = colorNode->getDoubleValue("blue",  c.b());
    result.a() = colorNode->getDoubleValue("alpha", c.a());
    return result;
}

static osgText::Text::AlignmentType readAlignment(const std::string& t)
{
    if (t == "left-top") {
        return osgText::Text::LEFT_TOP;
    } else if (t == "left-center") {
        return osgText::Text::LEFT_CENTER;
    } else if (t == "left-bottom") {
        return osgText::Text::LEFT_BOTTOM;
    } else if (t == "center-top") {
        return osgText::Text::CENTER_TOP;
    } else if (t == "center-center") {
        return osgText::Text::CENTER_CENTER;
    } else if (t == "center-bottom") {
        return osgText::Text::CENTER_BOTTOM;
    } else if (t == "right-top") {
        return osgText::Text::RIGHT_TOP;
    } else if (t == "right-center") {
        return osgText::Text::RIGHT_CENTER;
    } else if (t == "right-bottom") {
        return osgText::Text::RIGHT_BOTTOM;
    } else if (t == "left-baseline") {
        return osgText::Text::LEFT_BASE_LINE;
    } else if (t == "center-baseline") {
        return osgText::Text::CENTER_BASE_LINE;
    } else if (t == "right-baseline") {
        return osgText::Text::RIGHT_BASE_LINE;
    }
    
    return osgText::Text::BASE_LINE;
}

static string formatPropertyValue(SGPropertyNode* nd, const string& format)
{
    assert(nd);
    static char buf[512];
    if (format.find('d') != string::npos) {
        ::snprintf(buf, 512, format.c_str(), nd->getIntValue());
        return buf;
    }
    
    if (format.find('s') != string::npos) {
        ::snprintf(buf, 512, format.c_str(), nd->getStringValue());
        return buf;
    }
    
// assume it's a double/float
    ::snprintf(buf, 512, format.c_str(), nd->getDoubleValue());
    return buf;
}

static osg::Vec2 mult(const osg::Vec2& v, const osg::Matrixf& m)
{
    osg::Vec3 r = m.preMult(osg::Vec3(v.x(), v.y(), 0.0));
    return osg::Vec2(r.x(), r.y());
}

class NavDisplay::CacheListener : public SGPropertyChangeListener
{
public:
    CacheListener(NavDisplay *nd) : 
        _nd(nd)
    {}
    
    virtual void valueChanged (SGPropertyNode * prop)
    {
        _nd->invalidatePositionedCache();
        SG_LOG(SG_INSTR, SG_INFO, "invalidating NavDisplay cache");
    }
private:
    NavDisplay* _nd;
};

///////////////////////////////////////////////////////////////////

class SymbolDef
{
public:
  SymbolDef() :
      enable(NULL)
    {}
  
    bool initFromNode(SGPropertyNode* node)
    {
        type = node->getStringValue("type");
        SGPropertyNode* enableNode = node->getChild("enable");
        if (enableNode) { 
            enable = sgReadCondition(fgGetNode("/"), enableNode);
        }
      
        int n=0;
        while (node->hasChild("state", n)) {
            string m = node->getChild("state", n++)->getStringValue();
            if (m[0] == '!') {
                excluded_states.insert(m.substr(1));
            } else {
                required_states.insert(m);
            }
        } // of matches parsing
        
      if (node->hasChild("width")) {
        float w = node->getFloatValue("width");
        float h = node->getFloatValue("height", w);
        xy0.x() = -w * 0.5;
        xy0.y() = -h * 0.5;
        xy1.x() = w * 0.5;
        xy1.y() = h * 0.5;
      } else {
        xy0.x()  = node->getFloatValue("x0", 0.0);
        xy0.y()  = node->getFloatValue("y0", 0.0);
        xy1.x()  = node->getFloatValue("x1", 5);
        xy1.y()  = node->getFloatValue("y1", 5);
      }
      
        double texSize = node->getFloatValue("texture-size", 1.0);
        
        uv0.x()  = node->getFloatValue("u0", 0) / texSize;
        uv0.y()  = node->getFloatValue("v0", 0) / texSize;
        uv1.x()  = node->getFloatValue("u1", 1) / texSize;
        uv1.y()  = node->getFloatValue("v1", 1) / texSize;
        
        color = readColor(node->getChild("color"), osg::Vec4(1, 1, 1, 1));
        priority = node->getIntValue("priority", 0);
        zOrder = node->getIntValue("zOrder", 0);
        rotateToHeading = node->getBoolValue("rotate-to-heading", false);
        roundPos = node->getBoolValue("round-position", true);
        hasText = false;
        if (node->hasChild("text")) {
            hasText = true;
            alignment = readAlignment(node->getStringValue("text-align"));
            textTemplate = node->getStringValue("text");
            textOffset.x() = node->getFloatValue("text-offset-x", 0);
            textOffset.y() = node->getFloatValue("text-offset-y", 0);
            textColor = readColor(node->getChild("text-color"), color);
        }
        
        drawLine = node->getBoolValue("draw-line", false);
        lineColor = readColor(node->getChild("line-color"), color);
        drawRouteLeg = node->getBoolValue("draw-leg", false);
        
        stretchSymbol = node->getBoolValue("stretch-symbol", false);
        if (stretchSymbol) {
            stretchY2 = node->getFloatValue("y2");
            stretchY3 = node->getFloatValue("y3");
            stretchV2 = node->getFloatValue("v2");
            stretchV3 = node->getFloatValue("v3");
        }
      
        return true;
    }
    
    SGCondition* enable;
    bool enabled; // cached enabled state
    
    std::string type;
    string_set required_states;
    string_set excluded_states;
    
    osg::Vec2 xy0, xy1;
    osg::Vec2 uv0, uv1;
    osg::Vec4 color;
    
    int priority;
    int zOrder;
    bool rotateToHeading;
    bool roundPos; ///< should position be rounded to integer values
    bool hasText;
    osg::Vec4 textColor;
    osg::Vec2 textOffset;
    osgText::Text::AlignmentType alignment;
    string textTemplate;
    
    bool drawLine;
    osg::Vec4 lineColor;
    
// symbol stretching creates three quads (instead of one) - a start,
// middle and end quad, positioned along the line of the symbol.
// X (and U) axis values determined by the values above, so we only need
// to define the Y (and V) values to build the other quads.
    bool stretchSymbol;
    double stretchY2, stretchY3;
    double stretchV2, stretchV3;
    
    bool drawRouteLeg;
    
    bool matches(const string_set& states) const
    {
        BOOST_FOREACH(const string& s, required_states) {
            if (states.count(s) == 0) {
                return false;
            }
        }
        
        BOOST_FOREACH(const string& s, excluded_states) {
            if (states.count(s) != 0) {
                return false;
            }
        }
        
        return true;
    }
};

class SymbolInstance
{
public:
    SymbolInstance(const osg::Vec2& p, double h, SymbolDef* def, SGPropertyNode* vars) :
        pos(p),
        headingDeg(h),
        definition(def),
        props(vars)
    { }
    
    osg::Vec2 pos; // projected position
    osg::Vec2 endPos;
    double headingDeg;
    SymbolDef* definition;
    SGPropertyNode_ptr props;
    
    string text() const
    {
        assert(definition->hasText);
        string r;        
        size_t lastPos = 0;
        
        while (true) {
            size_t pos = definition->textTemplate.find('{', lastPos);
            if (pos == string::npos) { // no more replacements
                r.append(definition->textTemplate.substr(lastPos));
                break;
            }
            
            r.append(definition->textTemplate.substr(lastPos, pos - lastPos));
            
            size_t endReplacement = definition->textTemplate.find('}', pos+1);
            if (endReplacement <= pos) {
                return "bad replacement";
            }

            string spec = definition->textTemplate.substr(pos + 1, endReplacement - (pos + 1));
        // look for formatter in spec
            size_t colonPos = spec.find(':');
            if (colonPos == string::npos) {
            // simple replacement
                r.append(props->getStringValue(spec));
            } else {
                string format = spec.substr(colonPos + 1);
                string prop = spec.substr(0, colonPos);
                r.append(formatPropertyValue(props->getNode(prop), format));
            }
            
            lastPos = endReplacement + 1;
        }
        
        return r;
    }
};

//////////////////////////////////////////////////////////////////

NavDisplay::NavDisplay(SGPropertyNode *node) :
    _name(node->getStringValue("name", "nd")),
    _num(node->getIntValue("number", 0)),
    _time(0.0),
    _updateInterval(node->getDoubleValue("update-interval-sec", 0.1)),
    _odg(0),
    _scale(0),
    _view_heading(0),
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

    SGPropertyNode* symbolsNode = node->getNode("symbols");
    SGPropertyNode* symbol;
  
    for (int i = 0; (symbol = symbolsNode->getChild("symbol", i)) != NULL; ++i) {
        SymbolDef* def = new SymbolDef;
        if (!def->initFromNode(symbol)) {
          delete def;
          continue;
        }
        
        _rules.push_back(def);
    } // of symbol definition parsing
}


NavDisplay::~NavDisplay()
{
}


void
NavDisplay::init ()
{
    _cachedItemsValid = false;
    _cacheListener.reset(new CacheListener(this));
    
    _serviceable_node = _Instrument->getNode("serviceable", true);
    _rangeNode = _Instrument->getNode("range", true);
    _rangeNode->setDoubleValue(40.0);
    _rangeNode->addChangeListener(_cacheListener.get());
    
    // texture name to use in 2D and 3D instruments
    _texture_path = _Instrument->getStringValue("radar-texture-path",
        "Aircraft/Instruments/Textures/od_wxradar.rgb");
    _resultTexture = FGTextureManager::createTexture(_texture_path.c_str(), false);

    string path = _Instrument->getStringValue("symbol-texture-path",
        "Aircraft/Instruments/Textures/nd-symbols.png");
    SGPath tpath = globals->resolve_aircraft_path(path);
    if (!tpath.exists()) {
      SG_LOG(SG_INSTR, SG_WARN, "ND symbol texture not found:" << path);
    }
  
    // no mipmap or else alpha will mix with pixels on the border of shapes, ruining the effect
    _symbolTexture = SGLoadTexture2D(tpath, NULL, false, false);

    FGInstrumentMgr *imgr = (FGInstrumentMgr *)globals->get_subsystem("instrumentation");
    _odg = (FGODGauge *)imgr->get_subsystem("od_gauge");
    _odg->setSize(_Instrument->getIntValue("texture-size", 512));

    _route = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
    
    _navRadio1Node = fgGetNode("/instrumentation/nav[0]", true);
    _navRadio2Node = fgGetNode("/instrumentation/nav[1]", true);
    
    _excessDataNode = _Instrument->getChild("excess-data", 0, true);
    _excessDataNode->setBoolValue(false);
  
// OSG geometry setup
    _radarGeode = new osg::Geode;

    _geom = new osg::Geometry;
    _geom->setUseDisplayList(false);
    
    osg::StateSet *stateSet = _geom->getOrCreateStateSet();
    stateSet->setTextureAttributeAndModes(0, _symbolTexture.get());
    
    // Initially allocate space for 128 quads
    _vertices = new osg::Vec2Array;
    _vertices->setDataVariance(osg::Object::DYNAMIC);
    _vertices->reserve(128 * 4);
    _geom->setVertexArray(_vertices);
    _texCoords = new osg::Vec2Array;
    _texCoords->setDataVariance(osg::Object::DYNAMIC);
    _texCoords->reserve(128 * 4);
    _geom->setTexCoordArray(0, _texCoords);
    
    _quadColors = new osg::Vec4Array;
    _quadColors->setDataVariance(osg::Object::DYNAMIC);
    _geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    _geom->setColorArray(_quadColors);
    
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
    stateSet = _lineGeometry->getOrCreateStateSet();    
    osg::LineWidth *lw = new osg::LineWidth();
    lw->setWidth(2.0);
    stateSet->setAttribute(lw);
    
    _lineVertices = new osg::Vec2Array;
    _lineVertices->setDataVariance(osg::Object::DYNAMIC);
    _lineVertices->reserve(128 * 4);
    _lineGeometry->setVertexArray(_lineVertices);
    
                  
    _lineColors = new osg::Vec4Array;
    _lineColors->setDataVariance(osg::Object::DYNAMIC);
    _lineGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    _lineGeometry->setColorArray(_lineColors);
    
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
    osg::Texture2D* tex = _odg->getTexture();
    camera->setProjectionMatrixAsOrtho2D(0, tex->getTextureWidth(), 
        0, tex->getTextureHeight());
    
    updateFont();
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
  if (_time < _updateInterval){
    return;
  }
  _time -= _updateInterval;

  _rangeNm = _rangeNode->getFloatValue();
  if (_Instrument->getBoolValue("aircraft-heading-up", true)) {
    _view_heading = fgGetDouble("/orientation/heading-deg");
  } else {
    _view_heading = _Instrument->getFloatValue("heading-up-deg", 0.0);
  }
  
  _scale = _odg->size() / _rangeNm;
  
  double xCenterFrac = _Instrument->getDoubleValue("x-center", 0.5);
  double yCenterFrac = _Instrument->getDoubleValue("y-center", 0.5);
  _centerTrans = osg::Matrixf::translate(xCenterFrac * _odg->size(), 
      yCenterFrac * _odg->size(), 0.0);

// scale from nm to display units, rotate so aircraft heading is up
// (as opposed to north), and compensate for centering
  _projectMat = osg::Matrixf::scale(_scale, _scale, 1.0) * 
      degRotation(-_view_heading) * _centerTrans;
  
  _pos = globals->get_aircraft_position();

    // invalidate the cache of positioned items, if we travelled more than 1nm
    if (_cachedItemsValid) {
        SGVec3d cartNow(SGVec3d::fromGeod(_pos));
        double movedNm = dist(_cachedPos, cartNow) * SG_METER_TO_NM;
        _cachedItemsValid = (movedNm < 1.0);
        if (!_cachedItemsValid) {
            SG_LOG(SG_INSTR, SG_INFO, "invalidating NavDisplay cache due to moving: " << movedNm);
        }
    }
    
  _vertices->clear();
  _lineVertices->clear();
  _lineColors->clear();
  _quadColors->clear();
  _texCoords->clear();
  _textGeode->removeDrawables(0, _textGeode->getNumDrawables());
  
  BOOST_FOREACH(SymbolInstance* si, _symbols) {
      delete si;
  }
  _symbols.clear();
    
  BOOST_FOREACH(SymbolDef* def, _rules) {
    if (def->enable) {
      def->enabled = def->enable->test();
    } else {
      def->enabled = true;
    }
  }
  
  processRoute();
  processNavRadios();
  processAI();
  findItems();
  limitDisplayedSymbols();
  addSymbolsToScene();
  
  _symbolPrimSet->set(osg::PrimitiveSet::QUADS, 0, _vertices->size());
  _symbolPrimSet->dirty();
  _linePrimSet->set(osg::PrimitiveSet::LINES, 0, _lineVertices->size());
  _linePrimSet->dirty();
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

    osg::ref_ptr<osgDB::ReaderWriter::Options> fontOptions = new osgDB::ReaderWriter::Options("monochrome");
    osg::ref_ptr<osgText::Font> font = osgText::readFontFile(tpath.c_str(), fontOptions.get());

    if (font != 0) {
        _font = font;
        _font->setMinFilterHint(osg::Texture::NEAREST);
        _font->setMagFilterHint(osg::Texture::NEAREST);
        _font->setGlyphImageMargin(0);
        _font->setGlyphImageMarginRatio(0);
    }
}

void NavDisplay::addSymbolToScene(SymbolInstance* sym)
{
    SymbolDef* def = sym->definition;
    
    osg::Vec2 verts[4];
    verts[0] = def->xy0;
    verts[1] = osg::Vec2(def->xy1.x(), def->xy0.y());
    verts[2] = def->xy1;
    verts[3] = osg::Vec2(def->xy0.x(), def->xy1.y());
    
    if (def->rotateToHeading) {
        osg::Matrixf m(degRotation(sym->headingDeg));
        for (int i=0; i<4; ++i) {
            verts[i] = mult(verts[i], m);
        }
    }
    
    osg::Vec2 pos = sym->pos;
    if (def->roundPos) {
        pos = osg::Vec2((int) pos.x(), (int) pos.y());
    }
    
    _texCoords->push_back(def->uv0);
    _texCoords->push_back(osg::Vec2(def->uv1.x(), def->uv0.y()));
    _texCoords->push_back(def->uv1);
    _texCoords->push_back(osg::Vec2(def->uv0.x(), def->uv1.y()));

    for (int i=0; i<4; ++i) {
        _vertices->push_back(verts[i] + pos);
        _quadColors->push_back(def->color);
    }
    
    if (def->stretchSymbol) {
        osg::Vec2 stretchVerts[4];
        stretchVerts[0] = osg::Vec2(def->xy0.x(), def->stretchY2);
        stretchVerts[1] = osg::Vec2(def->xy1.x(), def->stretchY2);
        stretchVerts[2] = osg::Vec2(def->xy1.x(), def->stretchY3);
        stretchVerts[3] = osg::Vec2(def->xy0.x(), def->stretchY3);
        
        osg::Matrixf m(degRotation(sym->headingDeg));
        for (int i=0; i<4; ++i) {
            stretchVerts[i] = mult(stretchVerts[i], m);
        }
        
    // stretched quad
        _vertices->push_back(verts[2] + pos);
        _vertices->push_back(stretchVerts[1] + sym->endPos);
        _vertices->push_back(stretchVerts[0] + sym->endPos);
        _vertices->push_back(verts[3] + pos);
        
        _texCoords->push_back(def->uv1);
        _texCoords->push_back(osg::Vec2(def->uv1.x(), def->stretchV2));
        _texCoords->push_back(osg::Vec2(def->uv0.x(), def->stretchV2));
        _texCoords->push_back(osg::Vec2(def->uv0.x(), def->uv1.y()));
        
        for (int i=0; i<4; ++i) {
            _quadColors->push_back(def->color);
        }
        
    // quad three, for the end portion
        for (int i=0; i<4; ++i) {
            _vertices->push_back(stretchVerts[i] + sym->endPos);
            _quadColors->push_back(def->color);
        }
        
        _texCoords->push_back(osg::Vec2(def->uv0.x(), def->stretchV2));
        _texCoords->push_back(osg::Vec2(def->uv1.x(), def->stretchV2));
        _texCoords->push_back(osg::Vec2(def->uv1.x(), def->stretchV3));
        _texCoords->push_back(osg::Vec2(def->uv0.x(), def->stretchV3));
    }
    
    if (def->drawLine) {
        addLine(sym->pos, sym->endPos, def->lineColor);
    }
    
    if (!def->hasText) {
        return;
    }
    
    osgText::Text* t = new osgText::Text;
    t->setFont(_font.get());
    t->setFontResolution(12, 12);
    t->setCharacterSize(_font_size);
    t->setLineSpacing(_font_spacing);
    t->setColor(def->textColor);
    t->setAlignment(def->alignment);
    t->setText(sym->text());


    osg::Vec2 textPos = def->textOffset + pos;
// ensure we use ints here, or text visual quality goes bad
    t->setPosition(osg::Vec3((int)textPos.x(), (int)textPos.y(), 0));
    _textGeode->addDrawable(t);
}

class OrderByPriority
{
public:
    bool operator()(SymbolInstance* a, SymbolInstance* b)
    {
        return a->definition->priority > b->definition->priority;
    }    
};

void NavDisplay::limitDisplayedSymbols()
{
    unsigned int maxSymbols = _Instrument->getIntValue("max-symbols", 100);
    if (_symbols.size() <= maxSymbols) {
        _excessDataNode->setBoolValue(false);
        return;
    }
    
    std::sort(_symbols.begin(), _symbols.end(), OrderByPriority());
    _symbols.resize(maxSymbols);
    _excessDataNode->setBoolValue(true);
}

class OrderByZ
{
public:
    bool operator()(SymbolInstance* a, SymbolInstance* b)
    {
        return a->definition->zOrder > b->definition->zOrder;
    }
};

void NavDisplay::addSymbolsToScene()
{
    std::sort(_symbols.begin(), _symbols.end(), OrderByZ());
    BOOST_FOREACH(SymbolInstance* sym, _symbols) {
        addSymbolToScene(sym);
    }
}

void NavDisplay::addLine(osg::Vec2 a, osg::Vec2 b, const osg::Vec4& color)
{    
    _lineVertices->push_back(a);
    _lineVertices->push_back(b);
    _lineColors->push_back(color);
    _lineColors->push_back(color);
}

osg::Vec2 NavDisplay::projectBearingRange(double bearingDeg, double rangeNm) const
{
    osg::Vec3 p(0, rangeNm, 0.0);
    p = degRotation(bearingDeg).preMult(p);
    p = _projectMat.preMult(p);
    return osg::Vec2(p.x(), p.y());
}

osg::Vec2 NavDisplay::projectGeod(const SGGeod& geod) const
{
    double rangeM, bearing, az2;
    SGGeodesy::inverse(_pos, geod, bearing, az2, rangeM);
    return projectBearingRange(bearing, rangeM * SG_METER_TO_NM);
}

class Filter : public FGPositioned::Filter
{
public:
    double minRunwayLengthFt;
  
    virtual bool pass(FGPositioned* aPos) const
    {
        if (aPos->type() == FGPositioned::FIX) {
            string ident(aPos->ident());
            // ignore fixes which end in digits
            if ((ident.size() > 4) && isdigit(ident[3]) && isdigit(ident[4])) {
                return false;
            }
        }

        if (aPos->type() == FGPositioned::AIRPORT) {
          FGAirport* apt = (FGAirport*) aPos;
          if (!apt->hasHardRunwayOfLengthFt(minRunwayLengthFt)) {
            return false;
          }
        }
      
        return true;
    }

    virtual FGPositioned::Type minType() const {
        return FGPositioned::AIRPORT;
    }

    virtual FGPositioned::Type maxType() const {
        return FGPositioned::OBSTACLE;
    }
};

void NavDisplay::findItems()
{
    if (!_cachedItemsValid) {
        SG_LOG(SG_INSTR, SG_INFO, "re-validating NavDisplay cache");
        Filter filt;
        filt.minRunwayLengthFt = 2000;
        _itemsInRange = FGPositioned::findWithinRange(_pos, _rangeNm, &filt);
        _cachedItemsValid = true;
        _cachedPos = SGVec3d::fromGeod(_pos);
    }
    
    BOOST_FOREACH(FGPositioned* pos, _itemsInRange) {
        foundPositionedItem(pos);
    }
}

void NavDisplay::processRoute()
{
    _routeSources.clear();
    RoutePath path(_route->waypts());
    int current = _route->currentIndex();
    
    for (int w=0; w<_route->numWaypts(); ++w) {
        flightgear::WayptRef wpt(_route->wayptAtIndex(w));
        _routeSources.insert(wpt->source());
        
        string_set state;
        state.insert("on-active-route");
        
        if (w < current) {
            state.insert("passed");
        }
        
        if (w == current) {
            state.insert("current-wp");
        }
        
        if (w > current) {
            state.insert("future");
        }
        
        if (w == (current + 1)) {
            state.insert("next-wp");
        }
        
        SymbolDefVector rules;
        findRules("waypoint" , state, rules);
        if (rules.empty()) {
            return; // no rules matched, we can skip this item
        }

        SGGeod g = path.positionForIndex(w);
        SGPropertyNode* vars = _route->wayptNodeAtIndex(w);
        double heading;
        computeWayptPropsAndHeading(wpt, g, vars, heading);

        osg::Vec2 projected = projectGeod(g);
        BOOST_FOREACH(SymbolDef* r, rules) {
            addSymbolInstance(projected, heading, r, vars);
            
            if (r->drawRouteLeg) {
                SGGeodVec gv(path.pathForIndex(w));
                if (!gv.empty()) {
                    osg::Vec2 pr = projectGeod(gv[0]);
                    for (unsigned int i=1; i<gv.size(); ++i) {
                        osg::Vec2 p = projectGeod(gv[i]);
                        addLine(pr, p, r->lineColor);
                        pr = p;
                    }
                }
            } // of leg drawing enabled
        } // of matching rules iteration
    } // of waypoints iteration
}

void NavDisplay::computeWayptPropsAndHeading(flightgear::Waypt* wpt, const SGGeod& pos, SGPropertyNode* nd, double& heading)
{
    double rangeM, az2;
    SGGeodesy::inverse(_pos, pos, heading, az2, rangeM);
    nd->setIntValue("radial", heading);
    nd->setDoubleValue("distance-nm", rangeM * SG_METER_TO_NM);
    
    heading = nd->getDoubleValue("leg-bearing-true-deg");
}

void NavDisplay::processNavRadios()
{
    _nav1Station = processNavRadio(_navRadio1Node);
    _nav2Station = processNavRadio(_navRadio2Node);
    
    foundPositionedItem(_nav1Station);
    foundPositionedItem(_nav2Station);
}

FGNavRecord* NavDisplay::processNavRadio(const SGPropertyNode_ptr& radio)
{
    double mhz = radio->getDoubleValue("frequencies/selected-mhz", 0.0);
    FGNavRecord* nav = globals->get_navlist()->findByFreq(mhz, _pos);
    if (!nav || (nav->ident() != radio->getStringValue("nav-id"))) {
        // station was not found
        return NULL;
    }
    
    
    return nav;
}

bool NavDisplay::anyRuleForType(const string& type) const
{
    BOOST_FOREACH(SymbolDef* r, _rules) {
        if (!r->enabled) {
            continue;
        }
    
        if (r->type == type) {
            return true;
        }
    }
    
    return false;
}

bool NavDisplay::anyRuleMatches(const string& type, const string_set& states) const
{
    BOOST_FOREACH(SymbolDef* r, _rules) {
        if (!r->enabled || (r->type != type)) {
            continue;
        }

        if (r->matches(states)) {
            return true;
        }
    } // of rules iteration
    
    return false;
}

void NavDisplay::findRules(const string& type, const string_set& states, SymbolDefVector& rules)
{
    BOOST_FOREACH(SymbolDef* candidate, _rules) {
        if (!candidate->enabled || (candidate->type != type)) {
            continue;
        }
        
        if (candidate->matches(states)) {
            rules.push_back(candidate);
        }
    }
}

void NavDisplay::foundPositionedItem(FGPositioned* pos)
{
    if (!pos) {
        return;
    }
    
    string type = FGPositioned::nameForType(pos->type());
    if (!anyRuleForType(type)) {
        return; // not diplayed at all, we're done
    }
    
    string_set states;
    computePositionedState(pos, states);
    
    SymbolDefVector rules;
    findRules(type, states, rules);
    if (rules.empty()) {
        return; // no rules matched, we can skip this item
    }
    
    SGPropertyNode_ptr vars(new SGPropertyNode);
    double heading;
    computePositionedPropsAndHeading(pos, vars, heading);
    
    osg::Vec2 projected = projectGeod(pos->geod());
    BOOST_FOREACH(SymbolDef* r, rules) {
        addSymbolInstance(projected, heading, r, vars);
    }
}

void NavDisplay::computePositionedPropsAndHeading(FGPositioned* pos, SGPropertyNode* nd, double& heading)
{
    nd->setStringValue("id", pos->ident());
    nd->setStringValue("name", pos->name());
    nd->setDoubleValue("elevation-ft", pos->elevation());
    nd->setIntValue("heading-deg", 0);
    
    switch (pos->type()) {
    case FGPositioned::VOR:
    case FGPositioned::LOC: 
    case FGPositioned::TACAN: {
        FGNavRecord* nav = static_cast<FGNavRecord*>(pos);
        nd->setDoubleValue("frequency-mhz", nav->get_freq());
        
        if (pos == _nav1Station) {
            nd->setIntValue("heading-deg", _navRadio1Node->getDoubleValue("radials/target-radial-deg"));
        } else if (pos == _nav2Station) {
            nd->setIntValue("heading-deg", _navRadio2Node->getDoubleValue("radials/target-radial-deg"));
        }
        
        break;
    }

    case FGPositioned::AIRPORT:
    case FGPositioned::SEAPORT:
    case FGPositioned::HELIPORT:
        
        break;
        
    case FGPositioned::RUNWAY: {
        FGRunway* rwy = static_cast<FGRunway*>(pos);
        nd->setDoubleValue("heading-deg", rwy->headingDeg());
        nd->setIntValue("length-ft", rwy->lengthFt());
        nd->setStringValue("airport", rwy->airport()->ident());
        break;
    }

    default:
        break; 
    }
}

void NavDisplay::computePositionedState(FGPositioned* pos, string_set& states)
{
    if (_routeSources.count(pos) != 0) {
        states.insert("on-active-route");
    }
    
    switch (pos->type()) {
    case FGPositioned::VOR:
    case FGPositioned::LOC:
        if (pos == _nav1Station) {
            states.insert("tuned");
            states.insert("nav1");
        }
        
        if (pos == _nav2Station) {
            states.insert("tuned");
            states.insert("nav2");
        }
        break;
    
    case FGPositioned::AIRPORT:
    case FGPositioned::SEAPORT:
    case FGPositioned::HELIPORT:
        // mark alternates!
        // once the FMS system has some way to tell us about them, of course
        
        if (pos == _route->departureAirport()) {
            states.insert("departure");
        }
        
        if (pos == _route->destinationAirport()) {
            states.insert("destination");
        }
        break;
    
    case FGPositioned::RUNWAY:
        if (pos == _route->departureRunway()) {
            states.insert("departure");
        }
        
        if (pos == _route->destinationRunway()) {
            states.insert("destination");
        }
        break;
    
    case FGPositioned::OBSTACLE:
    #if 0    
        FGObstacle* obs = (FGObstacle*) pos;
        if (obj->isLit()) {
            states.insert("lit");
        }
        
        if (obj->getHeightAGLFt() >= 1000) {
            states.insert("greater-1000-ft");
        }
    #endif
        break;
    
    default:
        break;
    } // FGPositioned::Type switch
}

void NavDisplay::processAI()
{
    SGPropertyNode *ai = fgGetNode("/ai/models", true);
    for (int i = ai->nChildren() - 1; i >= 0; i--) {
        SGPropertyNode *model = ai->getChild(i);
        if (!model->nChildren()) {
            continue;
        }
        
    // prefix types with 'ai-', to avoid any chance of namespace collisions
    // with fg-positioned.
        string_set ss;
        computeAIStates(model, ss);        
        SymbolDefVector rules;
        findRules(string("ai-") + model->getName(), ss, rules);
        if (rules.empty()) {
            return; // no rules matched, we can skip this item
        }

        double heading = model->getDoubleValue("orientation/true-heading-deg");
        SGGeod aiModelPos = SGGeod::fromDegFt(model->getDoubleValue("position/longitude-deg"), 
                                            model->getDoubleValue("position/latitude-deg"), 
                                            model->getDoubleValue("position/altitude-ft"));
    // compute some additional props
        int fl = (aiModelPos.getElevationFt() / 1000);
        model->setIntValue("flight-level", fl * 10);
                                            
        osg::Vec2 projected = projectGeod(aiModelPos);
        BOOST_FOREACH(SymbolDef* r, rules) {
            addSymbolInstance(projected, heading, r, (SGPropertyNode*) model);
        }
    } // of ai models iteration
}

void NavDisplay::computeAIStates(const SGPropertyNode* ai, string_set& states)
{
    int threatLevel = ai->getIntValue("tcas/threat-level",-1);
    if (threatLevel >= 0) {
        states.insert("tcas");
    //    states.insert("tcas-threat-level-" + itoa(threatLevel));
    }
    
    double vspeed = ai->getDoubleValue("velocities/vertical-speed-fps");
    if (vspeed < -3.0) {
        states.insert("descending");
    } else if (vspeed > 3.0) {
        states.insert("climbing");
    }
}

bool NavDisplay::addSymbolInstance(const osg::Vec2& proj, double heading, SymbolDef* def, SGPropertyNode* vars)
{
    if (isProjectedClipped(proj)) {
        return false;
    }
    
    SymbolInstance* sym = new SymbolInstance(proj, heading, def, vars);
    _symbols.push_back(sym);
    return true;
}

bool NavDisplay::isProjectedClipped(const osg::Vec2& projected) const
{
    double size = _odg->size();
    return (projected.x() < 0.0) ||
        (projected.y() < 0.0) ||
        (projected.x() >= size) ||
            (projected.y() >= size);
}




