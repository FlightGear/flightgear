// altimeter.cxx - an altimeter and/or encoder tied to the static port.
// Written by David Megginson, started 2002.
// Modified by John Denker in 2007 to use a two layer atmosphere
// model in src/Environment/atmosphere.?xx
//
// This file is in the Public Domain and comes with no warranty.

// Example invocation, in the instrumentation.xml file:
//      <altimeter>
//        <name>encoder</name>
//        <number>0</number>
//        <static-pressure>/systems/static/pressure-inhg</static-pressure>
//        <quantum>10</quantum>
//        <tau>0</tau>
//      </altimeter>
// Note the non-default name, quantum, and tau values.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/interpolater.hxx>
#include <simgear/math/SGMath.hxx>
#include <boost/format.hpp>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <Environment/atmosphere.hxx>

#include "altimeter.hxx"

// Constructor
Altimeter::Altimeter ( SGPropertyNode *node, double quantum )
    : _name(node->getStringValue("name", "altimeter")),
      _num(node->getIntValue("number", 0)),
      _static_source(node->getStringValue("static-pressure", "/systems/static/pressure-inhg")),
      _tau(node->getDoubleValue("tau", 0.1)),
      _quantum(node->getDoubleValue("quantum", quantum)),
      _serviceable(1),
      _setting(29.921260),
      _kollsman(0.),            // corresponds to _setting(29.921260)
      _filtered_PA(0.),
      _altitude(0.),
      _press_alt(0.),
      _mode_c_alt(0.)
      /* We assume somebody else has initialized the static pressure node. */
      /* We use it read-only. */
{}

Altimeter::~Altimeter ()
{}

void
Altimeter::init ()
{
    _static_pressure_node = fgGetNode(_static_source.c_str(), true);
}

void
Altimeter::bind ()
{
    string branch = str(boost::format("/instrumentation/%s[%d]") 
        % _name % _num);

    SG_LOG( SG_COCKPIT, SG_DEBUG, "Altimeter binding to: " << branch);

    fgTie((branch + "/serviceable").c_str(), this,
      &Altimeter::get_serviceable, &Altimeter::set_serviceable);

    fgTie((branch + "/setting-inhg").c_str(), this,
      &Altimeter::get_setting, &Altimeter::set_setting);

    fgTie((branch + "/pressure-alt-ft").c_str(), this,
      &Altimeter::get_press_alt, &Altimeter::set_press_alt);

    fgTie((branch + "/mode-c-alt-ft").c_str(), this,
      &Altimeter::get_mode_c, &Altimeter::set_mode_c);

    fgTie((branch + "/indicated-altitude-ft").c_str(), this,
      &Altimeter::get_altitude, &Altimeter::set_altitude);
}

void
Altimeter::unbind ()
{
    string branch = str(boost::format("/instrumentation/%s[%d]") 
        % _name % _num);

    fgUntie((branch + "/serviceable").c_str());
    fgUntie((branch + "/setting-inhg").c_str());
    fgUntie((branch + "/pressure-alt-ft").c_str());
    fgUntie((branch + "/mode-c-alt-ft").c_str());
    fgUntie((branch + "/indicated-altitude-ft").c_str());
}

void
Altimeter::update (double dt)
{
    if (_serviceable) {
        double trat = _tau > 0 ? dt/_tau : 100;
        double new_PA = _altimeter.press_alt_ft(
                _static_pressure_node->getDoubleValue());
        if (dt > 0.0 && fabs(new_PA - _filtered_PA)/dt > 1000.) {
          SG_LOG( SG_COCKPIT, SG_DEBUG, "Altimeter snap: " << dt 
              << "  old: " << _filtered_PA
              << "  new: " << new_PA);
// 1000 fps = 60,000 fpm vertical = departure from normal physics
          _filtered_PA = new_PA;
        } else {
// Ordinarily the mechanism settles slowly toward the new pressure altitude:
          _filtered_PA = fgGetLowPass(_filtered_PA, new_PA, trat);
        }
        _mode_c_alt = 100 * SGMiscd::round(_filtered_PA/100);
        _kollsman = fgGetLowPass(_kollsman, _altimeter.kollsman_ft(_setting), trat);
        if (_quantum)
            _press_alt = _quantum * SGMiscd::round(_filtered_PA/_quantum);
        else
            _press_alt = _filtered_PA;
        _altitude = _press_alt - _kollsman;
    }
}

// end of altimeter.cxx
