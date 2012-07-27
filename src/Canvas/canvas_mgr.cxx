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
#include "canvas.hxx"

#include <boost/bind.hpp>

typedef boost::shared_ptr<Canvas> CanvasPtr;
CanvasPtr canvasFactory(SGPropertyNode* node)
{
  return CanvasPtr(new Canvas(node));
}


//------------------------------------------------------------------------------
CanvasMgr::CanvasMgr():
  PropertyBasedMgr("/canvas", "texture", &canvasFactory)
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
unsigned int CanvasMgr::getCanvasTexId(size_t index) const
{
  if(    index >= _elements.size()
      || !_elements[index] )
    return 0;

  return static_cast<Canvas*>(_elements[index].get())->getTexId();
}
