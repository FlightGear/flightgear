// soundmanager.cxx -- Wraps the SimGear OpenAl sound manager class
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/sound/soundmgr_openal.hxx>

#if defined(ENABLE_FLITE)
#include "VoiceSynthesizer.hxx"
#endif
#include "soundmanager.hxx"
#include "Main/globals.hxx"
#include "Main/fg_props.hxx"

#include <vector>
#include <string>

#ifdef ENABLE_AUDIO_SUPPORT
/**
 * Listener class that monitors the sim state.
 */
class Listener : public SGPropertyChangeListener
{
public:
    Listener(FGSoundManager *wrapper) : _wrapper(wrapper) {}
    virtual void valueChanged (SGPropertyNode * node);

private:
    FGSoundManager* _wrapper;
};

void Listener::valueChanged(SGPropertyNode * node)
{
    _wrapper->activate(node->getBoolValue());
}

FGSoundManager::FGSoundManager()
  : _is_initialized(false),
    _enabled(false),
    _listener(new Listener(this))
{
}

FGSoundManager::~FGSoundManager()
{
}

void FGSoundManager::init()
{
    _sound_working = fgGetNode("/sim/sound/working");
    _sound_enabled = fgGetNode("/sim/sound/enabled");
    _volume        = fgGetNode("/sim/sound/volume");
    _device_name   = fgGetNode("/sim/sound/device-name");

    _currentView   = fgGetNode("sim/current-view");
    _viewPosLon    = fgGetNode("sim/current-view/viewer-lon-deg");
    _viewPosLat    = fgGetNode("sim/current-view/viewer-lat-deg");
    _viewPosElev   = fgGetNode("sim/current-view/viewer-elev-ft");
  
    _velocityNorthFPS = fgGetNode("velocities/speed-north-fps", true);
    _velocityEastFPS  = fgGetNode("velocities/speed-east-fps", true);
    _velocityDownFPS  = fgGetNode("velocities/speed-down-fps", true);
  
    _viewXoffset      = _currentView->getNode("x-offset-m", true);
    _viewYoffset      = _currentView->getNode("y-offset-m", true);
    _viewZoffset      = _currentView->getNode("z-offset-m", true);

    SGPropertyNode_ptr scenery_loaded = fgGetNode("sim/sceneryloaded", true);
    scenery_loaded->addChangeListener(_listener.get());

    reinit();
}

void FGSoundManager::shutdown()
{
    SGPropertyNode_ptr scenery_loaded = fgGetNode("sim/sceneryloaded", true);
    scenery_loaded->removeChangeListener(_listener.get());
    
    stop();
    
    SGSoundMgr::shutdown();
}

void FGSoundManager::reinit()
{
    _is_initialized = false;

    if (_is_initialized && !_sound_working->getBoolValue())
    {
        // shutdown sound support
        stop();
        return;
    }

    if (!_sound_working->getBoolValue())
    {
        return;
    }

    update_device_list();

    select_device(_device_name->getStringValue());
    SGSoundMgr::reinit();
    _is_initialized = true;

    activate(fgGetBool("sim/sceneryloaded", true));
}

void FGSoundManager::activate(bool State)
{
    if (_is_initialized)
    {
        if (State)
        {
            SGSoundMgr::activate();
        }
    }
}

void FGSoundManager::update_device_list()
{
    std::vector <const char*>devices = get_available_devices();
    for (unsigned int i=0; i<devices.size(); i++) {
        SGPropertyNode *p = fgGetNode("/sim/sound/devices/device", i, true);
        p->setStringValue(devices[i]);
    }
    devices.clear();
}

bool FGSoundManager::stationaryView() const
{
  // this is an ugly hack to decide if the *viewer* is stationary,
  // since we don't model the viewer velocity directly.
  return (_viewXoffset->getDoubleValue() == 0.0) &&
         (_viewYoffset->getDoubleValue() == 0.0) &&
         (_viewZoffset->getDoubleValue() == 0.0);
}

// Update sound manager and propagate property values,
// since the sound manager doesn't read any properties itself.
// Actual sound update is triggered by the subsystem manager.
void FGSoundManager::update(double dt)
{
    if (_is_initialized && _sound_working->getBoolValue())
    {
        bool enabled = _sound_enabled->getBoolValue();
        if (enabled != _enabled)
        {
            if (enabled)
                resume();
            else
                suspend();
            _enabled = enabled;
        }
        if (enabled)
        {
            SGGeod viewPosGeod(SGGeod::fromDegFt(_viewPosLon->getDoubleValue(),
                                                 _viewPosLat->getDoubleValue(),
                                                 _viewPosElev->getDoubleValue()));
            SGVec3d cartPos = SGVec3d::fromGeod(viewPosGeod);

            set_position(cartPos, viewPosGeod);

            SGQuatd viewOrientation;
            for (int i=0; i<4; ++i) {
              viewOrientation[i] = _currentView->getChild("raw-orientation", i, true)->getDoubleValue();
            }

            set_orientation( viewOrientation );

            SGVec3d velocity(SGVec3d::zeros());
            if (!stationaryView()) {
              velocity = SGVec3d(_velocityNorthFPS->getDoubleValue(),
                                 _velocityEastFPS->getDoubleValue(),
                                 _velocityDownFPS->getDoubleValue() );
            }

            set_velocity( velocity );

            set_volume(_volume->getFloatValue());
            SGSoundMgr::update(dt);
        }
    }
}

#if defined(ENABLE_FLITE)
VoiceSynthesizer * FGSoundManager::getSynthesizer( const std::string & voice )
{
  std::map<std::string,VoiceSynthesizer*>::iterator it = _synthesizers.find(voice);
  if( it == _synthesizers.end() ) {
    VoiceSynthesizer * synthesizer = new FLITEVoiceSynthesizer( voice );
    _synthesizers[voice] = synthesizer;
    return synthesizer;
  }
  return it->second;
}
#endif
#endif // ENABLE_AUDIO_SUPPORT
