// main.cxx -- FlightGear Scenery/Aircraft Admin Tool
//
// Written by Curtis Olson, started February 2004.
//
// Copyright (c) 2004  Curtis L. Olson - curt@flightgear.org
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

#include <string>
#include <FL/Fl.H>

#include "fgadmin.h"

std::string def_fg_exe = "";
std::string def_fg_root = "";
std::string def_fg_scenery = "";

/**
 * --fg-exe=<PATH>
 * --fg-root=<DIR>
 * --fg-scenery=<DIR>
 */
static int
parse_args( int, char** argv, int& i )
{
    if (strncmp( argv[i], "--fg-exe=", 9 ) == 0)
    {
	def_fg_exe.assign( &argv[i][9] );
	++i;
	return 1;
    }

    if (strncmp( argv[i], "--fg-root=", 10 ) == 0)
    {
	def_fg_root.assign( &argv[i][10] );
	def_fg_scenery = def_fg_root;
	def_fg_scenery += "/Scenery";

	++i;
	return 1;
    }

    if (strncmp( argv[i], "--fg-scenery=", 13 ) == 0)
    {
	def_fg_scenery.assign( &argv[i][13] );
	++i;
	return 1;
    }

    return 0;
}

int
main( int argc, char* argv[] )
{
    int i = 0;
    if (Fl::args( argc, argv, i, parse_args ) < argc)
    {
	Fl::fatal("Options are:\n --fg-exe=<PATH>\n --fg-root=<DIR>\n --fg-scenery=<DIR>\n%s", Fl::help );
    }

    FGAdminUI ui;
    ui.init();
    ui.show();

    return Fl::run();
}
