// save.cxx -- class to save and restore a flight.
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  David Megginson - david@megginson.com
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

#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>

#include <simgear/misc/props.hxx>

				// FIXME: just for the temporary
				// tile cache update stuff.
#include <simgear/debug/logstream.hxx>
#include <simgear/constants.h>
#include <Aircraft/aircraft.hxx>
#include <GUI/gui.h>
#include <Scenery/tilemgr.hxx>
#include "globals.hxx"
				// end FIXME

using std::istream;
using std::ostream;


/**
 * Save the current state of the simulator to a stream.
 */
bool
fgSaveFlight (ostream &output)
{
  return writePropertyList(output, &current_properties);
}


/**
 * Restore the current state of the simulator from a stream.
 */
bool
fgLoadFlight (istream &input)
{
  bool retval = readPropertyList(input, &current_properties);

				// FIXME: from keyboard.cxx
				// this makes sure that the tile
				// cache is updated after a restore;
				// it would be better if FGFS just
				// noticed the new lat/lon.
  if (retval) {
    bool freeze;
    FG_LOG(FG_INPUT, FG_INFO, "ReIniting TileCache");
    if ( !freeze ) 
      globals->set_freeze( true );
    BusyCursor(0);
    if ( global_tile_mgr.init() ) {
      // Load the local scenery data
      global_tile_mgr.update( 
			     cur_fdm_state->get_Longitude() * RAD_TO_DEG,
			     cur_fdm_state->get_Latitude() * RAD_TO_DEG );
    } else {
      FG_LOG( FG_GENERAL, FG_ALERT, 
	      "Error in Tile Manager initialization!" );
      exit(-1);
    }
    BusyCursor(1);
    if ( !freeze )
      globals->set_freeze( false );
  }
				// end FIXME

  return retval;
}

// end of save.cxx
