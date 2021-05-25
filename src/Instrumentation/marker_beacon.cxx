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


#include <config.h>

#include <stdio.h>	// snprintf

#include <simgear/compiler.h>
#include <simgear/math/sg_random.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sound/sample_group.hxx>

#include <Main/fg_props.hxx>
#include <Navaids/navlist.hxx>

#include "marker_beacon.hxx"
#include <Sound/beacon.hxx>

#include <string>

using std::string;


static SGSoundSample* createSampleForBeacon(FGMarkerBeacon::fgMkrBeacType ty)
{
    switch (ty) {
    case FGMarkerBeacon::INNER:
        return FGBeacon::instance()->get_inner();
    case FGMarkerBeacon::MIDDLE:
        return FGBeacon::instance()->get_middle();
    case FGMarkerBeacon::OUTER:
        return FGBeacon::instance()->get_outer();
    default:
        return nullptr;
    }
}

static string sampleNameForBeacon(FGMarkerBeacon::fgMkrBeacType ty)
{
    switch (ty) {
    case FGMarkerBeacon::INNER: return "inner-marker";
    case FGMarkerBeacon::MIDDLE: return "middle-marker";
    case FGMarkerBeacon::OUTER: return "outer-marker";
    default:
        return {};
    }
}

// Constructor
FGMarkerBeacon::FGMarkerBeacon(SGPropertyNode *node) :
    _time_before_search_sec(0.0)
{
    // backwards-compatability supply path
    setDefaultPowerSupplyPath("/systems/electrical/outputs/nav[0]");
    readConfig(node, "marker-beacon");

    string blinkMode = node->getStringValue("blink-mode");
    if (blinkMode == "standard") {
        _blinkMode = BlinkMode::Standard;
    } else if (blinkMode == "continuous") {
        _blinkMode = BlinkMode::Continuous;
    }
}


// Destructor
FGMarkerBeacon::~FGMarkerBeacon()
{
}


void
FGMarkerBeacon::init ()
{
    SGPropertyNode *node = fgGetNode(nodePath(), true );
    initServicePowerProperties(node);
    
    // Inputs
    sound_working = fgGetNode("/sim/sound/working", true);
    audio_btn = node->getChild("audio-btn", 0, true);
    audio_vol = node->getChild("volume", 0, true);

    if (audio_btn->getType() == simgear::props::NONE)
        audio_btn->setBoolValue( true );

    SGSoundMgr *smgr = globals->get_subsystem<SGSoundMgr>();
    if (smgr) {
        _audioSampleGroup = smgr->find("avionics", true);
        _audioSampleGroup->tie_to_listener();

        sound_working->addChangeListener(this);
        audio_btn->addChangeListener(this);
        audio_vol->addChangeListener(this);
    }

    reinit();
}

void
FGMarkerBeacon::reinit ()
{
    _time_before_search_sec = 0.0;
    _lastBeacon = NOBEACON;
    updateOutputProperties(false);
}

void
FGMarkerBeacon::bind ()
{
    string branch = nodePath();

    _innerBlinkNode = fgGetNode(branch + "/inner", true);
    _middleBlinkNode = fgGetNode(branch + "/middle", true);
    _outerBlinkNode = fgGetNode(branch + "/outer", true);
}


void
FGMarkerBeacon::unbind ()
{
    string branch = nodePath();

    fgUntie((branch + "/inner").c_str());
    fgUntie((branch + "/middle").c_str());
    fgUntie((branch + "/outer").c_str());

    if (_audioSampleGroup) {
        sound_working->removeChangeListener(this);
        audio_btn->removeChangeListener(this);
        audio_vol->removeChangeListener(this);
    }

    AbstractInstrument::unbind();
}

// Update the various nav values based on position and valid tuned in navs
void
FGMarkerBeacon::update(double dt)
{
    if (!isServiceableAndPowered()) {
        _lastBeacon = NOBEACON;
        stopAudio();
        updateOutputProperties(false);
        return;
    }

    _time_before_search_sec -= dt;
    if ( _time_before_search_sec < 0 ) {
        search();
    }

    if (_audioPropertiesChanged) {
        updateAudio();
    }

    if (_lastBeacon != NOBEACON) {
        // compute blink to match audio
        // we use our own timing here (instead of dt) since audio rate is not affected
        // by pause or time acceleration, so this should stay in sync.
        const int elapasedUSec = (SGTimeStamp::now() - _audioStartTime).toUSecs();

        bool on = true;
        if (_blinkMode != BlinkMode::Continuous) {
            int t = elapasedUSec % _beaconTiming.durationUSec;
            for (int i = 0; i < 4; i++) {
                t -= _beaconTiming.periodsUSec.at(i);
                if (t < 0) {
                    // if value is negative, current time is within this
                    // period, so we are finished.
                    break;
                }

                // each period, the sense flips
                on = !on;
            } // of periods iteration
        }

        updateOutputProperties(on);
    }
}

static void lazyChangeBoolProp(SGPropertyNode* node, bool v)
{
    if (node->getBoolValue() != v) {
        node->setBoolValue(v);
    }
}

void FGMarkerBeacon::updateOutputProperties(bool on)
{
    // map our beacon nodes to indices which correspond to the fgMkrBeacType enum
    // this allows to use '_lastBeacon' to select whhich index should be on
    // we set all other ones to off to ensure consistency in weird cases, eg
    // going from one beaon type to another in a single update.
    SGPropertyNode* beacons[4] = {nullptr, _innerBlinkNode.get(), _middleBlinkNode.get(), _outerBlinkNode.get()};
    for (int b = INNER; b <= OUTER; b++) {
        const bool bOn = on && (_lastBeacon == b);
        lazyChangeBoolProp(beacons[b], bOn);
    }
}

void FGMarkerBeacon::updateAudio()
{
    _audioPropertiesChanged = false;
    if (!_audioSampleGroup)
        return;

    float volume = audio_vol->getFloatValue();
    if (!audio_btn->getBoolValue()) {
        // mute rather than stop, so we don't lose sync with the visual blink
        volume = 0.0;
    }

    SGSoundSample* mkr = _audioSampleGroup->find(sampleNameForBeacon(_lastBeacon));
    if (mkr) {
        mkr->set_volume(volume);
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
    FGPositioned::Type minType() const override
    {
        return FGPositioned::OM;
    }

    FGPositioned::Type maxType() const override
    {
        return FGPositioned::IM;
    }
};

// Update current nav/adf radio stations based on current postition
void FGMarkerBeacon::search()
{
    // reset search time
    _time_before_search_sec = 0.5;

    const SGGeod pos = globals->get_aircraft_position();

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

    if ( b == NULL || !inrange || !isServiceableAndPowered())
    {
        beacon_type = NOBEACON;
    }

    changeBeaconType(beacon_type);
}

void FGMarkerBeacon::changeBeaconType(fgMkrBeacType newType)
{
    if (newType == _lastBeacon)
        return;

    _lastBeacon = newType;
    stopAudio(); // stop any existing playback

    if (newType == NOBEACON) {
        updateOutputProperties(false);
        return;
    }

    if (_blinkMode == BlinkMode::Standard) {
        // get correct timings from the sounds generator
        switch (newType) {
        case INNER:
            _beaconTiming = FGBeacon::instance()->getTimingForInner();
            break;
        case MIDDLE:
            _beaconTiming = FGBeacon::instance()->getTimingForMiddle();
            break;
        case OUTER:
            _beaconTiming = FGBeacon::instance()->getTimingForOuter();
            break;
        default:
            break;
        }
    } else if (_blinkMode == BlinkMode::BackwardsCompatible) {
        // older FG versions used same timing for alll beacon types :(
        _beaconTiming = FGBeacon::BeaconTiming{};
        _beaconTiming.durationUSec = 500000;
        _beaconTiming.periodsUSec[0] = 400000;
        _beaconTiming.periodsUSec[1] = 100000;
    }


    if (_audioSampleGroup) {
        // create sample as required
        const auto name = sampleNameForBeacon(newType);
        if (!_audioSampleGroup->exists(name)) {
            SGSoundSample* sound = createSampleForBeacon(newType);
            if (sound) {
                _audioSampleGroup->add(sound, name);
            }
        }

        _audioSampleGroup->play_looped(name);
        updateAudio(); // sync volume+mute now
    }

    // we use this timing for visuals as well, so do this even if we have
    // no audio sample group
    _audioStartTime.stamp();
}

void FGMarkerBeacon::stopAudio()
{
    if (_audioSampleGroup) {
        _audioSampleGroup->stop("outer-marker");
        _audioSampleGroup->stop("middle-marker");
        _audioSampleGroup->stop("inner-marker");
    }
}

void FGMarkerBeacon::valueChanged(SGPropertyNode* val)
{
    _audioPropertiesChanged = true;
}

// Register the subsystem.
#if 0
SGSubsystemMgr::InstancedRegistrant<FGMarkerBeacon> registrantFGMarkerBeacon(
    SGSubsystemMgr::FDM,
    {{"instrumentation", SGSubsystemMgr::Dependency::HARD}},
    0.2);
#endif
