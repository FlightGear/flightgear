// fg_fx.hxx -- Sound effect management class
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

#ifndef __FGFX_HXX
#define __FGFX_HXX 1

#include <simgear/compiler.h>

#include <vector>

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>
#include <simgear/sound/sample_group.hxx>

class SGXmlSound;

/**
 * Generator for FlightGear sound effects.
 *
 * This module uses a FGSampleGroup class to generate sound effects based
 * on current flight conditions. The sound manager must be initialized
 * before this object is.
 *
 *    This module will load and play a set of sound effects defined in an
 *    xml file and tie them to various property states.
 */
class FGFX : public SGSampleGroup
{

public:

    FGFX ( SGSoundMgr *smgr, const string &refname, SGPropertyNode *props = 0 );
    virtual ~FGFX ();

    virtual void init ();
    virtual void reinit ();
    virtual void update (double dt);

private:

    bool _is_aimodel;
    SGSharedPtr<SGSampleGroup> _avionics;
    std::vector<SGXmlSound *> _sound;

    SGPropertyNode_ptr _props;
    SGPropertyNode_ptr _enabled;
    SGPropertyNode_ptr _volume;
    SGPropertyNode_ptr _avionics_enabled;
    SGPropertyNode_ptr _avionics_volume;
    SGPropertyNode_ptr _avionics_ext;
    SGPropertyNode_ptr _internal;
};


#endif

// end of fg_fx.hxx
