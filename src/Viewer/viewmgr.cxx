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

#include <string.h>        // strcmp

#include <simgear/compiler.h>
#include <simgear/scene/util/OsgMath.hxx>

#include <Main/fg_props.hxx>
#include "viewer.hxx"

#include "CameraGroup.hxx"

// Constructor
FGViewMgr::FGViewMgr( void ) :
  axis_long(0),
  axis_lat(0),
  inited(false),
  view_number(fgGetNode("/sim/current-view/view-number", true)),
  config_list(fgGetNode("/sim", true)->getChildren("view")),
  abs_viewer_position(SGVec3d::zeros()),
  current(0),
  current_view_orientation(SGQuatd::zeros()),
  current_view_or_offset(SGQuatd::zeros())
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

  copyToCurrent();
  do_bind();

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

typedef double (FGViewMgr::*double_getter)() const;

void
FGViewMgr::bind()
{
  // view-manager code was designed to init before bind, so
  // this is a no-op; init() calls the real bind() impl below

    current_x_offs = fgGetNode("/sim/current-view/x-offset-m", true);
    current_y_offs = fgGetNode("/sim/current-view/y-offset-m", true);
    current_z_offs = fgGetNode("/sim/current-view/z-offset-m", true);
    target_x_offs  = fgGetNode("/sim/current-view/target-x-offset-m", true);
    target_y_offs  = fgGetNode("/sim/current-view/target-y-offset-m", true);
    target_z_offs  = fgGetNode("/sim/current-view/target-z-offset-m", true);
}

void
FGViewMgr::do_bind()
{  
  // these are bound to the current view properties
  _tiedProperties.setRoot(fgGetNode("/sim/current-view", true));

  _tiedProperties.Tie("view-number", this,
                      &FGViewMgr::getView, &FGViewMgr::setView);
  SGPropertyNode* view_number =
    _tiedProperties.getRoot()->getNode("view-number");
  view_number->setAttribute(SGPropertyNode::ARCHIVE, false);
  // Keep view on reset/reinit
  view_number->setAttribute(SGPropertyNode::PRESERVE, true);

  _tiedProperties.Tie("axes/long", this,
                      (double_getter)0, &FGViewMgr::setViewAxisLong);
  fgSetArchivable("/sim/current-view/axes/long");

  _tiedProperties.Tie("axes/lat", this,
                      (double_getter)0, &FGViewMgr::setViewAxisLat);
  fgSetArchivable("/sim/current-view/axes/lat");

  _tiedProperties.Tie("field-of-view", this,
                      &FGViewMgr::getFOV_deg, &FGViewMgr::setFOV_deg);
  fgSetArchivable("/sim/current-view/field-of-view");

  _tiedProperties.Tie("aspect-ratio-multiplier", this,
                      &FGViewMgr::getARM_deg, &FGViewMgr::setARM_deg);
  fgSetArchivable("/sim/current-view/field-of-view");

  _tiedProperties.Tie("ground-level-nearplane-m", this,
                      &FGViewMgr::getNear_m, &FGViewMgr::setNear_m);
  fgSetArchivable("/sim/current-view/ground-level-nearplane-m");

  SGPropertyNode *n = fgGetNode("/sim/current-view", true);
  _tiedProperties.Tie(n->getNode("viewer-x-m", true),SGRawValuePointer<double>(&abs_viewer_position[0]));
  _tiedProperties.Tie(n->getNode("viewer-y-m", true),SGRawValuePointer<double>(&abs_viewer_position[1]));
  _tiedProperties.Tie(n->getNode("viewer-z-m", true),SGRawValuePointer<double>(&abs_viewer_position[2]));

  _tiedProperties.Tie("viewer-lon-deg", this, &FGViewMgr::getViewLon_deg);
  _tiedProperties.Tie("viewer-lat-deg", this, &FGViewMgr::getViewLat_deg);
  _tiedProperties.Tie("viewer-elev-ft", this, &FGViewMgr::getViewElev_ft);
  
  
  _tiedProperties.Tie("debug/orientation-w", this,
                      &FGViewMgr::getCurrentViewOrientation_w);
  _tiedProperties.Tie("debug/orientation-x", this,
                      &FGViewMgr::getCurrentViewOrientation_x);
  _tiedProperties.Tie("debug/orientation-y", this,
                      &FGViewMgr::getCurrentViewOrientation_y);
  _tiedProperties.Tie("debug/orientation-z", this,
                      &FGViewMgr::getCurrentViewOrientation_z);

  _tiedProperties.Tie("debug/orientation_offset-w", this,
                      &FGViewMgr::getCurrentViewOrOffset_w);
  _tiedProperties.Tie("debug/orientation_offset-x", this,
                      &FGViewMgr::getCurrentViewOrOffset_x);
  _tiedProperties.Tie("debug/orientation_offset-y", this,
                      &FGViewMgr::getCurrentViewOrOffset_y);
  _tiedProperties.Tie("debug/orientation_offset-z", this,
                      &FGViewMgr::getCurrentViewOrOffset_z);

  _tiedProperties.Tie("debug/frame-w", this,
                      &FGViewMgr::getCurrentViewFrame_w);
  _tiedProperties.Tie("debug/frame-x", this,
                      &FGViewMgr::getCurrentViewFrame_x);
  _tiedProperties.Tie("debug/frame-y", this,
                      &FGViewMgr::getCurrentViewFrame_y);
  _tiedProperties.Tie("debug/frame-z", this,
                      &FGViewMgr::getCurrentViewFrame_z);
}

void
FGViewMgr::unbind ()
{
    flightgear::View* v = get_current_view();
    if (v) {
        v->unbind();
    }

  _tiedProperties.Untie();
    config_list.clear();
    view_number.clear();
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
  if (!currentView) return;

  // Set up view location and orientation

    currentView->updateData();

    // these properties aren't tied - manually propogate them to the
    // currently active view
  setViewXOffset_m(current_x_offs->getDoubleValue());
  setViewYOffset_m(current_y_offs->getDoubleValue());
  setViewZOffset_m(current_z_offs->getDoubleValue());

  setViewTargetXOffset_m(target_x_offs->getDoubleValue());
  setViewTargetYOffset_m(target_y_offs->getDoubleValue());
  setViewTargetZOffset_m(target_z_offs->getDoubleValue());

  current_view_orientation = currentView->getViewOrientation();
  current_view_or_offset = currentView->getViewOrientationOffset();

  // Update the current view
  do_axes();
  currentView->update(dt);
  abs_viewer_position = currentView->getViewPosition();


// update the camera now
    osg::ref_ptr<flightgear::CameraGroup> cameraGroup = flightgear::CameraGroup::getDefault();
    cameraGroup->update(toOsg(abs_viewer_position),
                        toOsg(currentView->getViewOrientation()));
    cameraGroup->setCameraParameters(currentView->get_v_fov(),
                                     cameraGroup->getMasterAspectRatio());

  // expose the raw (OpenGL) orientation to the property tree,
  // for the sound-manager
  for (int i=0; i<4; ++i) {
    _tiedProperties.getRoot()->getChild("raw-orientation", i, true)->setDoubleValue(current_view_orientation[i]);
  }
}

void
FGViewMgr::copyToCurrent()
{

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
    view_number->fireValueChanged();
    return views[current];
}

flightgear::View*
FGViewMgr::prev_view()
{
    setView((0 < current) ? (current - 1) : (views.size() - 1));
    view_number->fireValueChanged();
    return views[current];
}

void
FGViewMgr::add_view( flightgear::View * v )
{
  views.push_back(v);
  v->init();
}

double
FGViewMgr::getViewXOffset_m () const
{
  const flightgear::View * view = get_current_view();
  if (view != 0) {
    return ((flightgear::View *)view)->getXOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewXOffset_m (double x)
{
  flightgear::View * view = get_current_view();
  if (view != 0) {
    view->setXOffset_m(x);
  }
}

double
FGViewMgr::getViewYOffset_m () const
{
  const flightgear::View * view = get_current_view();
  if (view != 0) {
    return ((flightgear::View *)view)->getYOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewYOffset_m (double y)
{
  flightgear::View * view = get_current_view();
  if (view != 0) {
    view->setYOffset_m(y);
  }
}

double
FGViewMgr::getViewZOffset_m () const
{
  const flightgear::View * view = get_current_view();
  if (view != 0) {
    return ((flightgear::View *)view)->getZOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewZOffset_m (double z)
{
  flightgear::View * view = get_current_view();
  if (view != 0) {
    view->setZOffset_m(z);
  }
}

double
FGViewMgr::getViewTargetXOffset_m () const
{
  const flightgear::View * view = get_current_view();
  if (view != 0) {
    return ((flightgear::View *)view)->getTargetXOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewTargetXOffset_m (double x)
{
  flightgear::View * view = get_current_view();
  if (view != 0) {
    view->setTargetXOffset_m(x);
  }
}

double
FGViewMgr::getViewTargetYOffset_m () const
{
  const flightgear::View * view = get_current_view();
  if (view != 0) {
    return ((flightgear::View *)view)->getTargetYOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewTargetYOffset_m (double y)
{
  flightgear::View * view = get_current_view();
  if (view != 0) {
    view->setTargetYOffset_m(y);
  }
}

double
FGViewMgr::getViewTargetZOffset_m () const
{
  const flightgear::View * view = get_current_view();
  if (view != 0) {
    return ((flightgear::View *)view)->getTargetZOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewTargetZOffset_m (double z)
{
  flightgear::View * view = get_current_view();
  if (view != 0) {
    view->setTargetZOffset_m(z);
  }
}

int
FGViewMgr::getView () const
{
  return ( current );
}

void
FGViewMgr::setView (int newview)
{
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

  // copy in view data
  copyToCurrent();



    // force an update now, to avoid returning bogus data.
    // real fix would to be make all the accessors use the dirty mechanism
    // on FGViewer, so update() is a no-op.
    update(0.0);
}


double
FGViewMgr::getFOV_deg () const
{
  const flightgear::View * view = get_current_view();
  return (view == 0 ? 0 : view->get_fov());
}

void
FGViewMgr::setFOV_deg (double fov)
{
  flightgear::View * view = get_current_view();
  if (view != 0)
    view->set_fov(fov);
}

double
FGViewMgr::getARM_deg () const
{
  const flightgear::View * view = get_current_view();
  return (view == 0 ? 0 : view->get_aspect_ratio_multiplier());
}

void
FGViewMgr::setARM_deg (double aspect_ratio_multiplier)
{  
  flightgear::View * view = get_current_view();
  if (view != 0)
    view->set_aspect_ratio_multiplier(aspect_ratio_multiplier);
}

double
FGViewMgr::getNear_m () const
{
  const flightgear::View * view = get_current_view();
  return (view == 0 ? 0.5f : view->getNear_m());
}

void
FGViewMgr::setNear_m (double near_m)
{
  flightgear::View * view = get_current_view();
  if (view != 0)
    view->setNear_m(near_m);
}

void
FGViewMgr::setViewAxisLong (double axis)
{
  axis_long = axis;
}

void
FGViewMgr::setViewAxisLat (double axis)
{
  axis_lat = axis;
}

double
FGViewMgr::getViewLon_deg() const
{
  const flightgear::View* view = get_current_view();
  return (view != NULL) ? view->getPosition().getLongitudeDeg() : 0.0;
}

double
FGViewMgr::getViewLat_deg() const
{
  const flightgear::View* view = get_current_view();
  return (view != NULL) ? view->getPosition().getLatitudeDeg() : 0.0;
}

double
FGViewMgr::getViewElev_ft() const
{
  const flightgear::View* view = get_current_view();
  return (view != NULL) ? view->getPosition().getElevationFt() : 0.0;
}


// reference frame orientation.
// This is the view orientation you get when you have no
// view offset, i.e. the offset operator is the identity.
// 
// For example, in the familiar "cockpit lookfrom" view,
// the reference frame is equal to the aircraft attitude,
// i.e. it is the view looking towards 12:00 straight ahead.
//
// FIXME:  Somebody needs to figure out what is the reference
// frame view for the other view modes.
// 
// Conceptually, this quat represents a rotation relative
// to the ECEF reference orientation, as described at
//    http://www.av8n.com/physics/coords.htm#sec-orientation
//
// See the NOTE concerning reference orientations, below.
//
// The components of this quat are expressed in 
// the conventional aviation basis set,
// i.e.  x=forward, y=starboard, z=bottom
double FGViewMgr::getCurrentViewFrame_w() const{
  return ((current_view_orientation*conj(fsb2sta())*conj(current_view_or_offset))).w();
}
double FGViewMgr::getCurrentViewFrame_x() const{
  return ((current_view_orientation*conj(fsb2sta())*conj(current_view_or_offset))).x();
}
double FGViewMgr::getCurrentViewFrame_y() const{
  return ((current_view_orientation*conj(fsb2sta())*conj(current_view_or_offset))).y();
}
double FGViewMgr::getCurrentViewFrame_z() const{
  return ((current_view_orientation*conj(fsb2sta())*conj(current_view_or_offset))).z();
}


// view offset.
// This rotation takes you from the aforementioned
// reference frame view orientation to whatever
// actual current view orientation is.
//
// The components of this quaternion are expressed in 
// the conventional aviation basis set,
// i.e.  x=forward, y=starboard, z=bottom
double FGViewMgr::getCurrentViewOrOffset_w() const{
   return current_view_or_offset.w();
}
double FGViewMgr::getCurrentViewOrOffset_x() const{
   return current_view_or_offset.x();
}
double FGViewMgr::getCurrentViewOrOffset_y() const{
   return current_view_or_offset.y();
}
double FGViewMgr::getCurrentViewOrOffset_z() const{
   return current_view_or_offset.z();
}


// current view orientation.
// This is a rotation relative to the earth-centered (ec)
// reference frame.
// 
// NOTE: Here we remove a factor of fsb2sta so that 
// the components of this quat are displayed using the 
// conventional ECEF basis set.  This is *not* the way
// the view orientation is stored in the views[] array,
// but is easier for non-graphics hackers to understand.
// If we did not remove this factor of fsb2sta here and
// in getCurrentViewFrame, that would be equivalent to
// the following peculiar reference orientation:
// Suppose you are over the Gulf of Guinea, at (lat,lon) = (0,0).
// Then the reference frame orientation can be achieved via:
//    -- The aircraft X-axis (nose) headed south.
//    -- The aircraft Y-axis (starboard wingtip) pointing up.
//    -- The aircraft Z-axis (belly) pointing west.
// To say the same thing in other words, and perhaps more to the
// point:  If we use the OpenGL camera orientation conventions, 
// i.e. Xprime=starboard, Yprime=top, Zprime=aft, then the
// aforementioned peculiar reference orientation at (lat,lon)
//  = (0,0) can be described as:
//    -- aircraft Xprime axis (starboard) pointed up
//    -- aircraft Yprime axis (top) pointed east
//    -- aircraft Zprime axis (aft) pointed north
// meaning the OpenGL axes are aligned with the ECEF axes.
double FGViewMgr::getCurrentViewOrientation_w() const{
  return (current_view_orientation * conj(fsb2sta())).w();
}
double FGViewMgr::getCurrentViewOrientation_x() const{
  return (current_view_orientation * conj(fsb2sta())).x();
}
double FGViewMgr::getCurrentViewOrientation_y() const{
  return (current_view_orientation * conj(fsb2sta())).y();
}
double FGViewMgr::getCurrentViewOrientation_z() const{
  return (current_view_orientation * conj(fsb2sta())).z();
}

void
FGViewMgr::do_axes ()
{
                // Take no action when hat is centered
  if ( ( axis_long <  0.01 ) &&
       ( axis_long > -0.01 ) &&
       ( axis_lat  <  0.01 ) &&
       ( axis_lat  > -0.01 )
     )
    return;

  double viewDir = 999;

  /* Do all the quick and easy cases */
  if (axis_long < 0) {        // Longitudinal axis forward
    if (axis_lat == axis_long)
      viewDir = fgGetDouble("/sim/view/config/front-left-direction-deg");
    else if (axis_lat == - axis_long)
      viewDir = fgGetDouble("/sim/view/config/front-right-direction-deg");
    else if (axis_lat == 0)
      viewDir = fgGetDouble("/sim/view/config/front-direction-deg");
  } else if (axis_long > 0) {    // Longitudinal axis backward
    if (axis_lat == - axis_long)
      viewDir = fgGetDouble("/sim/view/config/back-left-direction-deg");
    else if (axis_lat == axis_long)
      viewDir = fgGetDouble("/sim/view/config/back-right-direction-deg");
    else if (axis_lat == 0)
      viewDir = fgGetDouble("/sim/view/config/back-direction-deg");
  } else if (axis_long == 0) {    // Longitudinal axis neutral
    if (axis_lat < 0)
      viewDir = fgGetDouble("/sim/view/config/left-direction-deg");
    else if (axis_lat > 0)
      viewDir = fgGetDouble("/sim/view/config/right-direction-deg");
    else return; /* And assertion failure maybe? */
  }

                // Do all the difficult cases
  if ( viewDir > 900 )
    viewDir = SGD_RADIANS_TO_DEGREES * atan2 ( -axis_lat, -axis_long );
  if ( viewDir < -1 ) viewDir += 360;

  get_current_view()->setGoalHeadingOffset_deg(viewDir);
}
