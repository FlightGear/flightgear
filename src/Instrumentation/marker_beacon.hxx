// marker_beacon.hxx -- class to manage the marker beacons
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


#ifndef _FG_MARKER_BEACON_HXX
#define _FG_MARKER_BEACON_HXX

#include <simgear/compiler.h>

#include <Instrumentation/AbstractInstrument.hxx>
#include <Sound/beacon.hxx>
#include <simgear/timing/timestamp.hxx>

class SGSampleGroup;

class FGMarkerBeacon : public AbstractInstrument,
                       public SGPropertyChangeListener
{
public:
    enum fgMkrBeacType {
        NOBEACON = 0,
        INNER,
        MIDDLE,
        OUTER
    };

    FGMarkerBeacon(SGPropertyNode *node);
    ~FGMarkerBeacon();

    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "marker-beacon"; }

    void search ();

    void valueChanged(SGPropertyNode* val) override;

private:
    // Inputs
    SGPropertyNode_ptr audio_btn;
    SGPropertyNode_ptr audio_vol;
    SGPropertyNode_ptr sound_working;

    SGPropertyNode_ptr _innerBlinkNode;
    SGPropertyNode_ptr _middleBlinkNode;
    SGPropertyNode_ptr _outerBlinkNode;

    bool _audioPropertiesChanged = true;

    // internal periodic station search timer
    double _time_before_search_sec = 0.0;

    SGTimeStamp _audioStartTime;
    SGSharedPtr<SGSampleGroup> _audioSampleGroup;

    enum class BlinkMode {
        BackwardsCompatible, ///< all beacons use the OM blink rate
        Standard,            ///< beacones use the correct blink for their type
        Continuous           ///< blink disabled, so aircraft can do its own blink
    };

    BlinkMode _blinkMode = BlinkMode::BackwardsCompatible;

    void changeBeaconType(fgMkrBeacType newType);

    void updateAudio();
    void stopAudio();
    void updateOutputProperties(bool on);

    fgMkrBeacType _lastBeacon;

    FGBeacon::BeaconTiming _beaconTiming;
};


#endif // _FG_MARKER_BEACON_HXX
