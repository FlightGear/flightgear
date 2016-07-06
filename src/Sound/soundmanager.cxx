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

#include <simgear/sound/soundmgr.hxx>
#include <simgear/structure/commands.hxx>

#if defined(ENABLE_FLITE)
#include "VoiceSynthesizer.hxx"
#endif

#include "sample_queue.hxx"
#include "soundmanager.hxx"
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/view.hxx>

#include <stdio.h>

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

    _velocityNorthFPS = fgGetNode("velocities/speed-north-fps", true);
    _velocityEastFPS  = fgGetNode("velocities/speed-east-fps", true);
    _velocityDownFPS  = fgGetNode("velocities/speed-down-fps", true);
  
    SGPropertyNode_ptr scenery_loaded = fgGetNode("sim/sceneryloaded", true);
    scenery_loaded->addChangeListener(_listener.get());

    globals->get_commands()->addCommand("play-audio-sample", this, &FGSoundManager::playAudioSampleCommand);


    reinit();
}

void FGSoundManager::shutdown()
{
    SGPropertyNode_ptr scenery_loaded = fgGetNode("sim/sceneryloaded", true);
    scenery_loaded->removeChangeListener(_listener.get());
    
    stop();

    _chatterQueue.clear();
    globals->get_commands()->removeCommand("play-audio-sample");


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
  flightgear::View* _view = globals->get_current_view();
  return (_view->getXOffset_m () == 0.0 && _view->getYOffset_m () == 0.0 &&
          _view->getZOffset_m () == 0.0);
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
            flightgear::View* _view = globals->get_current_view();
            set_position( _view->getViewPosition(), _view->getPosition() );
            set_orientation( _view->getViewOrientation() );

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

/**
 * Built-in command: play an audio message (i.e. a wav file) This is
 * fire and forget.  Call this once per message and it will get dumped
 * into a queue.  Messages are played sequentially so they do not
 * overlap.
 */
bool FGSoundManager::playAudioSampleCommand(const SGPropertyNode * arg)
{
    string path = arg->getStringValue("path");
    string file = arg->getStringValue("file");
    float volume = arg->getFloatValue("volume");
    // cout << "playing " << path << " / " << file << endl;
    try {
        if ( !_chatterQueue ) {
            _chatterQueue = new FGSampleQueue(this, "chatter");
            _chatterQueue->tie_to_listener();
        }

        SGSoundSample *msg = new SGSoundSample(file.c_str(), path);
        msg->set_volume( volume );
        _chatterQueue->add( msg );

        return true;

    } catch (const sg_io_exception&) {
        SG_LOG(SG_GENERAL, SG_ALERT, "play-audio-sample: "
               "failed to load" << path << '/' << file);
        return false;
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
