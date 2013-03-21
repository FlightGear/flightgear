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
#include <osg/Material>
#include <osg/Matrix>
#include <osg/PolygonMode>
#include <osg/ShadeModel>
#include <osg/StateSet>
#include <osg/FrameBufferObject> // for GL_DEPTH_STENCIL_EXT on Windows

#include <osgDB/FileNameUtils>

#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/RenderConstants.hxx>

#include <Canvas/FGCanvasSystemAdapter.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include "od_gauge.hxx"

#include <cassert>

static simgear::canvas::SystemAdapterPtr system_adapter(
  new canvas::FGCanvasSystemAdapter
);

//------------------------------------------------------------------------------
FGODGauge::FGODGauge()
{
  setSystemAdapter(system_adapter);
}

//------------------------------------------------------------------------------
FGODGauge::~FGODGauge()
{

}

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
        _new_texture(new_texture)
    {}

    ReplaceStaticTextureVisitor( SGPropertyNode* placement,
                                 osg::Texture2D* new_texture,
                                 osg::NodeCallback* cull_callback = 0 ):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _tex_name( osgDB::getSimpleFileName(
          placement->getStringValue("texture"))
        ),
        _node_name( placement->getStringValue("node") ),
        _parent_name( placement->getStringValue("parent") ),
        _node(placement),
        _new_texture(new_texture),
        _cull_callback(cull_callback)
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
      simgear::EffectGeode* eg = dynamic_cast<simgear::EffectGeode*>(&node);
      if( !eg )
        return;

      osg::StateSet* ss = eg->getEffect()->getDefaultStateSet();
      if( !ss )
        return;

      osg::Group *parent = node.getParent(0);
      if( !_node_name.empty() && parent->getName() != _node_name )
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

      for( size_t unit = 0; unit < ss->getNumTextureAttributeLists(); ++unit )
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

        // insert a new group between the geode an it's parent which overrides
        // the texture
        GroupPtr group = new osg::Group;
        group->setName("canvas texture group");
        group->addChild(eg);
        parent->removeChild(eg);
        parent->addChild(group);

        if( _cull_callback )
          group->setCullCallback(_cull_callback);

        osg::StateSet* stateSet = group->getOrCreateStateSet();
        stateSet->setTextureAttribute( unit, _new_texture,
                                             osg::StateAttribute::OVERRIDE );
        stateSet->setTextureMode( unit, GL_TEXTURE_2D,
                                        osg::StateAttribute::ON );

        _placements.push_back( simgear::canvas::PlacementPtr(
          new ObjectPlacement(_node, group)
        ));

        SG_LOG
        (
          SG_GL,
          SG_INFO,
             "Replaced texture '" << _tex_name << "'"
          << " for object '" << parent->getName() << "'"
          << (!_parent_name.empty() ? " with parent '" + _parent_name + "'"
                                    : "")
        );
        return;
      }
    }

  protected:

    class ObjectPlacement:
      public simgear::canvas::Placement
    {
      public:

        ObjectPlacement( SGPropertyNode* node,
                         GroupPtr group ):
          Placement(node),
          _group(group)
        {
          // TODO make more generic and extendable for more properties
          if( node->hasValue("emission") )
            setEmission( node->getFloatValue("emission") );
        }

        virtual bool childChanged(SGPropertyNode* node)
        {
          if( node->getParent() != _node )
            return false;

          if( node->getNameString() == "emission" )
            setEmission( node->getFloatValue() );
          else
            return false;

          return true;
        }

        void setEmission(float emit)
        {
          emit = SGMiscf::clip(emit, 0, 1);

          if( !_material )
          {
            _material = new osg::Material;
            _material->setColorMode(osg::Material::OFF);
            _material->setDataVariance(osg::Object::DYNAMIC);
            _group->getOrCreateStateSet()
                  ->setAttribute(_material, ( osg::StateAttribute::ON
                                            | osg::StateAttribute::OVERRIDE ) );
          }

          _material->setEmission(
            osg::Material::FRONT_AND_BACK,
            osg::Vec4(emit, emit, emit, emit)
          );
        }

        /**
         * Remove placement from the scene
         */
        virtual ~ObjectPlacement()
        {
          assert( _group->getNumChildren() == 1 );
          osg::Node *child = _group->getChild(0);

          if( _group->getNumParents() )
          {
            osg::Group *parent = _group->getParent(0);
            parent->addChild(child);
            parent->removeChild(_group);
          }

          _group->removeChild(child);
        }

      private:
        GroupPtr            _group;
        MaterialPtr         _material;
    };

    std::string _tex_name,      ///<! Name of texture to be replaced
                _node_name,     ///<! Only replace if node name matches
                _parent_name;   ///<! Only replace if any parent node matches
                                ///   given name (all the tree upwards)

    SGPropertyNode_ptr  _node;
    osg::Texture2D     *_new_texture;
    osg::NodeCallback  *_cull_callback;

    simgear::canvas::Placements _placements;
};

//------------------------------------------------------------------------------
simgear::canvas::Placements
FGODGauge::set_texture( osg::Node* branch,
                        const char * name,
                        osg::Texture2D* new_texture )
{
  ReplaceStaticTextureVisitor visitor(name, new_texture);
  branch->accept(visitor);
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
                        osg::NodeCallback* cull_callback )
{
  ReplaceStaticTextureVisitor visitor(placement, new_texture, cull_callback);
  branch->accept(visitor);
  return visitor.getPlacements();
}

//------------------------------------------------------------------------------
simgear::canvas::Placements
FGODGauge::set_aircraft_texture( SGPropertyNode* placement,
                                 osg::Texture2D* new_texture,
                                 osg::NodeCallback* cull_callback )
{
  return set_texture
  (
    globals->get_scenery()->get_aircraft_branch(),
    placement,
    new_texture,
    cull_callback
  );
}
