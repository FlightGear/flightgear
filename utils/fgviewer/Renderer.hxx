// Viewer.hxx -- alternative flightgear viewer application
//
// Copyright (C) 2009 - 2012  Mathias Froehlich
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

#ifndef _FGVIEWER_RENDERER_HXX
#define _FGVIEWER_RENDERER_HXX

#include <string>
#include <simgear/structure/SGWeakReferenced.hxx>

namespace fgviewer  {

class Drawable;
class Viewer;
class SlaveCamera;

/// Default renderer, doing fixed function work
class Renderer : public SGWeakReferenced {
public:
    Renderer();
    virtual ~Renderer();

    virtual Drawable* createDrawable(Viewer& viewer, const std::string& name);
    virtual SlaveCamera* createSlaveCamera(Viewer& viewer, const std::string& name);

    virtual bool realize(Viewer& viewer);
    virtual bool update(Viewer& viewer);

private:
    Renderer(const Renderer&);
    Renderer& operator=(const Renderer&);

    class _SlaveCamera;
};

} // namespace fgviewer

#endif
