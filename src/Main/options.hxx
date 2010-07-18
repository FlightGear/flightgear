// options.hxx -- class to handle command line options
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _OPTIONS_HXX
#define _OPTIONS_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

extern void fgSetDefaults ();
extern void fgParseArgs (int argc, char ** argv);
extern void fgParseOptions (const string &file_path);
extern void fgUsage (bool verbose = false);

#endif /* _OPTIONS_HXX */
