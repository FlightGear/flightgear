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

#include <simgear/sound/soundmgr_openal.hxx>

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
    _listener(new Listener(this))
{
    SGPropertyNode_ptr scenery_loaded = fgGetNode("sim/sceneryloaded", true);
    scenery_loaded->addChangeListener(_listener);
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

    reinit();
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

// Update sound manager and propagate property values,
// since the sound manager doesn't read any properties itself.
// Actual sound update is triggered by the subsystem manager.
void FGSoundManager::update(double dt)
{
    if (_is_initialized && _sound_working->getBoolValue() && _sound_enabled->getBoolValue())
    {
        set_volume(_volume->getFloatValue());
        SGSoundMgr::update(dt);
    }
}

#endif // ENABLE_AUDIO_SUPPORT
