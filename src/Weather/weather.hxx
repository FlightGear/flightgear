// weather.hxx -- routines to model weather
//
// Written by Curtis Olson, started July 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@me.umn.edu
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


#ifndef _WEATHER_HXX
#define _WEATHER_HXX


#include <simgear/compiler.h>

#include <GL/gl.h>

#include <Main/fgfs.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <cmath>
#else
#  include <math.h>
#endif

// holds the current weather values
class FGWeather : public FGSubsystem
{

public:

  FGWeather();
  virtual ~FGWeather();
  
  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (int dt);
    
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
  GLfloat fog_exp_density;
  GLfloat fog_exp2_density;

};

extern FGWeather current_weather;

#endif // _WEATHER_HXX
