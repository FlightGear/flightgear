// dds_props.hxx -- FGFS "DDS" properties protocal class
//
// Written by Erik Hofman, started April 2021
//
// Copyright (C) 2021 by Erik Hofman <erik@ehofman.com>
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


#pragma once

#include <string>
#include <map>

#include <simgear/compiler.h>

#include <simgear/props/propsfwd.hxx>

#include "protocol.hxx"
#include "DDS/dds_props.h"


class FGDDSProps : public FGProtocol {

    std::map<uint32_t,SGPropertyNode_ptr> by_id;
    std::map<std::string,SGPropertyNode_ptr> by_path;

    static void setProp(FG_DDS_PROP& prop, SGPropertyNode_ptr p);
    
public:

    FGDDSProps() = default;
    ~FGDDSProps() = default;

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();
};
