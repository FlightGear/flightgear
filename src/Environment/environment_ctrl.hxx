// environment-ctrl.hxx -- controller for environment information.
//
// Written by David Megginson, started May 2002.
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

#ifndef _ENVIRONMENT_CTRL_HXX
#define _ENVIRONMENT_CTRL_HXX

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <cmath>
#else
#  include <math.h>
#endif

#include <vector>

SG_USING_STD(vector);

class SGPropertyNode;

#include "environment.hxx"



/**
 * Interface to control environment information for a specific location.
 */
class FGEnvironmentCtrl : public SGSubsystem
{

public:

  FGEnvironmentCtrl ();
  virtual ~FGEnvironmentCtrl ();

  virtual void setEnvironment (FGEnvironment * environment);

  virtual FGEnvironment * getEnvironment () const { return _environment; }

  virtual void setLongitudeDeg (double lon_deg);
  virtual void setLatitudeDeg (double lat_deg);
  virtual void setElevationFt (double elev_ft);
  virtual void setPosition (double lon_deg, double lat_deg, double elev_ft);

  virtual double getLongitudeDeg () const { return _lon_deg; }
  virtual double getLatitudeDeg () const { return _lat_deg; }
  virtual double getElevationFt () const { return _elev_ft; }

protected:

  FGEnvironment * _environment;
  double _lon_deg;
  double _lat_deg;
  double _elev_ft;

};



/**
 * Environment controller using user-supplied parameters.
 */
class FGUserDefEnvironmentCtrl : public FGEnvironmentCtrl
{
public:
  FGUserDefEnvironmentCtrl ();
  virtual ~FGUserDefEnvironmentCtrl ();

  virtual void init ();
  virtual void update (double dt);

private:

  SGPropertyNode * _base_wind_speed_node;
  SGPropertyNode * _gust_wind_speed_node;

  double _current_wind_speed_kt;
  double _delta_wind_speed_kt;

};



/**
 * Interplation controller using user-supplied parameters.
 */
class FGInterpolateEnvironmentCtrl : public FGEnvironmentCtrl
{
public:
    FGInterpolateEnvironmentCtrl ();
    virtual ~FGInterpolateEnvironmentCtrl ();
    
    virtual void init ();
    virtual void reinit ();
    virtual void update (double delta_time_sec);

private:
    
    struct bucket {
        double altitude_ft;
        FGEnvironment environment;
        bool operator< (const bucket &b) const;
    };

    void read_table (const SGPropertyNode * node, vector<bucket *> &table);
    void do_interpolate (vector<bucket *> &table, double altitude_ft,
                         FGEnvironment * environment);

    FGEnvironment env1, env2;   // temporaries

    vector<bucket *> _boundary_table;
    vector<bucket *> _aloft_table;
};


#endif // _ENVIRONMENT_CTRL_HXX
