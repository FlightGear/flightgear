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

#ifndef CANVAS_MGR_H_
#define CANVAS_MGR_H_

#include "canvas_fwd.hpp"
#include "property_based_mgr.hxx"

class CanvasMgr:
  public PropertyBasedMgr
{
  public:
    CanvasMgr();

    /**
     * Get ::Canvas by index
     *
     * @param index Index of texture node in /canvas/by-index/
     */
    CanvasPtr getCanvas(size_t index) const;

    /**
     * Get OpenGL texture name for given canvas
     *
     * @deprecated This was only meant to be used by the PUI CanvasWidget
     *             implementation as PUI can't handle osg::Texture objects.
     *             Use getCanvas(index)->getTexture() instead.
     *
     * @param Index of canvas
     * @return OpenGL texture name
     */
    unsigned int getCanvasTexId(size_t index) const;
};

#endif /* CANVAS_MGR_H_ */
