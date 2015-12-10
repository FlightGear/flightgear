// scenefx.cxx -- Scenery sound effect management implementation
//
// Started by Erik Hofman, November 2015
//
// Copyright (C) 2015  Erik Hofman <erik@ehofman.com>
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

#include <stdio.h>
#include <string>

#include "scenefx.hxx"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include <simgear/props/props_io.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/sound/xmlsound.hxx>


#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>


static std::string _refname = "SceneFX";
typedef boost::shared_ptr<SGSampleGroup> SGSampleGroupRef;
typedef boost::shared_ptr<FGSceneFX> FGSceneFXRef;


FGSceneFX::FGSceneFX()
{
    _props = _props = globals->get_props();
    _enabled = fgGetNode("/sim/sound/scene/enabled", true);
    _volume = fgGetNode("/sim/sound/scene/volume", true);
    _damping = fgGetNode("/sim/sound/model-damping", true);
    _smgr->add(this, _refname);

    nasal::Ghost<FGSceneFXRef>::init("soundfx.scene")
      .bases<SGSampleGroupRef>()
      .method("load", &FGSceneFX::load)
      .method("damping", &FGSceneFX::model_damping);
}

void FGSceneFX::unbind()
{
    if (_smgr)
    {
        _smgr->remove(_refname);
    }
}

FGSceneFX::~FGSceneFX()
{
    for (unsigned int i = 0; i < _sound.size(); i++ ) {
        delete _sound[i];
    }
    _sound.clear();
}

void FGSceneFX::init()
{
    if (!_smgr) {
        return;
    }

    SGPath path(globals->get_fg_root());
    path.append("Sounds/sounds.xml");

    SGPropertyNode root;
    try {
        readProperties(path.str(), &root);
    } catch (const sg_exception &) {
        SG_LOG(SG_SOUND, SG_ALERT,
               "Error reading file '" << path.str() << '\'');
        return;
    }

    SGPropertyNode *node = root.getNode("fx");
    if(node) {
        for (int i = 0; i < node->nChildren(); ++i) {
            SGXmlSound *soundfx = new SGXmlSound();

            try {
                soundfx->init( _props, node->getChild(i), this, 0, path.dir() );
                _sound.push_back( soundfx );
            } catch ( sg_exception &e ) {
                SG_LOG(SG_SOUND, SG_ALERT, e.getFormattedMessage());
                delete soundfx;
            }
        }
    }
}

void FGSceneFX::reinit()
{
    for ( unsigned int i = 0; i < _sound.size(); i++ ) {
        delete _sound[i];
    }
    _sound.clear();
    init();
}

void FGSceneFX::update (double dt)
{
    if (!_smgr) {
        return;
    }

    if (_enabled->getBoolValue())
    {
        float fact = 1.0f - _damping->getFloatValue();
        set_volume(_volume->getFloatValue() * fact);
        resume();

        // update sound effects if not paused
        for ( unsigned int i = 0; i < _sound.size(); i++ ) {
            _sound[i]->update(dt);
        }

        SGSampleGroup::update(dt);
    }
    else {
        suspend();
    }
}

/* Sets the scene properties from the models point of view      */
void FGSceneFX::model_damping(float damping)
{
    _damping->setFloatValue(damping);
}

/* (Over)load the sound file for a particular ref. name         */
bool FGSceneFX::load(const std::string& refname, const std::string& path_str)
{
    if (!_smgr) {
        return false;
    }

    SGPath path = globals->resolve_resource_path(path_str);
    if (!path.isNull() && path.exists())
    {
        SGSoundSample* sample;
        unsigned int i = 0;
        do
        {
            sample = find(refname, i);
            if (sample) {
                sample->set_sample_name(path_str);
            }
        }
        while(sample);

        return true;
    }

    throw sg_io_exception("SceneFX: couldn't find file: '" + path.str() + "'");

    return false;
}

/* Control the sounds from the generating side.                 */
size_t FGSceneFX::add(const std::string& refname)
{
    if (!_smgr) {
        return false;
    }

    unsigned int num = 0;
    std::string name;
    do {
        name = full_name(refname, num++);
    } while(SGSampleGroup::exists(name));

    SGSharedPtr<SGSoundSample> sample;
    sample = new SGSoundSample();
    SGSampleGroup::add(sample, name);

    return num;
}

bool FGSceneFX::remove(const std::string& refname, size_t num)
{
    std::string name = full_name(refname, num);
    return SGSampleGroup::remove(name);
}

void FGSceneFX::position(const std::string& refname, size_t num, double lon, double lat, double elevation)
{
    SGSoundSample* sample = find(refname, num);
    if (sample)
    {
        SGGeod pos = SGGeod::fromDegFt(lon, lat, elevation);
        sample->set_position(SGVec3d::fromGeod(pos));
    }
}

void FGSceneFX::pitch(const std::string& refname, size_t num, float pitch)
{
    SGSoundSample* sample = find(refname, num);
    if (sample) {
        sample->set_pitch(pitch);
    }
}

void FGSceneFX::volume(const std::string& refname, size_t num, float volume)
{
    SGSoundSample* sample = find(refname, num);
    if (sample)
    {
        sample->set_volume(volume);
    }
}

void FGSceneFX::properties(const std::string& refname, size_t num, float reference_dist, float maximum_dist)
{
    SGSoundSample* sample = find(refname, num);
    if (sample)
    {
        float fact = 1.0f - _damping->getFloatValue();
        sample->set_reference_dist(reference_dist * fact);
        if (maximum_dist > 0) {
            sample->set_max_dist(maximum_dist * fact);
        }
    }
}

void FGSceneFX::play(const std::string& refname, size_t num, bool looping)
{
    SGSampleGroup::play(full_name(refname, num), looping);
}

void FGSceneFX::stop(const std::string& refname, size_t num)
{
    SGSampleGroup::stop(full_name(refname, num));
}

const char* FGSceneFX::full_name(const std::string& refname, size_t num)
{
    static char nstr[128];
    snprintf(nstr, 127, "%s_%4zu", refname.c_str(), num);
    return nstr;
}

SGSoundSample *FGSceneFX::find(const std::string& refname, size_t num)
{
    return SGSampleGroup::find( full_name(refname, num) );
}

// end of scenefx.cxx
