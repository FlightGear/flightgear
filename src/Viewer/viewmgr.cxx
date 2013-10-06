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
#include <Main/fg_props.hxx>
#include "viewer.hxx"

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
  globals->set_viewmgr(this);
}

// Destructor
FGViewMgr::~FGViewMgr( void )
{
  globals->set_viewmgr(NULL);
}

void
FGViewMgr::init ()
{
  if (inited) {
    SG_LOG(SG_VIEW, SG_WARN, "duplicate init of view manager");
    return;
  }
  
  inited = true;
  
  // stash properties
  current_x_offs = fgGetNode("/sim/current-view/x-offset-m", true);
  current_y_offs = fgGetNode("/sim/current-view/y-offset-m", true);
  current_z_offs = fgGetNode("/sim/current-view/z-offset-m", true);
  target_x_offs  = fgGetNode("/sim/current-view/target-x-offset-m", true);
  target_y_offs  = fgGetNode("/sim/current-view/target-y-offset-m", true);
  target_z_offs  = fgGetNode("/sim/current-view/target-z-offset-m", true);

  double aspect_ratio_multiplier
      = fgGetDouble("/sim/current-view/aspect-ratio-multiplier");

  for (unsigned int i = 0; i < config_list.size(); i++) {
    SGPropertyNode *n = config_list[i];
    SGPropertyNode *config = n->getChild("config", 0, true);

    // find out if this is an internal view (e.g. in cockpit, low near plane)
    bool internal = n->getBoolValue("internal", false);

    // FIXME:
    // this is assumed to be an aircraft model...we will need to read
    // model-from-type as well.

    // find out if this is a model we are looking from...
    bool from_model = config->getBoolValue("from-model");
    int from_model_index = config->getIntValue("from-model-idx");

    double x_offset_m = config->getDoubleValue("x-offset-m");
    double y_offset_m = config->getDoubleValue("y-offset-m");
    double z_offset_m = config->getDoubleValue("z-offset-m");

    double heading_offset_deg = config->getDoubleValue("heading-offset-deg");
    config->setDoubleValue("heading-offset-deg", heading_offset_deg);
    double pitch_offset_deg = config->getDoubleValue("pitch-offset-deg");
    config->setDoubleValue("pitch-offset-deg", pitch_offset_deg);
    double roll_offset_deg = config->getDoubleValue("roll-offset-deg");
    config->setDoubleValue("roll-offset-deg", roll_offset_deg);

    double fov_deg = config->getDoubleValue("default-field-of-view-deg");
    double near_m = config->getDoubleValue("ground-level-nearplane-m");

    // supporting two types "lookat" = 1 and "lookfrom" = 0
    const char *type = n->getStringValue("type");
    if (!strcmp(type, "lookat")) {

      bool at_model = config->getBoolValue("at-model");
      int at_model_index = config->getIntValue("at-model-idx");

      double damp_roll = config->getDoubleValue("at-model-roll-damping");
      double damp_pitch = config->getDoubleValue("at-model-pitch-damping");
      double damp_heading = config->getDoubleValue("at-model-heading-damping");

      double target_x_offset_m = config->getDoubleValue("target-x-offset-m");
      double target_y_offset_m = config->getDoubleValue("target-y-offset-m");
      double target_z_offset_m = config->getDoubleValue("target-z-offset-m");

      add_view(new FGViewer ( FG_LOOKAT, from_model, from_model_index,
                              at_model, at_model_index,
                              damp_roll, damp_pitch, damp_heading,
                              x_offset_m, y_offset_m,z_offset_m,
                              heading_offset_deg, pitch_offset_deg,
                              roll_offset_deg, fov_deg, aspect_ratio_multiplier,
                              target_x_offset_m, target_y_offset_m,
                              target_z_offset_m, near_m, internal ));
    } else {
      add_view(new FGViewer ( FG_LOOKFROM, from_model, from_model_index,
                              false, 0, 0.0, 0.0, 0.0,
                              x_offset_m, y_offset_m, z_offset_m,
                              heading_offset_deg, pitch_offset_deg,
                              roll_offset_deg, fov_deg, aspect_ratio_multiplier,
                              0, 0, 0, near_m, internal ));
    }
  }

  copyToCurrent();
  do_bind();
}

void
FGViewMgr::shutdown()
{
    if (!inited) {
        return;
    }
    
    inited = false;
}

void
FGViewMgr::reinit ()
{
  int current_view_index = current;

  // reset offsets and fov to configuration defaults
  for (unsigned int i = 0; i < config_list.size(); i++) {
    SGPropertyNode *n = config_list[i];
    setView(i);

    fgSetDouble("/sim/current-view/x-offset-m",
        n->getDoubleValue("config/x-offset-m"));
    fgSetDouble("/sim/current-view/y-offset-m",
        n->getDoubleValue("config/y-offset-m"));
    fgSetDouble("/sim/current-view/z-offset-m",
        n->getDoubleValue("config/z-offset-m"));
    fgSetDouble("/sim/current-view/pitch-offset-deg",
        n->getDoubleValue("config/pitch-offset-deg"));
    fgSetDouble("/sim/current-view/heading-offset-deg",
        n->getDoubleValue("config/heading-offset-deg"));
    fgSetDouble("/sim/current-view/roll-offset-deg",
        n->getDoubleValue("config/roll-offset-deg"));

    double fov_deg = n->getDoubleValue("config/default-field-of-view-deg");
    if (fov_deg < 10.0)
      fov_deg = 55.0;
    fgSetDouble("/sim/current-view/field-of-view", fov_deg);

    // target offsets for lookat mode only...
    fgSetDouble("/sim/current-view/target-x-offset-m",
        n->getDoubleValue("config/target-x-offset-m"));
    fgSetDouble("/sim/current-view/target-y-offset-m",
        n->getDoubleValue("config/target-y-offset-m"));
    fgSetDouble("/sim/current-view/target-z-offset-m",
        n->getDoubleValue("config/target-z-offset-m"));
  }
  setView(current_view_index);
}

typedef double (FGViewMgr::*double_getter)() const;

void
FGViewMgr::bind()
{
  // view-manager code was designed to init before bind, so
  // this is a no-op; init() calls the real bind() impl below
}

void
FGViewMgr::do_bind()
{  
  // these are bound to the current view properties
  _tiedProperties.setRoot(fgGetNode("/sim/current-view", true));
  _tiedProperties.Tie("heading-offset-deg", this,
                      &FGViewMgr::getViewHeadingOffset_deg,
                      &FGViewMgr::setViewHeadingOffset_deg);
  fgSetArchivable("/sim/current-view/heading-offset-deg");
  _tiedProperties.Tie("goal-heading-offset-deg", this,
                      &FGViewMgr::getViewGoalHeadingOffset_deg,
                      &FGViewMgr::setViewGoalHeadingOffset_deg);
  fgSetArchivable("/sim/current-view/goal-heading-offset-deg");
  _tiedProperties.Tie("pitch-offset-deg", this,
                      &FGViewMgr::getViewPitchOffset_deg,
                      &FGViewMgr::setViewPitchOffset_deg);
  fgSetArchivable("/sim/current-view/pitch-offset-deg");
  _tiedProperties.Tie("goal-pitch-offset-deg", this,
                      &FGViewMgr::getGoalViewPitchOffset_deg,
                      &FGViewMgr::setGoalViewPitchOffset_deg);
  fgSetArchivable("/sim/current-view/goal-pitch-offset-deg");
  _tiedProperties.Tie("roll-offset-deg", this,
                      &FGViewMgr::getViewRollOffset_deg,
                      &FGViewMgr::setViewRollOffset_deg);
  fgSetArchivable("/sim/current-view/roll-offset-deg");
  _tiedProperties.Tie("goal-roll-offset-deg", this,
                      &FGViewMgr::getGoalViewRollOffset_deg,
                      &FGViewMgr::setGoalViewRollOffset_deg);
  fgSetArchivable("/sim/current-view/goal-roll-offset-deg");

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
  _tiedProperties.Untie();
    config_list.clear();
    view_number.clear();
}

void
FGViewMgr::update (double dt)
{
  FGViewer* currentView = (FGViewer *)get_current_view();
  if (!currentView) return;

  SGPropertyNode *n = config_list[current];
  SGPropertyNode *config = n->getChild("config", 0, true);
  double lon_deg, lat_deg, alt_ft, roll_deg, pitch_deg, heading_deg;

  // Set up view location and orientation

  if (!config->getBoolValue("from-model")) {
    lon_deg = fgGetDouble(config->getStringValue("eye-lon-deg-path"));
    lat_deg = fgGetDouble(config->getStringValue("eye-lat-deg-path"));
    alt_ft = fgGetDouble(config->getStringValue("eye-alt-ft-path"));
    roll_deg = fgGetDouble(config->getStringValue("eye-roll-deg-path"));
    pitch_deg = fgGetDouble(config->getStringValue("eye-pitch-deg-path"));
    heading_deg = fgGetDouble(config->getStringValue("eye-heading-deg-path"));

    currentView->setPosition(lon_deg, lat_deg, alt_ft);
    currentView->setOrientation(roll_deg, pitch_deg, heading_deg);
  } else {
    // force recalc in viewer
    currentView->set_dirty();
  }

  // if lookat (type 1) then get target data...
  if (currentView->getType() == FG_LOOKAT) {
    if (!config->getBoolValue("from-model")) {
      lon_deg = fgGetDouble(config->getStringValue("target-lon-deg-path"));
      lat_deg = fgGetDouble(config->getStringValue("target-lat-deg-path"));
      alt_ft = fgGetDouble(config->getStringValue("target-alt-ft-path"));
      roll_deg = fgGetDouble(config->getStringValue("target-roll-deg-path"));
      pitch_deg = fgGetDouble(config->getStringValue("target-pitch-deg-path"));
      heading_deg = fgGetDouble(config->getStringValue("target-heading-deg-path"));

      currentView->setTargetPosition(lon_deg, lat_deg, alt_ft);
      currentView->setTargetOrientation(roll_deg, pitch_deg, heading_deg);
    } else {
      currentView->set_dirty();
    }
  }

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
  
  // expose the raw (OpenGL) orientation to the property tree,
  // for the sound-manager
  for (int i=0; i<4; ++i) {
    _tiedProperties.getRoot()->getChild("raw-orientation", i, true)->setDoubleValue(current_view_orientation[i]);
  }
}

void
FGViewMgr::copyToCurrent()
{
    if (!inited) {
        return;
    }
  
    SGPropertyNode *n = config_list[current];
    fgSetString("/sim/current-view/name", n->getStringValue("name"));
    fgSetString("/sim/current-view/type", n->getStringValue("type"));

    // copy certain view config data for default values
    fgSetDouble("/sim/current-view/config/heading-offset-deg",
                n->getDoubleValue("config/default-heading-offset-deg"));
    fgSetDouble("/sim/current-view/config/pitch-offset-deg",
                n->getDoubleValue("config/pitch-offset-deg"));
    fgSetDouble("/sim/current-view/config/roll-offset-deg",
                n->getDoubleValue("config/roll-offset-deg"));
    fgSetDouble("/sim/current-view/config/default-field-of-view-deg",
                n->getDoubleValue("config/default-field-of-view-deg"));
    fgSetBool("/sim/current-view/config/from-model",
                n->getBoolValue("config/from-model"));

    // copy view data
    fgSetDouble("/sim/current-view/x-offset-m", getViewXOffset_m());
    fgSetDouble("/sim/current-view/y-offset-m", getViewYOffset_m());
    fgSetDouble("/sim/current-view/z-offset-m", getViewZOffset_m());

    fgSetDouble("/sim/current-view/goal-heading-offset-deg",
                get_current_view()->getGoalHeadingOffset_deg());
    fgSetDouble("/sim/current-view/goal-pitch-offset-deg",
                get_current_view()->getGoalPitchOffset_deg());
    fgSetDouble("/sim/current-view/goal-roll-offset-deg",
                get_current_view()->getRollOffset_deg());
    fgSetDouble("/sim/current-view/heading-offset-deg",
                get_current_view()->getHeadingOffset_deg());
    fgSetDouble("/sim/current-view/pitch-offset-deg",
                get_current_view()->getPitchOffset_deg());
    fgSetDouble("/sim/current-view/roll-offset-deg",
                get_current_view()->getRollOffset_deg());
    fgSetDouble("/sim/current-view/target-x-offset-m",
                get_current_view()->getTargetXOffset_m());
    fgSetDouble("/sim/current-view/target-y-offset-m",
                get_current_view()->getTargetYOffset_m());
    fgSetDouble("/sim/current-view/target-z-offset-m",
                get_current_view()->getTargetZOffset_m());
    fgSetBool("/sim/current-view/internal",
                get_current_view()->getInternal());
}

void FGViewMgr::clear()
{
    views.clear();
}

FGViewer*
FGViewMgr::get_current_view()
{
    if ( current < (int)views.size() ) {
        return views[current];
    } else {
        return NULL;
    }
}

const FGViewer*
FGViewMgr::get_current_view() const
{
    if ( current < (int)views.size() ) {
        return views[current];
    } else {
        return NULL;
    }
}


FGViewer*
FGViewMgr::get_view( int i )
{
    if ( i < 0 ) { i = 0; }
    if ( i >= (int)views.size() ) { i = views.size() - 1; }
    return views[i];
}

const FGViewer*
FGViewMgr::get_view( int i ) const
{
    if ( i < 0 ) { i = 0; }
    if ( i >= (int)views.size() ) { i = views.size() - 1; }
    return views[i];
}

FGViewer*
FGViewMgr::next_view()
{
    setView((current+1 < (int)views.size()) ? (current + 1) : 0);
    view_number->fireValueChanged();
    return views[current];
}

FGViewer*
FGViewMgr::prev_view()
{
    setView((0 < current) ? (current - 1) : (views.size() - 1));
    view_number->fireValueChanged();
    return views[current];
}

void
FGViewMgr::add_view( FGViewer * v )
{
  views.push_back(v);
  v->init();
}
    
double
FGViewMgr::getViewHeadingOffset_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->getHeadingOffset_deg());
}

void
FGViewMgr::setViewHeadingOffset_deg (double offset)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setGoalHeadingOffset_deg(offset);
    view->setHeadingOffset_deg(offset);
  }
}

double
FGViewMgr::getViewGoalHeadingOffset_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->getGoalHeadingOffset_deg());
}

void
FGViewMgr::setViewGoalHeadingOffset_deg (double offset)
{
  FGViewer * view = get_current_view();
  if (view != 0)
    view->setGoalHeadingOffset_deg(offset);
}

double
FGViewMgr::getViewPitchOffset_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->getPitchOffset_deg());
}

void
FGViewMgr::setViewPitchOffset_deg (double tilt)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setGoalPitchOffset_deg(tilt);
    view->setPitchOffset_deg(tilt);
  }
}

double
FGViewMgr::getGoalViewPitchOffset_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->getGoalPitchOffset_deg());
}

void
FGViewMgr::setGoalViewPitchOffset_deg (double tilt)
{
  FGViewer * view = get_current_view();
  if (view != 0)
    view->setGoalPitchOffset_deg(tilt);
}

double
FGViewMgr::getViewRollOffset_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->getRollOffset_deg());
}

void
FGViewMgr::setViewRollOffset_deg (double tilt)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setGoalRollOffset_deg(tilt);
    view->setRollOffset_deg(tilt);
  }
}

double
FGViewMgr::getGoalViewRollOffset_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->getGoalRollOffset_deg());
}

void
FGViewMgr::setGoalViewRollOffset_deg (double tilt)
{
  FGViewer * view = get_current_view();
  if (view != 0)
    view->setGoalRollOffset_deg(tilt);
}

double
FGViewMgr::getViewXOffset_m () const
{
  const FGViewer * view = get_current_view();
  if (view != 0) {
    return ((FGViewer *)view)->getXOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewXOffset_m (double x)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setXOffset_m(x);
  }
}

double
FGViewMgr::getViewYOffset_m () const
{
  const FGViewer * view = get_current_view();
  if (view != 0) {
    return ((FGViewer *)view)->getYOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewYOffset_m (double y)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setYOffset_m(y);
  }
}

double
FGViewMgr::getViewZOffset_m () const
{
  const FGViewer * view = get_current_view();
  if (view != 0) {
    return ((FGViewer *)view)->getZOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewZOffset_m (double z)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setZOffset_m(z);
  }
}

double
FGViewMgr::getViewTargetXOffset_m () const
{
  const FGViewer * view = get_current_view();
  if (view != 0) {
    return ((FGViewer *)view)->getTargetXOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewTargetXOffset_m (double x)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setTargetXOffset_m(x);
  }
}

double
FGViewMgr::getViewTargetYOffset_m () const
{
  const FGViewer * view = get_current_view();
  if (view != 0) {
    return ((FGViewer *)view)->getTargetYOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewTargetYOffset_m (double y)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setTargetYOffset_m(y);
  }
}

double
FGViewMgr::getViewTargetZOffset_m () const
{
  const FGViewer * view = get_current_view();
  if (view != 0) {
    return ((FGViewer *)view)->getTargetZOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setViewTargetZOffset_m (double z)
{
  FGViewer * view = get_current_view();
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

  // set new view
  current = newview;
  // copy in view data
  copyToCurrent();
}


double
FGViewMgr::getFOV_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->get_fov());
}

void
FGViewMgr::setFOV_deg (double fov)
{
  FGViewer * view = get_current_view();
  if (view != 0)
    view->set_fov(fov);
}

double
FGViewMgr::getARM_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->get_aspect_ratio_multiplier());
}

void
FGViewMgr::setARM_deg (double aspect_ratio_multiplier)
{  
  FGViewer * view = get_current_view();
  if (view != 0)
    view->set_aspect_ratio_multiplier(aspect_ratio_multiplier);
}

double
FGViewMgr::getNear_m () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0.5f : view->getNear_m());
}

void
FGViewMgr::setNear_m (double near_m)
{
  FGViewer * view = get_current_view();
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
  const FGViewer* view = get_current_view();
  return (view != NULL) ? view->getPosition().getLongitudeDeg() : 0.0;
}

double
FGViewMgr::getViewLat_deg() const
{
  const FGViewer* view = get_current_view();
  return (view != NULL) ? view->getPosition().getLatitudeDeg() : 0.0;
}

double
FGViewMgr::getViewElev_ft() const
{
  const FGViewer* view = get_current_view();
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
