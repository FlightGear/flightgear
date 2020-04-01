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

static void tieStar(const char* prop, Star* s, double (Star::*getter)() const)
{
  fgGetNode(prop, true)->tie(SGRawValueMethods<Star, double>(*s, getter, NULL));
}

static void tieMoonPos(const char* prop, MoonPos* s, double (MoonPos::*getter)() const)
{
  fgGetNode(prop, true)->tie(SGRawValueMethods<MoonPos, double>(*s, getter, NULL));
}

Ephemeris::Ephemeris()
{
}

Ephemeris::~Ephemeris()
{
}

SGEphemeris* Ephemeris::data()
{
    return _impl.get();
}

void Ephemeris::init()
{
  SGPath ephem_data_path(globals->get_fg_root());
  ephem_data_path.append("Astro");
  _impl.reset(new SGEphemeris(ephem_data_path));

  tieStar("/ephemeris/sun/xs", _impl->get_sun(), &Star::getxs);
  tieStar("/ephemeris/sun/ys", _impl->get_sun(), &Star::getys);
  tieStar("/ephemeris/sun/ze", _impl->get_sun(), &Star::getze);
  tieStar("/ephemeris/sun/ye", _impl->get_sun(), &Star::getye);
  tieStar("/ephemeris/sun/lat-deg", _impl->get_sun(), &Star::getLat);

  tieMoonPos("/ephemeris/moon/xg", _impl->get_moon(), &MoonPos::getxg);
  tieMoonPos("/ephemeris/moon/yg", _impl->get_moon(), &MoonPos::getyg);
  tieMoonPos("/ephemeris/moon/ze", _impl->get_moon(), &MoonPos::getze);
  tieMoonPos("/ephemeris/moon/ye", _impl->get_moon(), &MoonPos::getye);
  tieMoonPos("/ephemeris/moon/lat-deg", _impl->get_moon(), &MoonPos::getLat);
  tieMoonPos("/ephemeris/moon/age", _impl->get_moon(), &MoonPos::getAge);
  tieMoonPos("/ephemeris/moon/phase", _impl->get_moon(), &MoonPos::getPhase);

    _latProp = fgGetNode("/position/latitude-deg", true);

    _moonlight = fgGetNode("/environment/moonlight", true);

    update(0.0);
}

void Ephemeris::shutdown()
{
    _impl.reset();
}

void Ephemeris::postinit()
{
}

void Ephemeris::bind()
{
}

void Ephemeris::unbind()
{
    _latProp = 0;
    _latProp.reset();
    _moonlight.reset();
}

void Ephemeris::update(double)
{
    SGTime* st = globals->get_time_params();
    _impl->update(st->getMjd(), st->getLst(), _latProp->getDoubleValue());

    // Update the moonlight intensity.
    _moonlight->setDoubleValue(_impl->get_moon()->getIlluminanceFactor());
}


// Register the subsystem.
SGSubsystemMgr::Registrant<Ephemeris> registrantEphemeris;
