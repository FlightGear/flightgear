// Canvas with 2D rendering api
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

#include "canvas_mgr.hxx"

#include <Canvas/FGCanvasSystemAdapter.hxx>
#include <Cockpit/od_gauge.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/CameraGroup.hxx>

#include <simgear/canvas/Canvas.hxx>

using simgear::canvas::Canvas;

//------------------------------------------------------------------------------
CanvasMgr::CanvasMgr():
  simgear::canvas::CanvasMgr
  (
   fgGetNode("/canvas/by-index", true),
   simgear::canvas::SystemAdapterPtr( new canvas::FGCanvasSystemAdapter )
  ),
  _cb_model_reinit
  (
    this,
    &CanvasMgr::handleModelReinit,
    fgGetNode("/sim/signals/model-reinit", true)
  )
{
  Canvas::addPlacementFactory
  (
    "object",
    boost::bind
    (
      &FGODGauge::set_texture,
      _1,
      boost::bind(&Canvas::getTexture, _2),
      boost::bind(&Canvas::getCullCallback, _2)
    )
  );
}

//------------------------------------------------------------------------------
unsigned int
CanvasMgr::getCanvasTexId(const simgear::canvas::CanvasPtr& canvas) const
{
  osg::Texture2D* tex = canvas->getTexture();
  if( !tex )
    return 0;

//  osgViewer::Viewer::Contexts contexts;
//  globals->get_renderer()->getViewer()->getContexts(contexts);
//
//  if( contexts.empty() )
//    return 0;

  static osg::Camera* guiCamera =
    flightgear::getGUICamera(flightgear::CameraGroup::getDefault());

  osg::State* state = guiCamera->getGraphicsContext()->getState(); //contexts[0]->getState();
  if( !state )
    return 0;

  osg::Texture::TextureObject* tobj =
    tex->getTextureObject( state->getContextID() );
  if( !tobj )
    return 0;

  return tobj->_id;
}

//----------------------------------------------------------------------------
void CanvasMgr::handleModelReinit(SGPropertyNode*)
{
  for(size_t i = 0; i < _elements.size(); ++i)
    boost::static_pointer_cast<Canvas>(_elements[i])
      ->reloadPlacements("object");
}
