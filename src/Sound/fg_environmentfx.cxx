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

#include "fg_environmentfx.hxx"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

// #include <simgear/props/props.hxx>
// #include <simgear/props/props_io.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sound/soundmgr_openal.hxx>


static std::string _refname = "EnvironmentFX";


FGEnvironmentFX::FGEnvironmentFX()
{
    _enabled = fgGetNode("/sim/sound/environment/enabled", true);
    _volume = fgGetNode("/sim/sound/environment/volume", true);
    _smgr->add(this, _refname);
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

void FGEnvironmentFX::add(std::string& path_str, const std::string& refname)
{
    if (!_smgr) {
        return;
    }

    SGPath path = globals->resolve_resource_path(path_str);
    if (path.isNull())
    {
        SG_LOG(SG_SOUND, SG_ALERT, "File not found: '" << path_str);
        return;
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
    }
}

void FGEnvironmentFX::position(const std::string& refname, const SGVec3d& pos)
{
    SGSoundSample* sample = SGSampleGroup::find(refname);
    if (sample)
    {
        sample->set_position( pos );
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

// end of fg_environmentfx.cxx
