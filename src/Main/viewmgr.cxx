// viewmgr.cxx -- class for managing all the views in the flightgear world.
//
// Written by Curtis Olson, started October 2000.
//   partially rewritten by Jim Wilson March 2002
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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

#include "viewmgr.hxx"
#include "fg_props.hxx"

// strings 
string viewpath, nodepath, strdata;

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
  bool from_model = false;
  bool at_model = false;
  int from_model_index = 0;
  int at_model_index = 0;
  double x_offset_m, y_offset_m, z_offset_m;
  double near_m;

  for (int i = 0; i < fgGetInt("/sim/number-views"); i++) {
    viewpath = "/sim/view";
    sprintf(stridx, "[%d]", i);
    viewpath += stridx;

    // find out what type of view this is...
    nodepath = viewpath;
    nodepath += "/type";
    strdata = fgGetString(nodepath.c_str());

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
    nodepath += "/config/ground-level-nearplane-m";
    near_m = fgGetDouble(nodepath.c_str());

    // supporting two types now "lookat" = 1 and "lookfrom" = 0
    if ( strcmp("lookat",strdata.c_str()) == 0 )
      add_view(new FGViewer ( FG_LOOKAT, from_model, from_model_index,
                              at_model, at_model_index, x_offset_m, y_offset_m,
                              z_offset_m, near_m ));
    else
      add_view(new FGViewer ( FG_LOOKFROM, from_model, from_model_index, false,
                              0, x_offset_m, y_offset_m, z_offset_m, near_m ));

  }

  copyToCurrent();
  
}

typedef double (FGViewMgr::*double_getter)() const;

void
FGViewMgr::bind ()
{
  // these are bound to the current view properties
  fgTie("/sim/current-view/heading-offset-deg", this,
	&FGViewMgr::getViewOffset_deg, &FGViewMgr::setViewOffset_deg);
  fgSetArchivable("/sim/current-view/heading-offset-deg");
  fgTie("/sim/current-view/goal-heading-offset-deg", this,
	&FGViewMgr::getGoalViewOffset_deg, &FGViewMgr::setGoalViewOffset_deg);
  fgSetArchivable("/sim/current-view/goal-heading-offset-deg");
  fgTie("/sim/current-view/pitch-offset-deg", this,
	&FGViewMgr::getViewTilt_deg, &FGViewMgr::setViewTilt_deg);
  fgSetArchivable("/sim/current-view/pitch-offset-deg");
  fgTie("/sim/current-view/goal-pitch-offset-deg", this,
	&FGViewMgr::getGoalViewTilt_deg, &FGViewMgr::setGoalViewTilt_deg);
  fgSetArchivable("/sim/current-view/goal-pitch-offset-deg");

  fgTie("/sim/current-view/axes/long", this,
	(double_getter)0, &FGViewMgr::setViewAxisLong);
  fgTie("/sim/current-view/axes/lat", this,
	(double_getter)0, &FGViewMgr::setViewAxisLat);

  fgTie("/sim/current-view/field-of-view", this,
	&FGViewMgr::getFOV_deg, &FGViewMgr::setFOV_deg);
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
  fgUntie("/sim/field-of-view");
  fgUntie("/sim/current-view/axes/long");
  fgUntie("/sim/current-view/axes/lat");
}

void
FGViewMgr::update (int dt)
{
  char stridx [20];
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
  if (loop_view->getType() == 1) {
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
     
      loop_view ->setTargetPosition(lon_deg, lat_deg, alt_ft);
      loop_view->setTargetOrientation(roll_deg, pitch_deg, heading_deg);
    } else {
      loop_view->set_dirty();
    }
  }
  setPilotXOffset_m(fgGetDouble("/sim/current-view/x-offset-m"));
  setPilotYOffset_m(fgGetDouble("/sim/current-view/y-offset-m"));
  setPilotZOffset_m(fgGetDouble("/sim/current-view/z-offset-m"));

  			        // Update the current view
  do_axes();
  view->update(dt);
  double tmp;
}

void
FGViewMgr::copyToCurrent()
{
    fgSetDouble("/sim/current-view/x-offset-m", getPilotXOffset_m());
    fgSetDouble("/sim/current-view/y-offset-m", getPilotYOffset_m());
    fgSetDouble("/sim/current-view/z-offset-m", getPilotZOffset_m());
}


double
FGViewMgr::getViewOffset_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->getHeadingOffset_deg());
}

void
FGViewMgr::setViewOffset_deg (double offset)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setGoalHeadingOffset_deg(offset);
    view->setHeadingOffset_deg(offset);
  }
}

double
FGViewMgr::getGoalViewOffset_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->getGoalHeadingOffset_deg());
}

void
FGViewMgr::setGoalViewOffset_deg (double offset)
{
  FGViewer * view = get_current_view();
  if (view != 0)
    view->setGoalHeadingOffset_deg(offset);
}

double
FGViewMgr::getViewTilt_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->getPitchOffset_deg());
}

void
FGViewMgr::setViewTilt_deg (double tilt)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setGoalPitchOffset_deg(tilt);
    view->setPitchOffset_deg(tilt);
  }
}

double
FGViewMgr::getGoalViewTilt_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->getGoalPitchOffset_deg());
}

void
FGViewMgr::setGoalViewTilt_deg (double tilt)
{
  FGViewer * view = get_current_view();
  if (view != 0)
    view->setGoalPitchOffset_deg(tilt);
}

double
FGViewMgr::getPilotXOffset_m () const
{
  const FGViewer * view = get_current_view();
  if (view != 0) {
    return ((FGViewer *)view)->getXOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setPilotXOffset_m (double x)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setXOffset_m(x);
  }
}

double
FGViewMgr::getPilotYOffset_m () const
{
  const FGViewer * view = get_current_view();
  if (view != 0) {
    return ((FGViewer *)view)->getYOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setPilotYOffset_m (double y)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setYOffset_m(y);
  }
}

double
FGViewMgr::getPilotZOffset_m () const
{
  const FGViewer * view = get_current_view();
  if (view != 0) {
    return ((FGViewer *)view)->getZOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setPilotZOffset_m (double z)
{
  FGViewer * view = get_current_view();
  if (view != 0) {
    view->setZOffset_m(z);
  }
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
      viewDir = 45;
    else if (axis_lat == - axis_long)
      viewDir = 315;
    else if (axis_lat == 0)
      viewDir = 0;
  } else if (axis_long > 0) {	// Longitudinal axis backward
    if (axis_lat == - axis_long)
      viewDir = 135;
    else if (axis_lat == axis_long)
      viewDir = 225;
    else if (axis_lat == 0)
      viewDir = 180;
  } else if (axis_long == 0) {	// Longitudinal axis neutral
    if (axis_lat < 0)
      viewDir = 90;
    else if (axis_lat > 0)
      viewDir = 270;
    else return; /* And assertion failure maybe? */
  }

				// Do all the difficult cases
  if ( viewDir > 900 )
    viewDir = SGD_RADIANS_TO_DEGREES * atan2 ( -axis_lat, -axis_long );
  if ( viewDir < -1 ) viewDir += 360;

  get_current_view()->setGoalHeadingOffset_deg(viewDir);
}
