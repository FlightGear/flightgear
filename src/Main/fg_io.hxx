// fg_io.hxx -- Higher level I/O managment routines
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_IO_HXX
#define _FG_IO_HXX


#include <simgear/compiler.h>


// initialize I/O channels based on command line options (if any)
void fgIOInit();


// process any I/O work
void fgIOProcess();


// shutdown all I/O connections
void fgIOShutdownAll();


#endif // _FG_IO_HXX
