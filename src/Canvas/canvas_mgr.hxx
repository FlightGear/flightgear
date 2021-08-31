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

#include <simgear/canvas/CanvasMgr.hxx>
#include <simgear/props/PropertyBasedMgr.hxx>

class CanvasMgr : public simgear::canvas::CanvasMgr
{
public:
    CanvasMgr();

    // Subsystem API.
    void init() override;
    void shutdown() override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "Canvas"; }

    /**
     * Get OpenGL texture name for given canvas
     *
     * @deprecated This was only meant to be used by the PUI CanvasWidget
     *             implementation as PUI can't handle osg::Texture objects.
     *             Use getCanvas(index)->getTexture() instead.
     *
     * @return OpenGL texture name
     */
    unsigned int getCanvasTexId(const simgear::canvas::CanvasPtr& canvas) const;

protected:

    SGPropertyChangeCallback<CanvasMgr> _cb_model_reinit;

    void handleModelReinit(SGPropertyNode*);
};

#endif /* CANVAS_MGR_H_ */
