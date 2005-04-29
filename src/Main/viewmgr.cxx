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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#include <string.h>		// strcmp

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/compiler.h>

#include <Model/acmodel.hxx>

#include "viewmgr.hxx"
#include "fg_props.hxx"


// Constructor
FGViewMgr::FGViewMgr( void ) :
  axis_long(0),
  axis_lat(0),
  current(0)
{
}


// Destructor
FGViewMgr::~FGViewMgr( void ) {
}

void
FGViewMgr::init ()
{
  char stridx [ 20 ];
  string viewpath, nodepath, strdata;
  bool from_model = false;
  bool at_model = false;
  int from_model_index = 0;
  int at_model_index = 0;
  // double damp_alt;
  double damp_roll = 0.0, damp_pitch = 0.0, damp_heading = 0.0;
  double x_offset_m, y_offset_m, z_offset_m, fov_deg;
  double heading_offset_deg, pitch_offset_deg, roll_offset_deg;
  double target_x_offset_m, target_y_offset_m, target_z_offset_m;
  double near_m;
  bool internal;

  double aspect_ratio_multiplier
      = fgGetDouble("/sim/current-view/aspect-ratio-multiplier");

  for (int i = 0; i < fgGetInt("/sim/number-views"); i++) {
    viewpath = "/sim/view";
    sprintf(stridx, "[%d]", i);
    viewpath += stridx;

    // find out what type of view this is...
    nodepath = viewpath;
    nodepath += "/type";
    strdata = fgGetString(nodepath.c_str());

    // find out if this is an internal view (e.g. in cockpit, low near plane)
    internal = false; // default
    nodepath = viewpath;
    nodepath += "/internal";
    internal = fgGetBool(nodepath.c_str());

    // FIXME:
    // this is assumed to be an aircraft model...we will need to read
    // model-from-type as well.
    // find out if this is a model we are looking from...
    nodepath = viewpath;
    nodepath += "/config/from-model";
    from_model = fgGetBool(nodepath.c_str());

    // get model index (which model)
    if (from_model) {
      nodepath = viewpath;
      nodepath += "/config/from-model-idx";
      from_model_index = fgGetInt(nodepath.c_str());     
    }

    if ( strcmp("lookat",strdata.c_str()) == 0 ) {
      // find out if this is a model we are looking at...
      nodepath = viewpath;
      nodepath += "/config/at-model";
      at_model = fgGetBool(nodepath.c_str());

      // get model index (which model)
      if (at_model) {
        nodepath = viewpath;
        nodepath += "/config/at-model-idx";
        at_model_index = fgGetInt(nodepath.c_str());

        nodepath = viewpath;
        nodepath += "/config/at-model-roll-damping";
        damp_roll = fgGetDouble(nodepath.c_str(), 0.0);
        nodepath = viewpath;
        nodepath += "/config/at-model-pitch-damping";
        damp_pitch = fgGetDouble(nodepath.c_str(), 0.0);
        nodepath = viewpath;
        nodepath += "/config/at-model-heading-damping";
        damp_heading = fgGetDouble(nodepath.c_str(), 0.0);
      }
    }

    nodepath = viewpath;
    nodepath += "/config/x-offset-m";
    x_offset_m = fgGetDouble(nodepath.c_str());
    nodepath = viewpath;
    nodepath += "/config/y-offset-m";
    y_offset_m = fgGetDouble(nodepath.c_str());
    nodepath = viewpath;
    nodepath += "/config/z-offset-m";
    z_offset_m = fgGetDouble(nodepath.c_str());
    nodepath = viewpath;
    nodepath += "/config/pitch-offset-deg";
    pitch_offset_deg = fgGetDouble(nodepath.c_str());
    fgSetDouble(nodepath.c_str(),pitch_offset_deg);
    nodepath = viewpath;
    nodepath += "/config/heading-offset-deg";
    heading_offset_deg = fgGetDouble(nodepath.c_str());
    fgSetDouble(nodepath.c_str(),heading_offset_deg);
    nodepath = viewpath;
    nodepath += "/config/roll-offset-deg";
    roll_offset_deg = fgGetDouble(nodepath.c_str());
    fgSetDouble(nodepath.c_str(),roll_offset_deg);
    nodepath = viewpath;
    nodepath += "/config/default-field-of-view-deg";
    fov_deg = fgGetDouble(nodepath.c_str());

    // target offsets for lookat mode only...
    nodepath = viewpath;
    nodepath += "/config/target-x-offset-m";
    target_x_offset_m = fgGetDouble(nodepath.c_str());
    nodepath = viewpath;
    nodepath += "/config/target-y-offset-m";
    target_y_offset_m = fgGetDouble(nodepath.c_str());
    nodepath = viewpath;
    nodepath += "/config/target-z-offset-m";
    target_z_offset_m = fgGetDouble(nodepath.c_str());

    nodepath = viewpath;
    nodepath += "/config/ground-level-nearplane-m";
    near_m = fgGetDouble(nodepath.c_str());

    // supporting two types now "lookat" = 1 and "lookfrom" = 0
    if ( strcmp("lookat",strdata.c_str()) == 0 )
      add_view(new FGViewer ( FG_LOOKAT, from_model, from_model_index,
                              at_model, at_model_index,
                              damp_roll, damp_pitch, damp_heading,
                              x_offset_m, y_offset_m,z_offset_m,
                              heading_offset_deg, pitch_offset_deg,
                              roll_offset_deg, fov_deg, aspect_ratio_multiplier,
                              target_x_offset_m, target_y_offset_m,
                              target_z_offset_m, near_m, internal ));
    else
      add_view(new FGViewer ( FG_LOOKFROM, from_model, from_model_index,
                              false, 0, 0.0, 0.0, 0.0,
                              x_offset_m, y_offset_m, z_offset_m,
                              heading_offset_deg, pitch_offset_deg,
                              roll_offset_deg, fov_deg, aspect_ratio_multiplier,
                              0, 0, 0, near_m, internal ));
  }

  copyToCurrent();
  
}

void
FGViewMgr::reinit ()
{
  char stridx [ 20 ];
  string viewpath, nodepath, strdata;
  double fov_deg;

  // reset offsets and fov to configuration defaults

  for (int i = 0; i < fgGetInt("/sim/number-views"); i++) {
    viewpath = "/sim/view";
    sprintf(stridx, "[%d]", i);
    viewpath += stridx;

    setView(i);

    nodepath = viewpath;
    nodepath += "/config/x-offset-m";
    fgSetDouble("/sim/current-view/x-offset-m",fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/y-offset-m";
    fgSetDouble("/sim/current-view/y-offset-m",fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/z-offset-m";
    fgSetDouble("/sim/current-view/z-offset-m",fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/pitch-offset-deg";
    fgSetDouble("/sim/current-view/pitch-offset-deg",
                fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/heading-offset-deg";
    fgSetDouble("/sim/current-view/heading-offset-deg",
                fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/roll-offset-deg";
    fgSetDouble("/sim/current-view/roll-offset-deg",
                fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/default-field-of-view-deg";
    fov_deg = fgGetDouble(nodepath.c_str());
    if (fov_deg < 10.0) {
      fov_deg = 55.0;
    }
    fgSetDouble("/sim/current-view/field-of-view",fov_deg);

    // target offsets for lookat mode only...
    nodepath = viewpath;
    nodepath += "/config/target-x-offset-m";
    fgSetDouble("/sim/current-view/target-x-offset-deg",
                fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/target-y-offset-m";
    fgSetDouble("/sim/current-view/target-y-offset-deg",
                fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/target-z-offset-m";
    fgSetDouble("/sim/current-view/target-z-offset-deg",
                fgGetDouble(nodepath.c_str()));

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
  char stridx [20];
  string viewpath, nodepath;
  double lon_deg, lat_deg, alt_ft, roll_deg, pitch_deg, heading_deg;

  FGViewer * view = get_current_view();
  if (view == 0)
    return;

  // 
  int i = current;
  viewpath = "/sim/view";
  sprintf(stridx, "[%d]", i);
  viewpath += stridx;

  FGViewer *loop_view = (FGViewer *)get_view( i );

		// Set up view location and orientation

  nodepath = viewpath;
  nodepath += "/config/from-model";
  if (!fgGetBool(nodepath.c_str())) {
    nodepath = viewpath;
    nodepath += "/config/eye-lon-deg-path";
    lon_deg = fgGetDouble(fgGetString(nodepath.c_str()));
    nodepath = viewpath;
    nodepath += "/config/eye-lat-deg-path";
    lat_deg = fgGetDouble(fgGetString(nodepath.c_str()));
    nodepath = viewpath;
    nodepath += "/config/eye-alt-ft-path";
    alt_ft = fgGetDouble(fgGetString(nodepath.c_str()));
    nodepath = viewpath;
    nodepath += "/config/eye-roll-deg-path";
    roll_deg = fgGetDouble(fgGetString(nodepath.c_str()));
    nodepath = viewpath;
    nodepath += "/config/eye-pitch-deg-path";
    pitch_deg = fgGetDouble(fgGetString(nodepath.c_str()));
    nodepath = viewpath;
    nodepath += "/config/eye-heading-deg-path";
    heading_deg = fgGetDouble(fgGetString(nodepath.c_str()));
    loop_view->setPosition(lon_deg, lat_deg, alt_ft);
    loop_view->setOrientation(roll_deg, pitch_deg, heading_deg);
  } else {
    // force recalc in viewer
    loop_view->set_dirty();
  }

  // if lookat (type 1) then get target data...
  if (loop_view->getType() == FG_LOOKAT) {
    nodepath = viewpath;
    nodepath += "/config/from-model";
    if (!fgGetBool(nodepath.c_str())) {
      nodepath = viewpath;
      nodepath += "/config/target-lon-deg-path";
      lon_deg = fgGetDouble(fgGetString(nodepath.c_str()));
      nodepath = viewpath;
      nodepath += "/config/target-lat-deg-path";
      lat_deg = fgGetDouble(fgGetString(nodepath.c_str()));
      nodepath = viewpath;
      nodepath += "/config/target-alt-ft-path";
      alt_ft = fgGetDouble(fgGetString(nodepath.c_str()));
      nodepath = viewpath;
      nodepath += "/config/target-roll-deg-path";
      roll_deg = fgGetDouble(fgGetString(nodepath.c_str()));
      nodepath = viewpath;
      nodepath += "/config/target-pitch-deg-path";
      pitch_deg = fgGetDouble(fgGetString(nodepath.c_str()));
      nodepath = viewpath;
      nodepath += "/config/target-heading-deg-path";
      heading_deg = fgGetDouble(fgGetString(nodepath.c_str()));

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
    char stridx [20];
    string viewpath, nodepath;

    int i = current;
    viewpath = "/sim/view";
    sprintf(stridx, "[%d]", i);
    viewpath += stridx;

    // copy certain view config data for default values
    nodepath = viewpath;
    nodepath += "/config/default-heading-offset-deg";
    fgSetDouble("/sim/current-view/config/heading-offset-deg",
                fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/pitch-offset-deg";
    fgSetDouble("/sim/current-view/config/pitch-offset-deg",
                fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/roll-offset-deg";
    fgSetDouble("/sim/current-view/config/roll-offset-deg",
                fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/default-field-of-view-deg";
    fgSetDouble("/sim/current-view/config/default-field-of-view-deg",
                fgGetDouble(nodepath.c_str()));

    nodepath = viewpath;
    nodepath += "/config/from-model";
    fgSetBool("/sim/current-view/config/from-model",
                fgGetBool(nodepath.c_str()));


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
FGViewMgr::setView (int newview )
{
  // if newview number too low wrap to last view...
  if ( newview < 0 ) {
    newview = (int)views.size() -1;
  }
  // if newview number to high wrap to zero...
  if ( newview > ((int)views.size() -1) ) {
    newview = 0;
  }
  // set new view
  set_view( newview );
  // copy in view data
  copyToCurrent ();

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
