// parking.cxx - Implementation of a class to manage aircraft parking in
// FlightGear. This code is intended to be used by AI code and
// initial user-startup location selection.
//
// Written by Durk Talsma, started December 2004.
//
// Copyright (C) 2004 Durk Talsma.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

//#include <algorithm>

#include <simgear/compiler.h>

//#include <plib/sg.h>
//#include <plib/ul.h>

//#include <Environment/environment_mgr.hxx>
//#include <Environment/environment.hxx>
//#include <simgear/misc/sg_path.hxx>
//#include <simgear/props/props.hxx>
//#include <simgear/structure/subsystem_mgr.hxx>
//#include <simgear/debug/logstream.hxx>
//#include <Main/globals.hxx>
//#include <Main/fg_props.hxx>
//#include <Airports/runways.hxx>

#include STL_STRING

#include "parking.hxx"


/*****************************************************************************
 * Helper function for parsing position string
 ****************************************************************************/
double processPosition(const string &pos)
{
  string prefix;
  string subs;
  string degree;
  string decimal;
  int sign = 1;
  double value;
  subs = pos;
  prefix= subs.substr(0,1);
  if (prefix == string("S") || (prefix == string("W")))
    sign = -1;
  subs    = subs.substr(1, subs.length());
  degree  = subs.substr(0, subs.find(" ",0));
  decimal = subs.substr(subs.find(" ",0), subs.length());
  
	      
  //cerr << sign << " "<< degree << " " << decimal << endl;
  value = sign * (atof(degree.c_str()) + atof(decimal.c_str())/60.0);
  //cerr << value <<endl;
  //exit(1);
  return value;
}


/*********************************************************************************
 * FGParking
 ********************************************************************************/
FGParking::FGParking(double lat,
		     double lon,
		     double hdg,
		     double rad,
		     int idx,
		     const string &name,
		     const string &tpe,
		     const string &codes)
{
  latitude     = lat;
  longitude    = lon;
  heading      = hdg;
  parkingName  = name;
  index        = idx;
  type         = tpe;
  airlineCodes = codes;
}
