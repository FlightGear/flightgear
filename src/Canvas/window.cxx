// Window for placing a Canvas onto it (for dialogs, menus, etc.)
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#include "window.hxx"
#include <Canvas/canvas.hxx>

#include <osg/BlendFunc>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osgGA/GUIEventHandler>

/**
 * Callback to enable/disable rendering of canvas displayed inside windows
 */
class CullCallback:
  public osg::Drawable::CullCallback
{
  public:
    CullCallback(Canvas::CameraCullCallback* camera_cull);

  private:
    Canvas::CameraCullCallback *_camera_cull;

    virtual bool cull( osg::NodeVisitor* nv,
                       osg::Drawable* drawable,
                       osg::RenderInfo* renderInfo ) const;
};

//------------------------------------------------------------------------------
CullCallback::CullCallback(Canvas::CameraCullCallback* camera_cull):
  _camera_cull( camera_cull )
{

}

//------------------------------------------------------------------------------
bool CullCallback::cull( osg::NodeVisitor* nv,
                         osg::Drawable* drawable,
                         osg::RenderInfo* renderInfo ) const
{
  _camera_cull->enableRendering();
  return false;
}

namespace canvas
{
  //----------------------------------------------------------------------------
  Window::Window(SGPropertyNode* node):
    PropertyBasedElement(node),
    _dirty(true),
    _geometry( new osg::Geometry ),
    _vertices( new osg::Vec3Array(4) ),
    _tex_coords( new osg::Vec2Array(4) ),
    _x(node, "x"),
    _y(node, "y"),
    _width(node, "size[0]"),
    _height(node, "size[1]")
  {
    _x = 50;
    _y = 100;
    _width = 400;
    _height = 300;

    _geometry->setVertexArray(_vertices);
    _geometry->setTexCoordArray(0,_tex_coords);

    osg::Vec4Array* colors = new osg::Vec4Array(1);
    (*colors)[0].set(1.0f,1.0f,1.0,1.0f);
    _geometry->setColorArray(colors);
    _geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    _geometry->addPrimitiveSet(
      new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4)
    );
    _geometry->setDataVariance(osg::Object::DYNAMIC);

    osg::StateSet* stateSet = _geometry->getOrCreateStateSet();
    stateSet->setRenderBinDetails(1000, "RenderBin");

    // speed optimization?
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setAttribute(new osg::BlendFunc(
      osg::BlendFunc::SRC_ALPHA,
      osg::BlendFunc::ONE_MINUS_SRC_ALPHA)
    );
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
  }

  //----------------------------------------------------------------------------
  Window::~Window()
  {

  }

  //----------------------------------------------------------------------------
  void Window::update(double delta_time_sec)
  {
    if( !_dirty )
      return;
    _dirty = false;

    _region.set(_x, _y, _width, _height);

    int z = 0; // TODO do we need to use z for depth ordering?

    (*_vertices)[0].set(_region.l(), _region.t(), z);
    (*_vertices)[1].set(_region.r(), _region.t(), z);
    (*_vertices)[2].set(_region.r(), _region.b(), z);
    (*_vertices)[3].set(_region.l(), _region.b(), z);

    float l = 0, t = 1, b = 0, r = 1;
    (*_tex_coords)[0].set(l,t);
    (*_tex_coords)[1].set(r,t);
    (*_tex_coords)[2].set(r,b);
    (*_tex_coords)[3].set(l,b);

    _geometry->dirtyDisplayList();
  }

  //----------------------------------------------------------------------------
  void Window::valueChanged (SGPropertyNode * node)
  {
    if( node->getParent() != _node )
      return;

    const std::string& name = node->getNameString();
    if(    name == "x" || name == "y" || name == "size" )
      _dirty = true;
  }

  //----------------------------------------------------------------------------
  void Window::setCanvas(CanvasPtr canvas)
  {
    _canvas = canvas;
    _geometry->getOrCreateStateSet()
             ->setTextureAttribute(0, canvas ? canvas->getTexture() : 0);
    _geometry->dirtyDisplayList();
    _geometry->setCullCallback(
      canvas ? new CullCallback(canvas->getCameraCullCallback()) : 0
    );
  }

  //----------------------------------------------------------------------------
  CanvasWeakPtr Window::getCanvas() const
  {
    return _canvas;
  }

  //----------------------------------------------------------------------------
  bool Window::handleMouseEvent(const MouseEvent& event)
  {
    if( !_canvas.expired() )
      return _canvas.lock()->handleMouseEvent(event);
    else
      return false;
  }

} // namespace canvas
