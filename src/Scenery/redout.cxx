// redout.hxx
//
// Written by Mathias Froehlich,
//
// Copyright (C) 2007  Mathias Froehlich
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "redout.hxx"

#include <assert.h>

#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Switch>
#include <osg/StateSet>

#include <simgear/props/props.hxx>
#include <Main/fg_props.hxx>

class FGRedoutCallback : public osg::NodeCallback {
public:
  FGRedoutCallback(osg::Vec4Array* colorArray) :
    _colorArray(colorArray),
    _redoutNode(fgGetNode("/sim/rendering/redout", true))
  {
    fgGetNode("/sim/rendering/redout/alpha", true);
  }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<osg::Switch*>(node));
    osg::Switch* sw = static_cast<osg::Switch*>(node);

    // Check if we need to do something further ...
    float alpha = _redoutNode->getFloatValue("alpha", 0);
    bool enabled = (0 < alpha);
    sw->setValue(0, enabled);
    if (!enabled)
      return;

    (*_colorArray)[0][0] = _redoutNode->getFloatValue("red", 1);
    (*_colorArray)[0][1] = _redoutNode->getFloatValue("green", 0);
    (*_colorArray)[0][2] = _redoutNode->getFloatValue("blue", 0);
    (*_colorArray)[0][3] = alpha;
    _colorArray->dirty();
  }
private:
  osg::ref_ptr<osg::Vec4Array> _colorArray;
  SGSharedPtr<SGPropertyNode> _redoutNode;
};

osg::Node* FGCreateRedoutNode()
{
  osg::Geometry* geometry = new osg::Geometry;
  geometry->setUseDisplayList(false);

  osg::StateSet* stateSet = geometry->getOrCreateStateSet();
  stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  stateSet->setAttribute(new osg::BlendFunc);
  stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
  stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
  stateSet->setAttribute(new osg::Depth(osg::Depth::ALWAYS, 0, 1, false));
  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  stateSet->setRenderBinDetails(1000, "RenderBin");

  osg::Vec3Array* vertexArray = new osg::Vec3Array;
  vertexArray->push_back(osg::Vec3(-1, -1, 0));
  vertexArray->push_back(osg::Vec3( 1, -1, 0));
  vertexArray->push_back(osg::Vec3( 1,  1, 0));
  vertexArray->push_back(osg::Vec3(-1,  1, 0));
  geometry->setVertexArray(vertexArray);
  osg::Vec4Array* colorArray = new osg::Vec4Array;
  colorArray->push_back(osg::Vec4(1, 0, 0, 1));
  geometry->setColorArray(colorArray);
  geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  geometry->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, 0, 4));

  osg::Geode* geode = new osg::Geode;
  geode->addDrawable(geometry);

  osg::Camera* camera = new osg::Camera;
  camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
  camera->setProjectionMatrix(osg::Matrix::ortho2D(-1, 1, -1, 1));
  camera->setViewMatrix(osg::Matrix::identity());
  camera->setRenderOrder(osg::Camera::NESTED_RENDER);
  camera->setClearMask(0);
  camera->setAllowEventFocus(false);
  camera->setCullingActive(false);
  camera->addChild(geode);

  osg::Switch* sw = new osg::Switch;
  sw->setUpdateCallback(new FGRedoutCallback(colorArray));
  sw->addChild(camera);

  return sw;
}

