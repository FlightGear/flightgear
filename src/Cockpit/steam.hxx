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

#include <time.h>
#include STL_STRING

FG_USING_NAMESPACE(std);


/**
 * STEAM GAUGES
 *
 * This class is a mapping layer, which retrieves information from
 * the BFI (which reports truthful and ideal values) and generates
 * all the instrument errors and inaccuracies that pilots (err)
 * love, of course.  Please report any missing flaws (!).
 *
 * These should be used to operate cockpit instruments, 
 * and autopilot features where these are slaved thus.
 * They should not be used for other simulation purposes.
 *
 */
class FGSteam
{
public:

  static void update ( int timesteps );

				// Position
  static double get_ALT_ft ();
  static double get_TC_rad ();
  static double get_MH_deg ();
  static double get_DG_deg ();
  static double get_DG_err ();
  static void set_DG_err(double approx_magvar);

				// Velocities
  static double get_ASI_kias ();
  static double get_TC_std ();
  static double get_VSI_fps ();

				// Engine Gauges
  static double get_VACUUM_inhg ();

				// Atmosphere
  static double get_ALT_datum_mb ();
  static void set_ALT_datum_mb(double datum_mb);

				// Hacks ... temporary stuff
  // static double get_HackVOR1_deg ();
  static double get_HackOBS1_deg ();
  // static double get_HackGS_deg ();
  // static double get_HackVOR2_deg ();
  static double get_HackOBS2_deg ();
  static double get_HackADF_deg ();


private:
	static double	the_ALT_ft;
        static double	the_ALT_datum_mb;
        static double   the_TC_rad, the_TC_std;
	static double	the_STATIC_inhg, the_VACUUM_inhg;
	static double	the_VSI_fps, the_VSI_case;
        static double   the_MH_deg, the_MH_degps, the_MH_err;
        static double   the_DG_deg, the_DG_degps, the_DG_inhg, the_DG_err;

	static int	_UpdatesPending;
	static void	_CatchUp ();

	static void	set_lowpass ( double *outthe, 
				double inthe, double tc );
};


#endif // FG_STEAM
