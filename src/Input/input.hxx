// input.hxx -- handle user input from various sources.
//
// Written by David Megginson, started May 2001.
// Major redesign by Torsten Dreyer, started August 2009
//
// Copyright (C) 2001 David Megginson, david@megginson.com
// Copyright (C) 2009 Torsten Dreyer, Torsten (at) t3r _dot_ de
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


#ifndef _INPUT_HXX
#define _INPUT_HXX


#include <simgear/structure/subsystem_mgr.hxx>


////////////////////////////////////////////////////////////////////////
// General input mapping support.
////////////////////////////////////////////////////////////////////////


/**
 * Generic input module.
 *
 * <p>This module is designed to handle input from multiple sources --
 * keyboard, joystick, mouse, or even panel switches -- in a consistent
 * way, and to allow users to rebind any of the actions at runtime.</p>
 */
class FGInput : public SGSubsystemGroup
{
public:
    /**
     * Default constructor.
     */
    FGInput ();

    /**
     * Destructor.
     */
    virtual ~FGInput();

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "input"; }
};

#endif // _INPUT_HXX
