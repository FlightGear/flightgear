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

class SGInterpTable;


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
  FGEnvironment (const FGEnvironment &environment);
  virtual ~FGEnvironment();
  
  virtual double get_visibility_m () const;

  virtual double get_temperature_sea_level_degc () const;
  virtual double get_temperature_degc () const;
  virtual double get_pressure_sea_level_inhg () const;
  virtual double get_pressure_inhg () const;
  virtual double get_density_sea_level_slugft3 () const;
  virtual double get_density_slugft3 () const;

  virtual double get_wind_from_heading_deg () const;
  virtual double get_wind_speed_kt () const;
  virtual double get_wind_from_north_fps () const;
  virtual double get_wind_from_east_fps () const;
  virtual double get_wind_from_down_fps () const;

  virtual void set_visibility_m (double v);

  virtual void set_temperature_sea_level_degc (double t);
  virtual void set_temperature_degc (double t);
  virtual void set_pressure_sea_level_inhg (double p);
  virtual void set_pressure_inhg (double p);
  virtual void set_density_sea_level_slugft3 (double d);
  virtual void set_density_slugft3 (double d);

  virtual void set_wind_from_heading_deg (double h);
  virtual void set_wind_speed_kt (double s);
  virtual void set_wind_from_north_fps (double n);
  virtual void set_wind_from_east_fps (double e);
  virtual void set_wind_from_down_fps (double d);

protected:

  friend class FGEnvironmentMgr;

  virtual double get_elevation_ft () const;
  virtual void set_elevation_ft (double elevation_ft);

private:

  void _setup_tables ();

  void _recalc_hdgspd ();
  void _recalc_ne ();

  void _recalc_sl_temperature ();
  void _recalc_alt_temperature ();
  void _recalc_sl_pressure ();
  void _recalc_alt_pressure ();
  void _recalc_sl_density ();
  void _recalc_alt_density ();

  SGInterpTable * _temperature_degc_table;
  SGInterpTable * _pressure_inhg_table;
  SGInterpTable * _density_slugft3_table;

  double elevation_ft;

  double visibility_m;

				// Atmosphere
  double temperature_sea_level_degc;
  double temperature_degc;
  double pressure_sea_level_inhg;
  double pressure_inhg;
  double density_sea_level_slugft3;
  double density_slugft3;

  double wind_from_heading_deg;
  double wind_speed_kt;

  double wind_from_north_fps;
  double wind_from_east_fps;
  double wind_from_down_fps;

};

#endif // _ENVIRONMENT_HXX
