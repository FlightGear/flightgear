// environment.hxx -- routines to model the natural environment.
//
// Written by David Megginson, started February 2002.
//
// Copyright (C) 2002  David Megginson - david@megginson.com
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


#ifndef _ENVIRONMENT_HXX
#define _ENVIRONMENT_HXX

#include <simgear/compiler.h>

#include <Main/fgfs.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <cmath>
#else
#  include <math.h>
#endif


/**
 * Model the natural environment.
 *
 * This class models the natural environment at a specific place and
 * time.  A separate instance is necessary for each location or time.
 *
 * This class should eventually move to SimGear.
 */
class FGEnvironment
{

public:

  FGEnvironment();
  virtual ~FGEnvironment();
  
  inline virtual double get_visibility_m () const { return visibility_m; }
  inline virtual double get_wind_from_heading_deg () const {
    return wind_from_heading_deg;
  }
  inline virtual double get_wind_speed_kt () const { return wind_speed_kt; }
  inline virtual double get_wind_from_north_fps () const {
    return wind_from_north_fps;
  }
  inline virtual double get_wind_from_east_fps () const {
    return wind_from_east_fps;
  }
  inline virtual double get_wind_from_down_fps () const {
    return wind_from_down_fps;
  }

  virtual void set_visibility_m (double v);
  virtual void set_wind_from_heading_deg (double h);
  virtual void set_wind_speed_kt (double s);
  virtual void set_wind_from_north_fps (double n);
  virtual void set_wind_from_east_fps (double e);
  virtual void set_wind_from_down_fps (double d);

private:

  void _recalc_hdgspd ();
  void _recalc_ne ();

  double visibility_m;

  double wind_from_heading_deg;
  double wind_speed_kt;

  double wind_from_north_fps;
  double wind_from_east_fps;
  double wind_from_down_fps;

				// Do these belong here?
#if 0
  GLfloat fog_exp_density;
  GLfloat fog_exp2_density;
#endif

};

#endif // _ENVIRONMENT_HXX
