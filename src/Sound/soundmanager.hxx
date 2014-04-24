// soundmanager.hxx -- Wraps the SimGear OpenAl sound manager class
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

#ifndef __FG_SOUNDMGR_HXX
#define __FG_SOUNDMGR_HXX 1

#include <memory>
#include <map>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/sound/soundmgr_openal.hxx>

class SGSoundMgr;
class Listener;
#if defined(ENABLE_FLITE)
class VoiceSynthesizer;
#endif

#ifdef ENABLE_AUDIO_SUPPORT
class FGSoundManager : public SGSoundMgr
{
public:
    FGSoundManager();
    virtual ~FGSoundManager();

    void init(void);
    virtual void shutdown();
    void update(double dt);
    void reinit(void);

    void activate(bool State);
    void update_device_list();
#if defined(ENABLE_FLITE)
    VoiceSynthesizer * getSynthesizer( const std::string & voice );
#endif
private:
    bool stationaryView() const;
  
    bool _is_initialized, _enabled;
    SGPropertyNode_ptr _sound_working, _sound_enabled, _volume, _device_name;
    SGPropertyNode_ptr _currentView;
    SGPropertyNode_ptr _viewPosLon, _viewPosLat, _viewPosElev;
    SGPropertyNode_ptr _velocityNorthFPS, _velocityEastFPS, _velocityDownFPS;
    SGPropertyNode_ptr _viewXoffset, _viewYoffset, _viewZoffset;
    std::auto_ptr<Listener> _listener;
#if defined(ENABLE_FLITE)
    std::map<std::string,VoiceSynthesizer*> _synthesizers;
#endif
};
#else
#include "Main/fg_props.hxx"

// provide a dummy sound class
class FGSoundManager : public SGSubsystem
{
public:
    FGSoundManager() { fgSetBool("/sim/sound/working", false);}
    ~FGSoundManager() {}

    void update(double dt) {}
};

#endif // ENABLE_AUDIO_SUPPORT

#endif // __FG_SOUNDMGR_HXX
