// viewmgr.cxx -- class for managing all the views in the flightgear world.
//
// Written by Curtis Olson, started October 2000.
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
  add_view(new FGViewerRPH);
  add_view(new FGViewerLookAt);
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
  FGViewerRPH *pilot_view = (FGViewerRPH *)get_view( 0 );
  pilot_view ->set_geod_view_pos(lon_rad, lat_rad, alt_m);
  pilot_view->set_rph(roll_rad, pitch_rad, heading_rad);
  if (fgGetString("/sim/flight-model") == "ada") {
    //+ve x is aft, +ve z is up (see viewer.hxx)
    pilot_view->set_pilot_offset( -5.0, 0.0, 1.0 ); 
  }

				// Set up the chase view

				// FIXME: the matrix math belongs in
				// the viewer, not here.
  FGViewerLookAt *chase_view = (FGViewerLookAt *)get_view( 1 );

  sgVec3 po;		// chase view pilot_offset
  sgVec3 wup;		// chase view world up
  sgSetVec3( po, 0.0, 0.0, 100.0 );
  sgCopyVec3( wup, pilot_view->get_world_up() );
  sgMat4 CXFM;		// chase view + pilot offset xform
  sgMakeRotMat4( CXFM,
		 chase_view->get_view_offset() * SGD_RADIANS_TO_DEGREES -
		 heading_rad * SGD_RADIANS_TO_DEGREES,
		 wup );
  sgVec3 npo;		// new pilot offset after rotation
  sgVec3 *pPO = PilotOffsetGet();
  sgXformVec3( po, *pPO, pilot_view->get_UP() );
  sgXformVec3( npo, po, CXFM );

  chase_view->set_geod_view_pos(lon_rad, lat_rad, alt_m);
  chase_view->set_pilot_offset( npo[0], npo[1], npo[2] );
  chase_view->set_view_forward( pilot_view->get_view_pos() ); 
  chase_view->set_view_up( wup );

				// Update the current view
  do_axes();
  view->update(dt);
}

double
FGViewMgr::getViewOffset_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->get_view_offset() * SGD_RADIANS_TO_DEGREES);
}

void
FGViewMgr::setViewOffset_deg (double offset)
{
  FGViewer * view = get_current_view();
  if (view != 0)
    view->set_view_offset(offset * SGD_DEGREES_TO_RADIANS);
}

double
FGViewMgr::getGoalViewOffset_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->get_goal_view_offset() * SGD_RADIANS_TO_DEGREES);
}

void
FGViewMgr::setGoalViewOffset_deg (double offset)
{
  FGViewer * view = get_current_view();
  if (view != 0)
    view->set_goal_view_offset(offset * SGD_DEGREES_TO_RADIANS);
}

double
FGViewMgr::getViewTilt_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->get_view_tilt() * SGD_RADIANS_TO_DEGREES);
}

void
FGViewMgr::setViewTilt_deg (double tilt)
{
  FGViewer * view = get_current_view();
  if (view != 0)
    view->set_view_tilt(tilt * SGD_DEGREES_TO_RADIANS);
}

double
FGViewMgr::getGoalViewTilt_deg () const
{
  const FGViewer * view = get_current_view();
  return (view == 0 ? 0 : view->get_goal_view_tilt() * SGD_RADIANS_TO_DEGREES);
}

void
FGViewMgr::setGoalViewTilt_deg (double tilt)
{
  FGViewer * view = get_current_view();
  if (view != 0)
    view->set_goal_view_tilt(tilt * SGD_DEGREES_TO_RADIANS);
}

double
FGViewMgr::getPilotXOffset_m () const
{
				// FIXME: hard-coded pilot view position
  const FGViewer * pilot_view = get_view(0);
  if (pilot_view != 0) {
    float * offset = ((FGViewer *)pilot_view)->get_pilot_offset();
    return offset[0];
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
    float * offset = pilot_view->get_pilot_offset();
    pilot_view->set_pilot_offset(x, offset[1], offset[2]);
  }
}

double
FGViewMgr::getPilotYOffset_m () const
{
				// FIXME: hard-coded pilot view position
  const FGViewer * pilot_view = get_view(0);
  if (pilot_view != 0) {
    float * offset = ((FGViewer *)pilot_view)->get_pilot_offset();
    return offset[1];
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
    float * offset = pilot_view->get_pilot_offset();
    pilot_view->set_pilot_offset(offset[0], y, offset[2]);
  }
}

double
FGViewMgr::getPilotZOffset_m () const
{
				// FIXME: hard-coded pilot view position
  const FGViewer * pilot_view = get_view(0);
  if (pilot_view != 0) {
    float * offset = ((FGViewer *)pilot_view)->get_pilot_offset();
    return offset[2];
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
    float * offset = pilot_view->get_pilot_offset();
    pilot_view->set_pilot_offset(offset[0], offset[1], z);
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

  get_current_view()->set_goal_view_offset(viewDir*SGD_DEGREES_TO_RADIANS);
}
