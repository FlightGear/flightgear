// environment_ctrl.cxx -- manager for natural environment information.
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

#include <simgear/debug/logstream.hxx>

#include <stdlib.h>
#include <algorithm>

#include <Main/fg_props.hxx>

#include "environment_ctrl.hxx"

SG_USING_STD(sort);



////////////////////////////////////////////////////////////////////////
// Implementation of FGEnvironmentCtrl abstract base class.
////////////////////////////////////////////////////////////////////////

FGEnvironmentCtrl::FGEnvironmentCtrl ()
  : _environment(0),
    _lon_deg(0),
    _lat_deg(0),
    _elev_ft(0)
{
}

FGEnvironmentCtrl::~FGEnvironmentCtrl ()
{
}

void
FGEnvironmentCtrl::setEnvironment (FGEnvironment * environment)
{
  _environment = environment;
}

void
FGEnvironmentCtrl::setLongitudeDeg (double lon_deg)
{
  _lon_deg = lon_deg;
}

void
FGEnvironmentCtrl::setLatitudeDeg (double lat_deg)
{
  _lat_deg = lat_deg;
}

void
FGEnvironmentCtrl::setElevationFt (double elev_ft)
{
  _elev_ft = elev_ft;
}

void
FGEnvironmentCtrl::setPosition (double lon_deg, double lat_deg, double elev_ft)
{
  _lon_deg = lon_deg;
  _lat_deg = lat_deg;
  _elev_ft = elev_ft;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGUserDefEnvironmentCtrl.
////////////////////////////////////////////////////////////////////////

FGUserDefEnvironmentCtrl::FGUserDefEnvironmentCtrl ()
  : _base_wind_speed_node(0),
    _gust_wind_speed_node(0),
    _current_wind_speed_kt(0),
    _delta_wind_speed_kt(0)
{
}

FGUserDefEnvironmentCtrl::~FGUserDefEnvironmentCtrl ()
{
}

void
FGUserDefEnvironmentCtrl::init ()
{
				// Fill in some defaults.
  if (!fgHasNode("/environment/params/base-wind-speed-kt"))
    fgSetDouble("/environment/params/base-wind-speed-kt",
		fgGetDouble("/environment/wind-speed-kt"));
  if (!fgHasNode("/environment/params/gust-wind-speed-kt"))
    fgSetDouble("/environment/params/gust-wind-speed-kt",
		fgGetDouble("/environment/params/base-wind-speed-kt"));

  _base_wind_speed_node =
    fgGetNode("/environment/params/base-wind-speed-kt", true);
  _gust_wind_speed_node =
    fgGetNode("/environment/params/gust-wind-speed-kt", true);

  _current_wind_speed_kt = _base_wind_speed_node->getDoubleValue();
  _delta_wind_speed_kt = 0.1;
}

void
FGUserDefEnvironmentCtrl::update (double dt)
{
  double base_wind_speed = _base_wind_speed_node->getDoubleValue();
  double gust_wind_speed = _gust_wind_speed_node->getDoubleValue();

  if (gust_wind_speed < base_wind_speed) {
      gust_wind_speed = base_wind_speed;
      _gust_wind_speed_node->setDoubleValue(gust_wind_speed);
  }

  if (base_wind_speed == gust_wind_speed) {
    _current_wind_speed_kt = base_wind_speed;
  } else {
    int rn = rand() % 128;
    int sign = (_delta_wind_speed_kt < 0 ? -1 : 1);
    double gust = _current_wind_speed_kt - base_wind_speed;
    double incr = gust / 50;

    if (rn == 0)
      _delta_wind_speed_kt = - _delta_wind_speed_kt;
    else if (rn < 4)
      _delta_wind_speed_kt -= incr * sign;
    else if (rn < 16)
      _delta_wind_speed_kt += incr * sign;

    _current_wind_speed_kt += _delta_wind_speed_kt;

    if (_current_wind_speed_kt < base_wind_speed) {
      _current_wind_speed_kt = base_wind_speed;
      _delta_wind_speed_kt = 0.01;
    } else if (_current_wind_speed_kt > gust_wind_speed) {
      _current_wind_speed_kt = gust_wind_speed;
      _delta_wind_speed_kt = -0.01;
    }
  }
  
  if (_environment != 0)
    _environment->set_wind_speed_kt(_current_wind_speed_kt);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInterpolateEnvironmentCtrl.
////////////////////////////////////////////////////////////////////////

FGInterpolateEnvironmentCtrl::FGInterpolateEnvironmentCtrl ()
{
}

FGInterpolateEnvironmentCtrl::~FGInterpolateEnvironmentCtrl ()
{
    int i;
    for (i = 0; i < _boundary_table.size(); i++)
        delete _boundary_table[i];
    for (i = 0; i < _aloft_table.size(); i++)
        delete _aloft_table[i];
}



void
FGInterpolateEnvironmentCtrl::init ()
{
    read_table(fgGetNode("/environment/config/boundary", true),
               _boundary_table);
    read_table(fgGetNode("/environment/config/aloft", true),
               _aloft_table);
}

void
FGInterpolateEnvironmentCtrl::reinit ()
{
    int i;
    for (i = 0; i < _boundary_table.size(); i++)
        delete _boundary_table[i];
    for (i = 0; i < _aloft_table.size(); i++)
        delete _aloft_table[i];
    _boundary_table.clear();
    _aloft_table.clear();
    init();
}

void
FGInterpolateEnvironmentCtrl::read_table (const SGPropertyNode * node,
                                          vector<bucket *> &table)
{
    for (int i = 0; i < node->nChildren(); i++) {
        const SGPropertyNode * child = node->getChild(i);
        if (!strcmp(child->getName(), "entry") &&
            child->getStringValue("elevation-ft", "")[0] != '\0') {
            bucket * b = new bucket;
            if (i > 0)
                b->environment.copy(table[i-1]->environment);
            b->environment.read(child);
            b->altitude_ft = b->environment.get_elevation_ft();
            table.push_back(b);
        }
    }
    sort(table.begin(), table.end());
}

void
FGInterpolateEnvironmentCtrl::update (double delta_time_sec)
{
                                // FIXME
    double altitude_ft = fgGetDouble("/position/altitude-ft");
    double altitude_agl_ft = fgGetDouble("/position/altitude-agl-ft");
    double boundary_transition =
        fgGetDouble("/environment/config/boundary-transition-ft", 500);

    double ground_elevation_ft = altitude_ft - altitude_agl_ft;

    int length = _boundary_table.size();

    if (length > 0) {
                                // boundary table
        double boundary_limit = _boundary_table[length-1]->altitude_ft;
        if (boundary_limit >= altitude_agl_ft) {
            do_interpolate(_boundary_table, altitude_agl_ft,
                           _environment);
            return;
        } else if ((boundary_limit + boundary_transition) >= altitude_agl_ft) {
                                // both tables
            do_interpolate(_boundary_table, altitude_agl_ft, &env1);
            do_interpolate(_aloft_table, altitude_ft, &env2);
            double fraction =
                (altitude_agl_ft - boundary_limit) / boundary_transition;
            interpolate(&env1, &env2, fraction, _environment);
            return;
        }
    }

                                // aloft table
    do_interpolate(_aloft_table, altitude_ft, _environment);
}

void
FGInterpolateEnvironmentCtrl::do_interpolate (vector<bucket *> &table,
                                              double altitude_ft,
                                              FGEnvironment * environment)
{
    int length = table.size();
    if (length == 0)
        return;

                                // Boundary conditions
    if ((length == 1) || (table[0]->altitude_ft >= altitude_ft)) {
        environment->copy(table[0]->environment);
        return;
    } else if (table[length-1]->altitude_ft <= altitude_ft) {
        environment->copy(table[length-1]->environment);
        return;
    }
        
                                // Search the interpolation table
    for (int i = 0; i < length - 1; i++) {
        if ((i == length - 1) || (table[i]->altitude_ft <= altitude_ft)) {
                FGEnvironment * env1 = &(table[i]->environment);
                FGEnvironment * env2 = &(table[i+1]->environment);
                double fraction;
                if (table[i]->altitude_ft == table[i+1]->altitude_ft)
                    fraction = 1.0;
                else
                    fraction =
                        ((altitude_ft - table[i]->altitude_ft) /
                         (table[i+1]->altitude_ft - table[i]->altitude_ft));
                interpolate(env1, env2, fraction, environment);

                return;
        }
    }
}

bool
FGInterpolateEnvironmentCtrl::bucket::operator< (const bucket &b) const
{
    return (altitude_ft < b.altitude_ft);
}


// end of environment_ctrl.cxx
