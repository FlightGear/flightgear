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
#include <memory>

#include <Navaids/positioned.hxx>

class FGODGauge;
class FGRouteMgr;
class FGNavRecord;

class SymbolInstance;
class SymbolDef;
class SymbolRule;

namespace flightgear
{
    class Waypt;
}

typedef std::set<std::string> string_set;
typedef std::vector<SymbolRule*> SymbolRuleVector;
typedef std::vector<SymbolDef*> SymbolDefVector;

class NavDisplay : public SGSubsystem
{
public:

    NavDisplay(SGPropertyNode *node);
    virtual ~NavDisplay();

    virtual void init();
    virtual void update(double dt);

    void invalidatePositionedCache()
    {
        _cachedItemsValid = false;
    }
    
    double textureSize() const
    { return _textureSize; }
    
    void forceUpdate()
    { _forceUpdate = true; }
    
    bool anyRuleForType(const std::string& type) const;
    bool isPositionedShown(FGPositioned* pos);
protected:
    std::string _name;
    int _num;
    double _time;
    double _updateInterval;
    bool _forceUpdate;
    
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _Instrument;
    SGPropertyNode_ptr _radar_mode_control_node;
    SGPropertyNode_ptr _user_heading_node;
    SGPropertyNode_ptr _testModeNode;
  
    FGODGauge *_odg;

    // Convenience function for creating a property node with a
    // default value
    template<typename DefaultType>
    SGPropertyNode *getInstrumentNode(const char *name, DefaultType value);

private:
    friend class SymbolRule;
    friend class SymbolDef;
  
    void addRule(SymbolRule*);
  
    void addSymbolsToScene();
    void addSymbolToScene(SymbolInstance* sym);
    void limitDisplayedSymbols();
    
    void findItems();
    void isPositionedShownInner(FGPositioned* pos, SymbolRuleVector& rules);
    void foundPositionedItem(FGPositioned* pos);
    void computePositionedPropsAndHeading(FGPositioned* pos, SGPropertyNode* nd, double& heading);
    void computePositionedState(FGPositioned* pos, string_set& states);
    void processRoute();
    void computeWayptPropsAndHeading(flightgear::Waypt* wpt, const SGGeod& pos, SGPropertyNode* nd, double& heading);
    void processNavRadios();
    FGNavRecord* processNavRadio(const SGPropertyNode_ptr& radio);
    void processAI();
    void computeAIStates(const SGPropertyNode* ai, string_set& states);
    
    void findRules(const std::string& type, const string_set& states, SymbolRuleVector& rules);
    
    SymbolInstance* addSymbolInstance(const osg::Vec2& proj, double heading, SymbolDef* def, SGPropertyNode* vars);
    void addLine(osg::Vec2 a, osg::Vec2 b, const osg::Vec4& color);
    osg::Vec2 projectBearingRange(double bearingDeg, double rangeNm) const;
    osg::Vec2 projectGeod(const SGGeod& geod) const;
    bool isProjectedClipped(const osg::Vec2& projected) const;
    void updateFont();
    
    void addTestSymbol(const std::string& type, const std::string& states, const SGGeod& pos, double heading, SGPropertyNode* vars);
    void addTestSymbols();
  
    std::string _texture_path;
    unsigned int _textureSize;

    float _scale;   // factor to convert nm to display units
    float _view_heading;
    
    SGPropertyNode_ptr _Radar_controls;


    
    SGPropertyNode_ptr _font_node;
    SGPropertyNode_ptr _ai_enabled_node;
    SGPropertyNode_ptr _navRadio1Node;
    SGPropertyNode_ptr _navRadio2Node;
    SGPropertyNode_ptr _xCenterNode, _yCenterNode;
    SGPropertyNode_ptr _viewHeadingNode;
  
    osg::ref_ptr<osg::Texture2D> _symbolTexture;
    osg::ref_ptr<osg::Geode> _radarGeode;
    osg::ref_ptr<osg::Geode> _textGeode;
  
    osg::Geometry *_geom;
  
    osg::DrawArrays* _symbolPrimSet;
    osg::Vec2Array *_vertices;
    osg::Vec2Array *_texCoords;
    osg::Vec4Array* _quadColors;
    
    osg::Geometry* _lineGeometry;
    osg::DrawArrays* _linePrimSet;
    osg::Vec2Array* _lineVertices;
    osg::Vec4Array* _lineColors;
  
  
    osg::Matrixf _centerTrans;
    osg::Matrixf _projectMat;
    
    osg::ref_ptr<osgText::Font> _font;
    osg::Vec4 _font_color;
    float _font_size;
    float _font_spacing;

    FGRouteMgr* _route;
    SGGeod _pos;
    double _rangeNm;
    SGPropertyNode_ptr _rangeNode;
    
    SymbolDefVector _definitions;
    SymbolRuleVector _rules;
    FGNavRecord* _nav1Station;
    FGNavRecord* _nav2Station;
    std::vector<SymbolInstance*> _symbols;
    std::set<FGPositioned*> _routeSources;
    
    bool _cachedItemsValid;
    SGVec3d _cachedPos;
    FGPositioned::List _itemsInRange;
    SGPropertyNode_ptr _excessDataNode;
    int _maxSymbols;
  
    class CacheListener;
    std::auto_ptr<CacheListener> _cacheListener;
    
    class ForceUpdateListener;
    std::auto_ptr<ForceUpdateListener> _forceUpdateListener;
};

#endif // _INST_ND_HXX
