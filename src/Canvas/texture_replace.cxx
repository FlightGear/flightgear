/*
 * SPDX-FileName: texture_replace.cxx
 * SPDX-FileCopyrightText: Copyright (C) 2012  Thomas Geymayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <osg/Texture2D>
#include <osg/Geode>
#include <osg/NodeVisitor>
#include <osg/StateSet>

#include <simgear/canvas/CanvasObjectPlacement.hxx>
#include <simgear/scene/material/EffectGeode.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>

#include "texture_replace.hxx"

namespace canvas {

/*
 * Used to remember the located groups that require modification
 */
typedef struct {
    osg::ref_ptr<osg::Group> parent;
    osg::ref_ptr<osg::Geode> node;
    unsigned int unit;
} GroupListItem;

/**
 * Replace a texture in the airplane model with another.
 */
class ReplaceStaticTextureVisitor : public osg::NodeVisitor
{
  public:

    typedef osg::ref_ptr<osg::Group> GroupPtr;
    typedef osg::ref_ptr<osg::Material> MaterialPtr;

    ReplaceStaticTextureVisitor( const char* name,
                                 osg::Texture2D* new_texture ):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _tex_name(name),
        _new_texture(new_texture),
        _cull_callback(0)
    {}

    ReplaceStaticTextureVisitor( SGPropertyNode* placement,
                                 osg::Texture2D* new_texture,
                                 osg::NodeCallback* cull_callback = 0,
                                 const simgear::canvas::CanvasWeakPtr& canvas =
                                   simgear::canvas::CanvasWeakPtr() ):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _tex_name( placement->getStringValue("texture") ),
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
          SG_DEV_ALERT,
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

      // Assume that the parent node to the EffectGeode contains the object
      // name. This is true for AC3D and glTF models.
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

      // NOTE: The texture units that correspond to each texture type (e.g.
      // 0 for base color, 1 for normal map, etc.) must match the ones in:
      //  1. PBR Effect: $FG_ROOT/Effects/model-pbr.eff
      //  2. glTF loader: simgear/scene/model/ReaderWriterGLTF.cxx
      //  3. PBR animations: simgear/scene/model/SGPBRAnimation.cxx
      //  4. Canvas: flightgear/src/Canvas/texture_replace.cxx
      unsigned int unit = 0;
      if (_tex_name.empty() || _tex_name == "base-color") unit = 0;
      else if (_tex_name == "normalmap")                  unit = 1;
      else if (_tex_name == "orm")                        unit = 2;
      else if (_tex_name == "emissive")                   unit = 3;
      else {
        SG_LOG(SG_GL, SG_DEV_ALERT, "Unknown texture '" << _tex_name
               << "'. Using base-color by default");
      }

      groups_to_modify.push_back({parent, &node, unit});
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

        group->getOrCreateStateSet()->setTextureAttributeAndModes(
          g.unit,
          _new_texture,
          osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

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

    std::string _tex_name,      ///<! PBR texture name to be replaced
                                ///   (base-color, normalmap, orm, etc.).
                                ///   This is not the actual texture filename
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
set_texture( osg::Node* branch,
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
set_aircraft_texture( const char* name,
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
set_texture( osg::Node* branch,
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
set_aircraft_texture( SGPropertyNode* placement,
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

} // namespace canvas
