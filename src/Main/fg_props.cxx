// fg_props.cxx -- support for
//
// Written by David Megginson, started November 1999.
//
// Copyright (C) 1999, 2000 David Megginson - david@megginson.com
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

#include <iostream>

#include <Main/fgfs.hxx>
#include "fg_props.hxx"

using std::istream;
using std::ostream;


/**
 * Save the current state of the simulator to a stream.
 */
bool
fgSaveFlight (ostream &output)
{
  return writeProperties(output, globals->get_props());
}


/**
 * Restore the current state of the simulator from a stream.
 */
bool
fgLoadFlight (istream &input)
{
  SGPropertyNode props;
  if (readProperties(input, &props)) {
    copyProperties(&props, globals->get_props());
				// When loading a flight, make it the
				// new initial state.
    globals->saveInitialState();
  } else {
    FG_LOG(FG_INPUT, FG_ALERT, "Error restoring flight; aborted");
    return false;
  }
}

// end of fg_props.cxx

