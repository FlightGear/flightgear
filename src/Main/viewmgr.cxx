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

#include <GUI/sgVec3Slider.hxx>	// FIXME: this should NOT be needed

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
  add_view(new FGViewer, 0);
  add_view(new FGViewer, 1);
}

typedef double (FGViewMgr::*double_getter)() const;

void
FGViewMgr::bind ()
{
  fgTie("/sim/view/offset-deg", this,
	&FGViewMgr::getViewOffset_deg, &FGViewMgr::setViewOffset_deg);
  fgSetArchivable("/sim/view/offset-deg");
  fgTie("/sim/view/goal-offset-deg", this,
	&FGViewMgr::getGoalViewOffset_deg, &FGViewMgr::setGoalViewOffset_deg);
  fgSetArchivable("/sim/view/goal-offset-deg");
  fgTie("/sim/view/tilt-deg", this,
	&FGViewMgr::getViewTilt_deg, &FGViewMgr::setViewTilt_deg);
  fgSetArchivable("/sim/view/tilt-deg");
  fgTie("/sim/view/goal-tilt-deg", this,
	&FGViewMgr::getGoalViewTilt_deg, &FGViewMgr::setGoalViewTilt_deg);
  fgSetArchivable("/sim/view/goal-tilt-deg");
  fgTie("/sim/view/pilot/x-offset-m", this,
	&FGViewMgr::getPilotXOffset_m, &FGViewMgr::setPilotXOffset_m);
  fgSetArchivable("/sim/view/pilot/x-offset-m");
  fgTie("/sim/view/pilot/y-offset-m", this,
	&FGViewMgr::getPilotYOffset_m, &FGViewMgr::setPilotYOffset_m);
  fgSetArchivable("/sim/view/pilot/y-offset-m");
  fgTie("/sim/view/pilot/z-offset-m", this,
	&FGViewMgr::getPilotZOffset_m, &FGViewMgr::setPilotZOffset_m);
  fgSetArchivable("/sim/view/pilot/z-offset-m");
  fgTie("/sim/field-of-view", this,
	&FGViewMgr::getFOV_deg, &FGViewMgr::setFOV_deg);
  fgSetArchivable("/sim/field-of-view");
  fgTie("/sim/view/axes/long", this,
	(double_getter)0, &FGViewMgr::setViewAxisLong);
  fgTie("/sim/view/axes/lat", this,
	(double_getter)0, &FGViewMgr::setViewAxisLat);
}

void
FGViewMgr::unbind ()
{
  fgUntie("/sim/view/offset-deg");
  fgUntie("/sim/view/goal-offset-deg");
  fgUntie("/sim/view/tilt-deg");
  fgUntie("/sim/view/goal-tilt-deg");
  fgUntie("/sim/view/pilot/x-offset-m");
  fgUntie("/sim/view/pilot/y-offset-m");
  fgUntie("/sim/view/pilot/z-offset-m");
  fgUntie("/sim/field-of-view");
  fgUntie("/sim/view/axes/long");
  fgUntie("/sim/view/axes/lat");
}

void
FGViewMgr::update (int dt)
{
  FGViewer * view = get_current_view();
  if (view == 0)
    return;

				// Grab some values we'll need.
  double lon_rad = fgGetDouble("/position/longitude-deg")
    * SGD_DEGREES_TO_RADIANS;
  double lat_rad = fgGetDouble("/position/latitude-deg")
    * SGD_DEGREES_TO_RADIANS;
  double alt_m = fgGetDouble("/position/altitude-ft")
    * SG_FEET_TO_METER;
  double roll_rad = fgGetDouble("/orientation/roll-deg")
    * SGD_DEGREES_TO_RADIANS;
  double pitch_rad = fgGetDouble("/orientation/pitch-deg")
    * SGD_DEGREES_TO_RADIANS;
  double heading_rad = fgGetDouble("/orientation/heading-deg")
    * SGD_DEGREES_TO_RADIANS;

				// Set up the pilot view
  FGViewer *pilot_view = (FGViewer *)get_view( 0 );
  pilot_view ->setPosition(
	fgGetDouble("/position/longitude-deg"),
	fgGetDouble("/position/latitude-deg"),
	fgGetDouble("/position/altitude-ft"));
  pilot_view->setOrientation(
	fgGetDouble("/orientation/roll-deg"),
	fgGetDouble("/orientation/pitch-deg"),
	fgGetDouble("/orientation/heading-deg"));
  if (!strcmp(fgGetString("/sim/flight-model"), "ada")) {
    //+ve x is aft, +ve z is up (see viewer.hxx)
    pilot_view->setPositionOffsets( -5.0, 0.0, 1.0 );
  }

				// Set up the chase view

  FGViewer *chase_view = (FGViewer *)get_view( 1 );

  // get xyz Position offsets directly from GUI/sgVec3Slider
  // FIXME: change GUI/sgVec3Slider to store the xyz in properties
  // it would probably be faster than the way PilotOffsetGet()
  // triggers a recalc all the time.
  sgVec3 *pPO = PilotOffsetGet();
  sgVec3 zPO;
  sgCopyVec3( zPO, *pPO );
  chase_view->setPositionOffsets(zPO[0], zPO[1], zPO[2] );

  chase_view->setOrientation(
	fgGetDouble("/orientation/roll-deg"),
	fgGetDouble("/orientation/pitch-deg"),
	fgGetDouble("/orientation/heading-deg"));

  chase_view ->setTargetPosition(
	fgGetDouble("/position/longitude-deg"),
	fgGetDouble("/position/latitude-deg"),
	fgGetDouble("/position/altitude-ft"));
  chase_view->setPositionOffsets(zPO[0], zPO[1], zPO[2] );

				// Update the current view
  do_axes();
  view->update(dt);
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
  if (view != 0)
    view->setHeadingOffset_deg(offset);
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
  if (view != 0)
    view->setPitchOffset_deg(tilt);
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
				// FIXME: hard-coded pilot view position
  const FGViewer * pilot_view = get_view(0);
  if (pilot_view != 0) {
    return ((FGViewer *)pilot_view)->getXOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setPilotXOffset_m (double x)
{
				// FIXME: hard-coded pilot view position
  FGViewer * pilot_view = get_view(0);
  if (pilot_view != 0) {
    pilot_view->setXOffset_m(x);
  }
}

double
FGViewMgr::getPilotYOffset_m () const
{
				// FIXME: hard-coded pilot view position
  const FGViewer * pilot_view = get_view(0);
  if (pilot_view != 0) {
    return ((FGViewer *)pilot_view)->getYOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setPilotYOffset_m (double y)
{
				// FIXME: hard-coded pilot view position
  FGViewer * pilot_view = get_view(0);
  if (pilot_view != 0) {
    pilot_view->setYOffset_m(y);
  }
}

double
FGViewMgr::getPilotZOffset_m () const
{
				// FIXME: hard-coded pilot view position
  const FGViewer * pilot_view = get_view(0);
  if (pilot_view != 0) {
    return ((FGViewer *)pilot_view)->getZOffset_m();
  } else {
    return 0;
  }
}

void
FGViewMgr::setPilotZOffset_m (double z)
{
				// FIXME: hard-coded pilot view position
  FGViewer * pilot_view = get_view(0);
  if (pilot_view != 0) {
    pilot_view->setZOffset_m(z);
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



