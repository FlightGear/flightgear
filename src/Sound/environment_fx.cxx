// fg_environmentfx.cxx -- Sound effect management class implementation
//
// Started by David Megginson, October 2001
// (Reuses some code from main.cxx, probably by Curtis Olson)
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
// $Id$

#ifdef _MSC_VER
#pragma warning (disable: 4786)
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "environment_fx.hxx"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include <simgear/misc/sg_path.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>


static std::string _refname = "EnvironmentFX";
typedef boost::shared_ptr<SGSampleGroup> SGSampleGroupRef;
typedef boost::shared_ptr<FGEnvironmentFX> FGEnvironmentFXRef;


FGEnvironmentFX::FGEnvironmentFX()
{
    _enabled = fgGetNode("/sim/sound/environment/enabled", true);
    _volume = fgGetNode("/sim/sound/environment/volume", true);
    _smgr->add(this, _refname);

    nasal::Ghost<FGEnvironmentFXRef>::init("sound")
      .bases<SGSampleGroupRef>()
      .method("add", &FGEnvironmentFX::add)
      .method("position", &FGEnvironmentFX::position)
      .method("pitch", &FGEnvironmentFX::pitch)
      .method("volume", &FGEnvironmentFX::volume)
      .method("properties", &FGEnvironmentFX::properties)
      .method("play", &FGEnvironmentFX::play)
      .method("stop", &FGEnvironmentFX::stop);
}

FGEnvironmentFX::~FGEnvironmentFX()
{
}

void FGEnvironmentFX::unbind()
{
    if (_smgr) {
        _smgr->remove(_refname);
    }
}

void FGEnvironmentFX::init()
{
    if (!_smgr) {
        return;
    }
}

void FGEnvironmentFX::reinit()
{
    init();
}

void FGEnvironmentFX::update (double dt)
{
    if (!_smgr) {
        return;
    }

    if ( _enabled->getBoolValue() ) {
        set_volume( _volume->getDoubleValue() );
        resume();

        SGSampleGroup::update(dt);
    }
    else {
        suspend();
    }
}

bool FGEnvironmentFX::add(const std::string& path_str, const std::string& refname)
{
    if (!_smgr) {
        return false;
    }

    SGPath path = globals->resolve_resource_path(path_str);
    if (path.isNull())
    {
        SG_LOG(SG_SOUND, SG_ALERT, "File not found: '" << path_str);
        return false;
    }
    SG_LOG(SG_SOUND, SG_INFO, "Reading sound from " << path.str());

    SGSharedPtr<SGSoundSample> sample;
    sample = new SGSoundSample("", path);
    if (sample->file_path().exists()) {
        SGSampleGroup::add( sample, refname );
    }
    else
    {
        throw sg_io_exception("Environment FX: couldn't find file: '" + path.str() + "'");
        delete sample;
        return false;
    }
    return true;
}

void FGEnvironmentFX::position(const std::string& refname, double lon, double lat, double elevation)
{
    SGSoundSample* sample = SGSampleGroup::find(refname);
    if (sample)
    {
        SGGeod pos = SGGeod::fromDegFt(lon, lat, elevation);
        sample->set_position( SGVec3d::fromGeod(pos) );
    }
}

void FGEnvironmentFX::pitch(const std::string& refname, float pitch)
{
    SGSoundSample* sample = SGSampleGroup::find(refname);
    if (sample)
    {
        sample->set_volume( pitch );
    }
}

void FGEnvironmentFX::volume(const std::string& refname, float volume)
{
    SGSoundSample* sample = SGSampleGroup::find(refname);
    if (sample)
    {
        sample->set_volume( volume );
    }
}

void FGEnvironmentFX::properties(const std::string& refname, float reference_dist, float max_dist )
{
    SGSoundSample* sample = SGSampleGroup::find(refname);
    if (sample)
    {
        sample->set_reference_dist( reference_dist );
        if (max_dist > 0) {
            sample->set_max_dist( max_dist );
        }
    }
}

void FGEnvironmentFX::play(const std::string& refname, bool looping)
{
    SGSampleGroup::play( refname, looping );
}

void FGEnvironmentFX::stop(const std::string& refname)
{
    SGSampleGroup::stop( refname );
}

// end of fg_environmentfx.cxx
