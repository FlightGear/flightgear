// radiostack.cxx -- class to manage an instance of the radio stack
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>	// snprintf

#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>

#include <Aircraft/aircraft.hxx>
#include <Navaids/navlist.hxx>

#include "radiostack.hxx"

#include <string>
SG_USING_STD(string);


FGRadioStack *current_radiostack;


// Constructor
FGRadioStack::FGRadioStack() {
}


// Destructor
FGRadioStack::~FGRadioStack() 
{
    //adf.unbind();
    beacon.unbind();
    navcom1.unbind();
    navcom2.unbind();
    xponder.unbind();
}


void
FGRadioStack::init ()
{
    navcom1.set_bind_index( 0 );
    navcom1.init();

    navcom2.set_bind_index( 1 );
    navcom2.init();

    //adf.init();
    beacon.init();
    xponder.init();

    search();
    update(0);			// FIXME: use dt

    // Search radio database once per second
    globals->get_event_mgr()->addTask( "fgRadioSearch()", current_radiostack,
                                       &FGRadioStack::search, 1 );
}


void
FGRadioStack::bind ()
{
    //adf.bind();
    beacon.bind();
    dme.bind();
    navcom1.set_bind_index( 0 );
    navcom1.bind();
    navcom2.set_bind_index( 1 );
    navcom2.bind();
    xponder.bind();
}


void
FGRadioStack::unbind ()
{
    //adf.unbind();
    beacon.unbind();
    dme.unbind();
    navcom1.unbind();
    navcom2.unbind();
    xponder.unbind();
}


// Update the various nav values based on position and valid tuned in navs
void 
FGRadioStack::update(double dt) 
{
    //adf.update( dt );
    beacon.update( dt );
    navcom1.update( dt );
    navcom2.update( dt );
    dme.update( dt );           // dme is updated after the navcom's
    xponder.update( dt );
}


// Update current nav/adf radio stations based on current postition
void FGRadioStack::search() 
{
    //adf.search();
    beacon.search();
    navcom1.search();
    navcom2.search();
    dme.search();
    xponder.search();
}
