// globals.cxx -- Global state that needs to be shared among the sim modules
//
// Written by Curtis Olson, started July 2000.
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


#include <simgear/misc/commands.hxx>

#include <Environment/environment_mgr.hxx>

#include "globals.hxx"
#include "viewmgr.hxx"

#include "fg_props.hxx"
#include "fg_io.hxx"


////////////////////////////////////////////////////////////////////////
// Implementation of FGGlobals.
////////////////////////////////////////////////////////////////////////

// global global :-)
FGGlobals *globals;


// Constructor
FGGlobals::FGGlobals() :
    subsystem_mgr(new FGSubsystemMgr),
    sim_time_sec(0.0),
#if defined(FX) && defined(XMESA)
    fullscreen( true ),
#endif
    warp( 0 ),
    warp_delta( 0 ),
    props(new SGPropertyNode),
    initial_state(0),
    commands(new SGCommandMgr),
    io(new FGIO)
{
}


// Destructor
FGGlobals::~FGGlobals() 
{
  delete subsystem_mgr;
  delete initial_state;
  delete props;
  delete commands;
  delete io;
}


// Save the current state as the initial state.
void
FGGlobals::saveInitialState ()
{
  delete initial_state;
  initial_state = new SGPropertyNode();
  if (!copyProperties(props, initial_state))
    SG_LOG(SG_GENERAL, SG_ALERT, "Error saving initial state");
}


// Restore the saved initial state, if any
void
FGGlobals::restoreInitialState ()
{
  if (initial_state == 0) {
    SG_LOG(SG_GENERAL, SG_ALERT, "No initial state available to restore!!!");
  } else if (!copyProperties(initial_state, props)) {
    SG_LOG(SG_GENERAL, SG_INFO,
	   "Some errors restoring initial state (probably just read-only props)");
  } else {
    SG_LOG(SG_GENERAL, SG_INFO, "Initial state restored successfully");
  }
}

FGViewer *
FGGlobals::get_current_view () const
{
  return viewmgr->get_current_view();
}

// end of globals.cxx
