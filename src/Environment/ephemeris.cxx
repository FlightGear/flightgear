#include <Environment/ephemeris.hxx>

#include <simgear/timing/sg_time.hxx>
#include <simgear/ephemeris/ephemeris.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

Ephemeris::Ephemeris() :
  _impl(NULL),
  _latProp(NULL)
{
}

Ephemeris::~Ephemeris()
{
  delete _impl;
}

void Ephemeris::init()
{
  if (_impl) {
    return;
  }
  
  SGPath ephem_data_path(globals->get_fg_root());
  ephem_data_path.append("Astro");
  _impl = new SGEphemeris(ephem_data_path.c_str());
  globals->set_ephem(_impl);
}

void Ephemeris::postinit()
{
  update(0.0);
}

static void tieStar(const char* prop, Star* s, double (Star::*getter)() const)
{
  fgGetNode(prop, true)->tie(SGRawValueMethods<Star, double>(*s, getter, NULL));
} 

void Ephemeris::bind()
{
  _latProp = fgGetNode("/position/latitude-deg", true);
  
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
