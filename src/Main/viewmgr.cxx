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

#include <string.h>		// strcmp

#include <plib/sg.h>

#include <simgear/compiler.h>

#include <Model/acmodel.hxx>

#include "viewmgr.hxx"


// Constructor
FGViewMgr::FGViewMgr( void ) :
  axis_long(0),
  axis_lat(0),
  view_number(fgGetNode("/sim/current-view/view-number", true)),
  config_list(fgGetNode("/sim", true)->getChildren("view")),
  current(0)
{
}

// Destructor
FGViewMgr::~FGViewMgr( void ) {
}

void
FGViewMgr::init ()
{
  double aspect_ratio_multiplier
      = fgGetDouble("/sim/current-view/aspect-ratio-multiplier");

  for (unsigned int i = 0; i < config_list.size(); i++) {
    SGPropertyNode *n = config_list[i];

    // find out if this is an internal view (e.g. in cockpit, low near plane)
    bool internal = n->getBoolValue("internal", false);

    // FIXME:
    // this is assumed to be an aircraft model...we will need to read
    // model-from-type as well.

    // find out if this is a model we are looking from...
    bool from_model = n->getBoolValue("config/from-model");
    int from_model_index = n->getIntValue("config/from-model-idx");

    double x_offset_m = n->getDoubleValue("config/x-offset-m");
    double y_offset_m = n->getDoubleValue("config/y-offset-m");
    double z_offset_m = n->getDoubleValue("config/z-offset-m");

    double heading_offset_deg = n->getDoubleValue("config/heading-offset-deg");
    n->setDoubleValue("config/heading-offset-deg", heading_offset_deg);
    double pitch_offset_deg = n->getDoubleValue("config/pitch-offset-deg");
    n->setDoubleValue("config/pitch-offset-deg", pitch_offset_deg);
    double roll_offset_deg = n->getDoubleValue("config/roll-offset-deg");
    n->setDoubleValue("config/roll-offset-deg", roll_offset_deg);

    double fov_deg = n->getDoubleValue("config/default-field-of-view-deg");
    double near_m = n->getDoubleValue("config/ground-level-nearplane-m");

    // supporting two types "lookat" = 1 and "lookfrom" = 0
    const char *type = n->getStringValue("type");
    if (!strcmp(type, "lookat")) {

      bool at_model = n->getBoolValue("config/at-model");
      int at_model_index = n->getIntValue("config/at-model-idx");

      double damp_roll = n->getDoubleValue("config/at-model-roll-damping");
      double damp_pitch = n->getDoubleValue("config/at-model-pitch-damping");
      double damp_heading = n->getDoubleValue("config/at-model-heading-damping");

      double target_x_offset_m = n->getDoubleValue("config/target-x-offset-m");
      double target_y_offset_m = n->getDoubleValue("config/target-y-offset-m");
      double target_z_offset_m = n->getDoubleValue("config/target-z-offset-m");

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
}

void
FGViewMgr::reinit ()
{
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
    fgSetDouble("/sim/current-view/target-x-offset-deg",
        n->getDoubleValue("config/target-x-offset-m"));
    fgSetDouble("/sim/current-view/target-y-offset-deg",
        n->getDoubleValue("config/target-y-offset-m"));
    fgSetDouble("/sim/current-view/target-z-offset-deg",
        n->getDoubleValue("config/target-z-offset-m"));
  }
  setView(0);
}

typedef double (FGViewMgr::*double_getter)() const;

void
FGViewMgr::bind ()
{
  // these are bound to the current view properties
  fgTie("/sim/current-view/heading-offset-deg", this,
	&FGViewMgr::getViewHeadingOffset_deg,
        &FGViewMgr::setViewHeadingOffset_deg);
  fgSetArchivable("/sim/current-view/heading-offset-deg");
  fgTie("/sim/current-view/goal-heading-offset-deg", this,
	&FGViewMgr::getViewGoalHeadingOffset_deg,
        &FGViewMgr::setViewGoalHeadingOffset_deg);
  fgSetArchivable("/sim/current-view/goal-heading-offset-deg");
  fgTie("/sim/current-view/pitch-offset-deg", this,
	&FGViewMgr::getViewPitchOffset_deg,
        &FGViewMgr::setViewPitchOffset_deg);
  fgSetArchivable("/sim/current-view/pitch-offset-deg");
  fgTie("/sim/current-view/goal-pitch-offset-deg", this,
	&FGViewMgr::getGoalViewPitchOffset_deg,
        &FGViewMgr::setGoalViewPitchOffset_deg);
  fgSetArchivable("/sim/current-view/goal-pitch-offset-deg");
  fgTie("/sim/current-view/roll-offset-deg", this,
	&FGViewMgr::getViewRollOffset_deg,
        &FGViewMgr::setViewRollOffset_deg);
  fgSetArchivable("/sim/current-view/roll-offset-deg");
  fgTie("/sim/current-view/goal-roll-offset-deg", this,
	&FGViewMgr::getGoalViewRollOffset_deg,
        &FGViewMgr::setGoalViewRollOffset_deg);
  fgSetArchivable("/sim/current-view/goal-roll-offset-deg");

  fgTie("/sim/current-view/view-number", this,
                      &FGViewMgr::getView, &FGViewMgr::setView);
  fgSetArchivable("/sim/current-view/view-number", FALSE);

  fgTie("/sim/current-view/axes/long", this,
	(double_getter)0, &FGViewMgr::setViewAxisLong);
  fgSetArchivable("/sim/current-view/axes/long");

  fgTie("/sim/current-view/axes/lat", this,
	(double_getter)0, &FGViewMgr::setViewAxisLat);
  fgSetArchivable("/sim/current-view/axes/lat");

  fgTie("/sim/current-view/field-of-view", this,
	&FGViewMgr::getFOV_deg, &FGViewMgr::setFOV_deg);
  fgSetArchivable("/sim/current-view/field-of-view");

  fgTie("/sim/current-view/aspect-ratio-multiplier", this,
	&FGViewMgr::getARM_deg, &FGViewMgr::setARM_deg);
  fgSetArchivable("/sim/current-view/field-of-view");

  fgTie("/sim/current-view/ground-level-nearplane-m", this,
	&FGViewMgr::getNear_m, &FGViewMgr::setNear_m);
  fgSetArchivable("/sim/current-view/ground-level-nearplane-m");

}

void
FGViewMgr::unbind ()
{
  // FIXME:
  // need to redo these bindings to the new locations (move to viewer?)
  fgUntie("/sim/current-view/heading-offset-deg");
  fgUntie("/sim/current-view/goal-heading-offset-deg");
  fgUntie("/sim/current-view/pitch-offset-deg");
  fgUntie("/sim/current-view/goal-pitch-offset-deg");
  fgUntie("/sim/current-view/field-of-view");
  fgUntie("/sim/current-view/aspect-ratio-multiplier");
  fgUntie("/sim/current-view/view-number");
  fgUntie("/sim/current-view/axes/long");
  fgUntie("/sim/current-view/axes/lat");
  fgUntie("/sim/current-view/ground-level-nearplane-m");
}

void
FGViewMgr::update (double dt)
{
  FGViewer * view = get_current_view();
  if (view == 0)
    return;

  FGViewer *loop_view = (FGViewer *)get_view(current);
  SGPropertyNode *n = config_list[current];
  double lon_deg, lat_deg, alt_ft, roll_deg, pitch_deg, heading_deg;

  // Set up view location and orientation

  if (!n->getBoolValue("config/from-model")) {
    lon_deg = fgGetDouble(n->getStringValue("config/eye-lon-deg-path"));
    lat_deg = fgGetDouble(n->getStringValue("config/eye-lat-deg-path"));
    alt_ft = fgGetDouble(n->getStringValue("config/eye-alt-ft-path"));
    roll_deg = fgGetDouble(n->getStringValue("config/eye-roll-deg-path"));
    pitch_deg = fgGetDouble(n->getStringValue("config/eye-pitch-deg-path"));
    heading_deg = fgGetDouble(n->getStringValue("config/eye-heading-deg-path"));

    loop_view->setPosition(lon_deg, lat_deg, alt_ft);
    loop_view->setOrientation(roll_deg, pitch_deg, heading_deg);
  } else {
    // force recalc in viewer
    loop_view->set_dirty();
  }

  // if lookat (type 1) then get target data...
  if (loop_view->getType() == FG_LOOKAT) {
    if (!n->getBoolValue("config/from-model")) {
      lon_deg = fgGetDouble(n->getStringValue("config/target-lon-deg-path"));
      lat_deg = fgGetDouble(n->getStringValue("config/target-lat-deg-path"));
      alt_ft = fgGetDouble(n->getStringValue("config/target-alt-ft-path"));
      roll_deg = fgGetDouble(n->getStringValue("config/target-roll-deg-path"));
      pitch_deg = fgGetDouble(n->getStringValue("config/target-pitch-deg-path"));
      heading_deg = fgGetDouble(n->getStringValue("config/target-heading-deg-path"));

      loop_view->setTargetPosition(lon_deg, lat_deg, alt_ft);
      loop_view->setTargetOrientation(roll_deg, pitch_deg, heading_deg);
    } else {
      loop_view->set_dirty();
    }
  }

  setViewXOffset_m(fgGetDouble("/sim/current-view/x-offset-m"));
  setViewYOffset_m(fgGetDouble("/sim/current-view/y-offset-m"));
  setViewZOffset_m(fgGetDouble("/sim/current-view/z-offset-m"));

  setViewTargetXOffset_m(fgGetDouble("/sim/current-view/target-x-offset-m"));
  setViewTargetYOffset_m(fgGetDouble("/sim/current-view/target-y-offset-m"));
  setViewTargetZOffset_m(fgGetDouble("/sim/current-view/target-z-offset-m"));

  // Update the current view
  do_axes();
  view->update(dt);
}

void
FGViewMgr::copyToCurrent()
{
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
  set_view(newview);
  // copy in view data
  copyToCurrent();

  // Copy the fdm's position into the SGLocation which is shared with
  // some views ...
  globals->get_aircraft_model()->update(0);
  // Do the update ...
  update(0);
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
  if (axis_long < 0) {		// Longitudinal axis forward
    if (axis_lat == axis_long)
      viewDir = fgGetDouble("/sim/view/config/front-left-direction-deg");
    else if (axis_lat == - axis_long)
      viewDir = fgGetDouble("/sim/view/config/front-right-direction-deg");
    else if (axis_lat == 0)
      viewDir = fgGetDouble("/sim/view/config/front-direction-deg");
  } else if (axis_long > 0) {	// Longitudinal axis backward
    if (axis_lat == - axis_long)
      viewDir = fgGetDouble("/sim/view/config/back-left-direction-deg");
    else if (axis_lat == axis_long)
      viewDir = fgGetDouble("/sim/view/config/back-right-direction-deg");
    else if (axis_lat == 0)
      viewDir = fgGetDouble("/sim/view/config/back-direction-deg");
  } else if (axis_long == 0) {	// Longitudinal axis neutral
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
