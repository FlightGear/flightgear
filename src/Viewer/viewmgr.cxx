// viewmgr.cxx -- class for managing all the views in the flightgear world.
//
// Written by Curtis Olson, started October 2000.
//   partially rewritten by Jim Wilson March 2002
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// $Id$

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "viewmgr.hxx"

#include <simgear/compiler.h>
#include <simgear/scene/util/OsgMath.hxx>

#include <Main/fg_props.hxx>
#include "viewer.hxx"

#include "CameraGroup.hxx"

// Constructor
FGViewMgr::FGViewMgr( void ) :
  inited(false),
  config_list(fgGetNode("/sim", true)->getChildren("view")),
  current(0)
{
}

// Destructor
FGViewMgr::~FGViewMgr( void )
{
}

void
FGViewMgr::init ()
{
  if (inited) {
    SG_LOG(SG_VIEW, SG_WARN, "duplicate init of view manager");
    return;
  }
  
  inited = true;

  for (unsigned int i = 0; i < config_list.size(); i++) {
    SGPropertyNode *n = config_list[i];
    SGPropertyNode *config = n->getChild("config", 0, true);

      flightgear::View* v = flightgear::View::createFromProperties(config);
      if (v) {
          add_view(v);
      }
  }

    get_current_view()->bind();
}

void
FGViewMgr::postinit()
{
    // force update now so many properties of the current view are valid,
    // eg view position and orientation (as exposed via globals)
    update(0.0);
}

void
FGViewMgr::shutdown()
{
    if (!inited) {
        return;
    }
    
    inited = false;
    views.clear();
}

void
FGViewMgr::reinit ()
{
    viewer_list::iterator it;
    for (it = views.begin(); it != views.end(); ++it) {
        (*it)->resetOffsetsAndFOV();
    }
}

void
FGViewMgr::bind()
{
    // these are bound to the current view properties
    _tiedProperties.setRoot(fgGetNode("/sim/current-view", true));


    _tiedProperties.Tie("view-number", this,
                        &FGViewMgr::getView, &FGViewMgr::setView, false);
    _viewNumberProp = _tiedProperties.getRoot()->getNode("view-number");
    _viewNumberProp->setAttribute(SGPropertyNode::ARCHIVE, false);
    _viewNumberProp->setAttribute(SGPropertyNode::PRESERVE, true);

    current_x_offs = fgGetNode("/sim/current-view/x-offset-m", true);
    current_y_offs = fgGetNode("/sim/current-view/y-offset-m", true);
    current_z_offs = fgGetNode("/sim/current-view/z-offset-m", true);
    target_x_offs  = fgGetNode("/sim/current-view/target-x-offset-m", true);
    target_y_offs  = fgGetNode("/sim/current-view/target-y-offset-m", true);
    target_z_offs  = fgGetNode("/sim/current-view/target-z-offset-m", true);
}


void
FGViewMgr::unbind ()
{
    flightgear::View* v = get_current_view();
    if (v) {
        v->unbind();
    }

  _tiedProperties.Untie();
    _viewNumberProp.clear();
    
    config_list.clear();
    target_x_offs.clear();
    target_y_offs.clear();
    target_z_offs.clear();
    current_x_offs.clear();
    current_y_offs.clear();
    current_z_offs.clear();
}

void
FGViewMgr::update (double dt)
{
    flightgear::View* currentView = get_current_view();
    if (!currentView) {
        return;
    }

  // Set up view location and orientation
    currentView->updateData();

    // these properties aren't tied - manually propogate them to the
    // currently active view
    currentView->setXOffset_m(current_x_offs->getDoubleValue());
    currentView->setYOffset_m(current_y_offs->getDoubleValue());
    currentView->setZOffset_m(current_z_offs->getDoubleValue());

    currentView->setTargetXOffset_m(target_x_offs->getDoubleValue());
    currentView->setTargetYOffset_m(target_y_offs->getDoubleValue());
    currentView->setTargetZOffset_m(target_z_offs->getDoubleValue());

  // Update the current view
  currentView->update(dt);


// update the camera now
    osg::ref_ptr<flightgear::CameraGroup> cameraGroup = flightgear::CameraGroup::getDefault();
    cameraGroup->update(toOsg(currentView->getViewPosition()),
                        toOsg(currentView->getViewOrientation()));
    cameraGroup->setCameraParameters(currentView->get_v_fov(),
                                     cameraGroup->getMasterAspectRatio());
}

void FGViewMgr::clear()
{
    views.clear();
}

flightgear::View*
FGViewMgr::get_current_view()
{
    if ( current < (int)views.size() ) {
        return views[current];
    } else {
        return NULL;
    }
}

const flightgear::View*
FGViewMgr::get_current_view() const
{
    if ( current < (int)views.size() ) {
        return views[current];
    } else {
        return NULL;
    }
}


flightgear::View*
FGViewMgr::get_view( int i )
{
    if ( i < 0 ) { i = 0; }
    if ( i >= (int)views.size() ) { i = views.size() - 1; }
    return views[i];
}

const flightgear::View*
FGViewMgr::get_view( int i ) const
{
    if ( i < 0 ) { i = 0; }
    if ( i >= (int)views.size() ) { i = views.size() - 1; }
    return views[i];
}

flightgear::View*
FGViewMgr::next_view()
{
    setView((current+1 < (int)views.size()) ? (current + 1) : 0);
    _viewNumberProp->fireValueChanged();
    return views[current];
}

flightgear::View*
FGViewMgr::prev_view()
{
    setView((0 < current) ? (current - 1) : (views.size() - 1));
    _viewNumberProp->fireValueChanged();
    return views[current];
}

void
FGViewMgr::add_view( flightgear::View * v )
{
  views.push_back(v);
  v->init();
}

int FGViewMgr::getView () const
{
  return current;
}

void
FGViewMgr::setView (int newview)
{
    if (newview == current) {
        return;
    }

  // negative numbers -> set view with node index -newview
  if (newview < 0) {
    for (int i = 0; i < (int)config_list.size(); i++) {
      int index = -config_list[i]->getIndex();
      if (index == newview)
        newview = i;
    }
    if (newview < 0)
      return;
  }

  // if newview number too low wrap to last view...
  if (newview < 0)
    newview = (int)views.size() - 1;

  // if newview number to high wrap to zero...
  if (newview >= (int)views.size())
    newview = 0;

    if (get_current_view()) {
        get_current_view()->unbind();
    }

  // set new view
  current = newview;

    if (get_current_view()) {
        get_current_view()->bind();
    }

    // force an update now, to avoid returning bogus data.
    // real fix would to be make all the accessors use the dirty mechanism
    // on FGViewer, so update() is a no-op.
    update(0.0);
}
