// electrical.hxx - a flexible, generic electrical system model.
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


#ifndef _SYSTEMS_ELECTRICAL_HXX
#define _SYSTEMS_ELECTRICAL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/misc/props.hxx>
#include <Main/fgfs.hxx>


/**
 * Model an electrical system.  This is a simple system with the
 * alternator hardwired to engine[0]/rpm
 *
 * Input properties:
 *
 * /engines/engine[0]/rpm
 *
 * Output properties:
 *
 * 
 */

class ElectricalSystem : public FGSubsystem
{

public:

    ElectricalSystem ();
    virtual ~ElectricalSystem ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:

    SGPropertyNode *config_props;
    // SGPropertyNode_ptr _serviceable_node;
    
};

#endif // _SYSTEMS_ELECTRICAL_HXX
