// ephemeris.cxx -- wrap SGEphemeris code in a subsystem
//
// Written by James Turner, started June 2010.
//
// Copyright (C) 2010  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <Environment/ephemeris.hxx>

#include <simgear/timing/sg_time.hxx>
#include <simgear/ephemeris/ephemeris.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

Ephemeris::Ephemeris() :
  _impl(NULL),
  _latProp(NULL)
{
    SGPath ephem_data_path(globals->get_fg_root());
    ephem_data_path.append("Astro");
    _impl = new SGEphemeris(ephem_data_path.c_str());
    globals->set_ephem(_impl);
}

Ephemeris::~Ephemeris()
{
  delete _impl;
}

void Ephemeris::init()
{
  _latProp = fgGetNode("/position/latitude-deg", true);
  update(0.0);
}

void Ephemeris::postinit()
{
  
}

static void tieStar(const char* prop, Star* s, double (Star::*getter)() const)
{
  fgGetNode(prop, true)->tie(SGRawValueMethods<Star, double>(*s, getter, NULL));
} 

void Ephemeris::bind()
{
  tieStar("/ephemeris/sun/xs", _impl->get_sun(), &Star::getxs);
  tieStar("/ephemeris/sun/ys", _impl->get_sun(), &Star::getys);
  tieStar("/ephemeris/sun/ze", _impl->get_sun(), &Star::getze);
  tieStar("/ephemeris/sun/ye", _impl->get_sun(), &Star::getye);
  
  tieStar("/ephemeris/sun/lat-deg", _impl->get_sun(), &Star::getLat);
}

void Ephemeris::unbind()
{
}

void Ephemeris::update(double)
{
  SGTime* st = globals->get_time_params();
  _impl->update(st->getMjd(), st->getLst(), _latProp->getDoubleValue());
}
