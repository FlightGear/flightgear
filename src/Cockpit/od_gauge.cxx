// Owner Drawn Gauge helper class
//
// Written by Harald JOHNSEN, started May 2005.
//
// Copyright (C) 2005  Harald JOHNSEN
//
// Ported to OSG by Tim Moore - Jun 2007
//
// Heavily modified to be usable for the 2d Canvas by Thomas Geymayer - April 2012
// Supports now multisampling/mipmapping, usage of the stencil buffer and placing
// the texture in the scene by certain filter criteria
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

#include <osg/Texture2D>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/Geode>
#include <osg/NodeVisitor>
#include <osg/Matrix>
#include <osg/PolygonMode>
#include <osg/ShadeModel>
#include <osg/StateSet>
#include <osg/FrameBufferObject> // for GL_DEPTH_STENCIL_EXT on Windows

#include <osgDB/FileNameUtils>

#include <simgear/canvas/CanvasObjectPlacement.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/RenderConstants.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include "od_gauge.hxx"

#include <cassert>

//------------------------------------------------------------------------------
FGODGauge::FGODGauge()
{

}

//------------------------------------------------------------------------------
FGODGauge::~FGODGauge()
{

}

/*
 * Used to remember the located groups that require modification
 */
typedef struct {
    osg::ref_ptr<osg::Group> parent;
    osg::ref_ptr<osg::Geode> node;
    unsigned int unit;
} GroupListItem;

/**
 * Replace a texture in the airplane model with the gauge texture.
 */
class ReplaceStaticTextureVisitor:

  public osg::NodeVisitor
{
  public:

    typedef osg::ref_ptr<osg::Group> GroupPtr;
    typedef osg::ref_ptr<osg::Material> MaterialPtr;

    ReplaceStaticTextureVisitor( const char* name,
                                 osg::Texture2D* new_texture ):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _tex_name( osgDB::getSimpleFileName(name) ),
        _new_texture(new_texture),
        _cull_callback(0)
    {}

    ReplaceStaticTextureVisitor( SGPropertyNode* placement,
                                 osg::Texture2D* new_texture,
                                 osg::NodeCallback* cull_callback = 0,
                                 const simgear::canvas::CanvasWeakPtr& canvas =
                                   simgear::canvas::CanvasWeakPtr() ):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _tex_name( osgDB::getSimpleFileName(
          placement->getStringValue("texture"))
        ),
        _node_name( placement->getStringValue("node") ),
        _parent_name( placement->getStringValue("parent") ),
        _node(placement),
        _new_texture(new_texture),
        _cull_callback(cull_callback),
        _canvas(canvas)
    {
      if(    _tex_name.empty()
          && _node_name.empty()
          && _parent_name.empty() )
        SG_LOG
        (
          SG_GL,
          SG_WARN,
          "No filter criterion for replacing texture. "
          " Every texture will be replaced!"
        );
    }

    /**
     * Get a list of groups which have been inserted into the scene graph to
     * replace the given texture
     */
    simgear::canvas::Placements& getPlacements()
    {
      return _placements;
    }

    virtual void apply(osg::Geode& node)
    {
      simgear::EffectGeode* effectGeode = dynamic_cast<simgear::EffectGeode*>(&node);
      if( !effectGeode )
        return;
      simgear::Effect* eff = effectGeode->getEffect();
      if (!eff)
          return;
      osg::StateSet* ss = eff->getDefaultStateSet();
      if( !ss )
        return;

      osg::Group *parent = node.getParent(0);
      if( !_node_name.empty() && getNodeName(*parent) != _node_name )
        return;

      if( !_parent_name.empty() )
      {
        // Traverse nodes upwards starting at the parent node (skip current
        // node)
        const osg::NodePath& np = getNodePath();
        bool found = false;
        for( int i = static_cast<int>(np.size()) - 2; i >= 0; --i )
        {
          const osg::Node* path_segment = np[i];
          const osg::Node* path_parent = path_segment->getParent(0);

          // A node without a name is always the parent of the root node of
          // the model just containing the file name
          if( path_parent && path_parent->getName().empty() )
            return;

          if( path_segment->getName() == _parent_name )
          {
            found = true;
            break;
          }
        }

        if( !found )
          return;
      }

      for( unsigned int unit = 0; unit < ss->getNumTextureAttributeLists(); ++unit )
      {
        osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>
        (
          ss->getTextureAttribute(unit, osg::StateAttribute::TEXTURE)
        );

        if( !tex || !tex->getImage() || tex == _new_texture )
          continue;

        if( !_tex_name.empty() )
        {
          std::string tex_name = tex->getImage()->getFileName();
          std::string tex_name_simple = osgDB::getSimpleFileName(tex_name);
          if( !osgDB::equalCaseInsensitive(_tex_name, tex_name_simple) )
            continue;
        }
        /*
         * remember this group for modification once the scenegraph has been traversed
         */
        groups_to_modify.push_back({ parent, &node, unit });
        return;
      }
    }
    /*
     * this section of code used to be in the apply method above, however to work this requires modification of the scenegraph nodes
     * that are currently iterating, so instead the apply method will locate the groups to be modified and when finished then the 
     * nodes can actually be modified safely. Initially found thanks to the debug RTL in MSVC2015 throwing an exception.
     * should be called immediately after the visitor to ensure that the groups are still valid and that nothing else has modified these groups.
     */
    void modify_groups()
	{
        for (auto g : groups_to_modify) {
            // insert a new group between the geode an it's parent which overrides
			// the texture
			GroupPtr group = new osg::Group;
			group->setName("canvas texture group");
			group->addChild(g.node);
			g.parent->removeChild(g.node);
			g.parent->addChild(group);

			if (_cull_callback)
				group->setCullCallback(_cull_callback);

			osg::StateSet* stateSet = group->getOrCreateStateSet();
			stateSet->setTextureAttribute(g.unit, _new_texture,
				osg::StateAttribute::OVERRIDE);
			stateSet->setTextureMode(g.unit, GL_TEXTURE_2D,
				osg::StateAttribute::ON);

			_placements.push_back(simgear::canvas::PlacementPtr(
				new simgear::canvas::ObjectPlacement(_node, group, _canvas)
			));

			SG_LOG
			(
				SG_GL,
				SG_INFO,
				"Replaced texture '" << _tex_name << "'"
				<< " for object '" << g.parent->getName() << "'"
				<< (!_parent_name.empty() ? " with parent '" + _parent_name + "'"
					: "")
			);
		}
		groups_to_modify.clear();
	}

  protected:

    std::string _tex_name,      ///<! Name of texture to be replaced
                _node_name,     ///<! Only replace if node name matches
                _parent_name;   ///<! Only replace if any parent node matches
                                ///   given name (all the tree upwards)

    SGPropertyNode_ptr  _node;
    osg::Texture2D     *_new_texture;
    osg::NodeCallback  *_cull_callback;
	typedef std::vector<GroupListItem> GroupList;
	GroupList groups_to_modify;
    simgear::canvas::CanvasWeakPtr  _canvas;
    simgear::canvas::Placements     _placements;

    const std::string& getNodeName(const osg::Node& node) const
    {
      if( !node.getName().empty() )
        return node.getName();

      // Special handling for pick animation which clears the name of the object
      // and instead sets the name of a parent group with one or two groups
      // attached (one for normal rendering and one for the picking highlight).
      osg::Group const* parent = node.getParent(0);
      if( parent->getName() == "pick render group" )
        return parent->getParent(0)->getName();

      return node.getName();
    }
};

//------------------------------------------------------------------------------
simgear::canvas::Placements
FGODGauge::set_texture( osg::Node* branch,
                        const char * name,
                        osg::Texture2D* new_texture )
{
  ReplaceStaticTextureVisitor visitor(name, new_texture);
  branch->accept(visitor);
  visitor.modify_groups();
  return visitor.getPlacements();
}

//------------------------------------------------------------------------------
simgear::canvas::Placements
FGODGauge::set_aircraft_texture( const char* name,
                                 osg::Texture2D* new_texture )
{
  return set_texture
  (
    globals->get_scenery()->get_aircraft_branch(),
    name,
    new_texture
  );
}

//------------------------------------------------------------------------------
simgear::canvas::Placements
FGODGauge::set_texture( osg::Node* branch,
                        SGPropertyNode* placement,
                        osg::Texture2D* new_texture,
                        osg::NodeCallback* cull_callback,
                        const simgear::canvas::CanvasWeakPtr& canvas )
{
  ReplaceStaticTextureVisitor visitor( placement,
                                       new_texture,
                                       cull_callback,
                                       canvas );
  branch->accept(visitor);
  visitor.modify_groups();
  return visitor.getPlacements();
}

//------------------------------------------------------------------------------
simgear::canvas::Placements
FGODGauge::set_aircraft_texture( SGPropertyNode* placement,
                                 osg::Texture2D* new_texture,
                                 osg::NodeCallback* cull_callback,
                                 const simgear::canvas::CanvasWeakPtr& canvas )
{
  return set_texture
  (
    globals->get_scenery()->get_aircraft_branch(),
    placement,
    new_texture,
    cull_callback,
    canvas
  );
}
