// steam.hxx - Steam Gauge Indications
//
// Started by Alex Perry
//
// Copyright (C) 2000  Alexander Perry - alex.perry@ieee.org
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


#ifndef FG_STEAM
#define FG_STEAM


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <Main/fgfs.hxx>

#include <time.h>
#include STL_STRING

SG_USING_NAMESPACE(std);


/**
 * STEAM GAUGES
 *
 * This class is a mapping layer, which retrieves information from the
 * property manager (which reports truthful and ideal values) and
 * generates all the instrument errors and inaccuracies that pilots
 * (err) love, of course.  Please report any missing flaws (!).
 *
 * These should be used to operate cockpit instruments, 
 * and autopilot features where these are slaved thus.
 * They should not be used for other simulation purposes.
 */
class FGSteam : public FGSubsystem
{
public:

  FGSteam ();
  virtual ~FGSteam ();

  virtual void init ();

  virtual void update (double dt_sec);

  virtual void bind ();

  virtual void unbind ();

				// Position
  virtual double get_ALT_ft () const;
  virtual double get_TC_rad () const;
  virtual double get_MH_deg () const;
  virtual double get_DG_deg () const;
  virtual double get_DG_err () const;
  virtual void set_DG_err(double approx_magvar);

				// Velocities
  virtual double get_ASI_kias () const;
  virtual double get_TC_std () const;
  virtual double get_VSI_fps () const;

				// Engine Gauges
  virtual double get_VACUUM_inhg () const;

				// Atmosphere
  virtual double get_ALT_datum_mb () const;
  virtual void set_ALT_datum_mb(double datum_mb);

				// Hacks ... temporary stuff
  // static double get_HackVOR1_deg ();
  virtual double get_HackOBS1_deg () const;
  // static double get_HackGS_deg ();
  // static double get_HackVOR2_deg ();
  virtual double get_HackOBS2_deg () const;


private:

  void	_CatchUp ();
  void	set_lowpass ( double *outthe, double inthe, double tc );

  double the_ALT_ft;
  double the_ALT_datum_mb;
  double the_TC_rad, the_TC_std;
  double the_STATIC_inhg, the_VACUUM_inhg;
  double the_VSI_fps, the_VSI_case;
  double the_MH_deg, the_MH_degps, the_MH_err;
  double the_DG_deg, the_DG_degps, the_DG_inhg, the_DG_err;

  double	_UpdateTimePending;

  SGPropertyNode_ptr _heading_deg_node;
  SGPropertyNode_ptr _mag_var_deg_node;
  SGPropertyNode_ptr _mag_dip_deg_node;
  SGPropertyNode_ptr _engine_0_rpm_node;
  SGPropertyNode_ptr _pressure_inhg_node;

};


#endif // FG_STEAM
