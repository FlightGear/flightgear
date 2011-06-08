// main.cxx -- FlightGear Scenery/Aircraft Admin Tool
//
// Written by Curtis Olson, started February 2004.
//
// Copyright (c) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <FL/Fl.H>
#include <FL/filename.H>
#include <string.h>
#include "fgadmin.h"

using std::string;

string def_install_source;
string def_scenery_dest;
bool silent = false;

/**
 * --silent
 * --install-source=<DIR>
 * --scenery-dest=<DIR>
 */
static int
parse_args( int, char** argv, int& i )
{
    if (strcmp( argv[i], "--silent" ) == 0)
    {
	silent = true;
	++i;
	return 1;
    }
    else if (strncmp( argv[i], "--install-source=", 17 ) == 0)
    {
	def_install_source.assign( &argv[i][17] );
	++i;
	return 1;
    }
    else if (strncmp( argv[i], "--scenery-dest=", 15 ) == 0)
    {
	def_scenery_dest.assign( &argv[i][15] );
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
	Fl::fatal("Options are:\n --silent\n --install-source=<DIR>\n --scenery-dest=<DIR>\n%s", Fl::help );
    }

    if ( silent )
    {
        Fl_Preferences prefs( Fl_Preferences::USER, "flightgear.org", "fgadmin" );
	char abs_name[ FL_PATH_MAX ];

	fl_filename_absolute( abs_name, def_install_source.c_str() );
	prefs.set( "install-source", abs_name );

	fl_filename_absolute( abs_name, def_scenery_dest.c_str() );
        prefs.set( "scenery-dest", abs_name );

	return 0;
    }

    FGAdminUI ui;
    ui.init();
    ui.show();

    return Fl::run();
}
