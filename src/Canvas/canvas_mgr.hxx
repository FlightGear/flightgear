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

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <boost/shared_ptr.hpp>
#include <vector>

class Canvas;
typedef boost::shared_ptr<Canvas> CanvasPtr;

class CanvasMgr:
  public SGSubsystem,
  public SGPropertyChangeListener
{
  public:
    CanvasMgr();
    virtual ~CanvasMgr();

    virtual void init();
    virtual void reinit();
    virtual void shutdown();

    virtual void bind();
    virtual void unbind();

    virtual void update(double delta_time_sec);

    virtual void childAdded( SGPropertyNode * parent,
                             SGPropertyNode * child );
    virtual void childRemoved( SGPropertyNode * parent,
                               SGPropertyNode * child );

  private:

    /** Root node for everything concerning the canvas system */
    SGPropertyNode_ptr _props;

    /** The actual canvases */
    std::vector<CanvasPtr> _canvases;

    void textureAdded(SGPropertyNode* node);

    /**
     * Trigger a childAdded and valueChanged event for every child of node
     * (Unlimited depth) and node itself.
     */
    void triggerChangeRecursive(SGPropertyNode* node);

};

#endif /* CANVAS_MGR_H_ */
