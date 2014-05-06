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

#include <Cockpit/od_gauge.hxx>
#include <Main/fg_props.hxx>
#include <Scripting/NasalModelData.hxx>
#include <Viewer/CameraGroup.hxx>

#include <simgear/canvas/Canvas.hxx>

namespace sc = simgear::canvas;

//------------------------------------------------------------------------------
static sc::Placements addSceneObjectPlacement( SGPropertyNode* placement,
                                               sc::CanvasPtr canvas )
{
  int module_id = placement->getIntValue("module-id", -1);
  if( module_id < 0 )
    return sc::Placements();

  FGNasalModelData* model_data =
    FGNasalModelData::getByModuleId( static_cast<unsigned int>(module_id) );

  if( !model_data )
    return sc::Placements();

  if( !model_data->getNode() )
    return sc::Placements();

  return FGODGauge::set_texture
  (
    model_data->getNode(),
    placement,
    canvas->getTexture(),
    canvas->getCullCallback(),
    canvas
  );
}

//------------------------------------------------------------------------------
CanvasMgr::CanvasMgr():
  simgear::canvas::CanvasMgr( fgGetNode("/canvas/by-index", true) ),
  _cb_model_reinit
  (
    this,
    &CanvasMgr::handleModelReinit,
    fgGetNode("/sim/signals/model-reinit", true)
  )
{

}

//----------------------------------------------------------------------------
void CanvasMgr::init()
{
  _gui_camera = flightgear::getGUICamera(flightgear::CameraGroup::getDefault());
  assert(_gui_camera.valid());

  sc::Canvas::addPlacementFactory
  (
    "object",
    boost::bind
    (
      &FGODGauge::set_aircraft_texture,
      _1,
      boost::bind(&sc::Canvas::getTexture, _2),
      boost::bind(&sc::Canvas::getCullCallback, _2),
      _2
    )
  );
  sc::Canvas::addPlacementFactory("scenery-object", &addSceneObjectPlacement);

  simgear::canvas::CanvasMgr::init();
}

//----------------------------------------------------------------------------
void CanvasMgr::shutdown()
{
  simgear::canvas::CanvasMgr::shutdown();

  sc::Canvas::removePlacementFactory("object");
  sc::Canvas::removePlacementFactory("scenery-object");

  _gui_camera = 0;
}

//------------------------------------------------------------------------------
unsigned int
CanvasMgr::getCanvasTexId(const simgear::canvas::CanvasPtr& canvas) const
{
  if( !canvas )
    return 0;

  osg::Texture2D* tex = canvas->getTexture();
  if( !tex )
    return 0;

//  osgViewer::Viewer::Contexts contexts;
//  globals->get_renderer()->getViewer()->getContexts(contexts);
//
//  if( contexts.empty() )
//    return 0;

  osg::ref_ptr<osg::Camera> guiCamera;
  if( !_gui_camera.lock(guiCamera) )
    return 0;

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
    static_cast<sc::Canvas*>(_elements[i].get())->reloadPlacements("object");
}
