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

#include "config.h"

#include <algorithm> // for std::clamp

#include "viewmgr.hxx"
#include "ViewPropertyEvaluator.hxx"

#include <simgear/compiler.h>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/fg_props.hxx>
#include "view.hxx"

#include "CameraGroup.hxx"

namespace {

template <typename T>
T not_std_clamp(const T& v, const T& lower, const T& upper)
{
    return (v < lower) ? lower : ((v > upper) ? upper : v);
}

} // namespace

// Constructor
FGViewMgr::FGViewMgr( void ) :
  config_list(fgGetNode("/sim", true)->getChildren("view"))
{
    _current = fgGetInt("/sim/current-view/view-number");
}

// Destructor
FGViewMgr::~FGViewMgr( void )
{
}

void
FGViewMgr::init ()
{
    if (_inited) {
        SG_LOG(SG_VIEW, SG_WARN, "duplicate init of view manager");
        return;
    }

    _inited = true;

    // Ensure that /sim/chase-distance-m is negative if it is specified.
    // E.g. see https://sourceforge.net/p/flightgear/codetickets/2454/
    //
    SGPropertyNode* chase_distance_node = fgGetNode("/sim/chase-distance-m");
    if (chase_distance_node) {
        double chase_distance = chase_distance_node->getDoubleValue();
        if (chase_distance > 0) {
            chase_distance = -chase_distance;
            SG_LOG(SG_VIEW, SG_ALERT, "sim/chase-distance-m is positive; correcting to " << chase_distance);
            chase_distance_node->setDoubleValue(chase_distance);
        }
    }

    config_list = fgGetNode("/sim", true)->getChildren("view");
    _current = fgGetInt("/sim/current-view/view-number");
    if (_current != 0 && (_current < 0 || _current >= (int) views.size())) {
        SG_LOG(SG_VIEW, SG_ALERT,
                "Invalid /sim/current-view/view-number=" << _current
                << ". views.size()=" << views.size()
                << ". Will assert false and use zero."
                );
        assert(0);
        _current = 0;
    }

    for (unsigned int i = 0; i < config_list.size(); i++) {
        SGPropertyNode* n = config_list[i];
        SGPropertyNode* config = n->getChild("config", 0, true);

        flightgear::View* v = flightgear::View::createFromProperties(config, n->getIndex());
        if (v) {
            add_view(v);
        } else {
            SG_LOG(SG_VIEW, SG_DEV_WARN, "Failed to create view from:" << config->getPath());
        }
  }

  if (get_current_view()) {
      get_current_view()->bind();
  } else {
      SG_LOG(SG_VIEW, SG_DEV_WARN, "FGViewMgr::init: current view " << _current << " failed to create");
  }
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
    if (!_inited) {
        return;
    }

    _inited = false;
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
                        &FGViewMgr::getCurrentViewIndex,
                        &FGViewMgr::setCurrentViewIndex, false);
    _viewNumberProp = _tiedProperties.getRoot()->getNode("view-number");
    _viewNumberProp->setAttribute(SGPropertyNode::ARCHIVE, false);
    _viewNumberProp->setAttribute(SGPropertyNode::PRESERVE, true);
    _viewNumberProp->setAttribute(SGPropertyNode::LISTENER_SAFE, true);
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
    
    ViewPropertyEvaluator::clear();
}

void
FGViewMgr::update (double dt)
{
    flightgear::View* currentView = get_current_view();
    if (!currentView) {
        return;
    }

  // Update the current view
  currentView->update(dt);

// update the camera now
    osg::ref_ptr<flightgear::CameraGroup> cameraGroup = flightgear::CameraGroup::getDefault();
    if (!cameraGroup) {
        // attempting to diagnose the cause of FLIGHTGEAR-H9F
        throw sg_exception("FGViewMgr::update: no camera group exists");
    }

    cameraGroup->setCameraParameters(currentView->get_v_fov(),
                                        cameraGroup->getMasterAspectRatio());
    cameraGroup->update(toOsg(currentView->getViewPosition()),
                        toOsg(currentView->getViewOrientation()));
}

void FGViewMgr::clear()
{
    views.clear();
}

flightgear::View*
FGViewMgr::get_current_view()
{
    if (views.empty())
        return nullptr;
    assert(_current >= 0 && _current < (int) views.size());
    return views[_current];
}

const flightgear::View*
FGViewMgr::get_current_view() const
{
    return const_cast<FGViewMgr*>(this)->get_current_view();
}


flightgear::View*
FGViewMgr::get_view( int i )
{
    const auto lastView = static_cast<int>(views.size()) - 1;
    const int c = not_std_clamp(i, 0, lastView);
    return views.at(c);
}

const flightgear::View*
FGViewMgr::get_view( int i ) const
{
    const auto lastView = static_cast<int>(views.size()) - 1;
    const int c = not_std_clamp(i, 0, lastView);
    return views.at(c);
}

flightgear::View*
FGViewMgr::next_view()
{
    const auto numViews = static_cast<int>(views.size());
    setCurrentViewIndex((_current + 1) % numViews);
    _viewNumberProp->fireValueChanged();
    return get_current_view();
}

flightgear::View*
FGViewMgr::prev_view()
{
    const auto numViews = static_cast<int>(views.size());
    // subtract 1, but add a full numViews, to ensure the integer
    // modulo returns a +ve result; negative values mean something
    // else to setCurrentViewIndex
    setCurrentViewIndex((_current - 1 + numViews) % numViews);
    _viewNumberProp->fireValueChanged();
    return get_current_view();
}

void
FGViewMgr::add_view( flightgear::View * v )
{
  views.push_back(v);
  v->init();
}

int FGViewMgr::getCurrentViewIndex() const
{
    return _current;
}

void FGViewMgr::setCurrentViewIndex(int newview)
{
    if (newview == _current) {
        return;
    }
    // negative numbers -> set view with node index -newview
    if (newview < 0) {
        for (int i = 0; i < (int)config_list.size(); i++) {
            int index = -config_list[i]->getIndex();
            if (index == newview)
                newview = i;
        }
        if (newview < 0) {
            SG_LOG(SG_VIEW, SG_ALERT,
                    "Failed to find -ve newview=" << newview
                    << ". Will assert false and ignore."
                    );
            assert(0);
            return;
        }
    }
    if (newview < 0 || newview >= (int) views.size()) {
        SG_LOG(SG_VIEW, SG_ALERT, "Invalid newview=" << newview
                << ". views.size()=" << views.size()
                << ". Will assert false and ignore."
                );
        assert(0);
        return;
    }

    if (get_current_view()) {
	    get_current_view()->unbind();
    }

    _current = newview;

    if (get_current_view()) {
        get_current_view()->bind();
    }
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGViewMgr> registrantFGViewMgr(
    SGSubsystemMgr::DISPLAY);
