// marker_beacon.cxx -- class to manage the marker beacons
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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

#include <stdio.h>	// snprintf

#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>

#include <Navaids/navlist.hxx>

#include "marker_beacon.hxx"
#include <Sound/beacon.hxx>

#include <string>
using std::string;


// Constructor
FGMarkerBeacon::FGMarkerBeacon(SGPropertyNode *node) :
    audio_vol(NULL),
    need_update(true),
    outer_blink(false),
    middle_blink(false),
    inner_blink(false),
    name("marker-beacon"),
    num(0),
    _time_before_search_sec(0.0),
    _sgr(NULL)
{
    SGPath path( globals->get_fg_root() );
    SGPath term = path;
    term.append( "Navaids/range.term" );
    SGPath low = path;
    low.append( "Navaids/range.low" );
    SGPath high = path;
    high.append( "Navaids/range.high" );

    term_tbl = new SGInterpTable( term.str() );
    low_tbl = new SGInterpTable( low.str() );
    high_tbl = new SGInterpTable( high.str() );

    for ( int i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_INSTR, SG_WARN,
                    "Error in marker beacon config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}


// Destructor
FGMarkerBeacon::~FGMarkerBeacon()
{
    delete term_tbl;
    delete low_tbl;
    delete high_tbl;
}


void
FGMarkerBeacon::init ()
{
    string branch;
    branch = "/instrumentation/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    // Inputs
    sound_working = fgGetNode("/sim/sound/working", true);
    lon_node = fgGetNode("/position/longitude-deg", true);
    lat_node = fgGetNode("/position/latitude-deg", true);
    alt_node = fgGetNode("/position/altitude-ft", true);
    bus_power = fgGetNode("/systems/electrical/outputs/nav[0]", true);
    power_btn = node->getChild("power-btn", 0, true);
    audio_btn = node->getChild("audio-btn", 0, true);
    audio_vol = node->getChild("volume", 0, true);
    serviceable = node->getChild("serviceable", 0, true);

    if (power_btn->getType() == simgear::props::NONE)
        power_btn->setBoolValue( true );
    if (audio_btn->getType() == simgear::props::NONE)
        audio_btn->setBoolValue( true );
    if (serviceable->getType() == simgear::props::NONE)
        serviceable->setBoolValue( true );

    SGSoundMgr *smgr = globals->get_soundmgr();
    _sgr = smgr->find("avionics", true);
    _sgr->tie_to_listener();

    blink.stamp();

    outer_marker = middle_marker = inner_marker = false;
}


void
FGMarkerBeacon::bind ()
{
    string branch;
    branch = "/instrumentation/" + name;

    fgTie((branch + "/inner").c_str(), this,
          &FGMarkerBeacon::get_inner_blink);

    fgTie((branch + "/middle").c_str(), this,
          &FGMarkerBeacon::get_middle_blink);

    fgTie((branch + "/outer").c_str(), this,
          &FGMarkerBeacon::get_outer_blink);
}


void
FGMarkerBeacon::unbind ()
{
    string branch;
    branch = "/instrumentation/" + name;

    fgUntie((branch + "/inner").c_str());
    fgUntie((branch + "/middle").c_str());
    fgUntie((branch + "/outer").c_str());
}


// Update the various nav values based on position and valid tuned in navs
void
FGMarkerBeacon::update(double dt)
{
    need_update = false;

    // On timeout, scan again, this needs to run every iteration no
    // matter what the power or serviceable state.  If power is turned
    // off or the unit becomes unserviceable while a beacon sound is
    // playing, the search() routine still needs to be called so the
    // sound effect can be properly disabled.

    _time_before_search_sec -= dt;
    if ( _time_before_search_sec < 0 ) {
        search();
    }

    if ( has_power() && serviceable->getBoolValue()
            && sound_working->getBoolValue()) {

        // marker beacon blinking
        bool light_on = ( outer_blink || middle_blink || inner_blink );
        SGTimeStamp current = SGTimeStamp::now();

        if ( light_on && blink + SGTimeStamp::fromUSec(400000) < current ) {
            light_on = false;
            blink = current;
        } else if ( !light_on && blink + SGTimeStamp::fromUSec(100000) < current ) {
            light_on = true;
            blink = current;
        }

        if ( outer_marker ) {
            outer_blink = light_on;
        } else {
            outer_blink = false;
        }

        if ( middle_marker ) {
            middle_blink = light_on;
        } else {
            middle_blink = false;
        }

        if ( inner_marker ) {
            inner_blink = light_on;
        } else {
            inner_blink = false;
        }

        // cout << outer_blink << " " << middle_blink << " "
        //      << inner_blink << endl;
    } else {
        inner_blink = middle_blink = outer_blink = false;
    }
}


static bool check_beacon_range( const SGGeod& pos,
                                FGPositioned *b )
{
    double d = distSqr(b->cart(), SGVec3d::fromGeod(pos));
    // cout << "  distance = " << d << " ("
    //      << FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER
    //         * FG_ILS_DEFAULT_RANGE * SG_NM_TO_METER
    //      << ")" << endl;

    //std::cout << "  range = " << sqrt(d) << std::endl;

    // cout << "elev = " << elev * SG_METER_TO_FEET
    //      << " current->get_elev() = " << current->get_elev() << endl;
    double elev_ft = pos.getElevationFt();
    double delev = elev_ft - b->elevation();

    // max range is the area under r = 2.4 * alt or r^2 = 4000^2 - alt^2
    // whichever is smaller.  The intersection point is 1538 ...
    double maxrange2;	// feet^2
    if ( delev < 1538.0 ) {
        maxrange2 = 2.4 * 2.4 * delev * delev;
    } else if ( delev < 4000.0 ) {
        maxrange2 = 4000 * 4000 - delev * delev;
    } else {
        maxrange2 = 0.0;
    }
    maxrange2 *= SG_FEET_TO_METER * SG_FEET_TO_METER; // convert to meter^2
    //std::cout << "delev = " << delev << " maxrange = " << sqrt(maxrange2) << std::endl;

    // match up to twice the published range so we can model
    // reduced signal strength
    if ( d < maxrange2 ) {
        return true;
    } else {
        return false;
    }
}

class BeaconFilter : public FGPositioned::Filter
{
public:
  virtual FGPositioned::Type minType() const {
    return FGPositioned::OM;
  }

  virtual FGPositioned::Type maxType()  const {
    return FGPositioned::IM;
  }

};

// Update current nav/adf radio stations based on current postition
void FGMarkerBeacon::search()
{
    // reset search time
    _time_before_search_sec = 1.0;

    static fgMkrBeacType last_beacon = NOBEACON;

    SGGeod pos = SGGeod::fromDegFt(lon_node->getDoubleValue(),
                                   lat_node->getDoubleValue(),
                                   alt_node->getDoubleValue());

    ////////////////////////////////////////////////////////////////////////
    // Beacons.
    ////////////////////////////////////////////////////////////////////////

    // get closest marker beacon - within a 1nm cutoff
    BeaconFilter filter;
    FGPositionedRef b = FGPositioned::findClosest(pos, 1.0, &filter);

    fgMkrBeacType beacon_type = NOBEACON;
    bool inrange = false;
    if ( b != NULL ) {
        if ( b->type() == FGPositioned::OM ) {
            beacon_type = OUTER;
        } else if ( b->type() == FGPositioned::MM ) {
            beacon_type = MIDDLE;
        } else if ( b->type() == FGPositioned::IM ) {
            beacon_type = INNER;
        }
        inrange = check_beacon_range( pos, b.ptr() );
    }

    outer_marker = middle_marker = inner_marker = false;

    if ( b == NULL || !inrange || !has_power() || !serviceable->getBoolValue() )
    {
        // cout << "no marker" << endl;
        _sgr->stop( "outer-marker" );
        _sgr->stop( "middle-marker" );
        _sgr->stop( "inner-marker" );
    } else {

        string current_sound_name;

        if ( beacon_type == OUTER ) {
            outer_marker = true;
            current_sound_name = "outer-marker";
            // cout << "OUTER MARKER" << endl;
            if ( last_beacon != OUTER ) {
                if ( ! _sgr->exists( current_sound_name ) ) {
                    SGSoundSample *sound = FGBeacon::instance()->get_outer();
                    if ( sound ) {
                        _sgr->add( sound, current_sound_name );
                    }
                }
            }
            if ( audio_btn->getBoolValue() ) {
                if ( !_sgr->is_playing(current_sound_name) ) {
                    _sgr->play_looped( current_sound_name );
                }
            } else {
                _sgr->stop( current_sound_name );
            }
        } else if ( beacon_type == MIDDLE ) {
            middle_marker = true;
            current_sound_name = "middle-marker";
            // cout << "MIDDLE MARKER" << endl;
            if ( last_beacon != MIDDLE ) {
                if ( ! _sgr->exists( current_sound_name ) ) {
                    SGSoundSample *sound = FGBeacon::instance()->get_middle();
                    if ( sound ) {
                        _sgr->add( sound, current_sound_name );
                    }
                }
            }
            if ( audio_btn->getBoolValue() ) {
                if ( !_sgr->is_playing(current_sound_name) ) {
                    _sgr->play_looped( current_sound_name );
                }
            } else {
                _sgr->stop( current_sound_name );
            }
        } else if ( beacon_type == INNER ) {
            inner_marker = true;
            current_sound_name = "inner-marker";
            // cout << "INNER MARKER" << endl;
            if ( last_beacon != INNER ) {
                if ( ! _sgr->exists( current_sound_name ) ) {
                    SGSoundSample *sound = FGBeacon::instance()->get_inner();
                    if ( sound ) {
                        _sgr->add( sound, current_sound_name );
                    }
                }
            }
            if ( audio_btn->getBoolValue() ) {
                if ( !_sgr->is_playing(current_sound_name) ) {
                    _sgr->play_looped( current_sound_name );
                }
            } else {
                _sgr->stop( current_sound_name );
            }
        }
        // cout << "VOLUME " << audio_vol->getDoubleValue() << endl;
        SGSoundSample * mkr = _sgr->find( current_sound_name );
        if (mkr)
            mkr->set_volume( audio_vol->getFloatValue() );
    }

    if ( inrange ) {
        last_beacon = beacon_type;
    } else {
        last_beacon = NOBEACON;
    }
}
