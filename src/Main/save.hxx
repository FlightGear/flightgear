// save.hxx -- class to save and restore a flight.
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

#ifndef _SAVE_HXX
#define _SAVE_HXX

#ifndef __cplusplus
# error This library requires C++
#endif                                   

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/math/fg_types.hxx>
#include <iostream>

FG_USING_NAMESPACE(std);

extern bool fgSaveFlight (ostream &output);
extern bool fgLoadFlight (istream &input);

#endif __SAVE_HXX

// end of save.hxx
