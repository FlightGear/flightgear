// electrical.cxx - a flexible, generic electrical system model.
//
// Written by Curtis Olson, started September 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include <simgear/misc/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "electrical.hxx"

ElectricalSystem::ElectricalSystem ()
{
}

ElectricalSystem::~ElectricalSystem ()
{
}

void
ElectricalSystem::init ()
{
    config_props = new SGPropertyNode;

    SGPath config( globals->get_fg_root() );
    config.append( fgGetString("/systems/electrical/path") );

    SG_LOG( SG_ALL, SG_ALERT, "Reading electrical system model from "
            << config.str() );
    try {
        readProperties( config.str(), config_props );
    } catch (const sg_exception& exc) {
        SG_LOG( SG_ALL, SG_ALERT, "Failed to load electrical system model: "
                << config.str() );
    }

    delete config_props;
}

void
ElectricalSystem::bind ()
{
}

void
ElectricalSystem::unbind ()
{
}

void
ElectricalSystem::update (double dt)
{
}
