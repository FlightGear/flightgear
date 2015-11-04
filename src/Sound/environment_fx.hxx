// fg_environmentfx.hxx -- Sound effect management class
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

#ifndef __FGENVIRONMENTFX_HXX
#define __FGENVIRONMENTFX_HXX 1

#include <simgear/compiler.h>

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>
#include <simgear/sound/sample_group.hxx>
#include <simgear/math/SGMathFwd.hxx>



/**
 * Container for FlightGear envirnonmetal sound effects.
 */
class FGEnvironmentFX : public SGSampleGroup
{

public:

    FGEnvironmentFX();
    virtual ~FGEnvironmentFX();

    void init ();
    void reinit();
    void update (double dt);
    void unbind();

    bool add(const std::string& path_str, const std::string& refname);
    void position(const std::string& refname, double lon, double lat, double elevation = 0.0);
    void pitch(const std::string& refname, float pitch);
    void volume(const std::string& refname, float volume);
    void properties(const std::string& refname, float reference_dist, float max_dist = -1 );
    void play(const std::string& refname, bool looping = false);
    void stop(const std::string& refname);

private:
    SGPropertyNode_ptr _enabled;
    SGPropertyNode_ptr _volume;
};


#endif

// end of fg_environmentfx.hxx
