// scenefx.hxx -- Scene sound effect management class
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

#ifndef __FGSCENEFX_HXX
#define __FGSCENEFX_HXX 1

#include <simgear/compiler.h>

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>
#include <simgear/sound/sample_group.hxx>
#include <simgear/math/SGMathFwd.hxx>

#define _DEFAULT_MAX_DISTANCE		100000.0

/**
 * Container for FlightGear envirnonmetal sound effects.
 */
class FGSceneFX : public SGSampleGroup
{

public:

    FGSceneFX();
    virtual ~FGSceneFX();

    void init ();
    void reinit();
    void update (double dt);
    void unbind();

    /* Adjust volume and distance properties for the aircraft model */
    /* These work on all active sounds                              */
    void model_damping(float damping);

    /* (Over)load the sound file for a particular ref. name         */
    bool load(const std::string& refname, const std::string& path_str);


    /* Control the sounds from the generating side.
     *
     * Every sound has a reference name and an instance number:
     * A reference name would be for instance 'thunder' or 'rain'.
     * The number indicates the exact instance in case multiple
     * variants are active simultaniously.
     */
    size_t add(const std::string& refname);
    bool remove(const std::string& refname, size_t num);

    void position(const std::string& refname, size_t num, double lon, double lat, double elevation = 0.0);
    void pitch(const std::string& refname, size_t num, float pitch);
    void volume(const std::string& refname, size_t num, float volume);
    void properties(const std::string& refname, size_t num, float reference_dist, float max_dist = -1 );
    void play(const std::string& refname, size_t num, bool looping = false);
    void stop(const std::string& refname, size_t num);

private:
    SGPropertyNode_ptr _enabled;
    SGPropertyNode_ptr _volume;
    SGPropertyNode_ptr _damping;	// model sound damping

    const char* full_name(const std::string& refname, size_t num);
    SGSoundSample *find(const std::string& refname, size_t num);
};


#endif

// end of scenefx.hxx
